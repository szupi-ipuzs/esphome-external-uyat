#pragma once

#include <cinttypes>
#include <vector>
#include <deque>
#include <variant>

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/time.h"
#endif

#include "uyat_datapoint_types.h"

namespace esphome::uyat
{

enum UyatNetworkStatus: uint8_t {
  SMARTCONFIG = 0x00,
  AP_MODE = 0x01,
  WIFI_CONFIGURED = 0x02,
  WIFI_CONNECTED = 0x03,
  CLOUD_CONNECTED = 0x04,
};


enum FactoryResetType: uint8_t {
  BY_HW = 0x00, // Reset by hardware operation
  BY_APP = 0x01, // Reset performed on the mobile app
  BY_APP_WIPE = 0x02 // Factory reset performed on the mobile app
};


struct UyatDatapointListener {
  MatchingDatapoint configured;
  OnDatapointCallback on_datapoint;
};

enum class UyatCommandType : uint8_t {
  HEARTBEAT = 0x00,
  PRODUCT_QUERY = 0x01,
  CONF_QUERY = 0x02,
  WIFI_STATE = 0x03,
  WIFI_RESET = 0x04,
  WIFI_SELECT = 0x05,
  DATAPOINT_DELIVER = 0x06,
  DATAPOINT_REPORT_ASYNC = 0x07,
  DATAPOINT_QUERY = 0x08,
  WIFI_TEST = 0x0E,
  LOCAL_TIME_QUERY = 0x1C,
  DATAPOINT_REPORT_SYNC = 0x22,
  DATAPOINT_REPORT_ACK = 0x23,
  WIFI_RSSI = 0x24,
  DISABLE_HEARTBEATS = 0x25,
  VACUUM_MAP_UPLOAD = 0x28,
  GET_NETWORK_STATUS = 0x2B,
  GET_MAC_ADDRESS = 0x2D,
  EXTENDED_SERVICES = 0x34,
};

enum class UyatExtendedServicesCommandType : uint8_t {
  RESET_NOTIFICATION = 0x04,
  FACTORY_RESET = 0x05,
  GET_MODULE_INFORMATION = 0x07,
  UPDATE_IN_PROGRESS = 0x0A,
};

enum class UyatInitState : uint8_t {
  INIT_HEARTBEAT = 0x00,
  INIT_PRODUCT,
  INIT_CONF,
  INIT_WIFI,
  INIT_DATAPOINT,
  INIT_DONE,
};

struct UyatCommand {
  UyatCommandType cmd;
  std::vector<uint8_t> payload;
};

template<typename... Ts> class FactoryResetAction;

class Uyat : public Component, public uart::UARTDevice, public DatapointHandler {
  SUB_TEXT_SENSOR(product)
  SUB_SENSOR(num_garbage_bytes)
  SUB_TEXT_SENSOR(unknown_commands)
  SUB_TEXT_SENSOR(unknown_extended_commands)
  SUB_TEXT_SENSOR(unhandled_datapoints)
  SUB_TEXT_SENSOR(pairing_mode)
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  void register_datapoint_listener(const uint8_t datapoint_id, const OnDatapointCallback &func);
  void register_datapoint_listener(const uint8_t datapoint_id, const UyatDatapointType type, const OnDatapointCallback &func);
  void register_datapoint_listener(const MatchingDatapoint& matching_dp, const OnDatapointCallback &func) override;
  void set_datapoint_value(const UyatDatapoint& value, const bool forced = false) override;
  void set_status_pin(InternalGPIOPin *status_pin) { this->status_pin_ = status_pin; }
  void send_generic_command(const UyatCommand &command) { send_command_(command); }
  UyatInitState get_init_state();

#ifdef USE_TIME
  void set_time_id(time::RealTimeClock *time_id) { this->time_id_ = time_id; }
#endif
  void add_ignore_mcu_update_on_datapoints(uint8_t ignore_mcu_update_on_datapoints) {
    this->ignore_mcu_update_on_datapoints_.push_back(ignore_mcu_update_on_datapoints);
  }
  void add_on_initialized_callback(std::function<void()> callback) {
    this->initialized_callback_.add(std::move(callback));
  }

  void trigger_factory_reset(const FactoryResetType reset_type);


