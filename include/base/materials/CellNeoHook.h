#pragma once

#include "ComputeLagrangianStressPK2.h"
#include "RankTwoTensor.h"
#include "libmesh/tensor_value.h"

class CellNeoHook : public ComputeLagrangianStressPK2
{
public:
  static InputParameters validParams();
  CellNeoHook(const InputParameters & parameters);

protected:
  virtual void initQpStatefulProperties() override;
  virtual void computeQpPK2Stress() override;
  virtual void computeQpProperties() override;
  virtual void computeQpCauchyStress() override;

private:
  const RankTwoTensor I = RankTwoTensor::Identity();

  Real dt;
  Real k_exp0;

  // Shared-path activity factor. The old file carried optional nutrient hooks,
  // but for the shared implementation this factor is 1.
  const Real fa = 1.0;

  RankTwoTensor F_n_inv;
  Real bE_n_inv_trace;

  // Input parameters
  const Real _G;
  const Real _K;
  const Real _k_exp_max;
  const Real _t_str;
  const Real _m_exp;
  const Real _press_str;
  const Real _k_T1_max;
  const Real _chi_str;
  const Real _m_T1;
  const Real _epsilon;

  // Derived constants
  Real pow_t_str_m;
  Real pow_chi_str_m;

  // Kinematics
  const MaterialProperty<RankTwoTensor> & _F;
  const MaterialProperty<RankTwoTensor> & _F_old;

  // Stateful elastic tensors
  MaterialProperty<RankTwoTensor> & _GI;
  const MaterialProperty<RankTwoTensor> & _GI_old;
  MaterialProperty<RankTwoTensor> & _bE;
  const MaterialProperty<RankTwoTensor> & _bE_old;

  // Output properties
  MaterialProperty<Real> & _JE;
  MaterialProperty<Real> & _JE_cell;
  MaterialProperty<Real> & _J_cell;
  MaterialProperty<Real> & _J_total;
  MaterialProperty<RankTwoTensor> & _sigma;
  MaterialProperty<Real> & _press;
  MaterialProperty<Real> & _eta;
  const MaterialProperty<Real> & _eta_old;
  MaterialProperty<Real> & _chi;
  MaterialProperty<Real> & _ke;
  const MaterialProperty<Real> & _ke_old;
  MaterialProperty<Real> & _kT1;
  const MaterialProperty<Real> & _kT1_old;
};
