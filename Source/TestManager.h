#pragma once

class TestManager
{
public:
	TestManager();

	int getNumScripts() const;
	File getScript(int const index) const;
	void setCurrentScriptIndex(int const currentIndex_);
	int getCurrentScriptIndex() const;
	File getCurrentScript() const;

	void load(PropertiesFile *propfile);
	void save(PropertiesFile *propfile);

	void setAutostart(bool autostart_)
	{
		autostart = autostart_;
	}

	bool getAutostart() const
	{
		return autostart;
	}
    
	void setLoop(bool loop_)
	{
		loop = loop_;
	}

	bool getLoop() const
	{
		return loop;
	}

	void setLoopCount(int loopcount_)
	{
		loopcount = loopcount_;
	}

	int getLoopCount() const
	{
		return loopcount;
	}

protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestManager)
    
    Array<File> files;
    int currentIndex;
	bool autostart;
	bool loop;
	int loopcount;
};