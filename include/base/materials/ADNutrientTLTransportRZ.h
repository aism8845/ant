#pragma once

#include "ADMaterial.h"

class ADNutrientTLTransportRZ : public ADMaterial
{
public:
  static InputParameters validParams();
  ADNutrientTLTransportRZ(const InputParameters & parameters);

protected:
  void computeQpProperties() override;

  const ADVariableValue & _n;
  const ADVariableValue & _ur;
  const ADVariableValue & _uz;
  const ADVariableGradient & _grad_ur;
  const ADVariableGradient & _grad_uz;

  const VariableValue & _ur_old;
  const VariableValue & _uz_old;
  const VariableGradient & _grad_ur_old;
  const VariableGradient & _grad_uz_old;
  const VariableValue & _ur_older;
  const VariableValue & _uz_older;
  const VariableGradient & _grad_ur_older;
  const VariableGradient & _grad_uz_older;

  const bool _axisymmetric;
  const unsigned int _radial_coord;
  const Real _r_eps;
  const unsigned int _axial_coord;

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
  const bool _safe_F_inv;
  const Real _J_inv_floor;
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
  MaterialProperty<Real> & _J_mech_nutr;
  MaterialProperty<Real> & _F_inv_guard_flag_nutr;
};
