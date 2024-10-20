#pragma once
#include <set>
#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
namespace f60tdn {

class F60tdnFan : public Component, public remote_base::RemoteReceiverListener, 
public remote_base::RemoteTransmittable,
public fan::Fan {
 public:
  F60tdnFan() {}
  void setup() override;
  void dump_config() override;
  void set_has_direction(bool has_direction) { this->has_direction_ = has_direction; }
  void set_has_oscillating(bool has_oscillating) { this->has_oscillating_ = has_oscillating; }
  void set_speed_count(int count) { this->speed_count_ = count; }
  void set_preset_modes(const std::set<std::string> &presets) { this->preset_modes_ = presets; }
  fan::FanTraits get_traits() override { return this->traits_; }

 protected:
  void control(const fan::FanCall &call) override;
  void transmit_state();
  void send_code();
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override; 
  bool has_oscillating_{false};
  bool has_direction_{false};
  int speed_count_{};
  fan::FanTraits traits_;
  std::set<std::string> preset_modes_{};

 
};

}  // namespace f60tdn
}  // namespace esphome