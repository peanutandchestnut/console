#include <iostream>
#include "testOgreArgHandler.hpp"
#include "testOgreConsole.hpp"
#include "testOgreSI.hpp"
#include "testConsoleUI.hpp"
#include "testOgreParticleSystemSI.hpp"
#include "ogreTestApp.h"

using namespace pac;

int main(int argc, char **argv) 
{
  //init ogre
  OgreTestApp app;
  app.setup();
  //app.go();
  
	//init console
	ImplConsoleUI ui;
  OgreConsole oc(&ui, app.getSceneMgr());
	oc.init();
	
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

