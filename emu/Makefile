# Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
# GameCube Port Copyright 2016 FIX94
# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file LICENSE or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt


# Configuration:

# What toolchain prefix should we use
CROSS ?= powerpc-eabi-

# Where are the tools (http://git.infradead.org/users/segher/wii.git)
TOOLS ?= C:\segher_wii_tools

ifeq ($(OS),Windows_NT)
	CMPBIN = ../cmpbin/cmpbin.exe
else
	CMPBIN = ../cmpbin/cmpbin
endif

ifeq ($(OS),Windows_NT)
	BIN2H = ../bin2h/bin2h.exe
else
	BIN2H = ../bin2h/bin2h
endif

# End of configuration.


# Set CC, LD, OBJCOPY based on CROSS, unless they are set already

ifeq ($(origin CC), default)
	CC := $(CROSS)gcc -m32
endif
ifeq ($(origin LD), default)
	LD := $(CROSS)gcc
endif
OBJCOPY ?= $(CROSS)objcopy

# The compiler flags we need.

CFLAGS := -Wall -W -O3 -DDO_INLINE_ATTRIBS -Wno-attributes -flto -ffreestanding -mno-eabi -mno-sdata -mcpu=750

# Build with "V=1" to see the commands executed; be quiet otherwise.

ifeq ($(V),1)
	Q :=
else
	Q := @
	MAKEFLAGS += --no-print-directory
endif

all:
	@echo "  ASSEMBLE  start.o"
	$(Q)$(CC) $(CFLAGS) -c start.S -o start.o
	@echo "  COMPILE   main.o"
	$(Q)$(CC) $(CFLAGS) -c main.c -o main.o
	@echo "  COMPILE   apu.o"
	$(Q)$(CC) $(CFLAGS) -c apu.c -o apu.o
	@echo "  COMPILE   cpu.o"
	$(Q)$(CC) $(CFLAGS) -c cpu.c -o cpu.o
	@echo "  COMPILE   ppu.o"
	$(Q)$(CC) $(CFLAGS) -c ppu.c -o ppu.o
	@echo "  COMPILE   input.o"
	$(Q)$(CC) $(CFLAGS) -c input.c -o input.o
	@echo "  COMPILE   mem.o"
	$(Q)$(CC) $(CFLAGS) -c mem.c -o mem.o
	@echo "  LINK      emu.elf"
	$(Q)$(LD) start.o main.o apu.o cpu.o ppu.o input.o mem.o -T emu.ld -o emu.elf
	@echo "  OBJCOPY   emu_decmp.bin"
	$(Q)$(OBJCOPY) -Obinary -j .code emu.elf emu_decmp.bin
	@echo "  CMPBIN    emu_cmp.bin"
	$(Q)$(CMPBIN)
	@echo "  OUTPUT    emu_cmp.h"
	$(Q)$(BIN2H) emu_cmp.bin

clean:
	-rm -f start.o main.o apu.o cpu.o ppu.o input.o mem.o emu.elf emu_decmp.bin emu_cmp.bin emu_cmp.h
