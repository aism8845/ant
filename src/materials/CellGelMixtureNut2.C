#include "CellGelMixtureNut2.h"

#include <algorithm>
#include <cmath>

registerMooseObject("AntApp", CellGelMixtureNut2);

namespace
{
constexpr Real tiny_value = 1e-12;

Real
smoothPositive(const Real x, const Real eps)
{
  return 0.5 * (x + std::sqrt(x * x + eps * eps));
}

Real
smoothMax(const Real a, const Real b, const Real eps)
{
  return 0.5 * (a + b + std::sqrt((a - b) * (a - b) + eps * eps));
}

Real
smoothNegative(const Real x, const Real eps)
{
  return 0.5 * (x - std::sqrt(x * x + eps * eps));
}

Real
clamp01(const Real x)
{
  return std::max(0.0, std::min(1.0, x));
}

Real
smoothstep5(const Real x)
{
  return x * x * x * (10.0 + x * (-15.0 + 6.0 * x));
}

Real
faGate(const Real n_raw,
       const Real nut_str,
       const Real m_nut,
       const Real pow_nut_str_m,
       const Real smooth_eps_n)
{
  if (nut_str <= 0.0 || m_nut <= 0.0)
    return 1.0;

  const Real n_pos = smoothPositive(n_raw, smooth_eps_n);
  const Real num = std::pow(n_pos, m_nut);
  const Real den = num + pow_nut_str_m;
  return den > 0.0 ? num / den : 0.0;
}

Real
gpGate(const Real p_cell, const Real p_star, const Real press_gate_smooth)
{
  const Real width = std::max(press_gate_smooth, tiny_value);
  const Real p_comp = smoothNegative(p_cell, width);
  return std::exp(-std::pow(p_comp / p_star, 2.0));
}

Real
clampPhi(const Real phi, const Real eps, const Real phi_max = 1.0 - tiny_value)
{
  const Real phi_lo = eps;
  const Real phi_hi = std::max(phi_max, phi_lo);
  const Real width = std::max(10.0 * eps, 1e-6 * std::max(phi_hi - phi_lo, tiny_value));

  auto lower_clamp = [&](const Real x) {
    if (x <= phi_lo - width)
      return phi_lo;
    if (x >= phi_lo + width)
      return x;

    const Real s = clamp01((x - (phi_lo - width)) / (2.0 * width));
    const Real q = smoothstep5(s);
    return (1.0 - q) * phi_lo + q * x;
  };

  auto upper_clamp = [&](const Real x) {
    if (x <= phi_hi - width)
      return x;
    if (x >= phi_hi + width)
      return phi_hi;

    const Real s = clamp01((x - (phi_hi - width)) / (2.0 * width));
    const Real q = smoothstep5(s);
    return (1.0 - q) * x + q * phi_hi;
  };

  return upper_clamp(lower_clamp(phi));
}

Real
phiGateKe(const Real phi, const Real phi_gate_start, const Real phi_gate_end, const Real phi_max)
{
  const Real phi_safe = clampPhi(phi, tiny_value, phi_max);
  if (phi_safe <= phi_gate_start)
    return 1.0;
  if (phi_safe >= phi_gate_end)
    return 0.0;

  const Real s = clamp01((phi_safe - phi_gate_start) /
                         std::max(phi_gate_end - phi_gate_start, tiny_value));
  return 1.0 - smoothstep5(s);
}

Real
growthRamp(const Real t,
           const bool use_smoothstep_ramp,
           const Real ramp_time,
           const Real t_str,
           const Real m_exp,
           const Real pow_t_str_m)
{
  if (use_smoothstep_ramp)
  {
    const Real T = std::max(ramp_time, 1e-16);
    return smoothstep5(clamp01(t / T));
  }

  if (t_str > 0.0 && m_exp > 0.0)
  {
    const Real t_pos = std::max(t, 0.0);
    const Real num = std::pow(t_pos, m_exp);
    const Real den = num + pow_t_str_m;
    return den > 0.0 ? num / den : 1.0;
  }

  return 1.0;
}

Real
jeFromBE(const RankTwoTensor & bE)
{
  return std::sqrt(smoothPositive(bE.det(), tiny_value * tiny_value));
}

// Cell: compressible neo-Hookean.
RankTwoTensor
cellStress(const RankTwoTensor & bE, const RankTwoTensor & I, const Real G, const Real K)
{
  const Real JE = jeFromBE(bE);
  return (G / std::pow(JE, 5.0 / 3.0)) * bE.deviatoric() + K * (JE - 1.0) * I;
}

// Gel: Gent shear response with phi-dependent effective bulk modulus.
RankTwoTensor
gelStress(const RankTwoTensor & bE,
          const RankTwoTensor & I,
          const Real G,
          const Real Kg_eff,
          const Real Im,
          const Real gent_lock_floor)
{
  const Real JE = jeFromBE(bE);
  const Real I1bar = std::pow(JE, -2.0 / 3.0) * bE.trace();
  const Real gent_gap = smoothMax(Im - I1bar, gent_lock_floor, gent_lock_floor);
  const Real gent_factor = (Im - 3.0) / gent_gap;
  return (G / std::pow(JE, 5.0 / 3.0)) * gent_factor * bE.deviatoric() + Kg_eff * (JE - 1.0) * I;
}

Real
vonMises(const RankTwoTensor & sigma)
{
  const Real s11 = sigma(0, 0);
  const Real s22 = sigma(1, 1);
  const Real s33 = sigma(2, 2);
  const Real s12 = sigma(0, 1);
  const Real s13 = sigma(0, 2);
  const Real s23 = sigma(1, 2);
  const Real normal =
      0.5 * (std::pow(s11 - s22, 2.0) + std::pow(s22 - s33, 2.0) + std::pow(s33 - s11, 2.0));
  const Real shear = 3.0 * (s12 * s12 + s13 * s13 + s23 * s23);
  return std::sqrt(std::max(normal + shear, 0.0));
}

Real
ellisRate(const Real sigma_vm,
          const Real kD0,
          const Real kDinf,
          const Real sig_yield,
          const Real n_kD)
{
  const Real ratio = sigma_vm / std::max(sig_yield, tiny_value);
  return kDinf + (kD0 - kDinf) / (1.0 + std::pow(ratio, n_kD));
}

Real
kgEffFromPhi(const Real G_gel, const Real phi_cell, const Real phi_bulk_floor, const Real phi_max)
{
  const Real phi_safe = smoothMax(clampPhi(phi_cell, tiny_value, phi_max), phi_bulk_floor, phi_bulk_floor);
  return (4.0 / 3.0) * G_gel * (1.0 - phi_safe) / phi_safe;
}

Real
phiDot(const Real phi_cell, const Real ke, const Real k_ap)
{
  return phi_cell * (1.0 - phi_cell) * ke - phi_cell * k_ap;
}

Real
phiFromRefAndJ(const Real phi_ref, const Real J, const Real phi_max)
{
  const Real phi_ref_safe = clampPhi(phi_ref, tiny_value, phi_max);
  const Real denom = smoothMax((J - 1.0) * phi_ref_safe + 1.0, tiny_value, tiny_value);
  const Real phi = (J * phi_ref_safe) / denom;
  return clampPhi(phi, tiny_value, phi_max);
}
}

