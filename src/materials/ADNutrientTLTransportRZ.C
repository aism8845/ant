#include "ADNutrientTLTransportRZ.h"

#include <cmath>

#include "metaphysicl/raw_type.h"

registerMooseObject("AntApp", ADNutrientTLTransportRZ);

namespace
{
template <typename T>
T
smooth_max_rz(const T & x, const T & a, const Real eps)
{
  return 0.5 * (x + a + std::sqrt((x - a) * (x - a) + eps * eps));
}

template <typename T>
T
smooth_min_rz(const T & x, const T & b, const Real eps)
{
  return 0.5 * (x + b - std::sqrt((x - b) * (x - b) + eps * eps));
}

template <typename T>
T
smooth_clamp_rz(const T & x, const T & a, const T & b, const Real eps)
{
  return smooth_min_rz(smooth_max_rz(x, a, eps), b, eps);
}
}

InputParameters
ADNutrientTLTransportRZ::validParams()
{
  InputParameters params = ADMaterial::validParams();

  params.addRequiredCoupledVar("n", "Nutrient variable.");
  params.addRequiredCoupledVar("disp_r", "Radial displacement variable.");
  params.addRequiredCoupledVar("disp_z", "Axial displacement variable.");

  params.addParam<bool>("axisymmetric", true, "Include hoop stretch F_tt = 1 + u_r / r.");
  params.addParam<unsigned int>("radial_coord", 0, "Radial coordinate index.");
  params.addParam<Real>("r_eps", 1e-12, "Small-radius cutoff for u_r / r on axis.");

  params.addRequiredParam<MaterialPropertyName>("phi_cell_ref",
                                                "Referential cell-fraction property.");

  params.addRequiredParam<Real>("D_nutrient", "Reference nutrient diffusivity.");
  params.addParam<Real>("D_floor", 1e-12, "Lower diffusivity floor.");
  params.addRequiredParam<Real>("gamma_n0", "Nutrient consumption prefactor.");
  params.addParam<Real>("nut_str", 0.0, "Half-activation concentration for nutrient gating.");
  params.addParam<Real>("m_nut", 2.0, "Nutrient Hill exponent.");
  params.addParam<bool>("use_crowding_diffusion", true,
                        "Scale spatial diffusivity by current cell fraction.");
  params.addParam<Real>("phi_max", 0.999999, "Upper clamp for current cell fraction.");
  params.addParam<Real>("crowd_exp", 2.0, "Exponent in the crowding diffusivity law.");
  params.addParam<Real>("smooth_eps_c", 1e-12, "Smoothing epsilon for concentration clamps.");
  params.addParam<Real>("smooth_eps_D", 1e-12, "Smoothing epsilon for diffusivity floor.");
  params.addParam<bool>("safe_F_inv", false,
                        "If true, guard the inverse pull-back when det(F) is too small.");
  params.addParam<Real>("J_inv_floor", 1e-10, "Determinant floor used by safe_F_inv.");

  params.addClassDescription(
      "AD total-Lagrangian nutrient transport coefficients for axisymmetric RZ kinematics.");
  return params;
}

