#pragma once

#include "ComputeLagrangianStressPK2.h"
#include "RankTwoTensor.h"

class PolyGentYield : public ComputeLagrangianStressPK2
{
public:
  static InputParameters validParams();
  PolyGentYield(const InputParameters & parameters);

protected:
  void initQpStatefulProperties() override;
  void computeQpPK2Stress() override;
  void computeQpProperties() override;
  void computeQpCauchyStress() override;

private:
  const RankTwoTensor I = RankTwoTensor::Identity();

  Real dt;
  RankTwoTensor F_n_inv;
  Real bE_n_inv_trace;

  // Input parameters
  const Real _G;
  const Real _K;
  const Real _Im;
  const Real _kD0;
  const Real _kDinf;
  const Real _sig_yield;
  const Real _m_kD;
  const Real _epsilon;

  // Derived constant
  Real _pow_sig_yield_m_kD;

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
  MaterialProperty<Real> & _JE_gel;
  MaterialProperty<Real> & _J_gel;
  MaterialProperty<Real> & _J_total;
  MaterialProperty<RankTwoTensor> & _sigma;
  MaterialProperty<Real> & _kD;
  const MaterialProperty<Real> & _kD_old;
};
