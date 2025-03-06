#include <Arduino.h>

typedef struct lora_config {
  float LORA_FREQUENCY = 869.525;
  float bandwidth = 250.0;
  uint8_t spreading_factor = 11;
  uint8_t coding_rate = 5;
  uint8_t syncWord = 0x2B;
  int8_t power = 10;
  uint16_t preambleLength = 16;
  float tcxoVoltage = 1.6;
};
