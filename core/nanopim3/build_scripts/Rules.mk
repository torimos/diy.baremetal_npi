AARCH	 		:= 64
ARCH 			:= armv8-a+crc
CPU 			:= cortex-a53
TARGET 			:= kernel8
LINKERFILE 		:= spl.lds
STDLIB_SUPPORT 	:= 0
GC_SECTIONS 	:= 0

BOARD_CFLAGS := -mlittle-endian -march=$(ARCH) -mtune=$(CPU)

ifeq ($(strip $(AARCH)),64)
CROSS_COMPILER := aarch64
BOARD_CFLAGS += -mabi=lp64
else
$(error AARCH must be set to 64)
endif
