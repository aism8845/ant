#pragma once

#include "ComputeLagrangianStressPK1.h"
#include "DerivativeMaterialInterface.h"

class CellGelMixtureNut2 : public DerivativeMaterialInterface<ComputeLagrangianStressPK1>
{
public:
  static InputParameters validParams();
  CellGelMixtureNut2(const InputParameters & parameters);

protected:
  void initQpStatefulProperties() override;
  void computeQpPK1Stress() override;
  void computeQpProperties() override;
  void computeQpCauchyStress() override;

private:
  const RankTwoTensor _I = RankTwoTensor::Identity();

  // Constitutive parameters
  const Real _G_cell;
  const Real _K_cell;
  const Real _ke0;
  const Real _p_star;
  const Real _k_t;
  const Real _G_gel;
  const Real _Im;
  const Real _kD0;
  const Real _kDinf;
  const Real _sig_yield;
  const Real _n_kD;
  const Real _phi_cell_0;
  const Real _k_ap;
  const Real _epsilon;
  const Real _press_gate_smooth;
  const Real _nut_str;
  const Real _m_nut;
  const Real _smooth_eps_n;
  const Real _phi_bulk_floor;
  const Real _gent_lock_floor;
  const Real _phi_max;
  const bool _gate_gp_on_ke;
  const bool _gate_fa_on_ke;
  const bool _gate_phi_on_ke;
  const Real _phi_gate_start;
  const Real _phi_gate_end;

  // Optional ramp controls
  const Real _t_str;
  const Real _m_exp;
  const bool _use_smoothstep_ramp;
  const Real _ramp_time;

  // Optional coupled fields
  const bool _has_phi_ref_ic;
  const VariableValue * _phi_ref_ic;
  const bool _has_n;
  const VariableValue * _n;

  // Kinematics
  const MaterialProperty<RankTwoTensor> & _F;
  const MaterialProperty<RankTwoTensor> & _F_old;

  // Internal state
  MaterialProperty<Real> & _phi_cell;
  const MaterialProperty<Real> & _phi_cell_old;
  MaterialProperty<Real> & _phi_cell_ref;
  const MaterialProperty<Real> & _phi_cell_ref_old;

  MaterialProperty<RankTwoTensor> & _GI_cell;
  const MaterialProperty<RankTwoTensor> & _GI_cell_old;
  MaterialProperty<RankTwoTensor> & _bE_cell;
  const MaterialProperty<RankTwoTensor> & _bE_cell_old;

  MaterialProperty<RankTwoTensor> & _GI_pmat;
  const MaterialProperty<RankTwoTensor> & _GI_pmat_old;
  MaterialProperty<RankTwoTensor> & _bE_pmat;
  const MaterialProperty<RankTwoTensor> & _bE_pmat_old;

  // Outputs
  MaterialProperty<Real> & _press_cell;
  MaterialProperty<RankTwoTensor> & _sigma_cell;
  MaterialProperty<RankTwoTensor> & _sigma_pmat;
  MaterialProperty<Real> & _eta;
  const MaterialProperty<Real> & _eta_old;
  MaterialProperty<Real> & _ke;
  const MaterialProperty<Real> & _ke_old;
  MaterialProperty<Real> & _kT1;
  MaterialProperty<Real> & _k_diss;
  const MaterialProperty<Real> & _k_diss_old;
  MaterialProperty<Real> & _J_mix;
  MaterialProperty<Real> & _kh;
  MaterialProperty<Real> & _fa;
  MaterialProperty<Real> & _gp;
  MaterialProperty<Real> & _g_phi_ke;
  MaterialProperty<Real> & _gate_total;
  MaterialProperty<Real> & _pressure;
  MaterialProperty<RankTwoTensor> & _dpk1_stress_dn;

  const Real _pow_t_str_m;
  const Real _pow_nut_str_m;
};
