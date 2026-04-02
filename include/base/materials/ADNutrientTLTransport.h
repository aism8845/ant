#pragma once

#include "ADMaterial.h"

class ADNutrientTLTransport : public ADMaterial
{
public:
  static InputParameters validParams();
  ADNutrientTLTransport(const InputParameters & parameters);

protected:
  void computeQpProperties() override;

  const ADVariableValue & _n;
  const ADVariableValue & _ux;
  const ADVariableValue & _uy;
  const ADVariableGradient & _grad_ux;
  const ADVariableGradient & _grad_uy;

  const bool _has_uz;
  const ADVariableValue * _uz;
  const ADVariableGradient * _grad_uz;

  const VariableValue & _ux_old;
  const VariableValue & _uy_old;
  const VariableGradient & _grad_ux_old;
  const VariableGradient & _grad_uy_old;
  const VariableValue & _ux_older;
  const VariableValue & _uy_older;
  const VariableGradient & _grad_ux_older;
  const VariableGradient & _grad_uy_older;

  const VariableValue * _uz_old;
  const VariableValue * _uz_older;
  const VariableGradient * _grad_uz_old;
  const VariableGradient * _grad_uz_older;

  const MaterialProperty<Real> & _phi_cell_ref;

  const Real _D_nutrient;
  const Real _D_floor;
  const Real _gamma_n0;
  const Real _nut_str;
  const Real _m_nut;
  const Real _phi_max;
  const Real _crowd_exp;
  const Real _smooth_eps_c;
  const Real _smooth_eps_D;
  const bool _use_crowding_diffusion;
  const Real _pow_nut_str_m;

  ADMaterialProperty<Real> & _J_nutr;
  ADMaterialProperty<Real> & _Jdot_nutr;
  ADMaterialProperty<RealTensorValue> & _D_eff_nutr;
  ADMaterialProperty<Real> & _n_source_ref_nutr;

  MaterialProperty<Real> & _J_nutr_pp;
  MaterialProperty<Real> & _Jdot_nutr_pp;
  MaterialProperty<RealTensorValue> & _D_eff_nutr_pp;
  MaterialProperty<Real> & _n_source_ref_nutr_pp;
  MaterialProperty<Real> & _D_phys_nutr;
  MaterialProperty<Real> & _phi_cell_nutr;
};
