#pragma once

class Printer
{
public:
    static bool printerFound();
    static void print(String const text);
    
protected:
    static Result printInternal(String const text);
};