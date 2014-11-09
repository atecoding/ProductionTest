#ifndef _conwindow_h_
#define _conwindow_h_

class ehwlist;
class Content;
class AppWindow : public DocumentWindow
{
public:
	AppWindow(ehwlist *devlist, const StringArray &hardwareInstances);
	~AppWindow();

	virtual void closeButtonPressed();
	
    virtual void userTriedToCloseWindow()
    {
		closeButtonPressed();
    }
        
#ifdef JUCE_MAC
	//virtual bool keyPressed(const KeyPress &key);
#endif
        
    ehwlist *GetDevList()
    {
		return _devlist;
    }

protected:
    LookAndFeel_V3 lookAndFeelV3;
	ehwlist				*_devlist;
};

#endif