InputParameters
CellGelMixtureNut2::validParams()
{
  InputParameters params = DerivativeMaterialInterface<ComputeLagrangianStressPK1>::validParams();
  params.addClassDescription("Cell/gel mixture with manuscript Eq. (12)-(15) constitutive law.");

  params.addRequiredParam<Real>("G_cell", "Eq. (4) cell shear modulus G_c.");
  params.addRequiredParam<Real>("K_cell", "Eq. (4) cell bulk modulus K_c.");
  params.addParam<Real>("ke0", -1.0, "Eq. (1) baseline growth rate ke0.");
  params.addParam<Real>("k_exp_max", -1.0, "Deprecated alias for ke0.");
  params.addParam<Real>("p_star", -1.0, "Eq. (1) compression scale p_star.");
  params.addParam<Real>("press_str", -1.0, "Deprecated alias for p_star.");
  params.addParam<Real>("k_t", -1.0, "Eq. (5) constant T1/remodeling rate k_t.");
  params.addParam<Real>("k_T1_max", -1.0, "Deprecated alias for k_t.");

  params.addRequiredParam<Real>("G_gel", "Eq. (7) gel shear modulus G_g.");
  params.addParam<Real>("Im", 100.0, "Eq. (7) Gent limiting invariant I_m.");
  params.addParam<Real>("kD0", -1.0, "Eq. (9) low-stress relaxation rate kD0.");
  params.addParam<Real>("k_diss_0", -1.0, "Deprecated alias for kD0.");
  params.addParam<Real>("kDinf", -1.0, "Eq. (9) high-stress relaxation rate kDinf.");
  params.addParam<Real>("sig_yield", 1.0, "Eq. (9) yield stress sigma_yield.");
  params.addParam<Real>("n_kD", 1.0, "Eq. (9) Ellis exponent n.");
  params.addParam<Real>("m_kD", -1.0, "Deprecated alias for n_kD.");

  params.addRequiredParam<Real>("phi_cell_0", "Initial referential cell volume fraction phi_ref.");
  params.addParam<Real>("k_ap", 0.0, "Referential cell-fraction loss rate.");
  params.addParam<Real>("epsilon", 1e-8, "Finite-difference perturbation for dS/dE.");

  params.addParam<Real>(
      "t_str",
      0.0,
      "Optional legacy ramp timescale (active only when t_str>0 and m_exp>0).");
  params.addParam<Real>("m_exp", 1.0, "Optional legacy ramp exponent.");
  params.addParam<bool>("use_smoothstep_ramp",
                        false,
                        "If true, apply smoothstep5 ramp to ke0 up to ramp_time.");
  params.addParam<Real>("ramp_time", 0.0, "Optional smoothstep ramp horizon.");

  params.addParam<Real>("press_gate_smooth",
                        1e-3,
                        "Optional smoothing half-width around zero pressure for gp(p).");
  params.addParam<Real>("chi_str", 0.0, "Deprecated no-op.");
  params.addParam<Real>("m_T1", 0.0, "Deprecated no-op.");
  params.addParam<Real>("nut_str", 0.2, "Half-activation concentration for nutrient gate fa(n).");
  params.addParam<Real>("m_nut", 2.0, "Hill exponent for nutrient gate fa(n).");
  params.addParam<Real>(
      "smooth_eps_n",
      1e-8,
      "Smoothing epsilon used in smooth positive nutrient surrogates for fa(n) and d(pk1_stress)/dn.");
  params.addParam<Real>(
      "phi_bulk_floor",
      1e-3,
      "Smooth lower floor used in the gel bulk-modulus regularization Kg_eff(phi).");
  params.addParam<Real>(
      "gent_lock_floor",
      1e-6,
      "Smooth positive floor used for the Gent locking gap Im - I1bar.");
  params.addParam<Real>("phi_max",
                        0.999999,
                        "Upper clamp enforced on referential and current cell fractions in mechanics.");
  params.addParam<bool>("gate_gp_on_ke", true, "Apply pressure gate gp(p_cell) to ke.");
  params.addParam<bool>("gate_fa_on_ke", true, "Apply nutrient gate fa(n) to ke.");
  params.addParam<bool>(
      "gate_phi_on_ke", false, "Apply smooth crowding gate g_phi(phi_cell) to ke.");
  params.addParam<Real>("phi_gate_start",
                        0.70,
                        "Current cell fraction where the optional ke phi-gate starts.");
  params.addParam<Real>("phi_gate_end",
                        0.88,
                        "Current cell fraction where the optional ke phi-gate reaches zero.");
  params.addParam<bool>("gate_gp_on_kh", false, "Deprecated no-op.");
  params.addParam<bool>("gate_fa_on_kh", false, "Deprecated no-op.");

  params.addCoupledVar("n", "Nutrient coupling used by fa(n), gp(p), and d(pk1_stress)/dn.");
  params.addCoupledVar("phi_ref_ic", "Optional initial referential-fraction texture field.");

  return params;
}

