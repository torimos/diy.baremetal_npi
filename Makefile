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
	@echo " [DUMP]  $(TARGET).lst"
	@$(DMP) -l -S -D $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).lst
	@echo " [RELF]  $(TARGET).symtab"
	@$(RELF) -s $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).symtab
	@echo "[BUILD for $(BOARD) finished]"

init:
	@echo "[BUILD for $(BOARD) started]"
	@echo "  CCFLAGS = $(CCFLAGS)"
	@echo "  LDFLAGS = $(LDFLAGS)"
	@mkdir -p $(OBJ_DIR) && cd $(OBJ_DIR) && mkdir -p $(filter-out $(HOME),$(sort $(dir $(OBJS))))

clean:
	@rm -vrf $(BUILD_DIR) > /dev/null