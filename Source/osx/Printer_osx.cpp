#include "../base.h"
#include "Printer.h"

const String PrinterName("Brother_QL_710W");

Result Printer::findPrinter()
{
    //
    // Use "lpstat -p" to search for the Brother printer
    //
    ChildProcess process;
    
    if (process.start("lpstat -p"))
    {
        String output(process.readAllProcessOutput());
        DBG(output);
        
       if (false == output.contains("printer " + PrinterName))
           return Result::fail(PrinterName + " not found");
    }
    else
    {
        return Result::fail("Could not enumerate printers");
    }
    
    return Result::ok();
}


Result Printer::configurePrinter()
{
    return Result::ok();
}


Result Printer::printInternal(String const text)
{
    return Result::ok();
}
