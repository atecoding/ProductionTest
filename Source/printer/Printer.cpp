#include "../base.h"
#include "Printer.h"

void Printer::print(String const text)
{
    DBG("Printer::print");
    
    Result printResult(printInternal(text));
    if (printResult.failed())
    {
        DBG(printResult.getErrorMessage());
        return;
    }
}