ADNutrientTLTransportRZ::ADNutrientTLTransportRZ(const InputParameters & parameters)
  : ADMaterial(parameters),
    _n(adCoupledValue("n")),
    _ur(adCoupledValue("disp_r")),
    _uz(adCoupledValue("disp_z")),
    _grad_ur(adCoupledGradient("disp_r")),
    _grad_uz(adCoupledGradient("disp_z")),
    _ur_old(coupledValueOld("disp_r")),
    _uz_old(coupledValueOld("disp_z")),
    _grad_ur_old(coupledGradientOld("disp_r")),
    _grad_uz_old(coupledGradientOld("disp_z")),
    _ur_older(coupledValueOlder("disp_r")),
    _uz_older(coupledValueOlder("disp_z")),
    _grad_ur_older(coupledGradientOlder("disp_r")),
    _grad_uz_older(coupledGradientOlder("disp_z")),
    _axisymmetric(getParam<bool>("axisymmetric")),
    _radial_coord(getParam<unsigned int>("radial_coord")),
    _r_eps(getParam<Real>("r_eps")),
    _axial_coord((_radial_coord == 0) ? 1 : 0),
    _phi_cell_ref(getMaterialPropertyOld<Real>(getParam<MaterialPropertyName>("phi_cell_ref"))),
    _D_nutrient(getParam<Real>("D_nutrient")),
    _D_floor(getParam<Real>("D_floor")),
    _gamma_n0(getParam<Real>("gamma_n0")),
    _nut_str(getParam<Real>("nut_str")),
    _m_nut(getParam<Real>("m_nut")),
    _phi_max(getParam<Real>("phi_max")),
    _crowd_exp(getParam<Real>("crowd_exp")),
    _smooth_eps_c(getParam<Real>("smooth_eps_c")),
    _smooth_eps_D(getParam<Real>("smooth_eps_D")),
    _use_crowding_diffusion(getParam<bool>("use_crowding_diffusion")),
    _safe_F_inv(getParam<bool>("safe_F_inv")),
    _J_inv_floor(getParam<Real>("J_inv_floor")),
    _pow_nut_str_m(std::pow(_nut_str, _m_nut)),
    _J_nutr(declareADProperty<Real>("J_nutr")),
    _Jdot_nutr(declareADProperty<Real>("Jdot_nutr")),
    _D_eff_nutr(declareADProperty<RealTensorValue>("D_eff_nutr")),
    _n_source_ref_nutr(declareADProperty<Real>("n_source_ref_nutr")),
    _J_nutr_pp(declareProperty<Real>("J_nutr_pp")),
    _Jdot_nutr_pp(declareProperty<Real>("Jdot_nutr_pp")),
    _D_eff_nutr_pp(declareProperty<RealTensorValue>("D_eff_nutr_pp")),
    _n_source_ref_nutr_pp(declareProperty<Real>("n_source_ref_nutr_pp")),
    _D_phys_nutr(declareProperty<Real>("D_phys_nutr")),
    _phi_cell_nutr(declareProperty<Real>("phi_cell_nutr")),
    _J_mech_nutr(declareProperty<Real>("J_mech_nutr")),
    _F_inv_guard_flag_nutr(declareProperty<Real>("F_inv_guard_flag_nutr"))
{
  if (_radial_coord > 2)
    mooseError("ADNutrientTLTransportRZ: 'radial_coord' must be 0, 1, or 2.");
}

