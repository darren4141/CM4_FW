#pragma once

#include <stdint.h>

#include "global_enums.h"

#define MX_I2C_ADDR 0x57

#define MX_IS1 0x00

#define IS1_A_FULL (1U << 7)
#define IS1_PPG_RDY (1U << 6)
#define IS1_ALC_OVF (1U << 5)

#define MX_IS2 0x01
#define MX_IE1 0x02

#define IE1_A_FULL_EN (1U << 7)
#define IE1_PPG_RDY_EN (1U << 6)
#define IE1_ALC_OVF_EN (1U << 5)

#define MX_IE2 0x03

#define MX_FIFO_WR_PTR 0x04
#define MX_OVF_COUNTER 0x05
#define MX_FIFO_RD_PTR 0x06
#define MX_FIFO_DATA 0x07

#define MX_FIFO_CONFIG 0x08

#define FIFO_CONFIG_SAMPLE_AVERAGE_4 (0b010 << 5)
#define FIFO_CONFIG_SAMPLE_AVERAGE_8 (0b011 << 5)
#define FIFO_CONFIG_ROLLOVER_EN (1U << 4) /*Enables circular buffer*/
#define FIFO_CONFIG_A_FULL_16_SAMPLES 0xE

#define MX_MODE_CONFIG 0x09

#define MODE_CONFIG_RESET (1U << 6)
#define MODE_CONFIG_SPO2_MODE (0b011 << 0)

#define MX_SPO2_CONFIG 0x0A

#define SPO2_CONFIG_ADC_RGE_2048 (0b00 << 5)
#define SPO2_CONFIG_ADC_RGE_4096 (0b01 << 5)
#define SPO2_CONFIG_ADC_RGE_8192 (0b10 << 5)
#define SPO2_CONFIG_ADC_RGE_16384 (0b11 << 5)

#define SPO2_CONFIG_SAMPLE_RT_50 (0b000 << 2)
#define SPO2_CONFIG_SAMPLE_RT_100 (0b001 << 2)
#define SPO2_CONFIG_SAMPLE_RT_200 (0b010 << 2)

#define SPO2_CONFIG_LED_PW_15 (0b00 << 0)
#define SPO2_CONFIG_LED_PW_16 (0b01 << 0)
#define SPO2_CONFIG_LED_PW_17 (0b10 << 0)
#define SPO2_CONFIG_LED_PW_18 (0b11 << 0)

#define MX_LED1_PULSE_AMP 0x0C
#define MX_LED2_PULSE_AMP 0x0D

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

#define MAX30102_BUFFER_SIZE 256

typedef struct {
  uint32_t ir;
  uint32_t red;
} Max30102Sample;

StatusCode irled_init();

StatusCode irled_deinit();

StatusCode irled_start_reading();

StatusCode irled_stop_reading();

StatusCode irled_pop_sample(Max30102Sample *sample);
