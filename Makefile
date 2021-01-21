HOME 		:= $(shell pwd)

ifeq ($(strip $(BOARD)),)
BOARD = nanopim3
endif


include Rules.mk

all: clean init $(OBJS)
	@echo " [LD]   $(TARGET).elf"
	@$(LD) -o $(BUILD_DIR)/$(TARGET).elf -Map $(BUILD_DIR)/$(TARGET).map $(patsubst %.o,$(OBJ_DIR)/%.o,$(OBJS)) $(LDFLAGS)
	@echo " [IMG]  $(TARGET).img"
	@$(CPY) $(BUILD_DIR)/$(TARGET).elf -O binary $(BUILD_DIR)/$(TARGET).img
	@echo " [DUMP]  $(TARGET).disasm"
	@$(DMP) -l -S -D $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).disasm
	@echo "[BUILD for $(BOARD) finished]"

init:
	@echo "[BUILD for $(BOARD) started]"
	@mkdir -p $(OBJ_DIR) && cd $(OBJ_DIR) && mkdir -p $(filter-out $(HOME),$(sort $(dir $(OBJS))))

clean:
	@rm -vrf $(BUILD_DIR) > /dev/null