#ifndef _cwcomp_h_
#define _cwcomp_h_

class Content;

class DevChangeListener : public MessageListener
{
public :
    virtual void handleMessage(const Message &message);

	Content *_content;
};


class hwcaps;
class ehwlist;
class ehw;
class Content;
class AppWindow;
class ProductionUnit;

class Content : public Component, ButtonListener, SliderListener, public AsyncUpdater,
ComboBox::Listener
{
public:
	Content(ehwlist *devlist, const StringArray &hardwareInstances_);
	~Content();
	
	void Setup(AppWindow *window);
	
	ehwlist *GetDevList()
	{
		return _devlist;
	}

	void resized();

	virtual void handleCommandMessage(int commandId);

	void log(String msg);
	void setFinalResult(String text,Colour color);

	void AddResult(String &name,int pass);
	void FinishTests(bool pass,bool skipped);
	void Reset();

	void DevArrived(ehw *dev);
	void DevRemoved(ehw *dev);

	void handleAsyncUpdate();

protected:
	virtual void paint (Graphics& g);
	virtual void buttonClicked(Button *button);
	virtual bool keyPressed(const KeyPress &key);
	virtual void sliderValueChanged(Slider *s);
    virtual void comboBoxChanged (ComboBox* comboBoxThatHasChanged);

	Result GetSerialNumber(String &serialNumber);
	
	StringArray hardwareInstances;
	ehwlist		*_devlist;
	//Font		_f;
	String finalResult;
	Colour finalResultColour;

	TextEditor *_log;
	
	ScopedPointer<ProductionUnit> _unit;
	String _unit_name;

	TextButton *_start_button;
    
    ComboBox *scriptCombo;

	DevChangeListener _dev_listener;

	StringArray _group_names;
	Array<int> _results;

	friend class DevChangeListener;
};





#endif