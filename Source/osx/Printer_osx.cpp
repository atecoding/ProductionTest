#include "../base.h"
#if JUCE_MAC
#include "Printer.h"

const String PrinterName("Brother_QL_710W");

Result Printer::printInternal(String const text)
{
    File file(File::createTempFile(".txt"));
    file.appendText(text);
    Result result(Result::ok());
    
    ChildProcess process;
    String commandLine("lp ");
    
    //commandLine += "-P " + PrinterName + " ";
    commandLine += file.getFullPathName();
    commandLine += " -o landscape ";
    commandLine += " -o media=DC03 ";
    if (process.start(commandLine))
    {
        if (false == process.waitForProcessToFinish(2000))
        {
            result = Result::fail("lp task did not finish");
        }
    }
    
    file.deleteFile();
    
    return result;
}
#endif
