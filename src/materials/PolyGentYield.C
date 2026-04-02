#include "PolyGentYield.h"

#include "MooseMesh.h"
#include "MooseVariable.h"
#include "TransientInterface.h"
#include "libmesh/utility.h"

#include <algorithm>
#include <cmath>

registerMooseObject("AntApp", PolyGentYield);

namespace
{
Real
polyGentJE(const RankTwoTensor & bE)
{
  return std::max(std::sqrt(std::max(bE.det(), 0.0)), 1e-12);
}

RankTwoTensor
polyGentStress(const RankTwoTensor & bE,
               const RankTwoTensor & I,
               const Real G,
               const Real K,
               const Real Im)
{
  const Real JE = polyGentJE(bE);
  const Real I1_iso = std::pow(JE, -2.0 / 3.0) * bE.trace();
  return (G / std::pow(JE, 5.0 / 3.0)) * ((Im - 3.0) / std::max(Im - I1_iso, 1e-12)) *
             bE.deviatoric() +
         K * (JE - 1.0) * I;
}
}

InputParameters
PolyGentYield::validParams()
{
  InputParameters params = ComputeLagrangianStressPK2::validParams();
  params.addClassDescription("PolyGentYield Material model: Implements TNT for polymer networks "
                             "Uses 2nd order Objective time integration.");

  params.addRequiredParam<Real>("G", "Shear modulus of polymer network");
  params.addRequiredParam<Real>("K", "Bulk modulus of polymer network");
  params.addRequiredParam<Real>("Im", "finite extensibility parameter for Gent model");
  params.addRequiredParam<Real>("kD0", "Base bond dissociation rate for gel network");
  params.addRequiredParam<Real>("kDinf", "Max bond dissociation rate for gel network");
  params.addRequiredParam<Real>("sig_yield", "Yield stress for bond dissociation in gel network");
  params.addParam<Real>("m_kD", -1.0,
                        "Exponent for bond dissociation rate ramping from kD0 to kDinf");
  params.addParam<Real>("n_kD", -1.0, "Alias for m_kD.");
  params.addParam<Real>("epsilon", 1e-8, "Epsilon for Miehe-style algorithmic tangent");

  return params;
}

PolyGentYield::PolyGentYield(const InputParameters & parameters)
  : ComputeLagrangianStressPK2(parameters),
    _G(getParam<Real>("G")),
    _K(getParam<Real>("K")),
    _Im(getParam<Real>("Im")),
    _kD0(getParam<Real>("kD0")),
    _kDinf(getParam<Real>("kDinf")),
    _sig_yield(getParam<Real>("sig_yield")),
    _m_kD(getParam<Real>("m_kD") > 0.0 ? getParam<Real>("m_kD") : getParam<Real>("n_kD")),
    _epsilon(getParam<Real>("epsilon")),
    _pow_sig_yield_m_kD(std::pow(getParam<Real>("sig_yield"),
                                 getParam<Real>("m_kD") > 0.0 ? getParam<Real>("m_kD")
                                                                : getParam<Real>("n_kD"))),
    _F(getMaterialPropertyByName<RankTwoTensor>(_base_name + "deformation_gradient")),
    _F_old(getMaterialPropertyOldByName<RankTwoTensor>(_base_name + "deformation_gradient")),
    _GI(declareProperty<RankTwoTensor>("GI")),
    _GI_old(getMaterialPropertyOld<RankTwoTensor>("GI")),
    _bE(declareProperty<RankTwoTensor>("bE")),
    _bE_old(getMaterialPropertyOld<RankTwoTensor>("bE")),
    _JE(declareProperty<Real>("JE")),
    _JE_gel(declareProperty<Real>("JE_gel")),
    _J_gel(declareProperty<Real>("J_gel")),
    _J_total(declareProperty<Real>("J_total")),
    _sigma(declareProperty<RankTwoTensor>("sigma")),
    _kD(declareProperty<Real>("kD")),
    _kD_old(getMaterialPropertyOld<Real>("kD"))
{
  if (_m_kD <= 0.0)
    mooseError("PolyGentYield requires m_kD > 0 (or alias n_kD).");
}

