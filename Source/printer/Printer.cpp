#include "../base.h"
#include "Printer.h"

Printer::Printer()
{
}

void Printer::print(String const text)
{
    DBG("Printer::print");
    DBG(text);
    
    Result findResult(findPrinter());
    if (findResult.failed())
    {
        DBG(findResult.getErrorMessage());
        return;
    }
    
    Result configResult(configurePrinter());
    if (configResult.failed())
    {
        DBG(configResult.getErrorMessage());
        return;
    }
    
    Result printResult(printInternal(text));
    if (printResult.failed())
    {
        DBG(printResult.getErrorMessage());
        return;
    }
}