CellGelMixtureNut2::CellGelMixtureNut2(const InputParameters & parameters)
  : DerivativeMaterialInterface<ComputeLagrangianStressPK1>(parameters),
    _G_cell(getParam<Real>("G_cell")),
    _K_cell(getParam<Real>("K_cell")),
    _ke0(getParam<Real>("ke0") > 0.0 ? getParam<Real>("ke0") : getParam<Real>("k_exp_max")),
    _p_star(getParam<Real>("p_star") > 0.0 ? getParam<Real>("p_star")
                                           : getParam<Real>("press_str")),
    _k_t(getParam<Real>("k_t") >= 0.0 ? getParam<Real>("k_t") : getParam<Real>("k_T1_max")),
    _G_gel(getParam<Real>("G_gel")),
    _Im(getParam<Real>("Im")),
    _kD0(getParam<Real>("kD0") >= 0.0 ? getParam<Real>("kD0") : getParam<Real>("k_diss_0")),
    _kDinf(getParam<Real>("kDinf") >= 0.0 ? getParam<Real>("kDinf")
                                          : (getParam<Real>("kD0") >= 0.0
                                                 ? getParam<Real>("kD0")
                                                 : getParam<Real>("k_diss_0"))),
    _sig_yield(getParam<Real>("sig_yield")),
    _n_kD(getParam<Real>("m_kD") > 0.0 ? getParam<Real>("m_kD") : getParam<Real>("n_kD")),
    _phi_cell_0(getParam<Real>("phi_cell_0")),
    _k_ap(getParam<Real>("k_ap")),
    _epsilon(getParam<Real>("epsilon")),
    _press_gate_smooth(getParam<Real>("press_gate_smooth")),
    _nut_str(getParam<Real>("nut_str")),
    _m_nut(getParam<Real>("m_nut")),
    _smooth_eps_n(getParam<Real>("smooth_eps_n")),
    _phi_bulk_floor(getParam<Real>("phi_bulk_floor")),
    _gent_lock_floor(getParam<Real>("gent_lock_floor")),
    _phi_max(getParam<Real>("phi_max")),
    _gate_gp_on_ke(getParam<bool>("gate_gp_on_ke")),
    _gate_fa_on_ke(getParam<bool>("gate_fa_on_ke")),
    _gate_phi_on_ke(getParam<bool>("gate_phi_on_ke")),
    _phi_gate_start(getParam<Real>("phi_gate_start")),
    _phi_gate_end(getParam<Real>("phi_gate_end")),
    _t_str(getParam<Real>("t_str")),
    _m_exp(getParam<Real>("m_exp")),
    _use_smoothstep_ramp(getParam<bool>("use_smoothstep_ramp")),
    _ramp_time(getParam<Real>("ramp_time")),
    _has_phi_ref_ic(isCoupled("phi_ref_ic")),
    _phi_ref_ic(_has_phi_ref_ic ? &coupledValue("phi_ref_ic") : nullptr),
    _has_n(isCoupled("n")),
    _n(_has_n ? &coupledValue("n") : nullptr),
    _F(getMaterialPropertyByName<RankTwoTensor>(_base_name + "deformation_gradient")),
    _F_old(getMaterialPropertyOldByName<RankTwoTensor>(_base_name + "deformation_gradient")),
    _phi_cell(declareProperty<Real>("phi_cell")),
    _phi_cell_old(getMaterialPropertyOld<Real>("phi_cell")),
    _phi_cell_ref(declareProperty<Real>("phi_cell_ref")),
    _phi_cell_ref_old(getMaterialPropertyOld<Real>("phi_cell_ref")),
    _GI_cell(declareProperty<RankTwoTensor>("GI_cell")),
    _GI_cell_old(getMaterialPropertyOld<RankTwoTensor>("GI_cell")),
    _bE_cell(declareProperty<RankTwoTensor>("bE_cell")),
    _bE_cell_old(getMaterialPropertyOld<RankTwoTensor>("bE_cell")),
    _GI_pmat(declareProperty<RankTwoTensor>("GI_pmat")),
    _GI_pmat_old(getMaterialPropertyOld<RankTwoTensor>("GI_pmat")),
    _bE_pmat(declareProperty<RankTwoTensor>("bE_pmat")),
    _bE_pmat_old(getMaterialPropertyOld<RankTwoTensor>("bE_pmat")),
    _press_cell(declareProperty<Real>("press_cell")),
    _sigma_cell(declareProperty<RankTwoTensor>("sigma_cell")),
    _sigma_pmat(declareProperty<RankTwoTensor>("sigma_pmat")),
    _eta(declareProperty<Real>("eta")),
    _eta_old(getMaterialPropertyOld<Real>("eta")),
    _ke(declareProperty<Real>("ke")),
    _ke_old(getMaterialPropertyOld<Real>("ke")),
    _kT1(declareProperty<Real>("kT1")),
    _k_diss(declareProperty<Real>("k_diss")),
    _k_diss_old(getMaterialPropertyOld<Real>("k_diss")),
    _J_mix(declareProperty<Real>("J_mix")),
    _kh(declareProperty<Real>("kh")),
    _fa(declareProperty<Real>("fa")),
    _gp(declareProperty<Real>("gp")),
    _g_phi_ke(declareProperty<Real>("g_phi_ke")),
    _gate_total(declareProperty<Real>("gate_total")),
    _pressure(declareProperty<Real>("pressure")),
    _dpk1_stress_dn(declarePropertyDerivative<RankTwoTensor>("pk1_stress", "n")),
    _pow_t_str_m(std::pow(std::max(_t_str, 1e-16), _m_exp)),
    _pow_nut_str_m(std::pow(std::max(_nut_str, 1e-16), std::max(_m_nut, 1e-16)))
{
  if (_ke0 < 0.0)
    mooseError("CellGelMixtureNut2 requires ke0 (or deprecated alias k_exp_max).");
  if (_p_star <= 0.0)
    mooseError("CellGelMixtureNut2 requires p_star > 0 (or deprecated alias press_str).");
  if (_k_t < 0.0)
    mooseError("CellGelMixtureNut2 requires k_t >= 0 (or deprecated alias k_T1_max).");
  if (_kD0 < 0.0)
    mooseError("CellGelMixtureNut2 requires kD0 >= 0 (or deprecated alias k_diss_0).");
  if (_kDinf < 0.0)
    mooseError("CellGelMixtureNut2 requires kDinf >= 0.");
  if (_sig_yield <= 0.0)
    mooseError("CellGelMixtureNut2 requires sig_yield > 0.");
  if (_n_kD <= 0.0)
    mooseError("CellGelMixtureNut2 requires n_kD > 0 (or deprecated alias m_kD).");
  if (_Im <= 3.0)
    mooseError("CellGelMixtureNut2 requires Im > 3.");
  if (_press_gate_smooth < 0.0)
    mooseError("CellGelMixtureNut2 requires press_gate_smooth >= 0.");
  if (_has_n && _nut_str <= 0.0)
    mooseError("CellGelMixtureNut2 requires nut_str > 0 when nutrient coupling is enabled.");
  if (_has_n && _m_nut <= 0.0)
    mooseError("CellGelMixtureNut2 requires m_nut > 0 when nutrient coupling is enabled.");
  if (_smooth_eps_n <= 0.0)
    mooseError("CellGelMixtureNut2 requires smooth_eps_n > 0.");
  if (_phi_bulk_floor <= 0.0 || _phi_bulk_floor >= 1.0)
    mooseError("CellGelMixtureNut2 requires 0 < phi_bulk_floor < 1.");
  if (_gent_lock_floor <= 0.0)
    mooseError("CellGelMixtureNut2 requires gent_lock_floor > 0.");
  if (_phi_max <= tiny_value || _phi_max >= 1.0)
    mooseError("CellGelMixtureNut2 requires 0 < phi_max < 1.");
  if (_phi_bulk_floor >= _phi_max)
    mooseError("CellGelMixtureNut2 requires phi_bulk_floor < phi_max.");
  if (_phi_gate_start < 0.0 || _phi_gate_start >= _phi_max)
    mooseError("CellGelMixtureNut2 requires 0 <= phi_gate_start < phi_max.");
  if (_phi_gate_end <= _phi_gate_start || _phi_gate_end > _phi_max)
    mooseError("CellGelMixtureNut2 requires phi_gate_start < phi_gate_end <= phi_max.");
}

