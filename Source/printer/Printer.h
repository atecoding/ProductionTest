#pragma once

class Printer
{
public:
    static Result print(String const text);
    
protected:
    static Result printInternal(String const text);
};
