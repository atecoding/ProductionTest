#include "../base.h"
#include "Printer.h"

Result Printer::print(String const text)
{
    return printInternal(text);
}

