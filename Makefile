# ================================
# Compiler and flags
# ================================
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -fPIC
LDFLAGS = -shared

# Project directories
PROJECT  = project
INCDIR   = $(PROJECT)/inc
SRCDIR   = $(PROJECT)/src
BUILDDIR = build

# Add include path for gpio.h
CFLAGS  += -I$(INCDIR)

# Backend selector: sim (default) or rpi
BACKEND ?= sim

# Source files for each backend
SRCS_SIM = $(SRCDIR)/blinky.c $(SRCDIR)/gpio_sim.c
SRCS_RPI = $(SRCDIR)/blinky.c $(SRCDIR)/gpio.c

ifeq ($(BACKEND),sim)
    SRCS = $(SRCS_SIM)
else ifeq ($(BACKEND),rpi)
    SRCS = $(SRCS_RPI)
else
    $(error Unknown BACKEND '$(BACKEND)' (use 'sim' or 'rpi'))
endif

# Object files live in build/
OBJS   = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
TARGET = $(BUILDDIR)/lib.so

.PHONY: all clean sim rpi build builddir

# Default target: simulation backend
all: sim

# High-level targets
sim:
	$(MAKE) BACKEND=sim build

rpi:
	$(MAKE) BACKEND=rpi build

# Actual build
build: builddir $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile project/src/*.c -> build/*.o
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/gpio.h
	@if [ "$(BACKEND)" = "rpi" ]; then \
		echo "Compiling $< for Raspberry Pi backend"; \
		$(CC) $(CFLAGS) -DGPIO_REAL -c $< -o $@; \
	else \
		echo "Compiling $< for Simulation backend"; \
		$(CC) $(CFLAGS) -c $< -o $@; \
	fi

# Ensure build directory exists
builddir:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)
