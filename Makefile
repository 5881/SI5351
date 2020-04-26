PROJECT = syntez
BUILD_DIR = bin

SHARED_DIR = ./inc

CFILES = oledi2c.c oled_printf.c
CFILES += si5351.c
CFILES += main.c

# TODO - you will need to edit these two lines!
DEVICE=stm32f103c8t6
#OOCD_FILE = interface/stlink-v2.cfg
OOCD_INTERFACE = stlink-v2
OOCD_TARGET = stm32f1x

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=./libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ./rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