void
PolyGentYield::initQpStatefulProperties()
{
  _GI[_qp] = I;
  _bE[_qp] = I;
  _JE[_qp] = 1.0;
  _JE_gel[_qp] = 1.0;
  _J_gel[_qp] = 1.0;
  _J_total[_qp] = 1.0;
  _sigma[_qp].zero();
  _kD[_qp] = _kD0;
}

void
PolyGentYield::computeQpProperties()
{
  dt = std::max(_dt, 1e-30);

  const RankTwoTensor & F_n = _F_old[_qp];
  const RankTwoTensor & F_np1 = _F[_qp];
  F_n_inv = F_n.inverse();
  const RankTwoTensor F_nph = 0.5 * (F_n + F_np1);
  const RankTwoTensor F_nph_inv = F_nph.inverse();

  bE_n_inv_trace = std::max(_bE_old[_qp].inverse().trace(), 1e-12);

  const RankTwoTensor Lie_bE_n = -1.0 * _kD_old[_qp] * (_bE_old[_qp] - (3.0 / bE_n_inv_trace) * I);
  const RankTwoTensor GI_dot_n = F_n_inv * Lie_bE_n * F_n_inv.transpose();
  const RankTwoTensor GI_nph = _GI_old[_qp] + 0.5 * GI_dot_n * dt;
  const RankTwoTensor bE_nph = F_nph * GI_nph * F_nph.transpose();

  const RankTwoTensor cauchy_nph = polyGentStress(bE_nph, I, _G, _K, _Im);

  const Real sig_nph_11 = cauchy_nph(0, 0);
  const Real sig_nph_22 = cauchy_nph(1, 1);
  const Real sig_nph_33 = cauchy_nph(2, 2);
  const Real sig_nph_12 = cauchy_nph(0, 1);
  const Real sig_nph_13 = cauchy_nph(0, 2);
  const Real sig_nph_23 = cauchy_nph(1, 2);
  const Real sig_nph_vm =
      std::sqrt(0.5 * (std::pow((sig_nph_11 - sig_nph_22), 2.0) +
                       std::pow((sig_nph_22 - sig_nph_33), 2.0) +
                       std::pow((sig_nph_33 - sig_nph_11), 2.0)) +
                3.0 * (std::pow(sig_nph_12, 2.0) + std::pow(sig_nph_13, 2.0) +
                       std::pow(sig_nph_23, 2.0)));

  const Real kD_nph =
      _kDinf + (_kD0 - _kDinf) *
                   (_pow_sig_yield_m_kD / (_pow_sig_yield_m_kD + std::pow(sig_nph_vm, _m_kD)));

  const Real bE_nph_inv_trace = bE_nph.inverse().trace();
  const RankTwoTensor Lie_bE_nph = -1.0 * kD_nph * (bE_nph - (3.0 / bE_nph_inv_trace) * I);
  const RankTwoTensor GI_dot_nph = F_nph_inv * Lie_bE_nph * F_nph_inv.transpose();

  _GI[_qp] = _GI_old[_qp] + GI_dot_nph * dt;
  _bE[_qp] = F_np1 * _GI[_qp] * F_np1.transpose();
  _JE[_qp] = polyGentJE(_bE[_qp]);
  _JE_gel[_qp] = _JE[_qp];
  _J_gel[_qp] = _JE[_qp];
  _J_total[_qp] = std::max(F_np1.det(), 1e-12);

  _sigma[_qp] = polyGentStress(_bE[_qp], I, _G, _K, _Im);

  const Real sig_np1_11 = _sigma[_qp](0, 0);
  const Real sig_np1_22 = _sigma[_qp](1, 1);
  const Real sig_np1_33 = _sigma[_qp](2, 2);
  const Real sig_np1_12 = _sigma[_qp](0, 1);
  const Real sig_np1_13 = _sigma[_qp](0, 2);
  const Real sig_np1_23 = _sigma[_qp](1, 2);
  const Real sig_np1_vm =
      std::sqrt(0.5 * (std::pow((sig_np1_11 - sig_np1_22), 2.0) +
                       std::pow((sig_np1_22 - sig_np1_33), 2.0) +
                       std::pow((sig_np1_33 - sig_np1_11), 2.0)) +
                3.0 * (std::pow(sig_np1_12, 2.0) + std::pow(sig_np1_13, 2.0) +
                       std::pow(sig_np1_23, 2.0)));

  _kD[_qp] = _kDinf + (_kD0 - _kDinf) *
                           (_pow_sig_yield_m_kD / (_pow_sig_yield_m_kD + std::pow(sig_np1_vm, _m_kD)));

  ComputeLagrangianStressPK2::computeQpProperties();
}

