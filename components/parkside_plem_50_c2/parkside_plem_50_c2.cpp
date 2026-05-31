#include "esphome/core/component.h"

#include "esphome/components/sensor/sensor.h"

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"
#include "parkside_plem_50_c2.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace esphome {

namespace parkside_plem_50_c2 {

static const char *TAG = "parkside_plem_50_c2";

static const uint32_t MESSAGE_TIMEOUT = 10000;

void ParksidePlem50C2Component::setup() {

 // nothing to do here

}

void ParksidePlem50C2Component::start_read_message()

{

 buffer_index_ = 0;

 memset (buffer_, 0, BUFFER_SIZE);

 read_started_at_ = millis();

}

bool ParksidePlem50C2Component::read_message()

{

 while (available() && buffer_index_ < BUFFER_SIZE - 1) {

  uint8_t data;

  if (!read_byte(&data))

  {

   break;

  }

  buffer_[buffer_index_++] = (char) data;

  if ('>' == data)

  {

   ESP_LOGD(TAG, "Received: %s", (char *) buffer_);

   return true;

  }

 }

 if (buffer_index_ >= BUFFER_SIZE - 1)

 {

  ESP_LOGE(TAG, "Timeout, message not complete. Received: %s", (char *) buffer_);

  return true;

 }

 if (millis() - read_started_at_ > MESSAGE_TIMEOUT)

 {

  ESP_LOGE(TAG, "Timeout, message not complete. Received: %s", (char *) buffer_);

  return true;

 }

 return false;

}

void ParksidePlem50C2Component::write_message(const char * message)

{

 ESP_LOGD(TAG, "Sending: %s", message);

 write_str(message);

}

void ParksidePlem50C2Component::wait_for(const char * waitForString)

{
 if (strcmp (waitForString, buffer_))

 {

  ESP_LOGW(TAG, "Didn't receive expected value. Expected: '%s' Received: '%s'", waitForString, buffer_);

 }

}

void ParksidePlem50C2Component::start_measurement_attempt()

{

 this->write_message("<mAm>");

 this->start_read_message();

 state_ = MEASUREMENT_STATE_WAIT_MEASUREMENT;

}

void ParksidePlem50C2Component::loop() {

 if (state_ == MEASUREMENT_STATE_IDLE)

 {

  return;

 }

 if (!this->read_message())

 {

  return;

 }

 switch (state_)

 {

  case MEASUREMENT_STATE_WAIT_LAS:

   this->write_message("<RDRG><jzmode><STRG055><quitjz>");

   this->start_read_message();

   state_ = MEASUREMENT_STATE_WAIT_RG055_AFTER_INIT;

   break;

  case MEASUREMENT_STATE_WAIT_RG055_AFTER_INIT:

   this->wait_for("<RG055>");

   this->start_read_message();

   state_ = MEASUREMENT_STATE_WAIT_STRG055;

   break;

  case MEASUREMENT_STATE_WAIT_STRG055:

   this->wait_for("<STRG055>");

   this->write_message ("<RDRG><mAo>");

   this->start_read_message();

   state_ = MEASUREMENT_STATE_WAIT_RG055_AFTER_MEASUREMENT_ENABLE;

   break;

  case MEASUREMENT_STATE_WAIT_RG055_AFTER_MEASUREMENT_ENABLE:

   this->wait_for ("<RG055>");

   this->start_measurement_attempt();

   break;

  case MEASUREMENT_STATE_WAIT_MEASUREMENT:

   if (this->process_measurement(buffer_) && ++attempt_ < this->attempt_count_)

   {

    this->start_measurement_attempt();

    break;

   }

   this->write_message("<mAs>");

   this->start_read_message();

   state_ = MEASUREMENT_STATE_WAIT_M10S0;

   break;

  case MEASUREMENT_STATE_WAIT_M10S0:

   this->wait_for("<m10s0>");

   this->write_message("<mAp>"); // switch laser off

   this->start_read_message();

   state_ = MEASUREMENT_STATE_WAIT_M10P0;

   break;

  case MEASUREMENT_STATE_WAIT_M10P0:

   this->wait_for("<m10p0>");

   state_ = MEASUREMENT_STATE_IDLE;

   break;

  case MEASUREMENT_STATE_IDLE:

   break;

 }

}

void ParksidePlem50C2Component::update() {

 if (state_ != MEASUREMENT_STATE_IDLE)

 {

  ESP_LOGW(TAG, "Previous measurement still running");

  return;

 }

 attempt_ = 0;

 // init laser

 this->write_message("<mALas>");

 this->start_read_message(); // returns <Las:0> or <Las:1>

 state_ = MEASUREMENT_STATE_WAIT_LAS;

}

void ParksidePlem50C2Component::process_error (const char * buffer, const char * errorText)

{
 char err_msg[100] = "";

 sprintf(err_msg, "%s, buffer: '%s'", errorText, buffer);

 ESP_LOGE(TAG, "%s", err_msg);

 this->error_sensor_->publish_state(err_msg);

}

int ParksidePlem50C2Component::process_measurement (const char * measurement)

{

 // measurement looks like <m30m356>

 // where 3rd char is length of the value. Value itself

 // starts at 6th character

 if (!strncmp ("<0E", measurement, 3))

 {

  this->process_error (measurement, "Error received");

  return -1;

 }
 if (strncmp ("<m", measurement, 2))

 {

  this->process_error (measurement, "Response should start with '<m'");

  return -2;

 }

 // get value len

 int valueLen = measurement[2] - '0';

 if ( valueLen < 0 || valueLen > 9)

 {

  this->process_error (measurement, "3rd character must be digit - len of value");

  return -3;

 }

 char value[10] = "";

 strncpy (value, measurement + 5, valueLen);

 if ( valueLen != strlen (value))

 {
  this->process_error (measurement, "Error parsing value - invalid length?");

  return -4;

 }

 // previous checks aren't really necessary...

 int val = atoi(measurement + 5);

 if (val <= 0)

 {

  this->process_error (measurement, "Cannot get meaningfull value");

  return -5;

 }

 ESP_LOGD(TAG, "Buffer: '%s', value is %d", measurement, val);

 this->distance_sensor_->publish_state(val);

 this->error_sensor_->publish_state("");

 return 0;

}

void ParksidePlem50C2Component::dump_config() {
 ESP_LOGCONFIG(TAG, "ParksidePlem50C2 Component:");

 ESP_LOGCONFIG(TAG, " attempt_count: %d", this->attempt_count_);

}

} // parkside_plem_50_c2

} // esphome