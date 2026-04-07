# ESP32 Mini Kernel - Build System

# Toolchain
CROSS   ?= xtensa-esp32-elf-
CC      := $(CROSS)gcc
AS      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

# Project
TARGET  := build/kernel
FLASH_OFFSET := 0x1000

# Directories
INCDIR  := include
SRCDIR  := kernel drivers apps boot

# Compiler flags
CFLAGS  := -mlongcalls \
           -ffreestanding \
           -nostdlib \
           -nostdinc \
           -Wall \
           -Wextra \
           -Os \
           -g \
           -mtext-section-literals \
           -I$(INCDIR)

ASFLAGS := -mlongcalls \
           -ffreestanding \
           -nostdlib \
           -I$(INCDIR)

LDFLAGS := -nostdlib \
           -T ld/esp32.ld \
           -Map=$(TARGET).map

# Source files
C_SRCS   := $(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.c))
ASM_SRCS := $(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.S))

# Object files
C_OBJS   := $(patsubst %.c,build/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.S,build/%.o,$(ASM_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)

# Create object directory structure
OBJDIRS  := $(sort $(dir $(OBJS)))

.PHONY: all clean flash image size

all: $(TARGET).elf size

# Create build directories
$(OBJDIRS):
	@mkdir -p $@

# Compile C sources
build/%.o: %.c | $(OBJDIRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S sources
build/%.o: %.S | $(OBJDIRS)
	$(AS) $(ASFLAGS) -c $< -o $@

# Link
$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Generate flashable binary image
image: $(TARGET).elf
	esptool.py elf2image --flash_mode dio --flash_size 4MB $<

# Flash to ESP32
flash: image
	esptool.py -p $(PORT) -b 460800 write_flash $(FLASH_OFFSET) build/kernel.bin

# Monitor serial output
monitor:
	python -m serial.tools.miniterm $(PORT) 115200

# Show size
size: $(TARGET).elf
	$(SIZE) $<

clean:
	rm -rf build/

# Default serial port
PORT ?= COM3
