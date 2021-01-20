include $(HOME)/Common.mk

%.S.o: %.S
	@echo " [CC]   $<" 
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $(OBJ_DIR)/$@
%.c.o: %.c
	@echo " [CC]   $<" 
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $(OBJ_DIR)/$@
%.cpp.o: %.cpp
	@echo " [CXX]   $<" 
	@$(CXX) $(CCFLAGS) $(INCLUDES) -c $< -o $(OBJ_DIR)/$@

$(TARGET).img: $(OBJS)
	@echo " [LD]   $(TARGET).elf"
	@$(LD) -o $(BUILD_DIR)/$(TARGET).elf -Map $(BUILD_DIR)/$(TARGET).map $(patsubst %.o,$(OBJ_DIR)/%.o,$(OBJS)) $(LDFLAGS)
	@echo " [IMG]  $(TARGET)_nonsih.img"
	@$(CPY) $(BUILD_DIR)/$(TARGET).elf -O binary $(BUILD_DIR)/$(TARGET)_nonsih.img
	@echo " [IMG]  $(TARGET).img"
	@$(BLD) $(TOOLS_DIR)/nsih.bin $(BUILD_DIR)/$(TARGET)_nonsih.img $(BUILD_DIR)/$(TARGET).img
	@echo " [DUMP]  $(TARGET).disasm"
	@$(DMP) -l -S -D $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).disasm