void
CellGelMixtureNut2::initQpStatefulProperties()
{
  Real phi_init = _phi_cell_0;
  if (_has_phi_ref_ic)
    phi_init = (*_phi_ref_ic)[_qp];

  phi_init = clampPhi(phi_init, tiny_value, _phi_max);
  const Real ke_drive_0 =
      _ke0 * growthRamp(0.0, _use_smoothstep_ramp, _ramp_time, _t_str, _m_exp, _pow_t_str_m);
  const Real g_phi_init =
      _gate_phi_on_ke ? phiGateKe(phi_init, _phi_gate_start, _phi_gate_end, _phi_max) : 1.0;

  _phi_cell_ref[_qp] = phi_init;
  _phi_cell[_qp] = phi_init;
  _GI_cell[_qp] = _I;
  _GI_pmat[_qp] = _I;
  _bE_cell[_qp] = _I;
  _bE_pmat[_qp] = _I;
  _press_cell[_qp] = 0.0;
  _sigma_cell[_qp].zero();
  _sigma_pmat[_qp].zero();
  _eta[_qp] = 1.0;
  _ke[_qp] = ke_drive_0 * g_phi_init;
  _kT1[_qp] = _k_t;
  _k_diss[_qp] = _kD0;
  _J_mix[_qp] = 1.0;
  _kh[_qp] = 0.0;
  _fa[_qp] = 1.0;
  _gp[_qp] = 1.0;
  _g_phi_ke[_qp] = g_phi_init;
  _gate_total[_qp] = 1.0;
  _pressure[_qp] = 0.0;
  _pk1_stress[_qp].zero();
  _dpk1_stress_dn[_qp].zero();
}

