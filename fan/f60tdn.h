#pragma once
#include <vector>
#include <string>
#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome
{
    namespace f60tdn
    {

        class F60tdnFan : public Component, public remote_base::RemoteReceiverListener, public remote_base::RemoteTransmittable, public fan::Fan
        {
        public:
            F60tdnFan() = default;
            void setup() override;
            void dump_config() override;
            void set_has_direction(bool has_direction) { this->has_direction_ = has_direction; }
            void set_has_oscillating(bool has_oscillating) { this->has_oscillating_ = has_oscillating; }
            void set_speed_count(int count) { this->speed_count_ = count; }
            void set_preset_modes(const std::vector<std::string> &presets) { this->preset_modes_ = presets; }
            fan::FanTraits get_traits() override { return this->traits_; }

        protected:
            void control(const fan::FanCall &call) override;
            void transmit_state();
            void send_code();
            bool on_receive(remote_base::RemoteReceiveData data) override;


            // để mỗi quạt có state riêng
            uint16_t address_{0x0000};
            uint8_t first_on_temp_{0};

            // Mỗi instance có buffer riêng
            uint8_t remote_state_[15] = {
                0x40, 0x04, 0x0B, 0x21,
                0x8C, 0x80, 0x84, 0x0C,
                0xF2, 0xB2, 0x0E, 0x81,
                0xC1, 0x1A, 0x0A};

            bool has_oscillating_{false};
            bool has_direction_{false};
            int speed_count_{0};
            fan::FanTraits traits_;
            std::vector<std::string> preset_modes_{};
        };

    } // namespace f60tdn
} // namespace esphome