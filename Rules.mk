TOOL_PREFIX = arm-none-eabi-
CC = $(TOOL_PREFIX)gcc
OBJCOPY = $(TOOL_PREFIX)objcopy
LD = $(TOOL_PREFIX)ld

PYTHON = python3
ZIP = zip -r
UNZIP = unzip

ifneq ($(VERBOSE),1)
TOOL_PREFIX := @$(TOOL_PREFIX)
endif

FLAGS  = -g -Os -nostdlib -std=gnu99 -iquote $(ROOT)/inc
FLAGS += -Wall -Werror -Wno-format -Wdeclaration-after-statement
FLAGS += -Wstrict-prototypes -Wredundant-decls -Wnested-externs
FLAGS += -fno-common -fno-exceptions -fno-strict-aliasing
FLAGS += -mlittle-endian -mthumb -mfloat-abi=soft
FLAGS += -Wno-unused-value -ffunction-sections

ifeq ($(mcu),at32f4)
FLAGS += -mcpu=cortex-m4 -DAT32F4=4 -DMCU=4
DFU_DEV = 0x2e3c:0xdf11
at32f4=y
endif

ifneq ($(debug),y)
FLAGS += -DNDEBUG
endif

FLAGS += -MMD -MF .$(@F).d
DEPS = .*.d

FLAGS += $(FLAGS-y)

CFLAGS += $(CFLAGS-y) $(FLAGS) -include decls.h
AFLAGS += $(AFLAGS-y) $(FLAGS) -D__ASSEMBLY__
LDFLAGS += $(LDFLAGS-y) $(FLAGS) -Wl,--gc-sections

define cc_rule
$(1): $(SRCDIR)/$(2) $(SRCDIR)/Makefile
	@echo CC $$@
	$(CC) $$(CFLAGS) -c $$< -o $$@
endef

define as_rule
$(1): $(SRCDIR)/$(2) $(SRCDIR)/Makefile
	@echo AS $$@
	$(CC) $$(AFLAGS) -c $$< -o $$@
endef

SRCDIR := $(shell $(PYTHON) $(ROOT)/scripts/srcdir.py $(CURDIR))
include $(SRCDIR)/Makefile

SUBDIRS += $(SUBDIRS-y)
OBJS += $(OBJS-y) $(patsubst %,%/build.o,$(SUBDIRS))

# Force execution of pattern rules (for which PHONY cannot be directly used).
.PHONY: FORCE
FORCE:

.PHONY: clean

.SECONDARY:

build.o: $(OBJS)
	$(LD) -r -o $@ $^

%/build.o: FORCE
	+mkdir -p $*
	$(MAKE) -f $(ROOT)/Rules.mk -C $* build.o

%.ld: $(SRCDIR)/%.ld.S $(SRCDIR)/Makefile
	@echo CPP $@
	$(CC) -P -E $(AFLAGS) $< -o $@

%.elf: $(OBJS) %.ld $(SRCDIR)/Makefile
	@echo LD $@
	$(CC) $(LDFLAGS) -T$(*F).ld $(OBJS) -o $@
	chmod a-x $@

%.hex: %.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O ihex $< $@
	chmod a-x $@

%.bin: %.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@
	chmod a-x $@

%.dfu: %.hex
	$(PYTHON) $(ROOT)/scripts/dfu-convert.py -i $< -D $(DFU_DEV) $@

$(eval $(call cc_rule,%.o,%.c))
$(eval $(call as_rule,%.o,%.S))

-include $(DEPS)
