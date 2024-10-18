#include "f60tdn.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace f60tdn
  {

    static const char *TAG = "f60tdn.fan";
    uint16_t address;
    uint8_t first_on_temp;

    static const uint16_t BITWISE = 425;
    static const uint16_t HEADER_HIGH_US = BITWISE * 8;
    static const uint16_t HEADER_LOW_US = BITWISE * 4;
    static const uint16_t BIT_HIGH_US = BITWISE;
    static const uint16_t BIT_ONE_LOW_US = BITWISE * 3;
    static const uint16_t BIT_ZERO_LOW_US = BITWISE;
    static const uint16_t TRAILER = BITWISE;

    // Power
    const uint8_t FAN_ON = 0x0C;
    const uint8_t FAN_FIRST_ON = 0x4C;
    const uint8_t FAN_OFF = 0xCC;
    // Fan speed
    const uint8_t FAN_SPEED_1 = 0x84;
    const uint8_t FAN_SPEED_2 = 0x44;
    const uint8_t FAN_SPEED_3 = 0xC4;
    const uint8_t FAN_SPEED_4 = 0x24;
    const uint8_t FAN_SPEED_5 = 0xA4;
    const uint8_t FAN_SPEED_6 = 0x64;
    const uint8_t FAN_SPEED_7 = 0xE4;
    const uint8_t FAN_SPEED_8 = 0x14;
    const uint8_t FAN_SPEED_9 = 0x94;
    // i/f yuragi mode
    const uint8_t FAN_YURAGI_MODE_ON = 0x40;
    const uint8_t FAN_YURAGI_MODE_OFF = 0x80;
    
    uint8_t remote_state[] = {
          0x40, 0x04, 0x0B, 0x21,
          0x8C, 0x80, 0x84, 0x0C,
          0xF2, 0xB2, 0x0E, 0x81,
          0xC1, 0x1A, 0x0A
          };

    void F60tdnFan::setup()
    {
      auto restore = this->restore_state_();
      if (restore.has_value())
      {
        restore->apply(*this);
      }
      // Construct traits
      this->traits_ =
          fan::FanTraits(this->has_oscillating_, this->speed_count_ > 0, this->has_direction_, this->speed_count_);
      this->traits_.set_supported_preset_modes(this->preset_modes_);
    }

    void F60tdnFan::dump_config() { LOG_FAN("", "F60tdn Fan", this); }

    void F60tdnFan::control(const fan::FanCall &call)
    {
      if (call.get_state().has_value())
        this->state = *call.get_state();
      if (call.get_speed().has_value() && (this->speed_count_ > 0))
        this->speed = *call.get_speed();
      if (call.get_oscillating().has_value() && this->has_oscillating_)
        this->oscillating = *call.get_oscillating();
      if (call.get_direction().has_value() && this->has_direction_)
        this->direction = *call.get_direction();
      this->preset_mode = call.get_preset_mode();
      this->transmit_state();
      //this->publish_state();
    }

    bool F60tdnFan::on_receive(remote_base::RemoteReceiveData src)
    {
      uint8_t bytes_received[15] = {};
      ESP_LOGD(TAG, "Received some bytes");
      if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
        return false;

      for (uint16_t mask = 1 << 15; mask != 0; mask >>= 1)
      {
        if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US))
        {
          address |= mask;
        }
        else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US))
        {
          address &= ~mask;
        }
        else
        {
          return false;
        }
      }
      // Check the address
      if (address != 0x4004)
      {
        return false;
      }
      ESP_LOGD(TAG, "Received address 0x%04X", address);

      for (uint8_t pos = 0; pos < 15; pos++)
      {
        uint8_t data = 0;
        for (uint8_t mask = 1 << 7; mask != 0; mask >>= 1)
        {
          if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US))
          {
            data |= mask;
          }
          else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US))
          {
            data &= ~mask;
          }
          else if (pos > 1 && src.expect_mark(TRAILER))
          {
            break;
          }
          else
          {
            break;
          }
        }
        bytes_received[pos] = data;
      }
      for(int i=0;i<13;i++){
      ESP_LOGD(TAG, "Received bytes 0x%02X",bytes_received[i]);
      }
      auto powerMode = bytes_received[5];
      auto fanSpeed = bytes_received[4];
      auto yuragiMode = bytes_received[3];

      switch (powerMode){
      case FAN_OFF:
        this->state = false;
        first_on_temp = 0;
        break;
      case FAN_ON:
        this->state = true;
        break;
      case FAN_FIRST_ON:
        this->state = true;
        first_on_temp = 1;
        break;
      default:
        break;
      }

      switch (yuragiMode){
      case FAN_YURAGI_MODE_ON:
        this->oscillating = true;
        break;
      case FAN_YURAGI_MODE_OFF:
        this->oscillating = false;
        break;
      default:
        break;
      }

      switch (fanSpeed){
      case FAN_SPEED_1:
        this->speed = 1;
        break;
      case FAN_SPEED_2:
        this->speed = 2;
        break;
      case FAN_SPEED_3:
        this->speed = 3;
        break;
      case FAN_SPEED_4:
        this->speed = 4;
        break;
      case FAN_SPEED_5:
        this->speed = 5;
        break;
      case FAN_SPEED_6:
        this->speed = 6;
        break;
      case FAN_SPEED_7:
        this->speed = 7;
        break;
      case FAN_SPEED_8:
        this->speed = 8;
        break;
      case FAN_SPEED_9:
        this->speed = 9;
        break;
      default:
        break;
      }
      this->publish_state();
      return true;
    }

    void F60tdnFan::send_code(){
      auto transmit = this->transmitter_->transmit();
      auto data = transmit.get_data();
      data->set_carrier_frequency(38000);
      data->reserve(2 + 32 + (13 * 2) + 1);
      data->item(HEADER_HIGH_US, HEADER_LOW_US);
      for (uint8_t bit : remote_state)
      {
        for (uint8_t mask = 1 << 7; mask != 0; mask >>= 1)
        {
          if (bit & mask)
          {
            data->item(BIT_HIGH_US, BIT_ONE_LOW_US);
          }
          else
          {
            data->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
          }
        }
      }
      data->mark(TRAILER);
      transmit.perform();
    }
    
    void F60tdnFan::transmit_state(){
      auto powerMode = this->state;
      auto yuragiMode = this->oscillating;
      int fanSpeed = this->speed;

      if (powerMode == true && first_on_temp == 1)
      {
        remote_state[7] = FAN_ON;
      }
      else if (powerMode == true && first_on_temp == 0)
      {
        remote_state[7] = FAN_FIRST_ON;
      }
      else
      {
        remote_state[7] = FAN_OFF;
      }

      switch (fanSpeed)
      {
      case 1:
        remote_state[6] = FAN_SPEED_1;
        break;
      case 2:
        remote_state[6] = FAN_SPEED_2;
        break;
      case 3:
        remote_state[6] = FAN_SPEED_3;
        break;
      case 4:
        remote_state[6] = FAN_SPEED_4;
        break;
      case 5:
        remote_state[6] = FAN_SPEED_5;
        break;
      case 6:
        remote_state[6] = FAN_SPEED_6;
        break;
      case 7:
        remote_state[6] = FAN_SPEED_7;
        break;
      case 8:
        remote_state[6] = FAN_SPEED_8;
        break;
      case 9:
        remote_state[6] = FAN_SPEED_9;
        break;
      default:
        break;
      }

      if (yuragiMode)
      {
        remote_state[5] = FAN_YURAGI_MODE_ON;
      }
      else
      {
        remote_state[5] = FAN_YURAGI_MODE_OFF;
      }
      uint8_t checksum = 0;
      for (int i = 2; i < 14; i++)
      {
        checksum ^= remote_state[i];
      }
      remote_state[14] = checksum;

      ESP_LOGD(TAG, "Send some bytes start with checksum 0x%02X", checksum);
      this->send_code();    
      delay(50);
      this->send_code();
    }

  } // namespace f60tdn
} // namespace esphome