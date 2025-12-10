#pragma once

#include "global_enums.h"

#define MX_IS1 0x00
#define MX_IS2 0x01
#define MX_IE1 0x02
#define MX_IE2 0x03

#define MX_FIFO_WR 0x04
#define MX_OVF_COUNTER 0x05
#define MX_FIFO_RD 0x06
#define MX_FIFO_DATA 0x07

#define MX_FIFO_CONFIG 0x08
#define MX_MODE_CONFIG 0x09
#define MX_SPO2_CONFIG 0x0A

#define MX_LED1_PULSE 0x0B
#define MX_LED2_PULSE 0x0C

#define MX_MULT_LED1 0x11
#define MX_MULT_LED2 0x12

#define MX_DIE_TEMP_INT 0x1F
#define MX_DIE_TEMP_FRAC 0x20
#define MX_DIE_TEMP_CFG 0x21

#define MX_REV_ID 0xFE
#define MX_PART_ID 0xFF

#define IRLED_THREAD_FREQ_HZ 1000
#define IRLED_THREAD_PERIOD_S 1 / IRLED_THREAD_FREQ_HZ

#define INT_PIN_1 14
#define INT_PIN_2 15