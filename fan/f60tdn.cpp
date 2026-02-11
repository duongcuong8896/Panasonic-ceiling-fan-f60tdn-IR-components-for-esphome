#include "f60tdn.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace f60tdn
    {

        static const char *TAG = "f60tdn.fan";

        // Protocol timing constants
        static const uint16_t BITWISE = 425;
        static const uint16_t HEADER_HIGH_US = BITWISE * 8;
        static const uint16_t HEADER_LOW_US = BITWISE * 4;
        static const uint16_t BIT_HIGH_US = BITWISE;
        static const uint16_t BIT_ONE_LOW_US = BITWISE * 3;
        static const uint16_t BIT_ZERO_LOW_US = BITWISE;
        static const uint16_t TRAILER = BITWISE;

        // Power states
        const uint8_t FAN_ON = 0x0C;
        const uint8_t FAN_FIRST_ON = 0x4C;
        const uint8_t FAN_OFF = 0xCC;

        // Fan speed codes
        const uint8_t FAN_SPEED_1 = 0x84;
        const uint8_t FAN_SPEED_2 = 0x44;
        const uint8_t FAN_SPEED_3 = 0xC4;
        const uint8_t FAN_SPEED_4 = 0x24;
        const uint8_t FAN_SPEED_5 = 0xA4;
        const uint8_t FAN_SPEED_6 = 0x64;
        const uint8_t FAN_SPEED_7 = 0xE4;
        const uint8_t FAN_SPEED_8 = 0x14;
        const uint8_t FAN_SPEED_9 = 0x94;

        // Yuragi mode
        const uint8_t FAN_YURAGI_MODE_ON = 0x40;
        const uint8_t FAN_YURAGI_MODE_OFF = 0x80;

        void F60tdnFan::setup()
        {
            auto restore = this->restore_state_();
            if (restore.has_value())
            {
                restore->apply(*this);
            }

            // Construct traits
            this->traits_ = fan::FanTraits(
                this->has_oscillating_,
                this->speed_count_ > 0,
                this->has_direction_,
                this->speed_count_);

            std::vector<const char *> supported_presets;
            for (const auto &mode : this->preset_modes_)
            {
                supported_presets.push_back(mode.c_str());
            }
            this->traits_.set_supported_preset_modes(supported_presets);
        }

        void F60tdnFan::dump_config()
        {
            LOG_FAN("", "F60tdn Fan", this);
        }

        void F60tdnFan::control(const fan::FanCall &call)
        {
            // Xử lý State (Bật/Tắt)
            if (call.get_state().has_value())
                this->state = *call.get_state();

            // Xử lý Tốc độ
            if (call.get_speed().has_value() && (this->speed_count_ > 0))
                this->speed = *call.get_speed();

            // Xử lý Đảo gió (Oscillating)
            if (call.get_oscillating().has_value() && this->has_oscillating_)
                this->oscillating = *call.get_oscillating();

            // Xử lý Hướng gió (Direction)
            if (call.get_direction().has_value() && this->has_direction_)
                this->direction = *call.get_direction();

            // Xử lý Preset mode
            if (call.get_preset_mode() != nullptr)
            {
                this->set_preset_mode_(call.get_preset_mode());
            }

            this->transmit_state();
            this->publish_state();
        }

        bool F60tdnFan::on_receive(remote_base::RemoteReceiveData src)
        {
            uint8_t bytes_received[15] = {};

            ESP_LOGD(TAG, "Received some bytes");

            if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
                return false;

            // SỬA: Dùng this->address_ thay vì biến toàn cục
            for (uint16_t mask = 1 << 15; mask != 0; mask >>= 1)
            {
                if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US))
                {
                    this->address_ |= mask;
                }
                else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US))
                {
                    this->address_ &= ~mask;
                }
                else
                {
                    return false;
                }
            }

            // Check the address
            if (this->address_ != 0x4004)
            {
                return false;
            }
            ESP_LOGD(TAG, "Received address 0x%04X", this->address_);

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

            // Log bytes 0-12 (bytes 13-14 là checksum/control)
            for (int i = 0; i < 13; i++)
            {
                ESP_LOGD(TAG, "Received bytes 0x%02X", bytes_received[i]);
            }

            auto powerMode = bytes_received[7];
            auto fanSpeed = bytes_received[6];
            auto yuragiMode = bytes_received[5];

 
            switch (powerMode)
            {
            case FAN_OFF:
                this->state = false;
                this->first_on_temp_ = 0; 
                break;
            case FAN_ON:
                this->state = true;
                break;
            case FAN_FIRST_ON:
                this->state = true;
                this->first_on_temp_ = 1; 
                break;
            default:
                break;
            }

            switch (yuragiMode)
            {
            case FAN_YURAGI_MODE_ON:
                this->oscillating = true;
                break;
            case FAN_YURAGI_MODE_OFF:
                this->oscillating = false;
                break;
            default:
                break;
            }

            switch (fanSpeed)
            {
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

        void F60tdnFan::send_code()
        {
            auto transmit = this->transmitter_->transmit();
            auto data = transmit.get_data();

            data->set_carrier_frequency(38000);
            data->reserve(2 + 16 + (15 * 8) + 1);

            // Send header
            data->item(HEADER_HIGH_US, HEADER_LOW_US);

            // Send address (0x4004)
            uint16_t addr = 0x4004;
            for (uint16_t mask = 1 << 15; mask != 0; mask >>= 1)
            {
                if (addr & mask)
                {
                    data->item(BIT_HIGH_US, BIT_ONE_LOW_US);
                }
                else
                {
                    data->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
                }
            }

            // SỬA: Dùng this->remote_state_ thay vì biến toàn cục
            for (uint8_t byte : this->remote_state_)
            {
                for (uint8_t mask = 1 << 7; mask != 0; mask >>= 1)
                {
                    if (byte & mask)
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

        void F60tdnFan::transmit_state()
        {
            auto powerMode = this->state;
            auto yuragiMode = this->oscillating;
            int fanSpeed = this->speed;

            // GIỮ NGUYÊN logic first_on như code gốc của bạn
            // SỬA: Dùng this->first_on_temp_ và this->remote_state_
            if (powerMode == true && this->first_on_temp_ == 1)
            {
                this->remote_state_[7] = FAN_ON;
            }
            else if (powerMode == true && this->first_on_temp_ == 0)
            {
                this->remote_state_[7] = FAN_FIRST_ON;
            }
            else
            {
                this->remote_state_[7] = FAN_OFF;
            }

            switch (fanSpeed)
            {
            case 1:
                this->remote_state_[6] = FAN_SPEED_1;
                break;
            case 2:
                this->remote_state_[6] = FAN_SPEED_2;
                break;
            case 3:
                this->remote_state_[6] = FAN_SPEED_3;
                break;
            case 4:
                this->remote_state_[6] = FAN_SPEED_4;
                break;
            case 5:
                this->remote_state_[6] = FAN_SPEED_5;
                break;
            case 6:
                this->remote_state_[6] = FAN_SPEED_6;
                break;
            case 7:
                this->remote_state_[6] = FAN_SPEED_7;
                break;
            case 8:
                this->remote_state_[6] = FAN_SPEED_8;
                break;
            case 9:
                this->remote_state_[6] = FAN_SPEED_9;
                break;
            default:
                break;
            }

            if (yuragiMode)
            {
                this->remote_state_[5] = FAN_YURAGI_MODE_ON;
            }
            else
            {
                this->remote_state_[5] = FAN_YURAGI_MODE_OFF;
            }

            // Tính checksum
            uint8_t checksum = 0;
            for (int i = 2; i < 14; i++)
            {
                checksum ^= this->remote_state_[i];
            }
            this->remote_state_[14] = checksum;

            ESP_LOGD(TAG, "Send some bytes start with checksum 0x%02X", checksum);

            // SỬA: Thay delay(50) bằng set_timeout để không block
            this->send_code();
            this->set_timeout("resend", 50, [this]()
                              { this->send_code(); });
        }

    } // namespace f60tdn
} // namespace esphome