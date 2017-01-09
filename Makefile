# the path to the original Makefile, allows to use symlinks
BASE_PATH:=$(shell readlink $(MAKEFILE_LIST))
ifeq ($(BASE_PATH),)
	BASE_PATH:=$(MAKEFILE_LIST)
endif
BASE_PATH:=$(shell dirname $(BASE_PATH))




# BUILDING OPTIONS (CHANGE THIS IF NEEDED)
# the files to create
TARGET   := program
# the ide:ntifier of the board
BOARD    := leonardo
# the ard:uino installation directory
ARD_BASE := /usr/share/arduino
# the ser:ial port the board is connected to
PORT     := /dev/ttyACM0
# building directory for arduino core and libs
# WARNING: do not use "."
#          make clean / make destroy would wipe it out
#          it is safe to choose any unused directory (e.g. ./obj)
LIB_OBJ  := $(BASE_PATH)/obj
# location of the arduino.c folder
LIB_C    := $(BASE_PATH)/core
# allow classic Arduino C++ with libraries
#MODE:=cpp




















# =============================
# =                           =
# =    = GATHERING INFOS =    =
# =                           =
# =============================


# TARGET CONFIGURATION
CONF     := $(ARD_BASE)/hardware/arduino/boards.txt
NAME     := $(shell grep '^$(BOARD).name='            $(CONF) | cut -d'=' -f2)
PROTOCOL := $(shell grep '^$(BOARD).upload.protocol=' $(CONF) | cut -d'=' -f2)
MCU      := $(shell grep '^$(BOARD).build.mcu='       $(CONF) | cut -d'=' -f2)
F_CPU    := $(shell grep '^$(BOARD).build.f_cpu='     $(CONF) | cut -d'=' -f2)
VID      := $(shell grep '^$(BOARD).build.vid='       $(CONF) | cut -d'=' -f2)
PID      := $(shell grep '^$(BOARD).build.pid='       $(CONF) | cut -d'=' -f2)
VARIANT  := $(shell grep '^$(BOARD).build.variant='   $(CONF) | cut -d'=' -f2)
CORE     := $(shell grep '^$(BOARD).build.core='      $(CONF) | cut -d'=' -f2)
# in the first line of src_complete/todo.txt
REVISION := 0101




