cur_makefile := $(abspath $(lastword $(MAKEFILE_LIST)))
PWD := $(patsubst %/,%,$(dir $(cur_makefile)))

include $(PWD)/config.mk

LINKSCRIPTS := -T$(PWD)/.build/memory.ld \
	-T$(PWD)/include/n64/n64.ld \
	-T$(PWD)/include/$(GAME)/$(GAME)-$(VERSION).ld

SYMFILE := $(BUILDDIR)/symbols.sym
OUTPUT  := $(BUILDDIR)/$(NAME).elf

CFILES := $(wildcard $(PWD)/$(NAME)/*.c)
SFILES := $(wildcard $(PWD)/$(NAME)/*.s)
OBJS ?= $(CFILES:.c=.o) $(SFILES:.s=.o)

N64LIB := $(PWD)/include/n64/lib
OBJS += $(N64LIB)/64drive/64drive.o \
	$(N64LIB)/libc/libc.o \
	$(N64LIB)/rawio/rawio.o

.PHONY: all clean

all: $(OUTPUT)
	@echo done

clean:
	$(DELETE) .build

$(OUTPUT): $(OBJS)
	@echo " * Linking: $(notdir $(OBJS))"
	$(call LINK, $(OBJS), $(OUTPUT))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo " * Compiling: $<"
	$(call COMPILE, $<, $@)

$(BUILDDIR)/%.o: $(SRCDIR)/%.s
	@echo " * Assembling: $<"
	$(call ASSEMBLE, $<, $@)

$(BUILDDIR)/lib/%.o: $(PWD)/include/n64/lib/%.c
	@echo " * Compiling: $<"
	$(call COMPILE, $<, $@)
