#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

#include "core_types.h"
#include "core_state.h"
#include "core_frame.h"
#include "core_messages.h"

namespace esphome
{
  namespace sharp_ac
  {
    static const char *const TAG = "sharp_ac.climate";

    class SharpAcHardwareInterface {
    public:
      virtual ~SharpAcHardwareInterface() = default;
      virtual size_t read_array(uint8_t *data, size_t len) = 0;
      virtual size_t available() = 0;
      virtual void write_array(const uint8_t *data, size_t len) = 0;
      virtual uint8_t peek() = 0;
      virtual uint8_t read() = 0;
      virtual unsigned long get_millis() = 0;
      virtual void log_debug(const char* tag, const char* format, ...) = 0;
      virtual std::string format_hex_pretty(const uint8_t *data, size_t len) = 0;
    };

    class SharpAcStateCallback {
    public:
      virtual ~SharpAcStateCallback() = default;
      virtual void on_state_update() = 0;
      virtual void on_ion_state_update(bool state) = 0;
      virtual void on_vane_horizontal_update(SwingHorizontal val) = 0;
      virtual void on_vane_vertical_update(SwingVertical val) = 0;
      virtual void on_connection_status_update(int status) = 0;
    };

    class SharpAcCore
    {
    public:
      SharpAcCore(SharpAcHardwareInterface* hardware, SharpAcStateCallback* callback);
      virtual ~SharpAcCore() = default;

      void loop();
      void setup();

      void setIon(bool state);
      void setVaneHorizontal(SwingHorizontal val);
      void setVaneVertical(SwingVertical val);

      void publishUpdate();
      const SharpState& getState() const { return state; }
      float getCurrentTemperature() const { return currentTemperature; }

      void controlMode(PowerMode mode, bool state);
      void controlFan(FanMode fan);
      void controlSwing(SwingHorizontal h, SwingVertical v);
      void controlTemperature(int temperature);
      void controlPreset(Preset preset);
      void resetConnection();

    protected:
      std::string analyzeByte(uint8_t byte, size_t position, bool isStatusFrame);

      void write_frame(SharpFrame &frame)
      {
        frame.setChecksum();
        frame.print();
        
        if (frame.getSize() == 1 && frame.getData()[0] == 0x06) {
          hardware->log_debug(TAG, "TX: ACK");
        } else {
          hardware->log_debug(TAG, "TX: %s", hardware->format_hex_pretty(frame.getData(), frame.getSize()).c_str());
          awaitingResponse = true;
          lastRequestTime = hardware->get_millis();
        }
        
        hardware->write_array(frame.getData(), frame.getSize());
      }

      void write_ack();

    private:
      SharpAcHardwareInterface* hardware;
      SharpAcStateCallback* callback;

      void sendInitMsg(const uint8_t *arr, size_t size);
      int errCounter = 0;

    protected:
      SharpState state;
      void init(SharpFrame &frame);
      SharpFrame readMsg();
      void processUpdate(SharpFrame &frame);
      void startInit();
      void checkTimeout();
      int status = 0;
      unsigned long connectionStart = 0;
      unsigned long previousMillis = 0;
      unsigned long lastRequestTime = 0;
      bool awaitingResponse = false;
      const long interval = 60000;
      const long responseTimeout = 10000; // 10 seconds
      float currentTemperature = 0.0f;
    };
  }
}
