#pragma once
#include "esphome/core/component.h"

#include "esphome/components/sensor/sensor.h"

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {

namespace parkside_plem_50_c2 {

class ParksidePlem50C2Component : public uart::UARTDevice, public PollingComponent {

 public:

  void setup() override;

  void loop() override;

  void update() override;

  void dump_config() override;

  void write_message(const char * message);

  void set_distance_sensor(sensor::Sensor *s) { distance_sensor_ = s; }

  void set_error_sensor(text_sensor::TextSensor *s) { error_sensor_ = s; }
  void set_attempt_count(uint32_t s) { attempt_count_ = s; }

  void set_log_name(const char *s) { log_name_ = s; }

 protected:

  static const int BUFFER_SIZE = 100;

  enum MeasurementState {

    MEASUREMENT_STATE_IDLE,

    MEASUREMENT_STATE_WAIT_LAS,

    MEASUREMENT_STATE_WAIT_RG055_AFTER_INIT,

    MEASUREMENT_STATE_WAIT_STRG055,

    MEASUREMENT_STATE_WAIT_RG055_AFTER_MEASUREMENT_ENABLE,

    MEASUREMENT_STATE_WAIT_MEASUREMENT,

    MEASUREMENT_STATE_WAIT_M10S0,

    MEASUREMENT_STATE_WAIT_M10P0,

  };
  sensor::Sensor *distance_sensor_;

  text_sensor::TextSensor *error_sensor_;

  const char *log_name_ = "parkside_plem_50_c2";

  uint32_t attempt_count_ = 1;

  uint32_t attempt_ = 0;

  MeasurementState state_ = MEASUREMENT_STATE_IDLE;

  uint8_t buffer_index_ = 0;

  uint32_t read_started_at_ = 0;

  char buffer_[BUFFER_SIZE];

  void start_read_message();

  bool read_message();

  void start_measurement_attempt();

  void start_shutdown_();

  bool wait_for(const char * waitForString);
  void process_error (const char * buffer, const char * errorText);

  int process_measurement (const char * measurement);

};

} // parkside_plem_50_c2

} // esphome
