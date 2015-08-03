#include "../base.h"

#ifdef JUCE_WIN32

#include "../printer/Printer.h"

Result Printer::printInternal(String const text)
{
    return Result::fail("Not supported");
}

#endif