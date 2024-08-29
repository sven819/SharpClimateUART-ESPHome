#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"

#include "propertyTypes.h"
#include "state.h"
#include "frame.h"
#include "messages.h"

namespace esphome
{
  namespace sharp_ac
  {

    using climate::ClimateCall;
    using climate::ClimateFanMode;
    using climate::ClimateMode;
    using climate::ClimatePreset;
    using climate::ClimateSwingMode;
    using climate::ClimateTraits;

    class VaneSelectVertical;
    class VaneSelectHorizontal;

    class SharpAc : public climate::Climate, public esphome::uart::UARTDevice, public Component
    {
    public:
      SharpAc() {}
      void control(const climate::ClimateCall &call) override;
      void loop() override;
      void setup() override;
      esphome::climate::ClimateTraits traits() override;

      void setIon(bool state);
      void setVaneHorizontal(SwingHorizontal val);
      void setVaneVertical(SwingVertical val);

      void publishUpdate();

      size_t read_array(uint8_t *data, size_t len) noexcept
      {
        return uart::UARTDevice::read_array(data, len) ? len : 0;
      };

      size_t available() noexcept
      {
        return uart::UARTDevice::available();
      };

      void write_frame(SharpFrame &frame)
      {
        frame.setChecksum();
        frame.print();
        uart::UARTDevice::write_array(frame.getData(), frame.getSize());
      }

      void write_ack();

      void setIonSwitch(switch_::Switch *ionSwitch)
      {
        this->ionSwitch = ionSwitch;
      };
      void setVaneVerticalSelect(VaneSelectVertical *vane)
      {
        this->vaneVertical = vane;
      };
      void setVaneHorizontalSelect(VaneSelectHorizontal *vane)
      {
        this->vaneHorizontal = vane;
      };

    private:
      void sendInitMsg(const uint8_t *arr, size_t size);
      int errCounter = 0;

      switch_::Switch *ionSwitch;
      VaneSelectVertical *vaneVertical;
      VaneSelectHorizontal *vaneHorizontal;

    protected:
      SharpState state;
      void init(SharpFrame &frame);
      SharpFrame readMsg();
      void processUpdate(SharpFrame &frame);
      void startInit();
      int status = 0;
      unsigned long connectionStart = 0;
      unsigned long previousMillis = 0;
      const long interval = 60000;
    };
  }
}