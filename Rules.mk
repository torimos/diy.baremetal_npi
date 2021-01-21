include core/$(BOARD)/build_scripts/Rules.mk
include BuildEnvironment.mk

CC		= $(CC_PREFIX)gcc
CXX		= $(CC_PREFIX)g++
LD		= $(CC_PREFIX)ld
CPY		= $(CC_PREFIX)objcopy
DMP		= $(CC_PREFIX)objdump
NM		= $(CC_PREFIX)nm

CFLAGS 	= -g -Wall -fsigned-char -nostdlib -nostartfiles -ffreestanding $(BOARD_CFLAGS)
CCFLAGS = $(CFLAGS) -std=c++14 -Wno-aligned-new
LDFLAGS += -Bstatic \
			-T$(LINKER_DIR)/$(LINKERFILE) \
			--start-group \
			$(LIBS) \
			$(EXTRALIBS) \
			--end-group

ifeq ($(strip $(STDLIB_SUPPORT)),0)
LIBGCC	  = "$(shell $(CXX) $(CCFLAGS) -print-file-name=libgcc.a)"
LIBC	  = "$(shell $(CXX) $(CCFLAGS) -print-file-name=libc.a)"
ifneq ($(strip $(LIBGCC)),"libgcc.a")
EXTRALIBS += $(LIBGCC)
endif
ifneq ($(strip $(LIBC)),"libc.a")
EXTRALIBS += $(LIBC)
endif
CCFLAGS += -fno-exceptions -fno-rtti -nostdinc++
endif
ifeq ($(strip $(STDLIB_SUPPORT)),1)
LIBM = "$(shell $(CC) $(CCFLAGS) -print-file-name=libm.a)"
ifneq ($(strip $(LIBM)),"libm.a")
EXTRALIBS += $(LIBM)
endif
endif

ifeq ($(strip $(GC_SECTIONS)),1)
CFLAGS	+= -ffunction-sections -fdata-sections
LDFLAGS	+= --gc-sections
endif

define COLLEC_OBJ
$(foreach item,$(1),$(patsubst $(HOME)/$(item)/%,$(item)/%.o,$(shell find $(HOME)/$(item) -type f \( -iname "*.c" -o -name "*.cpp" -o -name "*.s" -o -name "*.S" \))))
endef
define COLLEC_INC
$(foreach item,$(1),$(patsubst %,-I%,$(filter-out $(HOME),$(sort $(dir $(shell find $(HOME)/$(item) -type f \( -iname "*.h" -o -name "*.hpp" \)))))))
endef

OBJS 		= $(call COLLEC_OBJ,$(SOURCES))
INCLUDES 	= $(call COLLEC_INC,$(SOURCES))

%.S.o: %.S
	@echo " [CC]    $<" 
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $(OBJ_DIR)/$@
%.c.o: %.c
	@echo " [CC]    $<" 
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $(OBJ_DIR)/$@
%.cpp.o: %.cpp
	@echo " [CXX]   $<" 
	@$(CXX) $(CCFLAGS) $(INCLUDES) -c $< -o $(OBJ_DIR)/$@