void
ADNutrientTLTransportRZ::computeQpProperties()
{
  const Real r = _q_point[_qp](_radial_coord);

  const auto & gur = _grad_ur[_qp];
  const auto & guz = _grad_uz[_qp];

  const ADReal dur_dr = gur(_radial_coord);
  const ADReal dur_dz = gur(_axial_coord);
  const ADReal duz_dr = guz(_radial_coord);
  const ADReal duz_dz = guz(_axial_coord);

  ADRankTwoTensor F;
  F.zero();
  F(0, 0) = 1.0 + dur_dr;
  F(0, 1) = dur_dz;
  F(1, 0) = duz_dr;
  F(1, 1) = 1.0 + duz_dz;
  F(2, 2) = 1.0;

  if (_axisymmetric)
  {
    const ADReal ur_here = _ur[_qp];
    if (r > _r_eps)
      F(2, 2) = 1.0 + ur_here / r;
    else
      F(2, 2) = 1.0 + dur_dr;
  }

  const ADReal J = F.det();
  _J_nutr[_qp] = J;

  const Real J_raw = MetaPhysicL::raw_value(J);
  _J_mech_nutr[_qp] = J_raw;
  _F_inv_guard_flag_nutr[_qp] = 0.0;
  if ((!std::isfinite(J_raw) || J_raw <= 0.0) && !_safe_F_inv)
  {
    const long long elem_id = _current_elem ? static_cast<long long>(_current_elem->id()) : -1;
    mooseError("ADNutrientTLTransportRZ detF invalid: t=",
               _t,
               " dt=",
               _dt,
               " elem=",
               elem_id,
               " qp=",
               _qp,
               " detF=",
               J_raw);
  }

  const bool guarded_inv = _safe_F_inv && (!std::isfinite(J_raw) || J_raw < _J_inv_floor);
  if (guarded_inv)
    _F_inv_guard_flag_nutr[_qp] = 1.0;
  const ADReal J_eff = guarded_inv ? ADReal(_J_inv_floor) : J;

  auto J_from_old = [&](const VariableValue & urv,
                        const VariableValue & uzv,
                        const VariableGradient & gurv,
                        const VariableGradient & guzv) -> Real
  {
    const auto & gur_old = gurv[_qp];
    const auto & guz_old = guzv[_qp];

    const Real dur_dr_o = gur_old(_radial_coord);
    const Real dur_dz_o = gur_old(_axial_coord);
    const Real duz_dr_o = guz_old(_radial_coord);
    const Real duz_dz_o = guz_old(_axial_coord);

    RankTwoTensor Fold;
    Fold.zero();
    Fold(0, 0) = 1.0 + dur_dr_o;
    Fold(0, 1) = dur_dz_o;
    Fold(1, 0) = duz_dr_o;
    Fold(1, 1) = 1.0 + duz_dz_o;
    Fold(2, 2) = 1.0;

    if (_axisymmetric)
    {
      const Real ur_o = urv[_qp];
      if (r > _r_eps)
        Fold(2, 2) = 1.0 + ur_o / r;
      else
        Fold(2, 2) = 1.0 + dur_dr_o;
    }

    return Fold.det();
  };

  const Real J_old =
      (_t_step >= 1) ? J_from_old(_ur_old, _uz_old, _grad_ur_old, _grad_uz_old) : 1.0;
  const Real J_older =
      (_t_step >= 2) ? J_from_old(_ur_older, _uz_older, _grad_ur_older, _grad_uz_older) : 1.0;

  ADReal Jdot = 0.0;
  if (_t_step >= 1)
  {
    if (_t_step == 1 || _dt_old <= 0.0)
      Jdot = (J - J_old) / _dt;
    else
    {
      const Real rdt = _dt / _dt_old;
      const Real a0 = (1.0 + 2.0 * rdt) / (1.0 + rdt);
      const Real a1 = -(1.0 + rdt);
      const Real a2 = (rdt * rdt) / (1.0 + rdt);
      Jdot = (a0 * J + a1 * J_old + a2 * J_older) / _dt;
    }
  }
  _Jdot_nutr[_qp] = Jdot;

  const Real phi_ref = _phi_cell_ref[_qp];
  const ADReal denom = (J - 1.0) * phi_ref + 1.0;
  ADReal phi = (J * phi_ref) / denom;
  phi = smooth_clamp_rz(phi, ADReal(0.0), ADReal(_phi_max), _smooth_eps_c);

  ADReal D_phys = _D_nutrient;
  if (_use_crowding_diffusion)
  {
    const ADReal one_minus_phi =
        smooth_clamp_rz(ADReal(1.0) - phi, ADReal(0.0), ADReal(1.0), _smooth_eps_c);
    D_phys = _D_nutrient * std::pow(one_minus_phi, _crowd_exp);
  }
  D_phys = smooth_max_rz(D_phys, ADReal(_D_floor), _smooth_eps_D);

  ADRankTwoTensor Finv;
  if (guarded_inv)
    Finv = ADRankTwoTensor::Identity();
  else
    Finv = F.inverse();
  const ADRankTwoTensor A = Finv * Finv.transpose();

  ADRealTensorValue K;
  K.zero();
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      K(i, j) = guarded_inv ? ADReal(0.0) : J_eff * D_phys * A(i, j);
  _D_eff_nutr[_qp] = K;

  ADReal n_pos = smooth_max_rz(_n[_qp], ADReal(0.0), _smooth_eps_c);
  const ADReal num = std::pow(n_pos, _m_nut);
  const ADReal den = num + ADReal(_pow_nut_str_m) + 1e-16;
  const ADReal fa = den > 0.0 ? num / den : ADReal(0.0);
  const ADReal gamma_local = _gamma_n0 * phi * fa;
  _n_source_ref_nutr[_qp] = J_eff * gamma_local;

  _J_nutr_pp[_qp] = MetaPhysicL::raw_value(_J_nutr[_qp]);
  _Jdot_nutr_pp[_qp] = MetaPhysicL::raw_value(_Jdot_nutr[_qp]);
  _n_source_ref_nutr_pp[_qp] = MetaPhysicL::raw_value(_n_source_ref_nutr[_qp]);
  _D_phys_nutr[_qp] = MetaPhysicL::raw_value(D_phys);
  _phi_cell_nutr[_qp] = MetaPhysicL::raw_value(phi);

  RealTensorValue K_pp;
  K_pp.zero();
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      K_pp(i, j) = MetaPhysicL::raw_value(_D_eff_nutr[_qp](i, j));
  _D_eff_nutr_pp[_qp] = K_pp;
}