# ARDUINO FILES (CORE, CORE VARIANT, LIBRARIES)
LIB_CORE := $(ARD_BASE)/hardware/arduino/cores/$(CORE)
LIB_VAR  := $(ARD_BASE)/hardware/arduino/variants/$(VARIANT)
ifeq ($(MODE),cpp)
LIB_ARD  := $(ARD_BASE)/libraries
INC_PATH := $(LIB_CORE) $(LIB_VAR) $(wildcard $(LIB_ARD)/*)
LIB_C    :=
else
LIB_ARD  :=
INC_PATH := $(LIB_CORE) $(LIB_VAR)
endif




# COMPILATION AND LINKING FLAGS
CC      := avr-gcc
ARD_OPT := -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DUSB_VID=$(VID) -DUSB_PID=$(PID) -DARDUINO=$(REVISION)
FLAGS   := -Wall -Wextra -pedantic -ansi -Os $(addprefix -I, $(INC_PATH))
SFLAGS  := $(FLAGS)
CFLAGS  := $(FLAGS) -ffunction-sections -fdata-sections -std=c99
XFLAGS  := $(FLAGS) -ffunction-sections -fdata-sections -fno-exceptions
LDFLAGS := -Os -Wl,--gc-sections




# LIST PROJECT SOURCE FILES AND CORRESPONDING OBJ FILES
SFILES := $(wildcard *.S)
CFILES := $(wildcard *.c)
XFILES := $(wildcard *.cpp)
IFILES := $(wildcard *.ino)
OFILES := $(SFILES:.S=.o) $(CFILES:.c=.o) $(XFILES:.cpp=.o) $(IFILES:.ino=.o)




# LIST CORE AND LIBRARY SOURCE FILES
# the %P format strips the root path
ifeq ($(MODE), cpp)
CLIB := $(shell find $(LIB_CORE) $(LIB_ARD) -name "*.c"   -printf "%P\n")
XLIB := $(shell find $(LIB_CORE) $(LIB_ARD) -name "*.cpp" -printf "%P\n")
else
CLIB := $(shell find $(LIB_CORE) $(LIB_C) $(LIB_ARD) -name "*.c"   -printf "%P\n")
XLIB :=
endif
OLIB := $(addprefix $(LIB_OBJ)/, $(CLIB:.c=.o) $(XLIB:.cpp=.o))





















# =============================
# =                           =
# =    = ACTUAL BUILDING =    =
# =                           =
# =============================

# basic target
all: $(TARGET).hex




# PROJECT FILES
%.o: %.S
	@echo $@
	@$(CC) $(ARD_OPT) $(SFLAGS) -c $< -o $@

%.o: %.c
	@echo $@
	@$(CC) $(ARD_OPT) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo $@
	@$(CC) $(ARD_OPT) $(XFLAGS) -c $< -o $@




# .ino files are compiled either as C or C++ files, depending on the mode
# the adequate include and main code are added
%.o: %.ino
	@echo $@
ifeq ($(MODE),cpp)
	@(echo "#include <Arduino.h>"; cat $<) | $(CC) $(ARD_OPT) $(XFLAGS) -x c++ -c - -o $@
else
	@(echo "#include <Arduino.h>"; cat $<) | $(CC) $(ARD_OPT) $(CFLAGS) -x c -c - -o $@
endif




# CORE LIBRARY
$(LIB_OBJ)/%.o: $(LIB_CORE)/%.c
	@echo $@
	@mkdir -p $(@D)
	@$(CC) $(ARD_OPT) $(CFLAGS) -c $< -o $@

$(LIB_OBJ)/%.o: $(LIB_CORE)/%.cpp
	@echo $@
	@mkdir -p $(@D)
	@$(CC) $(ARD_OPT) $(XFLAGS) -c $< -o $@




# ARDUINO.C LIBRARY
$(LIB_OBJ)/%.o: $(LIB_C)/%.c
	@echo $@
	@mkdir -p $(@D)
	@$(CC) $(ARD_OPT) $(CFLAGS) -I $(<D) -c $< -o $@




# OTHER LIBRARIES
# the -I options are hacks to fix invalid inclusion directives
$(LIB_OBJ)/%.o: $(LIB_ARD)/%.c
	@echo $@
	@mkdir -p $(@D)
	@$(CC) $(ARD_OPT) $(CFLAGS) -c $< -o $@

$(LIB_OBJ)/%.o: $(LIB_ARD)/%.cpp
	@echo $@
	@mkdir -p $(@D)
	$(CC) $(ARD_OPT) $(XFLAGS) -I $(<D) -I $(<D)/utility -c $< -o $@




# LINK OBJ FILES AND FILTER ADEQUATE SECTIONS
$(TARGET).hex: $(OFILES) $(OLIB)
	@echo $@
	@$(CC) $(ARD_OPT) $(LDFLAGS) $^ -o $(TARGET).elf
	@avr-objcopy -O ihex -j .text -j .data $(TARGET).elf $@
	@rm $(TARGET).elf




# UPLOAD PROGRAM TO CHIP
# the 'stty' call reset Leonardo and derivative by using the magic baudrate (1200)
upload: $(TARGET).hex
	@echo "Uploading..."
	@stty -F $(PORT) 1200 raw ignbrk hup
	@sleep 1
	@stty -F $(PORT) 9600
	@avrdude -D -b 9600 -p $(MCU) -c $(PROTOCOL) -P $(PORT) -U flash:w:$<:i


# OTHER TARGETS
clean:
	rm -f *.o

cleanobj:
	rm -Rf $(LIB_OBJ)

destroy: clean
	rm -f $(TARGET).hex

rebuild: destroy all

.PHONY: all upload clean cleanobj destroy rebuild
