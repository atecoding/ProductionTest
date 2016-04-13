#include "base.h"
#include "TestManager.h"

const char *CurrentTestProperty = "CurrentTest";

TestManager::TestManager() :
currentLoop(0),
autostart(false),
numLoops(0)
{
#ifdef JUCE_MAC
    File bundle(File::getSpecialLocation(File::currentApplicationFile));
    File resources(bundle.getChildFile("Contents/Resources"));
#endif

#ifdef JUCE_WIN32
	File resources(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory());
#endif

    resources.findChildFiles(files, File::findFiles, false, "*.xml");
    currentIndex = 0;
}

int TestManager::getNumScripts() const
{
    return files.size();
}

File TestManager::getScript(int const index) const
{
    return files[index];
}

void TestManager::setCurrentScriptIndex(int const currentIndex_)
{
    currentIndex = currentIndex_;
}

int TestManager::getCurrentScriptIndex() const
{
    return currentIndex;
}

File TestManager::getCurrentScript() const
{
    return files[currentIndex];
}

void TestManager::load(PropertiesFile *propfile)
{
    String currentTestName(propfile->getValue(CurrentTestProperty));
    if (currentTestName.isEmpty())
    {
        return;
    }
    
    for (int i = 0; i < files.size(); ++i)
    {
        if (files[i].getFileNameWithoutExtension() == currentTestName)
        {
            currentIndex = i;
            break;
        }
    }
}

void TestManager::save(PropertiesFile *propfile)
{
    propfile->setValue(CurrentTestProperty, files[currentIndex].getFileNameWithoutExtension());
}
