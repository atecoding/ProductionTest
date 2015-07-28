#pragma once

class ErrorCodes
{
public:
    ErrorCodes();
    
    void reset();
    void add(uint32 code);
    void add(uint32 code, uint32 channel);
    int getCount() const;
    uint32 getCode(int const index) const;
    String getCodeAsString(int const index) const;
    
    enum
    {
        LED = 0
    };
    
    enum
    {
        LEVEL = 0,
        THDN = 1,
        DNR = 2,
        TEDS = 4,
        MIC_SUPPLY_VOLTAGE,
        MIC_SUPPLY_CURRENT,
        AIOS_REFERENCE_VOLTAGE,
        FIRMWARE,
        FLASH,
        MODULE_TYPE,
        RESISTANCE_MEASUREMENT,
        CALIBRATION_VERIFICATION,
        
        LAST
    };
    
protected:
    Array<uint32, CriticalSection> codes;
};