# ================================
# Compiler and flags
# ================================
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -fPIC
LDFLAGS = -shared

# Project directories
PROJECT    = project
LIB        = libs
INCDIR_PR  = $(PROJECT)/inc
INCDIR_LIB = $(LIB)/inc
SRCDIR_PR  = $(PROJECT)/src
SRCDIR_LIB = $(LIB)/src
BUILDDIR   = build

# Include path for .h files
CFLAGS += -I$(INCDIR_LIB)
CFLAGS += -I$(INCDIR_PR)

# Object files
OBJS_SIM = $(BUILDDIR)/blinky.o \
           $(BUILDDIR)/gpio_sim.o \
           $(BUILDDIR)/i2c_sim.o

OBJS_RPI = $(BUILDDIR)/blinky.o \
           $(BUILDDIR)/gpio.o    \
           $(BUILDDIR)/i2c.o

TARGET = $(BUILDDIR)/lib.so

.PHONY: all clean sim rpi build builddir

# Default target: simulation backend
all: sim

# -------------------------
# High-level targets
# -------------------------
sim: builddir $(OBJS_SIM)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS_SIM)

rpi: builddir $(OBJS_RPI)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS_RPI)

# Optional: keep BACKEND=sim/rpi interface
BACKEND ?= sim
build:
	@if [ "$(BACKEND)" = "sim" ]; then \
		$(MAKE) sim; \
	elif [ "$(BACKEND)" = "rpi" ]; then \
		$(MAKE) rpi; \
	else \
		echo "Unknown BACKEND: $(BACKEND) (use 'sim' or 'rpi')"; \
		exit 1; \
	fi

# -------------------------
# Object rules
# -------------------------
$(BUILDDIR)/blinky.o: $(SRCDIR_PR)/blinky.c $(INCDIR_PR)/blinky.h
	@echo "Compiling blinky.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gpio_sim.o: $(SRCDIR_LIB)/gpio_sim.c $(INCDIR_LIB)/cm4_gpio.h
	@echo "Compiling gpio_sim.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gpio.o: $(SRCDIR_LIB)/gpio.c $(INCDIR_LIB)/cm4_gpio.h
	@echo "Compiling gpio.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/i2c_sim.o: $(SRCDIR_LIB)/i2c_sim.c $(INCDIR_LIB)/i2c.h
	@echo "Compiling i2c_sim.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/i2c.o: $(SRCDIR_LIB)/i2c.c $(INCDIR_LIB)/i2c.h
	@echo "Compiling i2c.c"
	$(CC) $(CFLAGS) -c $< -o $@

# -------------------------
# Utility targets
# -------------------------
builddir:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)
