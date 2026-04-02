#include "CellNeoHook.h"

#include "MooseMesh.h"
#include "MooseVariable.h"
#include "TransientInterface.h"
#include "libmesh/utility.h"

#include <algorithm>
#include <cmath>

registerMooseObject("AntApp", CellNeoHook);

InputParameters
CellNeoHook::validParams()
{
  InputParameters params = ComputeLagrangianStressPK2::validParams();
  params.addClassDescription("CellNeoHook Material model: Implements TNT for cell aggregates "
                             "Uses 2nd order Objective time integration.");

  params.addRequiredParam<Real>("G", "Shear modulus of cell aggregate");
  params.addRequiredParam<Real>("K", "Bulk modulus of cell aggregate");

  params.addParam<Real>("k_exp_max", -1.0,
                        "Max isotropic expansion rate after ramping (sets time scale)");
  params.addParam<Real>("ke0", -1.0, "Alias for k_exp_max.");
  params.addRequiredParam<Real>("t_str", "Expansion ramp paramter1 (time at which the rate ramps)");
  params.addRequiredParam<Real>("m_exp", "Expansion ramp paramter2 (steepness of ramp)");
  params.addParam<Real>("press_str", -1.0,
                        "pressure gating for compression inhibited cell expansion");
  params.addParam<Real>("p_star", -1.0, "Alias for press_str.");

  params.addParam<Real>("k_T1_max", -1.0, "T1 logistic amplitude (0 disables)");
  params.addParam<Real>("k_t", -1.0, "Alias for k_T1_max.");
  params.addParam<Real>("chi_str", 0.20, "shape distortion threshold for T1 yielding.");
  params.addParam<Real>("m_T1", 10.0, "exponent parameter for T1 transition ramping");
  params.addParam<Real>("epsilon", 1e-8, "Epsilon for Miehe-style algorithmic tangent");

  return params;
}

CellNeoHook::CellNeoHook(const InputParameters & parameters)
  : ComputeLagrangianStressPK2(parameters),
    _G(getParam<Real>("G")),
    _K(getParam<Real>("K")),
    _k_exp_max(getParam<Real>("k_exp_max") > 0.0 ? getParam<Real>("k_exp_max")
                                                   : getParam<Real>("ke0")),
    _t_str(getParam<Real>("t_str")),
    _m_exp(getParam<Real>("m_exp")),
    _press_str(getParam<Real>("press_str") > 0.0 ? getParam<Real>("press_str")
                                                   : getParam<Real>("p_star")),
    _k_T1_max(getParam<Real>("k_T1_max") >= 0.0 ? getParam<Real>("k_T1_max")
                                                  : (getParam<Real>("k_t") >= 0.0
                                                         ? getParam<Real>("k_t")
                                                         : 0.0)),
    _chi_str(getParam<Real>("chi_str")),
    _m_T1(getParam<Real>("m_T1")),
    _epsilon(getParam<Real>("epsilon")),
    _F(getMaterialPropertyByName<RankTwoTensor>(_base_name + "deformation_gradient")),
    _F_old(getMaterialPropertyOldByName<RankTwoTensor>(_base_name + "deformation_gradient")),
    _GI(declareProperty<RankTwoTensor>("GI")),
    _GI_old(getMaterialPropertyOld<RankTwoTensor>("GI")),
    _bE(declareProperty<RankTwoTensor>("bE")),
    _bE_old(getMaterialPropertyOld<RankTwoTensor>("bE")),
    _JE(declareProperty<Real>("JE")),
    _JE_cell(declareProperty<Real>("JE_cell")),
    _J_cell(declareProperty<Real>("J_cell")),
    _J_total(declareProperty<Real>("J_total")),
    _sigma(declareProperty<RankTwoTensor>("sigma")),
    _press(declareProperty<Real>("press")),
    _eta(declareProperty<Real>("eta")),
    _eta_old(getMaterialPropertyOld<Real>("eta")),
    _chi(declareProperty<Real>("chi")),
    _ke(declareProperty<Real>("ke")),
    _ke_old(getMaterialPropertyOld<Real>("ke")),
    _kT1(declareProperty<Real>("kT1")),
    _kT1_old(getMaterialPropertyOld<Real>("kT1"))
{
  if (_k_exp_max < 0.0)
    mooseError("CellNeoHook requires k_exp_max (or alias ke0).");
  if (_press_str <= 0.0)
    mooseError("CellNeoHook requires press_str > 0 (or alias p_star).");

  pow_t_str_m = std::pow(_t_str, _m_exp);
  pow_chi_str_m = std::pow(_chi_str, _m_T1);
}

