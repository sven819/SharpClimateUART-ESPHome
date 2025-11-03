#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome
{
  namespace sharp_ac
  {
    class SharpAc;

    class ReconnectButton : public button::Button, public Component
    {
    public:
      void set_parent(SharpAc *parent) { this->parent_ = parent; }

    protected:
      void press_action() override;
      SharpAc *parent_{nullptr};
    };

  } 
}
