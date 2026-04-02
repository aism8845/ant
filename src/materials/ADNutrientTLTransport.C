#include "ADNutrientTLTransport.h"

#include <cmath>

#include "metaphysicl/raw_type.h"

registerMooseObject("AntApp", ADNutrientTLTransport);

namespace
{
template <typename T>
T
smooth_max(const T & x, const T & a, const Real eps)
{
  return 0.5 * (x + a + std::sqrt((x - a) * (x - a) + eps * eps));
}

template <typename T>
T
smooth_clamp(const T & x, const T & a, const T & b, const Real eps)
{
  return 0.5 * (smooth_max(x, a, eps) + b -
                std::sqrt((smooth_max(x, a, eps) - b) * (smooth_max(x, a, eps) - b) +
                          eps * eps));
}
}

InputParameters
ADNutrientTLTransport::validParams()
{
  InputParameters params = ADMaterial::validParams();

  params.addRequiredCoupledVar("n", "Nutrient variable.");
  params.addRequiredCoupledVar("disp_x", "x-displacement.");
  params.addRequiredCoupledVar("disp_y", "y-displacement.");
  params.addCoupledVar("disp_z", "Optional z-displacement for 3D.");

  params.addRequiredParam<MaterialPropertyName>(
      "phi_cell_ref",
      "Stateful material property for referential cell volume fraction.");

  params.addRequiredParam<Real>("D_nutrient", "Reference nutrient diffusivity.");
  params.addParam<Real>("D_floor", 1e-12, "Lower diffusivity floor.");
  params.addRequiredParam<Real>("gamma_n0", "Nutrient consumption prefactor.");
  params.addParam<Real>("nut_str", 0.0, "Half-activation concentration for nutrient gating.");
  params.addParam<Real>("m_nut", 2.0, "Nutrient Hill exponent.");
  params.addParam<bool>("use_crowding_diffusion", true,
                        "If true, scale diffusivity by current cell fraction.");
  params.addParam<Real>("phi_max", 0.999999, "Upper clamp for current cell fraction.");
  params.addParam<Real>("crowd_exp", 2.0, "Exponent used by the crowding law.");
  params.addParam<Real>("smooth_eps_c", 1e-12, "Smoothing epsilon for concentration clamps.");
  params.addParam<Real>("smooth_eps_D", 1e-12, "Smoothing epsilon for diffusivity floor.");

  params.addClassDescription(
      "Minimal AD total-Lagrangian nutrient transport coefficients for Cartesian 2D/3D.");
  return params;
}

ADNutrientTLTransport::ADNutrientTLTransport(const InputParameters & parameters)
  : ADMaterial(parameters),
    _n(adCoupledValue("n")),
    _ux(adCoupledValue("disp_x")),
    _uy(adCoupledValue("disp_y")),
    _grad_ux(adCoupledGradient("disp_x")),
    _grad_uy(adCoupledGradient("disp_y")),
    _has_uz(isCoupled("disp_z")),
    _uz(_has_uz ? &adCoupledValue("disp_z") : nullptr),
    _grad_uz(_has_uz ? &adCoupledGradient("disp_z") : nullptr),
    _ux_old(coupledValueOld("disp_x")),
    _uy_old(coupledValueOld("disp_y")),
    _grad_ux_old(coupledGradientOld("disp_x")),
    _grad_uy_old(coupledGradientOld("disp_y")),
    _ux_older(coupledValueOlder("disp_x")),
    _uy_older(coupledValueOlder("disp_y")),
    _grad_ux_older(coupledGradientOlder("disp_x")),
    _grad_uy_older(coupledGradientOlder("disp_y")),
    _uz_old(_has_uz ? &coupledValueOld("disp_z") : nullptr),
    _uz_older(_has_uz ? &coupledValueOlder("disp_z") : nullptr),
    _grad_uz_old(_has_uz ? &coupledGradientOld("disp_z") : nullptr),
    _grad_uz_older(_has_uz ? &coupledGradientOlder("disp_z") : nullptr),
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
    _phi_cell_nutr(declareProperty<Real>("phi_cell_nutr"))
{
}