void
PolyGentYield::computeQpPK2Stress()
{
  const RankTwoTensor & F_np1 = _F[_qp];
  const RankTwoTensor F_np1_inv = F_np1.inverse();
  const RankTwoTensor F_np1_inv_trans = F_np1_inv.transpose();

  const Real J_def = F_np1.det();
  _S[_qp] = F_np1_inv * _sigma[_qp] * F_np1_inv_trans * J_def;

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

      const RankTwoTensor Lie_bE_trial_n =
          -1.0 * _kD_old[_qp] * (_bE_old[_qp] - (3.0 / bE_n_inv_trace) * I);
      const RankTwoTensor GI_dot_trial_n = F_n_inv * Lie_bE_trial_n * F_n_inv.transpose();
      const RankTwoTensor GI_trial_nph = _GI_old[_qp] + 0.5 * GI_dot_trial_n * dt;
      const RankTwoTensor bE_trial_nph = F_trial_nph * GI_trial_nph * F_trial_nph.transpose();

      const RankTwoTensor cauchy_trial_nph = polyGentStress(bE_trial_nph, I, _G, _K, _Im);

      const Real sig_trial_nph_11 = cauchy_trial_nph(0, 0);
      const Real sig_trial_nph_22 = cauchy_trial_nph(1, 1);
      const Real sig_trial_nph_33 = cauchy_trial_nph(2, 2);
      const Real sig_trial_nph_12 = cauchy_trial_nph(0, 1);
      const Real sig_trial_nph_13 = cauchy_trial_nph(0, 2);
      const Real sig_trial_nph_23 = cauchy_trial_nph(1, 2);
      const Real sig_trial_nph_vm =
          std::sqrt(0.5 * (std::pow((sig_trial_nph_11 - sig_trial_nph_22), 2.0) +
                           std::pow((sig_trial_nph_22 - sig_trial_nph_33), 2.0) +
                           std::pow((sig_trial_nph_33 - sig_trial_nph_11), 2.0)) +
                    3.0 * (std::pow(sig_trial_nph_12, 2.0) + std::pow(sig_trial_nph_13, 2.0) +
                           std::pow(sig_trial_nph_23, 2.0)));

      const Real kD_trial_nph =
          _kDinf + (_kD0 - _kDinf) *
                       (_pow_sig_yield_m_kD /
                        (_pow_sig_yield_m_kD + std::pow(sig_trial_nph_vm, _m_kD)));

      const Real bE_trial_nph_inv_trace = bE_trial_nph.inverse().trace();
      const RankTwoTensor Lie_bE_trial_nph =
          -1.0 * kD_trial_nph * (bE_trial_nph - (3.0 / bE_trial_nph_inv_trace) * I);
      const RankTwoTensor GI_dot_trial_nph =
          F_trial_nph_inv * Lie_bE_trial_nph * F_trial_nph_inv.transpose();

      const RankTwoTensor GI_trial = _GI_old[_qp] + dt * GI_dot_trial_nph;
      const RankTwoTensor bE_trial = F_trial * GI_trial * F_trial.transpose();
      const RankTwoTensor cauchy_trial = polyGentStress(bE_trial, I, _G, _K, _Im);

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
PolyGentYield::computeQpCauchyStress()
{
  _cauchy_stress[_qp] = _sigma[_qp];
}
