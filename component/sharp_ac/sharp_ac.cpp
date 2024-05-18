#include "sharp_ac.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <iostream>

namespace esphome
{
  namespace sharp_ac
  {
    static const char *const TAG = "sharp_ac.climate";
    void SharpAc::write_ack()
    {
      SharpACKFrame frame1;
      this->write_frame(frame1);
    };

    void SharpAc::setup()
    {
    }

    ClimateTraits SharpAc::traits()
    {
      auto traits = esphome::climate::ClimateTraits();
      traits.set_supports_current_temperature(true);
      traits.set_visual_min_temperature(16);
      traits.set_visual_max_temperature(30);
      traits.set_visual_temperature_step(1.0);

      traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);
      traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
      traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
      traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);

      traits.add_supported_mode(ClimateMode::CLIMATE_MODE_OFF);
      traits.add_supported_mode(ClimateMode::CLIMATE_MODE_COOL);
      traits.add_supported_mode(ClimateMode::CLIMATE_MODE_HEAT);
      traits.add_supported_mode(ClimateMode::CLIMATE_MODE_DRY);
      traits.add_supported_mode(ClimateMode::CLIMATE_MODE_FAN_ONLY);

      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_OFF);
      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_BOTH);
      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_HORIZONTAL);
      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_VERTICAL);

      return traits;
    }

    void SharpAc::startInit()
    {
      unsigned long currentMillis = millis();
      if ((currentMillis - this->connectionStart >= interval) || (this->connectionStart == 0 ) )
      {
        ESP_LOGD(TAG, "NEW CONNECTION");
        this->status = 0;
        this->connectionStart = millis();

        SharpFrame frame(init_msg, sizeof(init_msg) + 1);
        this->write_frame(frame);
      }
    }

    void SharpAc::sendInitMsg(const uint8_t *arr, size_t size)
    {
      SharpFrame frame(arr, size);
      this->write_frame(frame);
      this->status++;
    }

    void SharpAc::init(SharpFrame &frame)
    {
      if (frame.getSize() == 0)
        return;

      esphome::ESP_LOGD("ac_frame", "Sharp INIT Data: %s", esphome::format_hex_pretty(frame.getData(), frame.getSize()).c_str());

      switch (frame.getData()[0])
      {
      case 0x06:
        if (this->status == 2 || this->status == 7)
          this->status++;
        break;
      case 0x02:
        if (this->status == 0)
          sendInitMsg(init_msg2, sizeof(init_msg2) + 1);
        else if (this->status == 1)
          sendInitMsg(subscribe_msg, sizeof(subscribe_msg) + 1);
        break;
      case 0x03:
        if (this->status == 3)
          sendInitMsg(subscribe_msg2, sizeof(subscribe_msg2) + 1);
        else if (this->status == 4)
          sendInitMsg(get_state, sizeof(get_state) + 1);
        break;
      case 0xdc:
        if (this->status == 5)
          sendInitMsg(get_status, sizeof(get_status) + 1);
        else if (status == 6)
          sendInitMsg(connected_msg, sizeof(connected_msg) + 1);

        this->processUpdate(frame);
        break;
      }

    }

    SharpFrame SharpAc::readMsg()
    {
      uint8_t msg[18];
      char msg_id = this->peek();

      if (msg_id == 0x06 || msg_id == 0x00)
        return SharpFrame(this->read());

      int size = 8;

      if (this->available() < 8)
      {
        if (this->errCounter < 5)
        {
          this->errCounter++;
          return SharpFrame(msg, 0);
        }
        else{
          this->errCounter=0;
          return SharpFrame(this->read());

        }
      }
      this->errCounter=0;

      read_array(msg, 8);

      if (msg_id == 0x02)
        size += msg[6];
      else if (msg_id == 0x03 && msg[1] == 0xfe && msg[2] == 0x00)
        size = 17;
      else if (msg_id == 0xdc)
      {
        if (msg[1] == 0x0b )
          size = 14;
        else if (msg[1] == 0x0F)
          size = 18;
      }
      if (size > 8)
      {
        read_array(msg + 8, size - 8);
        return SharpFrame(msg, size);
      }

      return SharpFrame(msg, size);
    }

    void SharpAc::processUpdate(SharpFrame &frame)
    {
      if (frame.getSize() == 18)
      {
        SharpStatusFrame *status = static_cast<SharpStatusFrame *>(&frame);
        this->current_temperature = status->getTemperature();
      }
      else
      {
        SharpModeFrame *status = static_cast<SharpModeFrame *>(&frame);
        this->state.temperature = status->getTemperature();
        this->state.fan = status->getFanMode();
        this->state.mode = status->getPowerMode();
        this->state.state = status->getState();
        this->state.swingH = status->getSwingHorizontal();
        this->state.swingV = status->getSwingVertical();
      }
      this->publishUpdate();
    }
    void SharpAc::publishUpdate()
    {
      this->target_temperature = this->state.temperature;
      switch (this->state.fan)
      {
      case FanMode::auto_fan:
        this->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
        break;
      case FanMode::low:
        this->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
        break;
      case FanMode::mid:
        this->fan_mode = ClimateFanMode::CLIMATE_FAN_MEDIUM;
        break;
      case FanMode::high:
        this->fan_mode = ClimateFanMode::CLIMATE_FAN_MEDIUM;
        break;
      case FanMode::highest:
        this->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
        break;
      default:
        ESP_LOGD(TAG, "UNKNOWN FAN MODE");
      };
      switch (this->state.mode)
      {
      case PowerMode::fan:
        this->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
        break;
      case PowerMode::cool:
        this->mode = ClimateMode::CLIMATE_MODE_COOL;
        break;
      case PowerMode::heat:
        this->mode = ClimateMode::CLIMATE_MODE_HEAT;
        break;
      case PowerMode::dry:
        this->mode = ClimateMode::CLIMATE_MODE_DRY;
        break;
      default:
        ESP_LOGD(TAG, "UNKNOWN POWER MODE");
      };
      if (!this->state.state)
        this->mode = ClimateMode::CLIMATE_MODE_OFF;

      if (this->state.swingV == SwingVertical::swing && this->state.swingH == SwingHorizontal::swing)
      {
        this->swing_mode = ClimateSwingMode::CLIMATE_SWING_BOTH;
      }
      else if (this->state.swingV == SwingVertical::swing)
      {
        this->swing_mode = ClimateSwingMode::CLIMATE_SWING_VERTICAL;
      }
      else if (this->state.swingH == SwingHorizontal::swing)
      {
        this->swing_mode = ClimateSwingMode::CLIMATE_SWING_HORIZONTAL;
      }
      else
      {
        this->swing_mode = ClimateSwingMode::CLIMATE_SWING_OFF;
      }
      this->publish_state();
    }

    void SharpAc::loop()
    {
      if (this->available())
      {
        SharpFrame frame = this->readMsg();
        frame.print();
        if (status == 8)
        {
          esphome::ESP_LOGD("ac_frame", "Sharp Frame Data: %s", esphome::format_hex_pretty(frame.getData(), frame.getSize()).c_str());

          if (frame.getSize() > 1 && frame.validateChecksum())
          {
            this->write_ack();
            this->processUpdate(frame);
          }
        }
        else
        {
          this->init(frame);
        }
      }
      else
      {
        if (this->status != 8)
        {
          this->startInit();
        }
        else
        {
          unsigned long currentMillis = millis();
          if (currentMillis - previousMillis >= interval)
          {
            previousMillis = currentMillis;

            SharpFrame frame(get_status, sizeof(get_status) + 1);
            this->write_frame(frame);
          }
        }
      }
    };

    void SharpAc::control(const climate::ClimateCall &call)
    {
      SharpState clonedState(this->state);

      if (call.get_mode().has_value())
      {
        clonedState.state = true;
        if (call.get_mode() == ClimateMode::CLIMATE_MODE_FAN_ONLY)
          clonedState.mode = PowerMode::fan;
        else if (call.get_mode() == ClimateMode::CLIMATE_MODE_HEAT)
          clonedState.mode = PowerMode::heat;
        else if (call.get_mode() == ClimateMode::CLIMATE_MODE_COOL)
          clonedState.mode = PowerMode::cool;
        else if (call.get_mode() == ClimateMode::CLIMATE_MODE_OFF)
          clonedState.state = false;
      }
      if (call.get_fan_mode().has_value())
      {
        switch (call.get_fan_mode().value())
        {
        case ClimateFanMode::CLIMATE_FAN_AUTO:
        {
          clonedState.fan = FanMode::auto_fan;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_LOW:
        {
          clonedState.fan = FanMode::low;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_MEDIUM:
        {
          clonedState.fan = FanMode::mid;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_HIGH:
        {
          clonedState.fan = FanMode::highest;
          break;
        }
        }
      }
      if (call.get_swing_mode().has_value())
      {
        if (call.get_swing_mode() == ClimateSwingMode::CLIMATE_SWING_BOTH)
        {
          clonedState.swingH = SwingHorizontal::swing;
          clonedState.swingV = SwingVertical::swing;
        }
        else if (call.get_swing_mode() == ClimateSwingMode::CLIMATE_SWING_OFF)
        {
          clonedState.swingH = SwingHorizontal::middle;
          clonedState.swingV = SwingVertical::auto_position;
        }
        else if (call.get_swing_mode() == ClimateSwingMode::CLIMATE_SWING_HORIZONTAL)
        {
          clonedState.swingH = SwingHorizontal::swing;
          clonedState.swingV = SwingVertical::auto_position;
        }
        else if (call.get_swing_mode() == ClimateSwingMode::CLIMATE_SWING_VERTICAL)
        {
          clonedState.swingH = SwingHorizontal::middle;
          clonedState.swingV = SwingVertical::swing;
        }
      }
      if (call.get_target_temperature().has_value())
      {
        clonedState.temperature = static_cast<int>(call.get_target_temperature().value());
      }

      SharpCommandFrame frame = clonedState.toFrame();
      esphome::ESP_LOGD("ac_frame", "Sharp Frame Data: %s", esphome::format_hex_pretty(frame.getData(), frame.getSize()).c_str());

      this->write_frame(frame);
    }
  }
}