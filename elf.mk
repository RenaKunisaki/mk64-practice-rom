cur_makefile := $(abspath $(lastword $(MAKEFILE_LIST)))
PWD := $(patsubst %/,%,$(dir $(cur_makefile)))

ASFLAGS ?= --defsym .text=0x80400000

include $(PWD)/config.mk

LINKSCRIPTS := -T$(PWD)/include/n64/n64.ld \
	-T$(PWD)/include/$(GAME)/$(GAME)-$(VERSION).ld

SYMFILE := $(BUILDDIR)/symbols.sym
OUTPUT  := $(BUILDDIR)/$(NAME).elf

CFILES := $(wildcard $(PWD)/$(NAME)/*.c)
SFILES := $(wildcard $(PWD)/$(NAME)/*.s)
OBJS ?= $(CFILES:.c=.o) $(SFILES:.s=.o)
all: $(OUTPUT)
	@echo done

clean:
	$(DELETE) .build

$(OUTPUT): $(OBJS)
	@echo " * Linking: $(notdir $(OBJS))"
	@$(call LINK, $(OBJS), $(OUTPUT))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo " * Compiling: $<"
	@$(call COMPILE, $<, $@)

$(BUILDDIR)/%.o: $(SRCDIR)/%.s
	@echo " * Assembling: $<"
	@$(call ASSEMBLE, $<, $@)

$(BUILDDIR)/lib/%.o: n64/lib/%.c
	@echo " * Compiling: $<"
	@$(call COMPILE, $<, $@)