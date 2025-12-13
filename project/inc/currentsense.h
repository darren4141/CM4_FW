#pragma once

#include "global_enums.h"

#define INA_I2C_ADDRESS   0x45

#define INA_CONFIGURATION 0x00

#define CONFIG_RST        (1U << 15)
#define CONFIG_BRNG       (1U << 13)
#define CONFIG_PG         (0b11 << 11)
#define CONFIG_BADC       (0b0011 << 7)
#define CONFIG_SADC       (0b0011 << 3)
#define CONFIG_MODE       (0b111 << 0)

#define INA_SHUNT_VOLTAGE 0x01
#define INA_BUS_VOLTAGE   0x02
#define INA_POWER         0x03
#define INA_CURRENT       0x04
#define INA_CALIBRATION   0x05

#define MAX_CURRENT_AMPS  6
#define CURRENT_LSB       MAX_CURRENT_AMPS / 32768
#define R_SHUNT           0.01

#define CAL_VALUE         (uint32_t)(0.04096 / (float)(CURRENT_LSB * R_SHUNT))

StatusCode currentsense_init();
StatusCode currentsense_read(float *current_val);