void
CellGelMixtureNut2::computeQpProperties()
{
  const Real dt = std::max(_dt, 1e-30);
  const RankTwoTensor & F_n = _F_old[_qp];
  const RankTwoTensor & F_np1 = _F[_qp];
  const RankTwoTensor F_n_inv = F_n.inverse();
  const RankTwoTensor F_nph = 0.5 * (F_n + F_np1);
  const RankTwoTensor F_nph_inv = F_nph.inverse();
  const RankTwoTensor f_nph = F_nph * F_n_inv;
  const RankTwoTensor f_nph_inv = f_nph.inverse();
  const RankTwoTensor f_np1 = F_np1 * F_n_inv;
  const RankTwoTensor E_np1 = 0.5 * (f_np1.transpose() * f_np1 - _I);

  const Real n_here = _has_n ? (*_n)[_qp] : 0.0;
  const Real fa_here = faGate(n_here, _nut_str, _m_nut, _pow_nut_str_m, _smooth_eps_n);

  const Real ke_drive = _ke0 * growthRamp(
                                   _t, _use_smoothstep_ramp, _ramp_time, _t_str, _m_exp, _pow_t_str_m);

  const Real phi_ref_old = clampPhi(_phi_cell_ref_old[_qp], tiny_value, _phi_max);
  const Real J_nph = smoothMax(F_nph.det(), tiny_value, tiny_value);
  const Real J_np1 = smoothMax(F_np1.det(), tiny_value, tiny_value);
  const Real phi_gate_nph = phiFromRefAndJ(phi_ref_old, J_nph, _phi_max);
  const Real g_phi_nph =
      _gate_phi_on_ke ? phiGateKe(phi_gate_nph, _phi_gate_start, _phi_gate_end, _phi_max) : 1.0;

  const RankTwoTensor d_n = (1.0 / dt) * E_np1;
  const Real kh_n = d_n.trace();
  const RankTwoTensor d_nph = (1.0 / dt) * f_nph_inv.transpose() * E_np1 * f_nph_inv;
  const Real kh_nph = d_nph.trace();
  const Real kT1_n = _k_t;

  // Update the cell elastic metric with growth and deviatoric remodeling.
  const Real bE_cell_n_inv_trace =
      smoothMax(_bE_cell_old[_qp].inverse().trace(), tiny_value, tiny_value);
  const RankTwoTensor Lie_bE_cell_n =
      -(2.0 / 3.0) * _ke_old[_qp] * _bE_cell_old[_qp] -
      ((4.0 / 3.0) * kh_n + kT1_n) *
          (_bE_cell_old[_qp] - (3.0 / bE_cell_n_inv_trace) * _I);
  const RankTwoTensor GI_cell_dot_n = F_n_inv * Lie_bE_cell_n * F_n_inv.transpose();
  const RankTwoTensor GI_cell_nph = _GI_cell_old[_qp] + 0.5 * dt * GI_cell_dot_n;
  const RankTwoTensor bE_cell_nph = F_nph * GI_cell_nph * F_nph.transpose();

  const RankTwoTensor sigma_cell_nph = cellStress(bE_cell_nph, _I, _G_cell, _K_cell);
  const Real press_cell_nph = sigma_cell_nph.trace() / 3.0;
  const Real gp_nph = gpGate(press_cell_nph, _p_star, _press_gate_smooth);
  const Real ke_nph = ke_drive * (_gate_fa_on_ke ? fa_here : 1.0) *
                      (_gate_gp_on_ke ? gp_nph : 1.0) * g_phi_nph;
  const Real kT1_nph = _k_t;

  const Real bE_cell_nph_inv_trace = smoothMax(bE_cell_nph.inverse().trace(), tiny_value, tiny_value);
  const RankTwoTensor Lie_bE_cell_nph =
      -(2.0 / 3.0) * ke_nph * bE_cell_nph -
      ((4.0 / 3.0) * kh_nph + kT1_nph) * (bE_cell_nph - (3.0 / bE_cell_nph_inv_trace) * _I);
  const RankTwoTensor GI_cell_dot_nph = F_nph_inv * Lie_bE_cell_nph * F_nph_inv.transpose();

  _GI_cell[_qp] = _GI_cell_old[_qp] + dt * GI_cell_dot_nph;
  _bE_cell[_qp] = F_np1 * _GI_cell[_qp] * F_np1.transpose();
  _sigma_cell[_qp] = cellStress(_bE_cell[_qp], _I, _G_cell, _K_cell);
  _press_cell[_qp] = _sigma_cell[_qp].trace() / 3.0;
  const Real gp_here = gpGate(_press_cell[_qp], _p_star, _press_gate_smooth);
  _kh[_qp] = kh_nph;
  _eta[_qp] = _eta_old[_qp] * std::exp(_kh[_qp] * dt);
  _kT1[_qp] = _k_t;

  // Evolve the referential fraction, then reconstruct the current fraction from J.
  const Real phi_ref_dot_old = phiDot(phi_ref_old, _ke_old[_qp], _k_ap);
  const Real phi_ref_predictor = clampPhi(phi_ref_old + dt * phi_ref_dot_old, tiny_value, _phi_max);
  const Real phi_ref_dot_predictor = phiDot(phi_ref_predictor, ke_nph, _k_ap);
  const Real phi_ref_new =
      clampPhi(phi_ref_old + 0.5 * dt * (phi_ref_dot_old + phi_ref_dot_predictor), tiny_value, _phi_max);
  const Real phi_ref_mid = clampPhi(0.5 * (phi_ref_old + phi_ref_new), tiny_value, _phi_max);
  _phi_cell_ref[_qp] = phi_ref_new;

  const Real phi_mid = phiFromRefAndJ(phi_ref_mid, J_nph, _phi_max);
  _phi_cell[_qp] = phiFromRefAndJ(phi_ref_new, J_np1, _phi_max);
  const Real g_phi_here =
      _gate_phi_on_ke ? phiGateKe(_phi_cell[_qp], _phi_gate_start, _phi_gate_end, _phi_max) : 1.0;
  _ke[_qp] = ke_drive * (_gate_fa_on_ke ? fa_here : 1.0) * (_gate_gp_on_ke ? gp_here : 1.0) *
             g_phi_here;

  // Update the gel metric with the Ellis/yield relaxation law.
  const Real bE_pmat_n_inv_trace =
      smoothMax(_bE_pmat_old[_qp].inverse().trace(), tiny_value, tiny_value);
  const RankTwoTensor Lie_bE_pmat_n =
      -_k_diss_old[_qp] * (_bE_pmat_old[_qp] - (3.0 / bE_pmat_n_inv_trace) * _I);
  const RankTwoTensor GI_pmat_dot_n = F_n_inv * Lie_bE_pmat_n * F_n_inv.transpose();
  const RankTwoTensor GI_pmat_nph = _GI_pmat_old[_qp] + 0.5 * dt * GI_pmat_dot_n;
  const RankTwoTensor bE_pmat_nph = F_nph * GI_pmat_nph * F_nph.transpose();

  const Real Kg_eff_nph = kgEffFromPhi(_G_gel, phi_mid, _phi_bulk_floor, _phi_max);
  const RankTwoTensor sigma_pmat_nph =
      gelStress(bE_pmat_nph, _I, _G_gel, Kg_eff_nph, _Im, _gent_lock_floor);
  const Real k_diss_nph =
      ellisRate(vonMises(sigma_pmat_nph), _kD0, _kDinf, _sig_yield, _n_kD);

  const Real bE_pmat_nph_inv_trace =
      smoothMax(bE_pmat_nph.inverse().trace(), tiny_value, tiny_value);
  const RankTwoTensor Lie_bE_pmat_nph =
      -k_diss_nph * (bE_pmat_nph - (3.0 / bE_pmat_nph_inv_trace) * _I);
  const RankTwoTensor GI_pmat_dot_nph = F_nph_inv * Lie_bE_pmat_nph * F_nph_inv.transpose();

  _GI_pmat[_qp] = _GI_pmat_old[_qp] + dt * GI_pmat_dot_nph;
  _bE_pmat[_qp] = F_np1 * _GI_pmat[_qp] * F_np1.transpose();
  const Real Kg_eff = kgEffFromPhi(_G_gel, _phi_cell[_qp], _phi_bulk_floor, _phi_max);
  _sigma_pmat[_qp] = gelStress(_bE_pmat[_qp], _I, _G_gel, Kg_eff, _Im, _gent_lock_floor);
  _k_diss[_qp] = ellisRate(vonMises(_sigma_pmat[_qp]), _kD0, _kDinf, _sig_yield, _n_kD);

  _J_mix[_qp] = F_np1.det();
  const RankTwoTensor sigma_mix =
      _phi_cell[_qp] * _sigma_cell[_qp] + (1.0 - _phi_cell[_qp]) * _sigma_pmat[_qp];
  _pressure[_qp] = sigma_mix.trace() / 3.0;

  _fa[_qp] = fa_here;
  _gp[_qp] = gp_here;
  _g_phi_ke[_qp] = g_phi_here;
  _gate_total[_qp] = fa_here * gp_here;
  _dpk1_stress_dn[_qp].zero();

  ComputeLagrangianStressPK1::computeQpProperties();
}

