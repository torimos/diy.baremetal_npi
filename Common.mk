AARCH	 ?= 64

STDLIB_SUPPORT ?= 0

SRC_DIR			= $(HOME)/src
INC_DIR			= $(HOME)/include
LINKER_DIR		= $(HOME)/linker
BUILD_DIR		= $(HOME)/build
OBJ_DIR			= $(BUILD_DIR)/obj
TOOLS_DIR		= /tmp/tools/bin

ifeq ($(strip $(AARCH)),64)
	PREFIX ?= /home/ubuntu/cc/aarch64-none-elf/bin/aarch64-none-elf-
	CFLAGS += -mabi=lp64
else
	$(error AARCH must be set to 64)
endif

ARCH 		= armv8-a+crc
CPU 		= cortex-a53
TARGET 		= kernel8
LINKERFILE 	= $(LINKER_DIR)/spl.lds

CC		= $(PREFIX)gcc
CXX		= $(PREFIX)g++
LD		= $(PREFIX)ld
CPY		= $(PREFIX)objcopy
DMP		= $(PREFIX)objdump
NM		= $(PREFIX)nm
HDMP	= hexdump
BLD		= $(TOOLS_DIR)/build

INCLUDES 	+= -I$(SRC_DIR) -I$(SRC_DIR)/**
INCLUDES 	+= -I$(INC_DIR) -I$(INC_DIR)/**

CFLAGS 		+= -g -Wall -fsigned-char -nostdlib -nostartfiles -ffreestanding \
				-mlittle-endian -march=$(ARCH) -mtune=$(CPU)
CCFLAGS 	+= $(CFLAGS) -std=c++14 -Wno-aligned-new

ifeq ($(strip $(STDLIB_SUPPORT)),0)
	LIBGCC	  = "$(shell $(CC) $(CCFLAGS) $(INCLUDES) -print-file-name=libgcc.a)"
	LIBC	  = "$(shell $(CC) $(CCFLAGS) $(INCLUDES) -print-file-name=libc.a)"
	ifneq ($(strip $(LIBGCC)),"libgcc.a")
		EXTRALIBS += $(LIBGCC)
	endif
	ifneq ($(strip $(LIBC)),"libc.a")
		EXTRALIBS += $(LIBC)
	endif
	CCFLAGS += -fno-exceptions -fno-rtti -nostdinc++
endif
ifeq ($(strip $(STDLIB_SUPPORT)),1)
	LIBM = "$(shell $(CC) $(CCFLAGS) $(INCLUDES) -print-file-name=libm.a)"
	ifneq ($(strip $(LIBM)),"libm.a")
		EXTRALIBS += $(LIBM)
	endif
endif

LDFLAGS += -Bstatic -T$(LINKERFILE) \
			--start-group \
			$(LIBS) $(EXTRALIBS) \
			--end-group

OBJS = $(patsubst $(SRC_DIR)/%, %.o, $(shell find $(SRC_DIR)/ -type f \( -iname "*.c" -o -name "*.cpp" -o -name "*.S" \)))
DIRS = $(filter-out ./,$(sort $(dir $(OBJS))))

