#include "base.h"

class DelayTask : public ThreadWithProgressWindow
{
public:
    DelayTask(double delayMsec_) :
    ThreadWithProgressWindow(JUCEApplication::getInstance()->getApplicationName(),
                             true,
                             true),
    delay(delayMsec_ * 0.001)
    {
    }
    
    void run()
    {
        Time now(Time::getCurrentTime());
        Time timeout(now + delay);
        
        setStatusMessage("Waiting...");
        
        while (now < timeout)
        {
            if (threadShouldExit())
                return;
            
            sleep(100);
            
            now = Time::getCurrentTime();
            RelativeTime remainingTime = timeout - now;
            RelativeTime progress = delay - remainingTime;
            setProgress( progress.inSeconds() / delay.inSeconds());
        }
    }
    
protected:
    RelativeTime delay;
};

Result runDelayTask(XmlElement *element)
{
    double delayMsec = element->getAllSubText().getDoubleValue();
    if (delayMsec < 1.0 || delayMsec > 100000.0)
        return Result::fail("Value out of range");
    
    DelayTask task(delayMsec);
    
    if (task.runThread())
        return Result::ok();
    
    return Result::fail("User cancel");
}