  void set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value){
    set_datapoint_value(UyatDatapoint{datapoint_id, RawDatapointValue{value}}, false);
  }
  void set_boolean_datapoint_value(uint8_t datapoint_id, bool value){
    set_datapoint_value(UyatDatapoint{datapoint_id, BoolDatapointValue{value}}, false);
  }
  void set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value){
    set_datapoint_value(UyatDatapoint{datapoint_id, UIntDatapointValue{value}}, false);
  }
  void set_string_datapoint_value(uint8_t datapoint_id, const std::string &value){
    set_datapoint_value(UyatDatapoint{datapoint_id, StringDatapointValue{value}}, false);
  }
  void set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value){
    set_datapoint_value(UyatDatapoint{datapoint_id, EnumDatapointValue{value}}, false);
  }
  void set_bitmask32_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length){
    set_datapoint_value(UyatDatapoint{datapoint_id, Bitmask32DatapointValue{value}}, false);
  }
  void force_set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value){
    set_datapoint_value(UyatDatapoint{datapoint_id, RawDatapointValue{value}}, true);
  }
  void force_set_boolean_datapoint_value(uint8_t datapoint_id, bool value){
    set_datapoint_value(UyatDatapoint{datapoint_id, BoolDatapointValue{value}}, true);
  }
  void force_set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value){
    set_datapoint_value(UyatDatapoint{datapoint_id, UIntDatapointValue{value}}, true);
  }
  void force_set_string_datapoint_value(uint8_t datapoint_id, const std::string &value){
    set_datapoint_value(UyatDatapoint{datapoint_id, StringDatapointValue{value}}, true);
  }
  void force_set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value){
    set_datapoint_value(UyatDatapoint{datapoint_id, EnumDatapointValue{value}}, true);
  }
  void force_set_bitmask32_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length){
    set_datapoint_value(UyatDatapoint{datapoint_id, Bitmask32DatapointValue{value}}, true);
  }


 protected:
  void handle_input_buffer_();
  void handle_datapoints_(const uint8_t *buffer, size_t len);
  optional<UyatDatapoint> get_datapoint_(uint8_t datapoint_id);
  // returns number of bytes to remove from the beginning of rx buffer
  std::size_t validate_message_();

  void handle_command_(uint8_t command, uint8_t version, const uint8_t *buffer, size_t len);
  void send_raw_command_(UyatCommand command);
  void process_command_queue_();
  void send_command_(const UyatCommand &command);
  void send_empty_command_(UyatCommandType command);
  void set_datapoint_value_(const UyatDatapoint& dp, const bool force = false);
  void send_datapoint_command_(uint8_t datapoint_id, UyatDatapointTypeInternal datapoint_type, std::vector<uint8_t> data);
  void set_status_pin_();
  void send_wifi_status_(const uint8_t status);
  uint8_t get_wifi_rssi_();
  void report_wifi_connected_or_retry_(const uint32_t delay_ms);
  void report_cloud_connected_();
  void query_product_info_with_retries_();
  std::string process_get_module_information_(const uint8_t *buffer, size_t len);
  void schedule_heartbeat_(const bool initial);
  void stop_heartbeats_();
  void update_pairing_mode_();

#ifdef USE_TIME
  void send_local_time_();
  time::RealTimeClock *time_id_{nullptr};
  bool time_sync_callback_registered_{false};
#endif
  UyatInitState init_state_ = UyatInitState::INIT_HEARTBEAT;
  bool init_failed_{false};
  bool heartbeats_enabled_{true};
  int init_retries_{0};
  uint8_t protocol_version_ = -1;
  InternalGPIOPin *status_pin_{nullptr};
  int status_pin_reported_ = -1;
  int reset_pin_reported_ = -1;
  uint32_t last_command_timestamp_ = 0;
  uint32_t last_rx_char_timestamp_ = 0;
  std::string product_ = "";
  std::vector<UyatDatapointListener> listeners_;
  std::vector<UyatDatapoint> cached_datapoints_;
  std::deque<uint8_t> rx_message_;
  std::vector<uint8_t> ignore_mcu_update_on_datapoints_{};
  std::vector<UyatCommand> command_queue_;
  optional<UyatCommandType> expected_response_{};
  UyatNetworkStatus wifi_status_{UyatNetworkStatus::WIFI_CONFIGURED};
  optional<bool> requested_wifi_config_is_ap_{};
  CallbackManager<void()> initialized_callback_{};

  uint64_t num_garbage_bytes_{0};
  std::vector<uint8_t> unknown_commands_set_;
  std::vector<uint8_t> unknown_extended_commands_set_;
  std::vector<uint8_t> unhandled_datapoints_set_;
};

template<typename... Ts> class FactoryResetAction : public Action<Ts...> {
 public:
  FactoryResetAction(Uyat *uyat) : uyat_(uyat) {}
  TEMPLATABLE_VALUE(FactoryResetType, reset_type);

  void play(const Ts &...x) override {
    this->uyat_->trigger_factory_reset(this->reset_type_.value(x...));
  }

 protected:
  Uyat *uyat_;
};

}  // namespace esphome::uyat
