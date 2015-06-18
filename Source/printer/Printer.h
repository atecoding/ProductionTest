#pragma once

class Printer
{
public:
    Printer();
    
    bool printerFound();
    void print(String const text);
    
protected:
    Result findPrinter();
    Result configurePrinter();
    Result printInternal(String const text);
};