NAME = bootstrap
ASFLAGS ?= --defsym .text=0x803F0000

# bootstrap.o has to come first so that bootstrap is at 0x803F0000
#OBJS=$(BUILDDIR)/bootstrap.o #$(BUILDDIR)/lib.o $(BUILDDIR)/hooks.o
include ../elf.mk
