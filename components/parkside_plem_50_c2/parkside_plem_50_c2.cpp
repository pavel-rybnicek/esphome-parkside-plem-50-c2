#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"
#include "parkside_plem_50_c2.h"
#include <cstring>

namespace esphome {
namespace parkside_plem_50_c2 {

static const char *TAG = "parkside_plem_50_c2";

static const int BUFFER_SIZE = 100;

void ParksidePlem50C2Component::setup() {
  // nothing to do here
}

void ParksidePlem50C2Component::read_message(char buffer[])
{
  uint8_t buffer_index = 0;

  memset (buffer, 0, BUFFER_SIZE);

  for (int i = 0; buffer_index < BUFFER_SIZE; i++) {
    if (i > 10000)
    {
      ESP_LOGE(TAG, "Timeout, message not complete. Received: %s", (char *) buffer);
      break;
    }
    if (!available())
    {
      delay(1);
      continue;
    }
    uint8_t data;
    read_byte(&data);
    buffer[buffer_index++] = (char) data;
    if ('>' == data)
    {
      break;
    }
  }
  // terminate the string
  buffer[buffer_index] = 0;
  ESP_LOGD(TAG, "Received: %s", (char *) buffer);
}

void ParksidePlem50C2Component::write_message(const char * message)
{
  ESP_LOGD(TAG, "Sending: %s", message);
  write_str(message);
}

void ParksidePlem50C2Component::wait_for(const char * waitForString)
{
  char buffer[BUFFER_SIZE];

  this->read_message(buffer);

  if (strcmp (waitForString, buffer))
  {
    ESP_LOGW(TAG, "Didn't receive expected value. Expected: '%s' Received: '%s'", waitForString, buffer);
  }
}

void ParksidePlem50C2Component::update() {
  char buffer[BUFFER_SIZE];

  // init laser
  this->write_message("<mALas>");
  this->read_message(buffer); // returns <Las:0> or <Las:1>

  this->write_message("<RDRG><jzmode><STRG055><quitjz>");
  this->wait_for("<RG055>");
  this->wait_for("<STRG055>");

  this->write_message ("<RDRG><mAo>");
  this->wait_for ("<RG055>");

  // do measurement
  int i = 0;
  do
  {
    this->write_message("<mAm>");
    this->read_message(buffer);
  } while (this->process_measurement(buffer) && ++i < this->attempt_count_);

  // shutdown laser
  this->write_message("<mAs>");
  this->wait_for("<m10s0>");
  this->write_message("<mAp>");  // switch laser off
  this->wait_for("<m10p0>");
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
  if ( valueLen < 0 || valueLen > '9')
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
  ESP_LOGCONFIG(TAG, "  attempt_count: %d", this->attempt_count_);
}

} // parkside_plem_50_c2
} // esphome
