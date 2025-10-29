#include "core_logic.h"
#include <iostream>
#include <cstdarg>

namespace esphome
{
  namespace sharp_ac
  {

    SharpAcCore::SharpAcCore(SharpAcHardwareInterface* hardware, SharpAcStateCallback* callback)
      : hardware(hardware), callback(callback) 
    {
      hardware->log_debug(TAG, "SharpAcCore initialized successfully");
    }

    void SharpAcCore::write_ack()
    {
      SharpACKFrame frame;
      this->write_frame(frame);
    }

    void SharpAcCore::setup()
    {
    }

    void SharpAcCore::startInit()
    {
      // Don't send if we're already waiting for a response
      if (awaitingResponse) {
        return;
      }

      if (this->connectionStart == 0) {
        hardware->log_debug(TAG, "Initializing connection...");
        this->connectionStart = hardware->get_millis();
      }

      SharpFrame frame(init_msg, sizeof(init_msg) + 1);
      this->write_frame(frame);
    }

    void SharpAcCore::sendInitMsg(const uint8_t *arr, size_t size)
    {
      SharpFrame frame(arr, size);
      this->write_frame(frame);
      this->status++;
      
      if (callback) {
        callback->on_connection_status_update(this->status);
      }
      
      if (this->status < 8) {
        hardware->log_debug(TAG, "Connecting (%d/8)...", this->status);
      } else {
        hardware->log_debug(TAG, "Connected");
      }
    }

    void SharpAcCore::init(SharpFrame &frame)
    {
      if (frame.getSize() == 0)
        return;

      switch (frame.getData()[0])
      {
      case 0x06:
        if (this->status == 2 || this->status == 7)
        {
          this->status++;
          
          // Notify status change
          if (callback) {
            callback->on_connection_status_update(this->status);
          }
          
          // Log progress for ACK steps (status 2->3 and 7->8)
          if (this->status < 8) {
            hardware->log_debug(TAG, "Connecting (%d/8)...", this->status);
          } else {
            hardware->log_debug(TAG, "Connected");
          }
        }
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
        else if (this->status == 6)
          sendInitMsg(connected_msg, sizeof(connected_msg) + 1);

        this->processUpdate(frame);
        break;
      }
    }

    SharpFrame SharpAcCore::readMsg()
    {
      uint8_t msg[18];
      uint8_t msg_id = hardware->peek();

      if (msg_id == 0x06 || msg_id == 0x00)
      {
        uint8_t singleByte = hardware->read();
        SharpFrame frame(singleByte);

        hardware->log_debug(TAG, "RX: ACK");
        awaitingResponse = false;
        
        return frame;
      }

      int size = 8;

      if (hardware->available() < 8)
      {
        if (this->errCounter < 5)
        {
          this->errCounter++;
          return SharpFrame(msg, 0);
        }
        else
        {
          this->errCounter = 0;
          uint8_t singleByte = hardware->read();
          SharpFrame frame(singleByte);
          
          hardware->log_debug(TAG, "RX: %s (error recovery)", hardware->format_hex_pretty(&singleByte, 1).c_str());
          
          return frame;
        }
      }
      this->errCounter = 0;

      hardware->read_array(msg, 8);

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
        hardware->read_array(msg + 8, size - 8);
        SharpFrame frame(msg, size);
        
        hardware->log_debug(TAG, "RX: %s", hardware->format_hex_pretty(frame.getData(), frame.getSize()).c_str());
        
        awaitingResponse = false;
        
        return frame;
      }

      SharpFrame frame(msg, size);
      
      hardware->log_debug(TAG, "RX: %s", hardware->format_hex_pretty(frame.getData(), frame.getSize()).c_str());
      
      // Mark that we received a valid response
      awaitingResponse = false;
      
      return frame;
    }

