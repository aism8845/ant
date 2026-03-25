//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html
#include "AntTestApp.h"
#include "AntApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "MooseSyntax.h"

InputParameters
AntTestApp::validParams()
{
  InputParameters params = AntApp::validParams();
  params.set<bool>("use_legacy_material_output") = false;
  params.set<bool>("use_legacy_initial_residual_evaluation_behavior") = false;
  return params;
}

AntTestApp::AntTestApp(const InputParameters & parameters) : MooseApp(parameters)
{
  AntTestApp::registerAll(
      _factory, _action_factory, _syntax, getParam<bool>("allow_test_objects"));
}

AntTestApp::~AntTestApp() {}

void
AntTestApp::registerAll(Factory & f, ActionFactory & af, Syntax & s, bool use_test_objs)
{
  AntApp::registerAll(f, af, s);
  if (use_test_objs)
  {
    Registry::registerObjectsTo(f, {"AntTestApp"});
    Registry::registerActionsTo(af, {"AntTestApp"});
  }
}

void
AntTestApp::registerApps()
{
  registerApp(AntApp);
  registerApp(AntTestApp);
}

/***************************************************************************************************
 *********************** Dynamic Library Entry Points - DO NOT MODIFY ******************************
 **************************************************************************************************/
// External entry point for dynamic application loading
extern "C" void
AntTestApp__registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  AntTestApp::registerAll(f, af, s);
}
extern "C" void
AntTestApp__registerApps()
{
  AntTestApp::registerApps();
}
