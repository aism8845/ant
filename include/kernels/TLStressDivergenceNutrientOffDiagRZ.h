#pragma once

#include "TotalLagrangianStressDivergenceAxisymmetricCylindrical.h"

class TLStressDivergenceNutrientOffDiagRZ
  : public TotalLagrangianStressDivergenceAxisymmetricCylindrical
{
public:
  static InputParameters validParams();
  TLStressDivergenceNutrientOffDiagRZ(const InputParameters & parameters);

protected:
  Real computeQpResidual() override { return 0.0; }
  Real computeQpJacobian() override { return 0.0; }
  Real computeQpOffDiagJacobian(unsigned int jvar) override;

  const MooseVariable * _n_var;
  const unsigned int _n_var_num;
  const MaterialProperty<RankTwoTensor> & _dpk1_dn;
};
