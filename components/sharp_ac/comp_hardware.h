#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "core_logic.h"
#include "core_types.h"
#include "core_state.h"
#include "core_frame.h"
#include "core_messages.h"

#include <memory>
#include <cstdarg>

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
    class SharpAc; 

    class ESPHomeHardwareInterface : public SharpAcHardwareInterface {
    public:
      ESPHomeHardwareInterface(uart::UARTDevice* uart_device) : uart_device_(uart_device) {}

      size_t read_array(uint8_t *data, size_t len) override {
        return uart_device_->read_array(data, len) ? len : 0;
      }

      size_t available() override {
        return uart_device_->available();
      }

      void write_array(const uint8_t *data, size_t len) override {
        uart_device_->write_array(data, len);
      }

      uint8_t peek() override {
        return uart_device_->peek();
      }

      uint8_t read() override {
        return uart_device_->read();
      }

      unsigned long get_millis() override {
        return millis();
      }

      void log_debug(const char* tag, const char* format, ...) override {
        va_list args;
        va_start(args, format);
        esp_log_vprintf_(ESPHOME_LOG_LEVEL_DEBUG, tag, __LINE__, format, args);
        va_end(args);
      }

      std::string format_hex_pretty(const uint8_t *data, size_t len) override {
        return esphome::format_hex_pretty(data, len);
      }

    private:
      uart::UARTDevice* uart_device_;
    };

    class ESPHomeStateCallback : public SharpAcStateCallback {
    public:
      ESPHomeStateCallback(SharpAc* sharp_ac) : sharp_ac_(sharp_ac) {}

      void on_state_update() override;
      void on_ion_state_update(bool state) override;
      void on_vane_horizontal_update(SwingHorizontal val) override;
      void on_vane_vertical_update(SwingVertical val) override;

    private:
      SharpAc* sharp_ac_;
    };

    class SharpAc : public climate::Climate, public uart::UARTDevice, public Component
    {
    public:
      SharpAc();
      
      void control(const climate::ClimateCall &call) override;
      void loop() override;
      void setup() override;
      esphome::climate::ClimateTraits traits() override;

      void setIon(bool state);
      void setVaneHorizontal(SwingHorizontal val);
      void setVaneVertical(SwingVertical val);

      void publishUpdate();

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
      std::unique_ptr<ESPHomeHardwareInterface> hardware_interface_;
      std::unique_ptr<ESPHomeStateCallback> state_callback_;
      std::unique_ptr<SharpAcCore> core_;

      switch_::Switch *ionSwitch;
      VaneSelectVertical *vaneVertical;
      VaneSelectHorizontal *vaneHorizontal;
    };
  }
}