void
ADNutrientTLTransport::computeQpProperties()
{
  auto build_F_ad = [&](const ADVariableGradient & gx,
                        const ADVariableGradient & gy,
                        const ADVariableGradient * gz) {
    ADRankTwoTensor F;
    F.zero();

    F(0, 0) = 1.0 + gx[_qp](0);
    F(0, 1) = gx[_qp](1);
    F(1, 0) = gy[_qp](0);
    F(1, 1) = 1.0 + gy[_qp](1);
    F(2, 2) = 1.0;

    if (_mesh.dimension() == 3)
    {
      F(0, 2) = gx[_qp](2);
      F(1, 2) = gy[_qp](2);
      if (gz)
      {
        F(2, 0) = (*gz)[_qp](0);
        F(2, 1) = (*gz)[_qp](1);
        F(2, 2) = 1.0 + (*gz)[_qp](2);
      }
    }

    return F;
  };

  auto build_F_old = [&](const VariableGradient & gx,
                         const VariableGradient & gy,
                         const VariableGradient * gz) {
    RankTwoTensor F;
    F.zero();

    F(0, 0) = 1.0 + gx[_qp](0);
    F(0, 1) = gx[_qp](1);
    F(1, 0) = gy[_qp](0);
    F(1, 1) = 1.0 + gy[_qp](1);
    F(2, 2) = 1.0;

    if (_mesh.dimension() == 3)
    {
      F(0, 2) = gx[_qp](2);
      F(1, 2) = gy[_qp](2);
      if (gz)
      {
        F(2, 0) = (*gz)[_qp](0);
        F(2, 1) = (*gz)[_qp](1);
        F(2, 2) = 1.0 + (*gz)[_qp](2);
      }
    }

    return F;
  };

  const ADRankTwoTensor F = build_F_ad(_grad_ux, _grad_uy, _grad_uz);
  const ADReal J = F.det();
  _J_nutr[_qp] = J;

  const RankTwoTensor F_old = (_t_step >= 1) ? build_F_old(_grad_ux_old, _grad_uy_old, _grad_uz_old)
                                             : RankTwoTensor::Identity();
  const RankTwoTensor F_older =
      (_t_step >= 2) ? build_F_old(_grad_ux_older, _grad_uy_older, _grad_uz_older)
                     : RankTwoTensor::Identity();

  const Real J_old = (_t_step >= 1) ? F_old.det() : 1.0;
  const Real J_older = (_t_step >= 2) ? F_older.det() : 1.0;

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
  phi = smooth_clamp(phi, ADReal(0.0), ADReal(_phi_max), _smooth_eps_c);

  ADReal D_phys = _D_nutrient;
  if (_use_crowding_diffusion)
  {
    const ADReal one_minus_phi =
        smooth_clamp(ADReal(1.0) - phi, ADReal(0.0), ADReal(1.0), _smooth_eps_c);
    D_phys = _D_nutrient * std::pow(one_minus_phi, _crowd_exp);
  }
  D_phys = smooth_max(D_phys, ADReal(_D_floor), _smooth_eps_D);

  const ADRankTwoTensor Finv = F.inverse();
  const ADRankTwoTensor A = Finv * Finv.transpose();

  ADRealTensorValue K;
  K.zero();
  for (unsigned int i = 0; i < 3; ++i)
    for (unsigned int j = 0; j < 3; ++j)
      K(i, j) = J * D_phys * A(i, j);
  _D_eff_nutr[_qp] = K;

  ADReal n_pos = smooth_max(_n[_qp], ADReal(0.0), _smooth_eps_c);
  const ADReal num = std::pow(n_pos, _m_nut);
  const ADReal den = num + ADReal(_pow_nut_str_m) + 1e-16;
  const ADReal fa = num / den;
  const ADReal gamma_local = _gamma_n0 * phi * fa;
  _n_source_ref_nutr[_qp] = J * gamma_local;

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
