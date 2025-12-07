#include "gpio.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define GPIO_BLOCK_SIZE 4096

#define GPFSEL0_INDEX 0 // 0x00 / 4
#define GPSET0_INDEX 7  // 0x1C / 4
#define GPCLR0_INDEX 10 // 0x28 / 4
#define GPLEV0_INDEX 13 // 0x34 / 4

static volatile uint32_t *s_gpio_regs = NULL;

int gpio_init(void)
{
    if (s_gpio_regs != NULL)
    {
        return 0; // already initialized
    }

    int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);

    if (fd < 0)
    {
        perror("open /dev/gpiomem");
        return -1;
    }

    s_gpio_regs = (uint32_t *)mmap(
        NULL,
        GPIO_BLOCK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    close(fd);

    if (s_gpio_regs == MAP_FAILED)
    {
        perror("mmap");
        s_gpio_regs = NULL;
        return -1;
    }

    return 0;
}

void gpio_set_mode(int pin, GpioMode mode)
{
    if (!s_gpio_regs)
    {
        return;
    }

    int reg_index = GPFSEL0_INDEX + (pin / 10);
    int shift = (pin % 10) * 3;

    uint32_t val = s_gpio_regs[reg_index];
    val &= ~(7U << shift);

    if (mode == GPIO_MODE_OUTPUT)
    {
        val |= (1U << shift);
    }
    else
    {
        // nothing
    }
    s_gpio_regs[reg_index] = val;
}

void gpio_write(int pin, int value)
{
    if (!s_gpio_regs)
    {
        return;
    }

    if (value)
    {
        // high
        int reg_index = GPSET0_INDEX + (pin / 32);
        s_gpio_regs[reg_index] = (1U << (pin % 32));
    }
    else
    {
        // low
        int reg_index = GPCLR0_INDEX + (pin / 32);
        s_gpio_regs[reg_index] = (1U << (pin % 32));
    }
}

int gpio_read(int pin)
{
    if (!s_gpio_regs)
    {
        return -1;
    }

    int reg_index = GPLEV0_INDEX + (pin / 32);
    uint32_t val = s_gpio_regs[reg_index];

    return (val & (1U << (pin % 32))) ? 1 : 0;
}