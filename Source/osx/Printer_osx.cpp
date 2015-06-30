#include "../base.h"
#include "Printer.h"

const String PrinterName("Brother_QL_710W");

String Printer::dumpPrinters()
{
    ChildProcess process;
    
    if (process.start("lpstat -p"))
    {
        return process.readAllProcessOutput();
    }
    
    return String::empty;
}

bool Printer::printerFound(String &status)
{
#if 0
    //
    // Use "lpstat -p" to search for the Brother printer
    //
    ChildProcess process;
    
    if (process.start("lpstat -p "))
    {
        status = process.readAllProcessOutput();
        
        int start = status.indexOf("printer " + PrinterName);
        if (start < 0)
        {
            return false;
        }
        int nextPrinter = status.indexOf( start + 1, "printer ");
        String section;
        if (nextPrinter < 0)
            section = status.substring(start,nextPrinter);
        else
            section = status.substring(start);
        
        return status.contains("printer " + PrinterName + " is idle.");
    }

    status = String::empty;
    return false;
#else
    return true;
#endif
}


Result Printer::printInternal(String const text)
{
    File file(File::createTempFile(".txt"));
    file.appendText(text);
    
    ChildProcess process;
    String commandLine("lp ");
    
    //commandLine += "-P " + PrinterName + " ";
    commandLine += file.getFullPathName();
    commandLine += " -o landscape ";
    commandLine += " -o media=DC03 ";
    if (process.start(commandLine))
    {
        String status = process.readAllProcessOutput();
        //process.waitForProcessToFinish(2000);
        DBG(status);
    }
    
    file.deleteFile();
    
    return Result::ok();
}
