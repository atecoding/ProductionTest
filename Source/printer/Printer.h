#pragma once

class Printer
{
public:
    static String dumpPrinters();
    static bool printerFound(String &status);
    static void print(String const text);
    
protected:
    static Result printInternal(String const text);
};