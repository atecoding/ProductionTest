#pragma once

#include "CalibrationProcedure.h"

class CalibrationProcedureAIOM : public CalibrationProcedure
{
public:
    CalibrationProcedureAIOM(CalibrationUnit * const calibrationUnit_, AIOModule * const module_) :
    CalibrationProcedure(calibrationUnit_, module_)
    {
    }
    
    virtual bool isDone() const override
    {
        return false;
    }
    
    virtual int getNumCalibrationStages() const override
    {
        return 0;
    }
    
protected:
};
