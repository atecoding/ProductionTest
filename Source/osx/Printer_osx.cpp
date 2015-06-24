#include "../base.h"
#include "Printer.h"

const String PrinterName("Brother_QL_710W");

bool Printer::printerFound()
{
    //
    // Use "lpstat -p" to search for the Brother printer
    //
    ChildProcess process;
    
    if (process.start("lpstat -p"))
    {
        String output(process.readAllProcessOutput());
        DBG(output);
        
        return output.contains("printer " + PrinterName);
    }

    return false;
}


Result Printer::printInternal(String const text)
{
    File file(File::createTempFile(".txt"));
    file.appendText(text);
    
    ChildProcess process;
    String commandLine("lp ");
    
    commandLine += "-d " + PrinterName + " ";
    commandLine += file.getFullPathName();
    commandLine += " -o landscape ";
    commandLine += " -o media=DC03 ";
    if (process.start(commandLine))
    {
        process.waitForProcessToFinish(2000);
    }
    
    file.deleteFile();
    
    return Result::ok();
}
