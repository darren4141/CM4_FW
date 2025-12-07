#pragma once

#include <stdint.h>

#include "global_enums.h"

#define PERIPH_BASE 0xFE000000UL
#define BSC1_BASE (PERIPH_BASE + 0x804000)
#define BSC2_BASE (PERIPH_BASE + 0x205600)
#define BSC_LEN 0x20

#define BSC_C 0x00
#define BSC_S 0x01
#define BSC_DLEN 0x02
#define BSC_A 0x03
#define BSC_FIFO 0x04
#define BSC_DIV 0x05
#define BSC_DEL 0x06
#define BSC_CLKT 0x07

#define REG32(offset) bsc1[(offset)]

#define C_I2CEN (1u << 15)
#define C_ST (1u << 7)
#define C_CLEAR (1u << 4)
#define C_READ (1u << 0)

#define S_CLKT (1u << 9)
#define S_ERR (1u << 8)
#define S_DONE (1u << 1)
#define S_TA (1u << 0)
#define S_TXW (1u << 2)
#define S_RXR (1u << 3)
#define S_TXD (1u << 4)
#define S_RXD (1u << 5)
#define S_TXE (1u << 6)

typedef enum {
  I2C_BUS_1 = 1,
  I2C_BUS_2 = 2,
} I2cBus;

StatusCode i2c_init(I2cBus i2c_bus, uint32_t i2c_hz);

StatusCode i2c_deinit(I2cBus i2c_bus);

StatusCode i2c_write(I2cBus i2c_bus, uint8_t addr, const uint8_t *buf,
                     uint32_t len);

StatusCode i2c_read(I2cBus i2c_bus, uint8_t addr, uint8_t *buf, uint32_t len);