void
CellGelMixtureNut2::computeQpCauchyStress()
{
  _cauchy_stress[_qp] =
      _phi_cell[_qp] * _sigma_cell[_qp] + (1.0 - _phi_cell[_qp]) * _sigma_pmat[_qp];
  _pressure[_qp] = _cauchy_stress[_qp].trace() / 3.0;
}

void
CellGelMixtureNut2::computeQpPK1Stress()
{
  const RankTwoTensor & F_n = _F_old[_qp];
  const RankTwoTensor & F_np1 = _F[_qp];
  const RankTwoTensor F_n_inv = F_n.inverse();
  const Real dt = std::max(_dt, 1e-30);

  const auto compute_stress_for_state = [&](const RankTwoTensor & F_trial,
                                            const Real n_eval,
                                            RankTwoTensor & sigma_out,
                                            RankTwoTensor & P_out) {
    // Re-run the constitutive update for a trial state; this is shared by dP/dF and dP/dn.
    const RankTwoTensor F_trial_inv = F_trial.inverse();
    const RankTwoTensor F_trial_inv_trans = F_trial_inv.transpose();
    const RankTwoTensor F_trial_nph = 0.5 * (F_trial + F_n);
    const RankTwoTensor F_trial_nph_inv = F_trial_nph.inverse();
    const RankTwoTensor f_trial_nph = F_trial_nph * F_n_inv;
    const RankTwoTensor f_trial_nph_inv = f_trial_nph.inverse();
    const RankTwoTensor f_trial_np1 = F_trial * F_n_inv;
    const RankTwoTensor E_trial_np1 = 0.5 * (f_trial_np1.transpose() * f_trial_np1 - _I);

    const RankTwoTensor d_trial_n = (1.0 / dt) * E_trial_np1;
    const Real kh_trial_n = d_trial_n.trace();
    const RankTwoTensor d_trial_nph =
        (1.0 / dt) * f_trial_nph_inv.transpose() * E_trial_np1 * f_trial_nph_inv;
    const Real kh_trial_nph = d_trial_nph.trace();
    const Real kT1_trial_n = _k_t;

    const Real ke_drive = _ke0 * growthRamp(
                                     _t, _use_smoothstep_ramp, _ramp_time, _t_str, _m_exp, _pow_t_str_m);
    const Real fa_trial = faGate(n_eval, _nut_str, _m_nut, _pow_nut_str_m, _smooth_eps_n);
    const Real phi_ref_old = clampPhi(_phi_cell_ref_old[_qp], tiny_value, _phi_max);
    const Real J_trial_nph = smoothMax(F_trial_nph.det(), tiny_value, tiny_value);
    const Real J_trial = smoothMax(F_trial.det(), tiny_value, tiny_value);
    const Real phi_gate_trial_nph = phiFromRefAndJ(phi_ref_old, J_trial_nph, _phi_max);
    const Real g_phi_trial_nph = _gate_phi_on_ke
                                     ? phiGateKe(phi_gate_trial_nph,
                                                 _phi_gate_start,
                                                 _phi_gate_end,
                                                 _phi_max)
                                     : 1.0;

    const Real bE_cell_n_inv_trace =
        smoothMax(_bE_cell_old[_qp].inverse().trace(), tiny_value, tiny_value);
    const RankTwoTensor Lie_bE_cell_trial_n =
        -(2.0 / 3.0) * _ke_old[_qp] * _bE_cell_old[_qp] -
        ((4.0 / 3.0) * kh_trial_n + kT1_trial_n) *
            (_bE_cell_old[_qp] - (3.0 / bE_cell_n_inv_trace) * _I);
    const RankTwoTensor GI_cell_dot_trial_n =
        F_n_inv * Lie_bE_cell_trial_n * F_n_inv.transpose();
    const RankTwoTensor GI_cell_trial_nph = _GI_cell_old[_qp] + 0.5 * dt * GI_cell_dot_trial_n;
    const RankTwoTensor bE_cell_trial_nph = F_trial_nph * GI_cell_trial_nph * F_trial_nph.transpose();

    const RankTwoTensor sigma_cell_trial_nph = cellStress(bE_cell_trial_nph, _I, _G_cell, _K_cell);
    const Real press_cell_trial_nph = sigma_cell_trial_nph.trace() / 3.0;
    const Real gp_trial_nph = gpGate(press_cell_trial_nph, _p_star, _press_gate_smooth);
    const Real ke_trial_nph = ke_drive * (_gate_fa_on_ke ? fa_trial : 1.0) *
                              (_gate_gp_on_ke ? gp_trial_nph : 1.0) * g_phi_trial_nph;
    const Real kT1_trial_nph = _k_t;

    const Real bE_cell_trial_nph_inv_trace =
        smoothMax(bE_cell_trial_nph.inverse().trace(), tiny_value, tiny_value);
    const RankTwoTensor Lie_bE_cell_trial_nph =
        -(2.0 / 3.0) * ke_trial_nph * bE_cell_trial_nph -
        ((4.0 / 3.0) * kh_trial_nph + kT1_trial_nph) *
            (bE_cell_trial_nph - (3.0 / bE_cell_trial_nph_inv_trace) * _I);
    const RankTwoTensor GI_cell_dot_trial_nph =
        F_trial_nph_inv * Lie_bE_cell_trial_nph * F_trial_nph_inv.transpose();

    const RankTwoTensor GI_cell_trial = _GI_cell_old[_qp] + dt * GI_cell_dot_trial_nph;
    const RankTwoTensor bE_cell_trial = F_trial * GI_cell_trial * F_trial.transpose();
    const RankTwoTensor sigma_cell_trial = cellStress(bE_cell_trial, _I, _G_cell, _K_cell);

    const Real phi_ref_dot_old = phiDot(phi_ref_old, _ke_old[_qp], _k_ap);
    const Real phi_ref_predictor = clampPhi(phi_ref_old + dt * phi_ref_dot_old, tiny_value, _phi_max);
    const Real phi_ref_dot_predictor = phiDot(phi_ref_predictor, ke_trial_nph, _k_ap);
    const Real phi_ref_trial =
        clampPhi(phi_ref_old + 0.5 * dt * (phi_ref_dot_old + phi_ref_dot_predictor), tiny_value, _phi_max);
    const Real phi_ref_trial_mid = clampPhi(0.5 * (phi_ref_old + phi_ref_trial), tiny_value, _phi_max);

    const Real phi_trial_mid = phiFromRefAndJ(phi_ref_trial_mid, J_trial_nph, _phi_max);
    const Real phi_trial = phiFromRefAndJ(phi_ref_trial, J_trial, _phi_max);

    const Real bE_pmat_n_inv_trace =
        smoothMax(_bE_pmat_old[_qp].inverse().trace(), tiny_value, tiny_value);
    const RankTwoTensor Lie_bE_pmat_trial_n =
        -_k_diss_old[_qp] * (_bE_pmat_old[_qp] - (3.0 / bE_pmat_n_inv_trace) * _I);
    const RankTwoTensor GI_pmat_dot_trial_n =
        F_n_inv * Lie_bE_pmat_trial_n * F_n_inv.transpose();
    const RankTwoTensor GI_pmat_trial_nph = _GI_pmat_old[_qp] + 0.5 * dt * GI_pmat_dot_trial_n;
    const RankTwoTensor bE_pmat_trial_nph = F_trial_nph * GI_pmat_trial_nph * F_trial_nph.transpose();

    const Real Kg_eff_trial_nph = kgEffFromPhi(_G_gel, phi_trial_mid, _phi_bulk_floor, _phi_max);
    const RankTwoTensor sigma_pmat_trial_nph =
        gelStress(bE_pmat_trial_nph, _I, _G_gel, Kg_eff_trial_nph, _Im, _gent_lock_floor);
    const Real k_diss_trial_nph =
        ellisRate(vonMises(sigma_pmat_trial_nph), _kD0, _kDinf, _sig_yield, _n_kD);

    const Real bE_pmat_trial_nph_inv_trace =
        smoothMax(bE_pmat_trial_nph.inverse().trace(), tiny_value, tiny_value);
    const RankTwoTensor Lie_bE_pmat_trial_nph =
        -k_diss_trial_nph * (bE_pmat_trial_nph - (3.0 / bE_pmat_trial_nph_inv_trace) * _I);
    const RankTwoTensor GI_pmat_dot_trial_nph =
        F_trial_nph_inv * Lie_bE_pmat_trial_nph * F_trial_nph_inv.transpose();

    const RankTwoTensor GI_pmat_trial = _GI_pmat_old[_qp] + dt * GI_pmat_dot_trial_nph;
    const RankTwoTensor bE_pmat_trial = F_trial * GI_pmat_trial * F_trial.transpose();
    const Real Kg_eff_trial = kgEffFromPhi(_G_gel, phi_trial, _phi_bulk_floor, _phi_max);
    const RankTwoTensor sigma_pmat_trial =
        gelStress(bE_pmat_trial, _I, _G_gel, Kg_eff_trial, _Im, _gent_lock_floor);

    sigma_out = phi_trial * sigma_cell_trial + (1.0 - phi_trial) * sigma_pmat_trial;
    const RankTwoTensor S_trial = F_trial_inv * sigma_out * F_trial_inv_trans * J_trial;
    P_out = F_trial * S_trial;
  };

  RankTwoTensor sigma_cauchy;
  compute_stress_for_state(F_np1, _has_n ? (*_n)[_qp] : 0.0, sigma_cauchy, _pk1_stress[_qp]);
  _cauchy_stress[_qp] = sigma_cauchy;
  _pressure[_qp] = sigma_cauchy.trace() / 3.0;

  for (unsigned int C = 0; C < 3; ++C)
    for (unsigned int D = 0; D < 3; ++D)
    {
      RankTwoTensor dF;
      dF.zero();
      dF(C, D) = _epsilon;

      const RankTwoTensor F_trial_plus = F_np1 + dF;
      const RankTwoTensor F_trial_minus = F_np1 - dF;
      RankTwoTensor sigma_trial_plus;
      RankTwoTensor P_trial_plus;
      RankTwoTensor sigma_trial_minus;
      RankTwoTensor P_trial_minus;
      compute_stress_for_state(
          F_trial_plus, _has_n ? (*_n)[_qp] : 0.0, sigma_trial_plus, P_trial_plus);
      compute_stress_for_state(F_trial_minus,
                               _has_n ? (*_n)[_qp] : 0.0,
                               sigma_trial_minus,
                               P_trial_minus);

      for (unsigned int A = 0; A < 3; ++A)
        for (unsigned int B = 0; B < 3; ++B)
          _pk1_jacobian[_qp](A, B, C, D) =
              (P_trial_plus(A, B) - P_trial_minus(A, B)) / (2.0 * _epsilon);
    }

  if (_has_n)
  {
    const Real n_here = (*_n)[_qp];
    const Real dn = std::max(1e-8, 1e-6 * std::max(1.0, std::abs(n_here)));
    const Real n_plus = n_here + dn;
    const Real n_minus = n_here - dn;
    const Real delta_n = 2.0 * dn;

    RankTwoTensor sigma_plus;
    RankTwoTensor P_plus;
    RankTwoTensor sigma_minus;
    RankTwoTensor P_minus;
    compute_stress_for_state(F_np1, n_plus, sigma_plus, P_plus);
    compute_stress_for_state(F_np1, n_minus, sigma_minus, P_minus);
    _dpk1_stress_dn[_qp] = (P_plus - P_minus) * (1.0 / delta_n);
  }
  else
    _dpk1_stress_dn[_qp].zero();
}
