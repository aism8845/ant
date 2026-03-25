#include "AntApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "ModulesApp.h"
#include "MooseSyntax.h"

InputParameters
AntApp::validParams()
{
  InputParameters params = MooseApp::validParams();
  params.set<bool>("use_legacy_material_output") = false;
  params.set<bool>("use_legacy_initial_residual_evaluation_behavior") = false;
  return params;
}

AntApp::AntApp(const InputParameters & parameters) : MooseApp(parameters)
{
  AntApp::registerAll(_factory, _action_factory, _syntax);
}

AntApp::~AntApp() {}

void
AntApp::registerAll(Factory & f, ActionFactory & af, Syntax & syntax)
{
  ModulesApp::registerAllObjects<AntApp>(f, af, syntax);
  Registry::registerObjectsTo(f, {"AntApp"});
  Registry::registerActionsTo(af, {"AntApp"});

  /* register custom execute flags, action syntax, etc. here */
}

void
AntApp::registerApps()
{
  registerApp(AntApp);
}

/***************************************************************************************************
 *********************** Dynamic Library Entry Points - DO NOT MODIFY ******************************
 **************************************************************************************************/
extern "C" void
AntApp__registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  AntApp::registerAll(f, af, s);
}
extern "C" void
AntApp__registerApps()
{
  AntApp::registerApps();
}
