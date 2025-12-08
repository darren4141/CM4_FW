#pragma once

#include <stdint.h>

#include "global_enums.h"

#define REG_SIZE_BITS 32

#define PERIPH_BASE 0xFE000000UL
#define BSC1_BASE (PERIPH_BASE + 0x804000)
#define BSC2_BASE (PERIPH_BASE + 0x205600)
#define BSC_LEN 0x20

#define BSC_C 0x00 / 4
#define BSC_S 0x04 / 4
#define BSC_DLEN 0x08 / 4
#define BSC_A 0x0C / 4
#define BSC_FIFO 0x10 / 4
#define BSC_DIV 0x14 / 4
#define BSC_DEL 0x18 / 4
#define BSC_CLKT 0x1C / 4

#define C_I2CEN (1U << 15)
#define C_ST (1U << 7)
#define C_CLEAR (1U << 4)
#define C_READ (1U << 0)

#define S_CLKT (1U << 9)
#define S_ERR (1U << 8)
#define S_DONE (1U << 1)
#define S_TA (1U << 0)
#define S_TXW (1U << 2)
#define S_RXR (1U << 3)
#define S_TXD (1U << 4)
#define S_RXD (1U << 5)
#define S_TXE (1U << 6)

typedef enum {
  I2C_BUS_1 = 1,
  I2C_BUS_2 = 2,
} I2cBus;

StatusCode i2c_init(I2cBus i2c_bus, uint32_t i2c_hz);

StatusCode i2c_deinit(I2cBus i2c_bus);

StatusCode i2c_write(I2cBus i2c_bus, uint8_t addr, const uint8_t *buf,
                     uint32_t len);

StatusCode i2c_write_byte(I2cBus i2c_bus, uint8_t addr, uint8_t data);

StatusCode i2c_read(I2cBus i2c_bus, uint8_t addr, uint8_t *buf, uint32_t len);