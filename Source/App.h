#ifndef _conapp_h_
#define _conapp_h_

class ehw;
class ehwlist;

#include "AppWindow.h"
#include "TestManager.h"
#include "CalibrationV2/CalibrationManagerV2.h"

class App : public JUCEApplication
{
protected :
	friend class AppListener;
	
	bool SingleInstanceCheck();
	
    ScopedPointer<LookAndFeel_V3> lookAndFeelV3;
    ScopedPointer<AppWindow> _window;
    ScopedPointer<ehwlist> _hwlist;
    StringArray hardwareInstances;

    ScopedPointer<InterProcessLock> _processlock;

	ScopedPointer <PropertiesFile> props;
	    
public:

    //==============================================================================
    App()
    {
    }

    ~App()
    {
        // avoid doing anything complicated in the destructor - use shutdown() instead.
    }

    //==============================================================================
    
    void initialise (const String& commandLine);
	void parseCommandLine(const String& commandLine, bool &autostart, int &loopcount);
    void shutdown();
    

    //==============================================================================
    const String getApplicationName();

    const String getApplicationVersion()
    {
        return T("1.0");	// fixme
    }

    bool moreThanOneInstanceAllowed()
    {
        return true;
    }

    //==============================================================================
    
    void MakeWindow();
    
    AppWindow *MatchDevId(String *devid);
    
    //==============================================================================
    static void RemoveNotify(void *context,char *devid);
    
    //==============================================================================	
	 virtual void systemRequestedQuit();
    
    ScopedPointer<TestManager> testManager;

#if ACOUSTICIO_BUILD
	ScopedPointer<CalibrationManagerV2> calibrationManager;
#endif
};

extern App *application;

#endif