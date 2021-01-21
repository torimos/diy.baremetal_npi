ifeq ($(strip $(CROSS_COMPILER)),aarch64)
CC_PREFIX=/home/ubuntu/cc/aarch64-none-elf/bin/aarch64-none-elf-
endif
ifeq ($(strip $(CROSS_COMPILER)),arm32)
CC_PREFIX=arm-none-eabi-
endif

BOARD_DIR		= core/$(BOARD)
SOURCES			= $(BOARD_DIR) app
LINKER_DIR		= $(BOARD_DIR)/build_scripts
BUILD_DIR		= $(HOME)/build/$(BOARD)
OBJ_DIR			= $(BUILD_DIR)/obj