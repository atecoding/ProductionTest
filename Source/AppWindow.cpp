#include "base.h"
#include "AppWindow.h"
#include "App.h"
#include "Content.h"
#include "ehw.h"
#include "ehwlist.h"
#include "hwcaps.h"

//
// constructor
//
AppWindow::AppWindow(ehwlist *devlist, const StringArray &hardwareInstances) :  
DocumentWindow(T("Production test"), 
			   Colours::white,
			   minimiseButton | closeButton,
			   true)
{
	//
	// Open the driver for this device
	//
	_devlist = devlist;
	#ifdef _WIN32
	int i;

	for (i = 0; i < devlist->GetNumDevices(); i++)
		devlist->GetNthDevice(i)->OpenDriver();
	#endif

	#ifdef _WIN32
	setTitleBarTextCentred(false);
	#endif

	//
	// configure the window
	//
	setOpaque(true);
	setUsingNativeTitleBar(true);
	setResizable(true,false);

	//
	// make the content component
	//
	Content *comp;
	
	comp = new Content(devlist, hardwareInstances);
	setContentOwned(comp,false);
	setSize(900,600);
	setCentreRelative(0.5f,0.5f);
}


AppWindow::~AppWindow()
{
}


void AppWindow::closeButtonPressed()
{
	JUCEApplication::quit();
}



