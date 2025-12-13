#include "cm4_i2s.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cm4_gpio.h"

static volatile uint32_t *pcm_regs = NULL;

StatusCode i2c_get_initialized()
{
  if (pcm_regs == NULL) {
    return STATUS_CODE_NOT_INITIALIZED;
  }
  return STATUS_CODE_OK;
}

StatusCode i2s_init()
{
  StatusCode ret = gpio_get_regs_initialized();
  if (ret != STATUS_CODE_OK) {
    printf("gpio regs are not initialized");
    return ret;
  }

  if (pcm_regs != NULL) {
    printf("i2s bus already initialized\n");
    return STATUS_CODE_ALREADY_INITIALIZED;
  }

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  pcm_regs = (uint32_t *)mmap(NULL, PCM_LEN, PROT_READ | PROT_WRITE, MAP_SHARED,
                              fd, PCM_BASE);

  close(fd);
  if (pcm_regs == MAP_FAILED) {
    pcm_regs = NULL;
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  uint32_t cs = pcm_regs[PCM_CS_A];
  cs &= ~(CS_A_EN | CS_A_TXON | CS_A_RXON);
  pcm_regs[PCM_CS_A] = cs | CS_A_TXCLR | CS_A_RXCLR;

  const uint32_t frame_len_clocks = 32;
  const uint32_t fs_pulse_len = 1;

  uint32_t mode = MODE_A_CLKM_MASTER | MODE_A_FSM_MASTER
                  | ((frame_len_clocks << MODE_A_FLEN_SHIFT) & MODE_A_FLEN_MASK)
                  | ((fs_pulse_len << MODE_A_FSLEN_SHIFT) & MODE_A_FSLEN_MASK);

  pcm_regs[PCM_MODE_A] = mode;

  int32_t txc = 0;

  const uint32_t ch1_pos = 0;
  const uint32_t ch1_wid = 8;

  const uint32_t ch2_pos = 16;
  const uint32_t ch2_wid = 8;

  txc |= TXC_A_CH1EN
         | ((ch1_pos << TXC_A_CH1POS_SHIFT) & TXC_A_CH1POS_MASK)
         | ((ch1_wid << TXC_A_CH1WID_SHIFT) & TXC_A_CH1WID_MASK);

  txc |= TXC_A_CH2EN
         | ((ch2_pos << TXC_A_CH2POS_SHIFT) & TXC_A_CH2POS_MASK)
         | ((ch2_wid << TXC_A_CH2WID_SHIFT) & TXC_A_CH2WID_MASK);

  pcm_regs[PCM_TXC_A] = txc;

  cs = pcm_regs[PCM_CS_A];
  cs |= CS_A_EN | CS_A_TXON;
  pcm_regs[PCM_CS_A] = cs;

  TRY(gpio_set_mode(18, GPIO_MODE_ALT0));
  TRY(gpio_set_mode(19, GPIO_MODE_ALT0));
  TRY(gpio_set_mode(20, GPIO_MODE_ALT0));
  TRY(gpio_set_mode(21, GPIO_MODE_ALT0));

  return STATUS_CODE_OK;
}

StatusCode i2s_deinit()
{
  if (!pcm_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  uint32_t cs = pcm_regs[PCM_CS_A];
  cs &= ~(CS_A_EN | CS_A_TXON | CS_A_RXON);
  pcm_regs[PCM_CS_A] = cs;

  munmap((void *)pcm_regs, PCM_LEN);
  pcm_regs = NULL;

  return STATUS_CODE_OK;
}
