HOME := .
include Common.mk

FILES = $(shell find $(SRC_DIR)/ -type f -name '*.*')

all: clean
	@mkdir -p $(TOOLS_DIR) && cp -r $(HOME)/tools/bin/* $(TOOLS_DIR) && chmod u+x $(TOOLS_DIR)/build 
	@mkdir -p $(OBJ_DIR) && cd $(OBJ_DIR) && mkdir -p $(DIRS)
	@echo " LDFLAGS := $(LDFLAGS)"
	@echo " CFLAGS  := $(CFLAGS)"
	@echo " CCFLAGS := $(CCFLAGS)"
	@$(MAKE) -C $(SRC_DIR)

clean:
	@echo cleaning up ...
	@rm -vrf $(BUILD_DIR) $(TOOLS_DIR) > /dev/null
