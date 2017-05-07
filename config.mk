GAME=mk64
VERSION=us

TOOL=mips64-elf-
SRCDIR ?= .
BUILDDIR ?= $(PWD)/.build
#BINDIR ?= bin
N64BASE=$(PWD)/include/n64

CC=$(TOOL)g++
AS=$(TOOL)as
AR=$(TOOL)ar
LD=$(TOOL)g++
NM=$(TOOL)nm
OBJCOPY=$(TOOL)objcopy
OBJDUMP=$(TOOL)objdump
READELF=$(TOOL)readelf
SIZE=$(TOOL)size
DELETE=rm -rf
MKDIR=mkdir -p

CFLAGS += -mips3 -mabi=32 -mlong32 -mxgot -mhard-float -G0 -O3 -I$(N64BASE) -I$(N64BASE)/ultra64/
ASFLAGS += -mips3 -mabi=32 -mhard-float -G0 -O3
LDFLAGS += -nostdlib $(LINKSCRIPTS) -Wl,--nmagic -Wl,--no-gc-sections -Wl,--just-symbols=$(SYMFILE)

# function COMPILE(infile, outfile)
COMPILE=$(CC) $(CFLAGS) -c $1 -o $2

# function ASSEMBLE(infile, outfile)
ASSEMBLE=$(AS) $(ASFLAGS) $1 -o $2

# function LINK(infile, outfile)
LINK=$(CC) $1 $(LDFLAGS) -o $2

ELF=$(BUILDDIR)/$(PROJECT).elf
HEX=$(BINDIR)/$(PROJECT).hex
BIN=$(BINDIR)/$(PROJECT).bin
DIRS=$(BINDIR) $(BUILDDIR)