void
CellNeoHook::initQpStatefulProperties()
{
  _GI[_qp] = I;
  _bE[_qp] = I;
  _JE[_qp] = 1.0;
  _JE_cell[_qp] = 1.0;
  _J_cell[_qp] = 1.0;
  _J_total[_qp] = 1.0;
  _sigma[_qp].zero();
  _press[_qp] = 0.0;
  _eta[_qp] = 1.0;
  _chi[_qp] = 0.0;
  _ke[_qp] = 0.0;
  _kT1[_qp] = 0.0;
}

void
CellNeoHook::computeQpProperties()
{
  dt = std::max(_dt, 1e-30);
  const Real num_exp = std::pow(_t, _m_exp);
  k_exp0 = _k_exp_max * (num_exp / (num_exp + pow_t_str_m));

  const RankTwoTensor & F_n = _F_old[_qp];
  const RankTwoTensor & F_np1 = _F[_qp];
  F_n_inv = F_n.inverse();
  const RankTwoTensor F_nph = 0.5 * (F_n + F_np1);
  const RankTwoTensor F_nph_inv = F_nph.inverse();

  const RankTwoTensor f_nph = F_nph * F_n_inv;
  const RankTwoTensor f_nph_inv = f_nph.inverse();
  const RankTwoTensor f_np1 = F_np1 * F_n_inv;
  const RankTwoTensor E_np1 = 0.5 * (f_np1.transpose() * f_np1 - I);

  const RankTwoTensor d_n = (1.0 / dt) * E_np1;
  const Real kh_n = fa * d_n.trace();

  bE_n_inv_trace = _bE_old[_qp].inverse().trace();

  const RankTwoTensor Lie_bE_n =
      -(2.0 / 3.0) * _ke_old[_qp] * _bE_old[_qp] -
      ((4.0 / 3.0) * kh_n + _kT1_old[_qp]) * (_bE_old[_qp] - (3.0 / bE_n_inv_trace) * I);

  const RankTwoTensor GI_dot_n = F_n_inv * Lie_bE_n * F_n_inv.transpose();
  const RankTwoTensor GI_nph = _GI_old[_qp] + 0.5 * GI_dot_n * dt;
  const RankTwoTensor bE_nph = F_nph * GI_nph * F_nph.transpose();

  const RankTwoTensor d_nph = (1.0 / dt) * f_nph_inv.transpose() * E_np1 * f_nph_inv;
  const Real kh_nph = fa * d_nph.trace();

  const Real JE_nph = std::max(std::sqrt(bE_nph.det()), 1e-12);
  const Real press_nph = _K * (JE_nph - 1.0);

  Real ke_nph;
  if (press_nph >= 0.0)
    ke_nph = fa * k_exp0;
  else
    ke_nph = fa * k_exp0 * std::exp(-1.0 * std::pow(press_nph / _press_str, 2.0));

  const RankTwoTensor dev_bE_nph = bE_nph.deviatoric();
  const Real chi_nph = std::sqrt(1.5 * dev_bE_nph.doubleContraction(dev_bE_nph)) / bE_nph.trace();
  const Real num_chi_nph = std::pow(chi_nph, _m_T1);
  const Real kT1_nph = _k_T1_max * (num_chi_nph / (num_chi_nph + pow_chi_str_m));
  const Real bE_nph_inv_trace = bE_nph.inverse().trace();

  const RankTwoTensor Lie_bE_nph =
      -(2.0 / 3.0) * ke_nph * bE_nph -
      ((4.0 / 3.0) * kh_nph + kT1_nph) * (bE_nph - (3.0 / bE_nph_inv_trace) * I);

  const RankTwoTensor GI_dot_nph = F_nph_inv * Lie_bE_nph * F_nph_inv.transpose();

  _GI[_qp] = _GI_old[_qp] + GI_dot_nph * dt;
  _bE[_qp] = F_np1 * _GI[_qp] * F_np1.transpose();
  const RankTwoTensor dev_bE_np1 = _bE[_qp].deviatoric();
  _JE[_qp] = std::max(std::sqrt(_bE[_qp].det()), 1e-12);
  _JE_cell[_qp] = _JE[_qp];
  _J_cell[_qp] = _JE[_qp];
  _J_total[_qp] = std::max(F_np1.det(), 1e-12);

  const Real JE = _JE[_qp];
  _sigma[_qp] = (_G / std::pow(JE, 5.0 / 3.0)) * dev_bE_np1 + _K * (JE - 1.0) * I;

  _press[_qp] = _K * (JE - 1.0);
  if (_press[_qp] >= 0.0)
    _ke[_qp] = fa * k_exp0;
  else
    _ke[_qp] = fa * k_exp0 * std::exp(-1.0 * std::pow(_press[_qp] / _press_str, 2.0));

  _eta[_qp] = _eta_old[_qp] * std::exp(kh_nph * dt);
  _chi[_qp] = std::sqrt(1.5 * dev_bE_np1.doubleContraction(dev_bE_np1)) / _bE[_qp].trace();
  const Real num_chi = std::pow(_chi[_qp], _m_T1);
  _kT1[_qp] = _k_T1_max * (num_chi / (num_chi + pow_chi_str_m));

  ComputeLagrangianStressPK2::computeQpProperties();
}

