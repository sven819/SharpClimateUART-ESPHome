#pragma once
#include "propertyTypes.h"
#include "frame.h"

class SharpState
{
public:
    bool state;
    PowerMode mode;
    FanMode fan;
    SwingHorizontal swingH; 
    SwingVertical swingV; 
    int temperature;

    SharpCommandFrame toFrame(){
        SharpCommandFrame frame; 
        frame.setData(this); 
        return frame; 
    }
    
    SharpState() : state(false), mode(PowerMode::fan), fan(FanMode::low), temperature(16) {}

    SharpState(const SharpState &other)
    {
        state = other.state;
        mode = other.mode;
        fan = other.fan;
        temperature = other.temperature;
        swingH = other.swingH; 
        swingV = other.swingV;
    }
};