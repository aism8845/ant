#include "TLStressDivergenceNutrientOffDiagRZ.h"

registerMooseObject("AntApp", TLStressDivergenceNutrientOffDiagRZ);

InputParameters
TLStressDivergenceNutrientOffDiagRZ::validParams()
{
  InputParameters params = TotalLagrangianStressDivergenceAxisymmetricCylindrical::validParams();
  params.addRequiredCoupledVar("n", "Nutrient variable for mechanics off-diagonal coupling.");
  params.addClassDescription(
      "Axisymmetric TL mechanics off-diagonal coupling to nutrient via dPK1/dn.");
  return params;
}

TLStressDivergenceNutrientOffDiagRZ::TLStressDivergenceNutrientOffDiagRZ(
    const InputParameters & parameters)
  : TotalLagrangianStressDivergenceAxisymmetricCylindrical(parameters),
    _n_var(getVar("n", 0)),
    _n_var_num(_n_var->number()),
    _dpk1_dn(getMaterialPropertyDerivative<RankTwoTensor>(_base_name + "pk1_stress",
                                                          coupledName("n")))
{
}

Real
TLStressDivergenceNutrientOffDiagRZ::computeQpOffDiagJacobian(unsigned int jvar)
{
  if (jvar != _n_var_num)
    return 0.0;

  return gradTest(_alpha).doubleContraction(_dpk1_dn[_qp]) * _n_var->phi()[_j][_qp];
}
