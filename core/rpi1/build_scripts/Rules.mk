CPU 			:= arm1176jzf-s
FLOAT_ABI 		:= hard
TARGET 			:= kernel
LINKERFILE 		:= rpi32.lds
STDLIB_SUPPORT 	:= 0
GC_SECTIONS 	:= 0

CROSS_COMPILER := arm32
BOARD_CFLAGS := -Ofast -mcpu=$(CPU) -marm -mfpu=vfp -mfloat-abi=$(FLOAT_ABI) -DRPI1