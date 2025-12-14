#include "cm4_i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cm4_gpio.h"

static volatile uint32_t *bsc1 = NULL;
static volatile uint32_t *bsc2 = NULL;

static StatusCode map_periph(int fd,
                             uintptr_t phys_base,
                             size_t len,
                             volatile uint32_t **out)
{
  long page_size = sysconf(_SC_PAGESIZE);
  uintptr_t mask = (uintptr_t)(page_size - 1);

  uintptr_t aligned = phys_base & ~mask;
  uintptr_t offset = phys_base - aligned;

  void *map = mmap(NULL,
                   len + offset,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   fd,
                   (off_t)aligned);

  if (map == MAP_FAILED) {
    int e = errno;
    fprintf(stderr,
            "mmap(fd=%d, phys=0x%lx aligned=0x%lx len=%zu off=%lu) failed: %s (errno=%d)\n",
            fd,
            (unsigned long)phys_base,
            (unsigned long)aligned,
            len,
            (unsigned long)offset,
            strerror(e),
            e);
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  *out = (volatile uint32_t *)((uint8_t *)map + offset);
  return STATUS_CODE_OK;
}

StatusCode i2c_get_initialized(I2cBus i2c_bus)
{
  volatile uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_init(I2cBus i2c_bus, uint32_t i2c_hz)
{

  StatusCode ret = gpio_get_regs_initialized();
  printf("Attempting to initialize i2c\n");

  if (ret != STATUS_CODE_OK) {
    printf("gpio regs are not initialized");
    return ret;
  }

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("Failed to access memory\n");
    fprintf(stderr, "open(/dev/mem) failed: %s (errno=%d)\n", strerror(errno), errno);
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  volatile uint32_t **bsc_ptr = NULL;
  uintptr_t phys_base = 0;

  if (i2c_bus == I2C_BUS_1) {
    bsc_ptr = &bsc1;
    phys_base = BSC1_BASE;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc_ptr = &bsc2;
    phys_base = BSC2_BASE;
  }
  else {
    perror("i2c_bus");
    close(fd);
    return STATUS_CODE_INVALID_ARGS;
  }

  if (*bsc_ptr != NULL) {
    close(fd);
    return STATUS_CODE_ALREADY_INITIALIZED;
  }

  ret = map_periph(fd, (uintptr_t)phys_base, BSC_LEN, bsc_ptr);
  close(fd);

  if (ret != STATUS_CODE_OK) {
    printf("map periph failed\n");
    return ret;
  }

  if (i2c_bus == I2C_BUS_1) {
    TRY(gpio_set_mode(2, GPIO_MODE_ALT0));
    TRY(gpio_set_mode(3, GPIO_MODE_ALT0));
  }
  else if (i2c_bus == I2C_BUS_2) {
    TRY(gpio_set_mode(4, GPIO_MODE_ALT5));
    TRY(gpio_set_mode(5, GPIO_MODE_ALT5));
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  volatile uint32_t *bsc = *bsc_ptr;

  printf("bsc=%p BSC_DIV=%u offset=%u BSC_LEN=%u\n", (void *)bsc,
         (unsigned)BSC_DIV, (unsigned)(BSC_DIV * 4), (unsigned)BSC_LEN);

  uint32_t cdiv = CORE_FREQ_HZ / i2c_hz;

  if (cdiv < 2) {
    cdiv = 2;
  }

  bsc[BSC_DIV] = cdiv;
  TRY(i2c_scan(i2c_bus));

  return STATUS_CODE_OK;
}

StatusCode i2c_deinit(I2cBus i2c_bus)
{
  volatile uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (bsc && (bsc != MAP_FAILED)) {
    munmap((void *)bsc, BSC_LEN);
    bsc = NULL;
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_scan(I2cBus i2c_bus)
{

  volatile uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (bsc == MAP_FAILED) {
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("Scanning I2C bus %d\n", i2c_bus);

  uint8_t addr = 0x03;

  for ( ; addr <= 0x77; addr++) {
    bsc[BSC_C] = C_I2CEN | C_CLEAR;
    bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;

    bsc[BSC_A] = addr;
    bsc[BSC_DLEN] = 0;

    bsc[BSC_C] = C_I2CEN | C_ST;

    while (!(bsc[BSC_S] & S_DONE)) {
      // wait
    }

    if (bsc[BSC_S] & S_CLKT) {
      printf("CLK stretch timeout while scanning i2c address %d\n", addr);
      bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;
      continue;
    }

    if (bsc[BSC_S] & S_ERR) {
      // NACK
      bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;
      continue;
    }

    printf("  Found I2C device at 0x%02X on bus %d\n", addr, i2c_bus);

    bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_write(I2cBus i2c_bus, uint8_t addr, const uint8_t *buf,
                     uint32_t len)
{
  volatile uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((bsc == MAP_FAILED) || !addr || !buf || !len) {
    return STATUS_CODE_INVALID_ARGS;
  }

  bsc[BSC_C] = C_I2CEN | C_CLEAR;
  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;   // clear sticky flags

  bsc[BSC_A] = addr;
  bsc[BSC_DLEN] = len;

  uint32_t idx = 0;
  while (idx < len && bsc[BSC_S] & S_TXD) {
    bsc[BSC_FIFO] = buf[idx];
    idx++;
  }

  bsc[BSC_C] = C_I2CEN | C_ST;

  while (!(bsc[BSC_S] & (S_DONE | S_ERR | S_CLKT))) {
    while (idx < len && bsc[BSC_S] & S_TXD) {
      bsc[BSC_FIFO] = buf[idx];
      idx++;
    }
  }

  int rc = 0;

  if (bsc[BSC_S] & S_ERR) {
    rc = -2;     // NACK
    fprintf(stderr, "i2c: NACK from addr 0x%02X (S=0x%08X)\n", addr, (unsigned)bsc[BSC_S]);
  }
  if (bsc[BSC_S] & S_CLKT) {
    rc = -3;     // clock stretch timeout
    fprintf(stderr, "i2c: clock stretch timeout at addr 0x%02X (S=0x%08X)\n", addr, (unsigned)bsc[BSC_S]);
  }

  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;   // clear flags
  return rc == 0 ? STATUS_CODE_OK : STATUS_CODE_FAILED;
}

StatusCode i2c_write_byte(I2cBus i2c_bus, uint8_t addr, uint8_t data)
{
  uint8_t data_buf[1];
  data_buf[0] = data;
  StatusCode ret = i2c_write(i2c_bus, addr, data_buf, 1);
  return ret;
}

StatusCode i2c_read(I2cBus i2c_bus, uint8_t addr, uint8_t *buf, uint32_t len)
{
  volatile uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((bsc == MAP_FAILED) || !addr || !buf || !len) {
    return STATUS_CODE_INVALID_ARGS;
  }

  bsc[BSC_C] = C_I2CEN | C_CLEAR;
  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;

  bsc[BSC_A] = addr;
  bsc[BSC_DLEN] = len;

  bsc[BSC_C] = C_I2CEN | C_ST | C_READ;

  uint32_t idx = 0;

  while (!(bsc[BSC_S] & (S_DONE | S_ERR | S_CLKT))) {
    while (idx < len && (bsc[BSC_S] & S_RXD)) {
      buf[idx] = bsc[BSC_FIFO] & 0xFF;
      idx++;
    }
  }

  while (idx < len && (bsc[BSC_S] & S_RXD)) {
    buf[idx] = bsc[BSC_FIFO] & 0xFF;
    idx++;
  }

  int rc = 0;

  if (bsc[BSC_S] & S_ERR) {
    rc = -2;     // NACK
    perror("i2c failure code -2");
  }
  if (bsc[BSC_S] & S_CLKT) {
    rc = -3;     // clock stretch timeout
    perror("i2c failure code -3");
  }

  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;   // clear flags
  return rc == 0 ? STATUS_CODE_OK : STATUS_CODE_FAILED;
}

StatusCode i2c_write_then_read(I2cBus i2c_bus, uint8_t addr, const uint8_t *write_buf, uint32_t write_len, uint8_t *read_buf, uint32_t read_len)
{
  volatile uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((bsc == MAP_FAILED) || !addr || !write_buf || !read_buf) {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (write_len > 16) {
    // Not invalid but implementation is harder. will implement if needed
    return STATUS_CODE_INVALID_ARGS;
  }

  bsc[BSC_C] = C_I2CEN | C_CLEAR;
  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;   // clear sticky flags

  bsc[BSC_A] = addr;
  bsc[BSC_DLEN] = write_len;

  for (uint32_t i = 0; i < write_len; i++) {
    while (!(bsc[BSC_S] & S_TXD)) {
      // wait for FIFO ready
    }
    bsc[BSC_FIFO] = write_buf[i];
  }

  bsc[BSC_C] = C_I2CEN | C_ST;

  while (!(bsc[BSC_S] & S_TA)) {
    if (bsc[BSC_S] & (S_ERR | S_CLKT)) {
      bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;       // clear sticky flags
      return STATUS_CODE_FAILED;
    }
  }

  bsc[BSC_DLEN] = read_len;

  bsc[BSC_C] = C_I2CEN | C_ST | C_READ;

  uint32_t idx = 0;

  while (!(bsc[BSC_S] & (S_DONE | S_ERR | S_CLKT))) {
    while (idx < read_len && (bsc[BSC_S] & S_RXD)) {
      read_buf[idx] = bsc[BSC_FIFO] & 0xFF;
      idx++;
    }
  }

  while (idx < read_len && (bsc[BSC_S] & S_RXD)) {
    read_buf[idx] = bsc[BSC_FIFO] & 0xFF;
    idx++;
  }

  int rc = 0;

  if (bsc[BSC_S] & S_ERR) {
    rc = -2;     // NACK
    perror("i2c failure code -2");
  }
  if (bsc[BSC_S] & S_CLKT) {
    rc = -3;     // clock stretch timeout
    perror("i2c failure code -3");
  }

  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;   // clear flags
  return rc == 0 ? STATUS_CODE_OK : STATUS_CODE_FAILED;
}
