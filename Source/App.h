#ifndef _conapp_h_
#define _conapp_h_

class ehw;
class ehwlist;
class App : public JUCEApplication
{
protected :
	friend class AppListener;
	
	bool SingleInstanceCheck();
	
    AppWindow *_window;
    ehwlist *_hwlist;
	 StringArray hardwareInstances;

    InterProcessLock	*_processlock;

	ScopedPointer <PropertiesFile> props;
    
public:

    //==============================================================================
    App()
    {
        // avoid doing anything complicated in the constructor - use initialise() instead.
        _window = NULL;
		_hwlist = NULL;
		
		_processlock = NULL;
    }

    ~App()
    {
        // avoid doing anything complicated in the destructor - use shutdown() instead.
    }

    //==============================================================================
    
    void initialise (const String& commandLine);
    void App::shutdown();
    

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
};

extern App *application;

#endif