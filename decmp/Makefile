
PREFIX = powerpc-eabi-

ifeq ($(OS),Windows_NT)
	BIN2H = ../bin2h/bin2h.exe
else
	BIN2H = ../bin2h/bin2h
endif

LDSCRIPT = decmp.ld

AR = $(PREFIX)ar
AS = $(PREFIX)as
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
RANLIB = $(PREFIX)ranlib
STRIP = $(PREFIX)strip

MACHDEP = -mcpu=750 -mno-eabi -mhard-float -DHW_DOL=1
CFLAGS = $(MACHDEP) -Os -Wall
LDFLAGS = $(MACHDEP) -n -nostartfiles -Wl,-T,$(LDSCRIPT) -L.
ASFLAGS = -DHW_DOL=1 -D_LANGUAGE_ASSEMBLY

TARGET_LINKED = decmp.elf
TARGET_BIN = decmp.bin
TARGET = decmp.h

OBJS = crt0.o main.o xz_dec_lzma2.o xz_dec_stream.o 

DEPDIR = .deps

LIBS = 

all: $(TARGET)

%.o: %.s
	@echo " ASSEMBLE    $<"
	@$(CC) $(CFLAGS) $(DEFINES) $(ASFLAGS) -c $< -o $@

%.o: %.S
	@echo " ASSEMBLE    $<"
	@$(CC) $(CFLAGS) $(DEFINES) $(ASFLAGS) -c $< -o $@

%.o: %.c
	@echo " COMPILE     $<"
	@$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

$(TARGET_LINKED): $(OBJS)
	@echo " LINK        $@"
	@$(CC) -g -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

$(TARGET_BIN): $(TARGET_LINKED)
	@echo " OBJCOPY     $@"
	@$(OBJCOPY) -Obinary -j .code $< $@

$(TARGET): $(TARGET_BIN)
	@echo " BIN2H       $@"
	@$(BIN2H) $<

clean:
	rm -rf $(TARGET_LINKED) $(TARGET_BIN) $(TARGET) $(OBJS) $(DEPDIR)
