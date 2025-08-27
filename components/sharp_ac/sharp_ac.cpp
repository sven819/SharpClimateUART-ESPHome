#include "sharp_ac.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <iostream>
#include "vane_select_horizontal.h"
#include "vane_select_vertical.h"

namespace esphome
{
  namespace sharp_ac
  {
    static const char *const TAG = "sharp_ac.climate";
    void SharpAc::write_ack()
    {
      SharpACKFrame frame;
      this->write_frame(frame);
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

      traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_ECO); 
      traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_BOOST); 
      traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE); 

      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_OFF);
      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_BOTH);
      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_HORIZONTAL);
      traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_VERTICAL);

      return traits;
    }

    void SharpAc::startInit()
    {
      unsigned long currentMillis = millis();
      if ((currentMillis - this->connectionStart >= interval) || (this->connectionStart == 0))
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

      ESP_LOGD("ac_frame", "Sharp INIT Data: %s", esphome::format_hex_pretty(frame.getData(), frame.getSize()).c_str());

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
        else
        {
          this->errCounter = 0;
          return SharpFrame(this->read());
        }
      }
      this->errCounter = 0;

      read_array(msg, 8);

      if (msg_id == 0x02)
        size += msg[6];
      else if (msg_id == 0x03 && msg[1] == 0xfe && msg[2] == 0x00)
        size = 17;
      else if (msg_id == 0xdc)
      {
        if (msg[1] == 0x0b)
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

    std::string SharpAc::analyzeByte(uint8_t byte, size_t position, bool isStatusFrame) {
      std::string result;
      char hex[8];

      if (isStatusFrame) {
        switch(position) {
          case 7:
            snprintf(hex, sizeof(hex), "0x%02X", byte & 0x0F);
            result = std::string("Temperatur LSB: ") + hex + " (" + std::to_string((byte & 0x0F) + 16) + "°C)";
            break;
          case 8:
            snprintf(hex, sizeof(hex), "0x%02X", byte);
            result = std::string("Temperatur MSB: ") + hex;
            break;
          default:
            snprintf(hex, sizeof(hex), "0x%02X", byte);
            result = std::string("Unbekannt: ") + hex;
        }
      } else {
        switch(position) {
          case 4:
            {
              std::string mode;
              switch(static_cast<PowerMode>(byte & 0x0F)) {
                case PowerMode::heat: mode = "Heat"; break;
                case PowerMode::cool: mode = "Cool"; break;
                case PowerMode::dry: mode = "Dry"; break;
                case PowerMode::fan: mode = "Fan"; break;
                default: mode = "Unknown";
              }
              result = std::string("Power Mode: ") + mode + " (0x" + esphome::format_hex_pretty(&byte, 1) + ")";
            }
            break;
          case 5:
            {
              std::string fan;
              switch(static_cast<FanMode>(byte & 0x0F)) {
                case FanMode::low: fan = "Low"; break;
                case FanMode::mid: fan = "Mid"; break;
                case FanMode::high: fan = "High"; break;
                case FanMode::highest: fan = "Highest"; break;
                case FanMode::auto_fan: fan = "Auto"; break;
                default: fan = "Unknown";
              }
              result = std::string("Fan Mode: ") + fan + " (0x" + esphome::format_hex_pretty(&byte, 1) + ")";
            }
            break;
          case 6:
            {
              std::string swing_v;
              switch(static_cast<SwingVertical>(byte & 0x0F)) {
                case SwingVertical::swing: swing_v = "Swing"; break;
                case SwingVertical::auto_position: swing_v = "Auto"; break;
                case SwingVertical::highest: swing_v = "Highest"; break;
                case SwingVertical::high: swing_v = "High"; break;
                case SwingVertical::mid: swing_v = "Mid"; break;
                case SwingVertical::low: swing_v = "Low"; break;
                case SwingVertical::lowest: swing_v = "Lowest"; break;
                default: swing_v = "Unknown";
              }
              result = std::string("Swing Vertical: ") + swing_v + " (0x" + esphome::format_hex_pretty(&byte, 1) + ")";
            }
            break;
          case 7:
            {
              std::string swing_h;
              switch(static_cast<SwingHorizontal>(byte & 0x0F)) {
                case SwingHorizontal::swing: swing_h = "Swing"; break;
                case SwingHorizontal::middle: swing_h = "Middle"; break;
                case SwingHorizontal::right: swing_h = "Right"; break;
                case SwingHorizontal::left: swing_h = "Left"; break;
                default: swing_h = "Unknown";
              }
              result = std::string("Swing Horizontal: ") + swing_h + " (0x" + esphome::format_hex_pretty(&byte, 1) + ")";
              
              bool ion = (byte & IonMode) != 0;
              result += std::string(", Ion: ") + (ion ? "ON" : "OFF");
            }
            break;
          default:
            snprintf(hex, sizeof(hex), "0x%02X", byte);
            result = std::string("Unknown: ") + hex;
        }
      }
      return result;
    }

    void SharpAc::processUpdate(SharpFrame &frame)
    {
      ESP_LOGD("sharp_ac", "Receive Frame (%d Bytes):", frame.getSize());
      
      bool isStatusFrame = (frame.getSize() == 18);
      const uint8_t* data = frame.getData();
      
      for (size_t i = 0; i < frame.getSize(); i++) {
        std::string analysis = analyzeByte(data[i], i, isStatusFrame);
        ESP_LOGD("sharp_ac", "Byte %2d: %s", i, analysis.c_str());
      }
      
      if (isStatusFrame)
      {
        SharpStatusFrame *status = static_cast<SharpStatusFrame *>(&frame);
        this->current_temperature = status->getTemperature();
        ESP_LOGD("sharp_ac", "Status Frame - Aktuelle Temperatur: %.1f°C", this->current_temperature);
      }
      else
      {
        SharpModeFrame *status = static_cast<SharpModeFrame *>(&frame);
        this->state.fan = status->getFanMode();
        this->state.mode = status->getPowerMode();
        this->state.state = status->getState();
        this->state.swingH = status->getSwingHorizontal();
        this->state.swingV = status->getSwingVertical();
        this->state.preset = status->getPreset();

        if (this->state.state)
        {
          this->state.ion = status->getIon();
          if (this->state.mode == PowerMode::cool || this->state.mode == PowerMode::heat)
          {
            this->state.temperature = status->getTemperature();
          }
        }

        ESP_LOGD("sharp_ac", "Zusammenfassung des Mode-Frames:");
        ESP_LOGD("sharp_ac", "  Power: %s", this->state.state ? "ON" : "OFF");
        ESP_LOGD("sharp_ac", "  Modus: %s", this->state.mode == PowerMode::heat ? "Heat" :
                                           this->state.mode == PowerMode::cool ? "Cool" :
                                           this->state.mode == PowerMode::dry ? "Dry" :
                                           this->state.mode == PowerMode::fan ? "Fan" : "Unknown");
        ESP_LOGD("sharp_ac", "  Lüfter: %s", this->state.fan == FanMode::auto_fan ? "Auto" :
                                             this->state.fan == FanMode::low ? "Low" :
                                             this->state.fan == FanMode::mid ? "Mid" :
                                             this->state.fan == FanMode::high ? "High" :
                                             this->state.fan == FanMode::highest ? "Highest" : "Unknown");
        if (this->state.mode == PowerMode::cool || this->state.mode == PowerMode::heat)
        {
          ESP_LOGD("sharp_ac", "  Temperatur: %.1f°C", this->state.temperature);
        }
        ESP_LOGD("sharp_ac", "  Ion: %s", this->state.ion ? "ON" : "OFF");
        ESP_LOGD("sharp_ac", "  Preset: %s", this->state.preset == Preset::NONE ? "None" :
                                             this->state.preset == Preset::ECO ? "ECO" :
                                             this->state.preset == Preset::FULLPOWER ? "Full Power" : "Unknown");
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

      if(this->state.preset == Preset::NONE){
        this->preset = ClimatePreset::CLIMATE_PRESET_NONE; 
      }else if(this->state.preset == Preset::ECO){
        this->preset = ClimatePreset::CLIMATE_PRESET_ECO; 
      } else if(this->state.preset == Preset::FULLPOWER){
        this->preset = ClimatePreset::CLIMATE_PRESET_BOOST; 
      }

      this->publish_state();
      if (this->vaneHorizontal != nullptr)
        this->vaneHorizontal->setVal(this->state.swingH);
      if (this->vaneVertical != nullptr)
        this->vaneVertical->setVal(this->state.swingV);
      if (this->ionSwitch != nullptr)
      {
        this->ionSwitch->publish_state(this->state.ion);
      }
    }

    void SharpAc::setIon(bool state)
    {
      this->state.ion = state;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAc::setVaneHorizontal(SwingHorizontal state)
    {
      this->state.swingH = state;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAc::setVaneVertical(SwingVertical state)
    {
      this->state.swingV = state;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAc::loop()
    {
      if (this->available())
      {
        SharpFrame frame = this->readMsg();
        frame.print();
        if (status == 8)
        {
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
        case ClimateFanMode::CLIMATE_FAN_ON:
        {
          clonedState.fan = FanMode::auto_fan;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_OFF:
        {
          clonedState.state = false;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_MIDDLE:
        {
          clonedState.fan = FanMode::mid;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_FOCUS:
        {
          clonedState.fan = FanMode::high;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_DIFFUSE:
        {
          clonedState.fan = FanMode::low;
          break;
        }
        case ClimateFanMode::CLIMATE_FAN_QUIET:
        {
          clonedState.fan = FanMode::low;
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
      if(call.get_preset().has_value())
      {
        if(call.get_preset() == ClimatePreset::CLIMATE_PRESET_NONE)
        {
          clonedState.preset = Preset::NONE;
        }else  if(call.get_preset() == ClimatePreset::CLIMATE_PRESET_ECO)
        {
          clonedState.preset = Preset::ECO;

        }else  if(call.get_preset() == ClimatePreset::CLIMATE_PRESET_BOOST)
        {
          clonedState.preset = Preset::FULLPOWER;
        }
      }

      SharpCommandFrame frame = clonedState.toFrame();
      this->write_frame(frame);
    }
  }
}