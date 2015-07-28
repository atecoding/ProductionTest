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

class Content : public Component, Button::Listener, public AsyncUpdater,
public ComboBox::Listener,
public ListBoxModel
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

	void AddResult(String const &name,int pass);
	void FinishTests(bool pass,bool skipped);
	void Reset();

	void DevArrived(ehw *dev);
	void DevRemoved(ehw *dev);

	void handleAsyncUpdate();

protected:
	virtual void paint (Graphics& g);
	virtual void buttonClicked(Button *button);
    virtual bool keyPressed(KeyPress const & key);
    virtual void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    virtual int getNumRows();
    virtual void paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected);

	Result promptForSerialNumber(String &serialNumber);
	
	StringArray hardwareInstances;
	ehwlist		*_devlist;
	//Font		_f;
	String finalResult;
	Colour finalResultColour;

	ScopedPointer<ProductionUnit> _unit;
	String _unit_name;

    TextEditor logDisplay;
    TextButton startButton;
    TextButton stopButton;
#if ALLOW_USER_SCRIPT_SELECT
    ComboBox scriptCombo;
#endif
    ListBox resultsListBox;

	DevChangeListener _dev_listener;

	StringArray _group_names;
	Array<int> _results;

	friend class DevChangeListener;
};





#endif