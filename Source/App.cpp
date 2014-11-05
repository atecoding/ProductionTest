#include "base.h"
#include "AppWindow.h"
#include "App.h"
#include "ehw.h"
#include "ehwlist.h"
#include "Content.h"
#include "hwcaps.h"
#if ACOUSTICIO_BUILD
#include "AIOTestAdapter.h"
#endif

App *application;
const char boundariesName[] = "Window boundaries";

#if ACOUSTICIO_BUILD
String ProductionTestsXmlFileName("AIOProductionTests.xml");
#endif

#if ANALYZERBR_BUILD
String ProductionTestsXmlFileName("ABRProductionTests.xml");
#endif

//==============================================================================

void App::initialise (const String& commandLine)
{
    //DBG("App::initialise " << commandLine);
    
	application = this;

	//
	// Make sure this is the only instance running
	//
	if ( SingleInstanceCheck() )
	{
		quit();
		return;
	}

	//
	// Command line options
	//
	if (commandLine.isNotEmpty())
	{
		parseCommandLine(commandLine);
	}

	//
	// Properties file
	//

	PropertiesFile::Options options;
	options.applicationName     = "ProductionTest";
	options.folderName          = "Echo";
	options.filenameSuffix      = "settings";
	options.osxLibrarySubFolder = "Application Support";

	props = new PropertiesFile (options);

	//
	// For the PCI test, disable all the PCI cards
	//
#ifdef PCI_BUILD
	void EnableDevices(const StringArray &instances);
	StringArray DisableAllPCIDevices();

	EnableDevices(hardwareInstances);	// unconditionally enable PCI devices
	hardwareInstances = DisableAllPCIDevices();	// store the list of hardware instances
#endif


	//
	// Build the hardware list
	//
	_hwlist = new ehwlist(0,NULL);

	//
	// Make a window
	//
	MakeWindow();

	/*  ..and now return, which will fall into to the main event
        dispatch loop, and this will run until something calls
        JUCEAppliction::quit().
    */
}

//==============================

void App::MakeWindow()
{
	AppWindow *window;

	//
	// Create the window
	//
	window = new AppWindow( _hwlist, hardwareInstances );

   window->setVisible(true);
	window->toFront(true);

	_window = window;

	if (props->containsKey(boundariesName))
	{
		String text (props->getValue(boundariesName));

		_window->setBounds(juce::Rectangle <int>::fromString( text));
	}

	window->setAlwaysOnTop(true);
	window->setAlwaysOnTop(false);
}

//==============================================================================

void App::shutdown()
{
	if (_window)
	{
		juce::Rectangle <int> bounds (_window->getBounds());

		props->setValue(boundariesName,bounds.toString());
        _window = nullptr;
	}
    
    _hwlist = nullptr;
    _processlock = nullptr;
}

//==============================================================================

const String App::getApplicationName()
{
    return T("Production test");
}

//==============================================================================

bool App::SingleInstanceCheck()
{
	bool locked;

	_processlock = new InterProcessLock("Production test lock");
	locked = _processlock->enter(0);
	if (false == locked)
	{
		DBG("Could not get interprocess lock");

		return true;
	}

	return false;
}

//==============================================================================
void App::systemRequestedQuit()
{
	JUCEApplication::systemRequestedQuit();
}

void App::parseCommandLine(const String& commandLine)
{
    StringArray tokens;
    
    tokens.addTokens(commandLine,true);
    
    String script("-script");
    for (int i =0; i < tokens.size(); ++i)
    {
        if (tokens[i] == script)
        {
            ProductionTestsXmlFileName = tokens[i+1];
            break;
        }
    }
}

//==============================================================================
// This macro creates the application's main() function..

START_JUCE_APPLICATION (App)