#include "TLStressDivergenceNutrientOffDiag.h"

registerMooseObject("AntApp", TLStressDivergenceNutrientOffDiag);

InputParameters
TLStressDivergenceNutrientOffDiag::validParams()
{
  InputParameters params = TotalLagrangianStressDivergence::validParams();
  params.addRequiredCoupledVar("n", "Nutrient variable for mechanics off-diagonal coupling.");
  params.addClassDescription(
      "Off-diagonal Jacobian coupling of TL mechanics to nutrient via dPK1/dn.");
  return params;
}

TLStressDivergenceNutrientOffDiag::TLStressDivergenceNutrientOffDiag(
    const InputParameters & parameters)
  : TotalLagrangianStressDivergence(parameters),
    _n_var(getVar("n", 0)),
    _n_var_num(_n_var->number()),
    _dpk1_dn(getMaterialPropertyDerivative<RankTwoTensor>(_base_name + "pk1_stress",
                                                          coupledName("n")))
{
}

Real
TLStressDivergenceNutrientOffDiag::computeQpOffDiagJacobian(unsigned int jvar)
{
  if (jvar != _n_var_num)
    return 0.0;

  return gradTest(_alpha).doubleContraction(_dpk1_dn[_qp]) * _n_var->phi()[_j][_qp];
}
