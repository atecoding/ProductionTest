#include "base.h"

class DelayTask : public ThreadWithProgressWindow
{
public:
    DelayTask(double delayMsec_, bool &running_) :
    ThreadWithProgressWindow(JUCEApplication::getInstance()->getApplicationName(),
                             true,
                             true),
    delay(delayMsec_ * 0.001),
    running(running)
    {
    }
    
    void run()
    {
        Time now(Time::getCurrentTime());
        Time timeout(now + delay);
        
        setStatusMessage("Waiting...");
        
        while (now < timeout && running)
        {
            if (threadShouldExit())
                return;
            
            sleep(100);
            
            now = Time::getCurrentTime();
            RelativeTime remainingTime = timeout - now;
            RelativeTime elapsed = delay - remainingTime;
            setProgress(elapsed.inSeconds() / delay.inSeconds());
        }
    }
    
protected:
    RelativeTime delay;
    bool running;
};

Result runDelayTask(XmlElement *element, bool &running_)
{
    double delayMsec = element->getAllSubText().getDoubleValue();
    if (delayMsec < 1.0 || delayMsec > 100000.0)
        return Result::fail("Value out of range");
    
    DelayTask task(delayMsec, running_);
    
    if (task.runThread())
        return Result::ok();
    
    return Result::fail("User cancel");
}