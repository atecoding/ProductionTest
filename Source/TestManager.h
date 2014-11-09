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
    
protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestManager)
    
    Array<File> files;
    int currentIndex;
};