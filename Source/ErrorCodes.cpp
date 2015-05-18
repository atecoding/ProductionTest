#include "base.h"
#include "ErrorCodes.h"

ErrorCodes::ErrorCodes()
{
}

void ErrorCodes::reset()
{
    codes.clear();
}

void ErrorCodes::add(uint32 code)
{
    codes.addIfNotAlreadyThere(code);
}

void ErrorCodes::add(uint32 code, uint32 channel)
{
    uint32 value = code;
    value |= channel << 4;
    codes.addIfNotAlreadyThere(value);
}

int ErrorCodes::getCount() const
{
    return codes.size();
}

uint32 ErrorCodes::getCode(int const index) const
{
    return codes[index];
}