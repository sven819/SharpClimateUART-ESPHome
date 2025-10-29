#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome
{
  namespace sharp_ac
  {
    class SharpAc;

    class ConnectionStatusSensor : public text_sensor::TextSensor, public Component
    {
    public:
      void set_parent(SharpAc *parent) { this->parent_ = parent; }
      
      void publish_status(const std::string &status)
      {
        if (this->state != status)
        {
          this->publish_state(status);
        }
      }

    protected:
      SharpAc *parent_{nullptr};
    };

  } 
}
