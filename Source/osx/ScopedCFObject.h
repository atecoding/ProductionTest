#pragma once

#ifdef JUCE_MAC

template<class ObjectType> class ScopedCFObject
{
public:
    ScopedCFObject() :
    object(0)
    {

    }
    
    ~ScopedCFObject()
    {
        if (object)
        {
            CFRelease(object);
            object = 0;
        }
    }

    ObjectType object;
};


template<class ObjectType> struct OwnedCFObjectArray
{
    Array<ObjectType> objects;
    
    ~OwnedCFObjectArray()
    {
        for (int i = 0; i < objects.size(); ++i)
        {
            ObjectType object = objects[i];
            
            objects.set(i, 0);
            if (object)
                CFRelease(object);
        }
    }
};

#endif