void
CellNeoHook::computeQpPK2Stress()
{
  const RankTwoTensor & F_np1 = _F[_qp];
  const RankTwoTensor F_np1_inv = F_np1.inverse();
  const RankTwoTensor F_np1_inv_trans = F_np1_inv.transpose();
  const RankTwoTensor & sigma_cauchy = _sigma[_qp];

  const Real J_def = F_np1.det();
  _S[_qp] = F_np1_inv * sigma_cauchy * F_np1_inv_trans * J_def;

  const RankTwoTensor & F_n = _F_old[_qp];

  for (unsigned int C = 0; C < 3; C++)
  {
    for (unsigned int D = C; D < 3; D++)
    {
      RankTwoTensor dE;
      dE.zero();
      if (C == D)
        dE(C, C) = _epsilon;
      else
      {
        dE(C, D) = 0.5 * _epsilon;
        dE(D, C) = 0.5 * _epsilon;
      }

      const RankTwoTensor F_trial = F_np1 + F_np1_inv_trans * dE;
      const RankTwoTensor F_trial_inv = F_trial.inverse();

      const RankTwoTensor F_trial_nph = 0.5 * (F_trial + F_n);
      const RankTwoTensor F_trial_nph_inv = F_trial_nph.inverse();
      const RankTwoTensor f_trial_nph = F_trial_nph * F_n_inv;
      const RankTwoTensor f_trial_nph_inv = f_trial_nph.inverse();
      const RankTwoTensor f_trial_np1 = F_trial * F_n_inv;
      const RankTwoTensor E_trial_np1 = 0.5 * (f_trial_np1.transpose() * f_trial_np1 - I);

      const RankTwoTensor d_trial_n = (1.0 / dt) * E_trial_np1;
      const Real kh_trial_n = fa * d_trial_n.trace();
      const RankTwoTensor Lie_bE_trial_n =
          -(2.0 / 3.0) * _ke_old[_qp] * _bE_old[_qp] -
          ((4.0 / 3.0) * kh_trial_n + _kT1_old[_qp]) * (_bE_old[_qp] - (3.0 / bE_n_inv_trace) * I);

      const RankTwoTensor GI_dot_trial_n = F_n_inv * Lie_bE_trial_n * F_n_inv.transpose();
      const RankTwoTensor GI_trial_nph = _GI_old[_qp] + 0.5 * GI_dot_trial_n * dt;
      const RankTwoTensor bE_trial_nph = F_trial_nph * GI_trial_nph * F_trial_nph.transpose();

      const RankTwoTensor d_trial_nph =
          (1.0 / dt) * f_trial_nph_inv.transpose() * E_trial_np1 * f_trial_nph_inv;
      const Real kh_trial_nph = fa * d_trial_nph.trace();

      const Real JE_trial_nph = std::max(std::sqrt(bE_trial_nph.det()), 1e-12);
      const Real press_trial_nph = _K * (JE_trial_nph - 1.0);

      Real ke_trial_nph;
      if (press_trial_nph >= 0.0)
        ke_trial_nph = fa * k_exp0;
      else
        ke_trial_nph = fa * k_exp0 * std::exp(-1.0 * std::pow(press_trial_nph / _press_str, 2.0));

      const RankTwoTensor dev_bE_trial_nph = bE_trial_nph.deviatoric();
      const Real chi_trial_nph =
          std::sqrt(1.5 * dev_bE_trial_nph.doubleContraction(dev_bE_trial_nph)) /
          bE_trial_nph.trace();
      const Real num_chi_trial_nph = std::pow(chi_trial_nph, _m_T1);
      const Real kT1_trial_nph =
          _k_T1_max * (num_chi_trial_nph / (num_chi_trial_nph + pow_chi_str_m));

      const Real bE_trial_nph_inv_trace = bE_trial_nph.inverse().trace();
      const RankTwoTensor Lie_bE_trial_nph =
          -(2.0 / 3.0) * ke_trial_nph * bE_trial_nph -
          ((4.0 / 3.0) * kh_trial_nph + kT1_trial_nph) *
              (bE_trial_nph - (3.0 / bE_trial_nph_inv_trace) * I);

      const RankTwoTensor GI_dot_trial_nph =
          F_trial_nph_inv * Lie_bE_trial_nph * F_trial_nph_inv.transpose();

      const RankTwoTensor GI_trial = _GI_old[_qp] + dt * GI_dot_trial_nph;
      const RankTwoTensor bE_trial = F_trial * GI_trial * F_trial.transpose();
      const Real JE_trial = std::max(std::sqrt(bE_trial.det()), 1e-12);
      const RankTwoTensor cauchy_trial =
          (_G / std::pow(JE_trial, 5.0 / 3.0)) * bE_trial.deviatoric() + _K * (JE_trial - 1.0) * I;

      const Real J_trial = F_trial.det();
      const RankTwoTensor S_trial = F_trial_inv * cauchy_trial * F_trial_inv.transpose() * J_trial;

      for (unsigned int A = 0; A < 3; A++)
      {
        for (unsigned int B = A; B < 3; B++)
        {
          const Real val = (S_trial(A, B) - _S[_qp](A, B)) / _epsilon;
          _C[_qp](A, B, C, D) = val;

          if (A != B)
            _C[_qp](B, A, C, D) = val;

          if (C != D)
          {
            _C[_qp](A, B, D, C) = val;
            if (A != B)
              _C[_qp](B, A, D, C) = val;
          }
        }
      }
    }
  }
}

void
CellNeoHook::computeQpCauchyStress()
{
  _cauchy_stress[_qp] = _sigma[_qp];
}
