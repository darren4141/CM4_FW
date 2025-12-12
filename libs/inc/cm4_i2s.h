#pragma once

#include "global_enums.h"

#define PCM_BASE (PERIPH_BASE + 0x00203000U)
#define PCM_LEN 0x24

#define PCM_CS_A 0x00 / 4
#define PCM_FIFO_A 0x04 / 4
#define PCM_MODE_A 0x08 / 4
#define PCM_RXC_A 0x0C / 4
#define PCM_TXC_A 0x10 / 4
#define PCM_DREQ_A 0x14 / 4
#define PCM_INTEN_A 0x18 / 4
#define PCM_INTSTC_A 0x1C / 4
#define PCM_GRAY 0x20 / 4

#define CS_A_EN (1U << 0)
#define CS_A_RXON (1U << 1)
#define CS_A_TXON (1U << 2)
#define CS_A_TXCLR (1U << 3)
#define CS_A_RXCLR (1U << 4)
#define CS_A_TXW (1U << 17)
#define CS_A_RXR (1U << 18)

#define MODE_A_CLKM_MASTER (0u << 23)
#define MODE_A_CLKM_SLAVE (1u << 23)
#define MODE_A_CLKI_INVERT (1u << 22)
#define MODE_A_FSM_MASTER (0u << 21)
#define MODE_A_FSM_SLAVE (1u << 21)
#define MODE_A_FLEN_SHIFT 10
#define MODE_A_FLEN_MASK (0x3FFu << MODE_A_FLEN_SHIFT)
#define MODE_A_FSLEN_SHIFT 0
#define MODE_A_FSLEN_MASK (0x3FFu << MODE_A_FSLEN_SHIFT)

#define TXC_A_CH1WEX (1u << 31)
#define TXC_A_CH1EN (1u << 30)
#define TXC_A_CH1POS_SHIFT 20
#define TXC_A_CH1POS_MASK (0x3FFu << TXC_A_CH1POS_SHIFT)
#define TXC_A_CH1WID_SHIFT 16
#define TXC_A_CH1WID_MASK (0xFu << TXC_A_CH1WID_SHIFT)

#define TXC_A_CH2WEX (1u << 15)
#define TXC_A_CH2EN (1u << 14)
#define TXC_A_CH2POS_SHIFT 10
#define TXC_A_CH2POS_MASK (0x3FFu << TXC_A_CH2POS_MASK)
#define TXC_A_CH2WID_SHIFT 0
#define TXC_A_CH2WID_MASK (0xFu << TXC_A_CH2WID_SHIFT)