    std::string SharpAcCore::analyzeByte(uint8_t byte, size_t position, bool isStatusFrame) {
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
                default: mode = "Unknown"; break;
              }
              result = std::string("Power Mode: ") + mode + " (0x" + hardware->format_hex_pretty(&byte, 1) + ")";
              break;
            }
          case 5:
            {
              std::string fan;
              switch(static_cast<FanMode>(byte & 0x0F)) {
                case FanMode::low: fan = "Low"; break;
                case FanMode::mid: fan = "Mid"; break;
                case FanMode::high: fan = "High"; break;
                case FanMode::highest: fan = "Highest"; break;
                case FanMode::auto_fan: fan = "Auto"; break;
                default: fan = "Unknown"; break;
              }
              result = std::string("Fan Mode: ") + fan + " (0x" + hardware->format_hex_pretty(&byte, 1) + ")";
              break;
            }
          case 6:
            {
              std::string swing;
              // Horizontal swing (upper 4 bits)
              switch(static_cast<SwingHorizontal>((byte >> 4) & 0x0F)) {
                case SwingHorizontal::swing: swing += "Swing"; break;
                case SwingHorizontal::left: swing += "Left"; break;
                case SwingHorizontal::middle: swing += "Middle"; break;
                case SwingHorizontal::right: swing += "Right"; break;
                default: swing += "Unknown H"; break;
              }
              swing += "/";
              // Vertical swing (lower 4 bits)
              switch(static_cast<SwingVertical>(byte & 0x0F)) {
                case SwingVertical::swing: swing += "Swing"; break;
                case SwingVertical::auto_position: swing += "Auto"; break;
                case SwingVertical::highest: swing += "Highest"; break;
                case SwingVertical::high: swing += "High"; break;
                case SwingVertical::mid: swing += "Middle"; break;
                case SwingVertical::low: swing += "Low"; break;
                case SwingVertical::lowest: swing += "Lowest"; break;
                default: swing += "Unknown V"; break;
              }
              result = swing + " (0x" + hardware->format_hex_pretty(&byte, 1) + ")";
              break;
            }
          case 11:
            {
              std::string features;
              Preset preset = static_cast<Preset>((byte >> 1) & 0x03);
              switch(preset) {
                case Preset::NONE: features += "None"; break;
                case Preset::ECO: features += "Eco"; break;
                case Preset::FULLPOWER: features += "Full Power"; break;
                default: features += "Unknown"; break;
              }
              result = features + " (0x" + hardware->format_hex_pretty(&byte, 1) + ")";
              break;
            }
          default:
            snprintf(hex, sizeof(hex), "0x%02X", byte);
            result = std::string("Position ") + std::to_string(position) + ": " + hex;
        }
      }
      return result;
    }

    void SharpAcCore::processUpdate(SharpFrame &frame)
    {
      if (frame.getSize() == 0 || frame.getSize() == 1) {
        return;
      }
      
      // Status-Frames (18 Byte): Only Temperature (no state update needed)
      if (frame.getSize() == 18)
      {
        SharpStatusFrame *status = static_cast<SharpStatusFrame *>(&frame);
        this->currentTemperature = status->getTemperature();
        hardware->log_debug(TAG, "Current temp: %.1f°C", this->currentTemperature);
        // Publish only temperature update without changing state
        this->publishUpdate();
      }
      // Mode-Frames (14 Byte): Full State
      else if (frame.getSize() >= 14)
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
            this->state.temperature = status->getTemperature();
        }
        
        // Only publish update if we have received at least one temperature reading
        // This prevents showing 0°C before the first status frame
        if (this->currentTemperature > 0.0f) {
          this->publishUpdate();
        } else {
          hardware->log_debug(TAG, "Waiting for temperature reading...");
        }
      }
    }

    void SharpAcCore::publishUpdate()
    {
      if (callback) {
        callback->on_state_update();
        callback->on_ion_state_update(this->state.ion);
        callback->on_vane_horizontal_update(this->state.swingH);
        callback->on_vane_vertical_update(this->state.swingV);
      }
    }

    void SharpAcCore::setIon(bool state)
    {
      this->state.ion = state;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::setVaneHorizontal(SwingHorizontal state)
    {
      this->state.swingH = state;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::setVaneVertical(SwingVertical state)
    {
      this->state.swingV = state;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::controlMode(PowerMode mode, bool state)
    {
      this->state.state = state;
      if (state)
        this->state.mode = mode;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::controlFan(FanMode fan)
    {
      this->state.fan = fan;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::controlSwing(SwingHorizontal h, SwingVertical v)
    {
      this->state.swingH = h;
      this->state.swingV = v;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::controlTemperature(int temperature)
    {
      this->state.temperature = temperature;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::controlPreset(Preset preset)
    {
      this->state.preset = preset;
      SharpCommandFrame frame = this->state.toFrame();
      this->write_frame(frame);
    }

    void SharpAcCore::resetConnection()
    {
      this->status = 0;
      this->awaitingResponse = false;
      this->connectionStart = 0;
      
      if (callback) {
        callback->on_connection_status_update(0);
      }
    }

    void SharpAcCore::checkTimeout()
    {
      if (!awaitingResponse) {
        return; 
      }

      unsigned long currentMillis = hardware->get_millis();
      if (currentMillis - lastRequestTime >= responseTimeout) {
        hardware->log_debug(TAG, "Timeout - no response for 10s, reconnecting...");
        resetConnection();
      }
    }

    void SharpAcCore::loop()
    {
      unsigned long currentMillis = hardware->get_millis();

      checkTimeout();

      if (hardware->available() > 0) {
      
          SharpFrame frame = this->readMsg();
          frame.print();

          if (status < 8)
          {
              this->init(frame);
          }
          else
          {
            this->processUpdate(frame);
            if (frame.getSize() > 1) {
            this->write_ack();
          }
          }
          
      
     }
      if (this->status != 8)
      {
        this->startInit();
      }
      else
      {
        if (currentMillis - previousMillis >= interval)
        {
          previousMillis = currentMillis;

          SharpFrame frame(get_status, sizeof(get_status) + 1);
          this->write_frame(frame);
        }
      }
     
      }
    }
}
