# ================================
# Compiler and flags
# ================================
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -fPIC
LDFLAGS = -shared

# ================================
# Project directories
# ================================
PROJECT    = project
LIB        = libs
INCDIR_PR  = $(PROJECT)/inc
INCDIR_LIB = $(LIB)/inc
SRCDIR_PR  = $(PROJECT)/src
SRCDIR_LIB = $(LIB)/src

# Include paths
CFLAGS += -I$(INCDIR_LIB)
CFLAGS += -I$(INCDIR_PR)

# ================================
# Backend selection
# ================================
BACKEND ?= sim
BUILDDIR = build/$(BACKEND)
TARGET   = $(BUILDDIR)/lib.so

# ================================
# Object files
# ================================
OBJS_SIM = \
	$(BUILDDIR)/blinky.o \
	$(BUILDDIR)/gpio_sim.o \
	$(BUILDDIR)/i2c_sim.o \
	$(BUILDDIR)/pwm_controller.o \
	$(BUILDDIR)/servo.o \
	$(BUILDDIR)/irled.o \
	$(BUILDDIR)/currentsense.o

OBJS_RPI = \
	$(BUILDDIR)/blinky.o \
	$(BUILDDIR)/gpio.o \
	$(BUILDDIR)/i2c.o \
	$(BUILDDIR)/pwm_controller.o \
	$(BUILDDIR)/servo.o \
	$(BUILDDIR)/irled.o \
	$(BUILDDIR)/currentsense.o \
	$(BUILDDIR)/i2s.o

.PHONY: all sim rpi build clean builddir

# ================================
# Top-level targets
# ================================
all: sim

sim:
	$(MAKE) build BACKEND=sim

rpi:
	$(MAKE) build BACKEND=rpi

build: builddir
ifeq ($(BACKEND),sim)
	$(MAKE) $(TARGET) OBJS="$(OBJS_SIM)"
else ifeq ($(BACKEND),rpi)
	$(MAKE) $(TARGET) OBJS="$(OBJS_RPI)"
else
	$(error Unknown BACKEND $(BACKEND))
endif

# ================================
# Link
# ================================
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lasound

# ================================
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

$(BUILDDIR)/i2c_sim.o: $(SRCDIR_LIB)/i2c_sim.c $(INCDIR_LIB)/cm4_i2c.h
	@echo "Compiling i2c_sim.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/i2c.o: $(SRCDIR_LIB)/i2c.c $(INCDIR_LIB)/cm4_i2c.h
	@echo "Compiling i2c.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/i2s.o: $(SRCDIR_LIB)/i2s.c $(INCDIR_LIB)/cm4_i2s.h
	@echo "Compiling i2s.c"
	$(CC) $(CFLAGS) -c $< -o $@ 

$(BUILDDIR)/pwm_controller.o: $(SRCDIR_LIB)/pwm_controller.c $(INCDIR_LIB)/pwm_controller.h
	@echo "Compiling pwm_controller.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/servo.o: $(SRCDIR_PR)/servo.c $(INCDIR_PR)/servo.h
	@echo "Compiling servo.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/irled.o: $(SRCDIR_PR)/irled.c $(INCDIR_PR)/irled.h
	@echo "Compiling irled.c"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/currentsense.o: $(SRCDIR_PR)/currentsense.c $(INCDIR_PR)/currentsense.h
	@echo "Compiling currentsense.c"
	$(CC) $(CFLAGS) -c $< -o $@

# -------------------------
# Utility targets
# -------------------------
builddir:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf build
