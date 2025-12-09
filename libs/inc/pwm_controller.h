#pragma once

#include <stdint.h>

#include "global_enums.h"

#define PCA_REG_SIZE_BITS 8

#define PCA9685_LEN 0xFF
#define PCA_I2C_ADDR 0x47
#define PCA_I2C_HZ KHZ(100)

#define PCA_DEFAULT_FREQ MHZ(25)

// Core control registers
#define PCA_MODE1 0x00
#define PCA_MODE2 0x01
#define PWM_RESOLUTION 4096

#define MODE1_RESTART (1U << 7)
#define MODE1_EXTCLK (1U << 6)
#define MODE1_AI (1U << 5)
#define MODE1_SLEEP (1U << 4)
#define MODE1_ALLCALL (1U << 0)

#define MODE2_INVRT (1U << 4)
#define MODE2_OCH (1U << 3)
#define MODE2_OUTDRV (1U << 2)
#define MODE2_OUTNE1 (1U << 1)
#define MODE2_OUTNE0 (1U << 0)

// I2C subaddress + all-call registers
#define PCA_SUBADR1 0x02
#define PCA_SUBADR2 0x03
#define PCA_SUBADR3 0x04
#define PCA_ALLCALLADR 0x05

/* PCA registers have the following format:
    PCA_LED0_ON_L = OFFSET
    PCA_LED0_ON_H = OFFSET + 1
    PCA_LED0_OFF_L = OFFSET + 2
    PCA_LED0_OFF_H = OFFSET + 3
*/

typedef enum {
  PCA_LED0_ON_L = 0x06,
  PCA_LED1_ON_L = 0x0A,
  PCA_LED2_ON_L = 0x0E,
  PCA_LED3_ON_L = 0x12,
  PCA_LED4_ON_L = 0x16,
  PCA_LED5_ON_L = 0x1A,
  INVALID_PCA_CHANNEL = -1,
} PCAChannel;

#define LEDX_FULL_OFF (1U << 4)
#define LEDX_FULL_ON (1U << 4)

// "All LED" broadcast PWM registers
#define PCA_ALL_LED_ON_L 0xFA
#define PCA_ALL_LED_ON_H 0xFB
#define PCA_ALL_LED_OFF_L 0xFC
#define PCA_ALL_LED_OFF_H 0xFD

// Prescaler + test mode
#define PCA_PRE_SCALE 0xFE
#define PCA_TESTMODE 0xFF

StatusCode pwm_controller_init(uint32_t pwm_freq);

StatusCode pwm_controller_set_channel(PCAChannel channel,
                                      float delay_percentage, float duty_cycle);

/*Sets pwm channel to 0% duty cycle*/
StatusCode pwm_controller_stop_channel(PCAChannel channel);

/*Sets pwm channel to 100% duty cycle*/
StatusCode pwm_controller_digital_set_channel(PCAChannel channel);