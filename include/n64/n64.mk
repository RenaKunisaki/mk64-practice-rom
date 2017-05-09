cur_makefile := $(abspath $(lastword $(MAKEFILE_LIST)))
cur_dir := $(patsubst %/,%,$(dir $(cur_makefile)))
base_path := $(cur_dir)/../..

#OBJS += $(N64BASE)/lib/64drive/64drive.o \
#	$(N64BASE)/lib/libc/libc.o \
#	$(N64BASE)/lib/rawio/rawio.o

.PHONY: all dirs clean patch run-emu run-ed64 run-64drive

all: $(ELF)
	@echo done

clean:
	$(DELETE) $(BUILDDIR)
	$(shell find $(N64BASE) -name *.o -delete)

patch: $(ELF)
	#$(OBJCOPY) -O binary -j PATCH $(ELF) $(BINDIR)/patches.bin
	#$(NM) -n $(ELF) | grep _patch_ > $(BINDIR)/patches.txt
	#$(NM) -S $(ELF) > $(BINDIR)/symbols.txt
	#$(OBJDUMP) -hw $(ELF) > $(BINDIR)/sections.txt
	cp -n $(INPUT) $(OUTPUT)
	#./patch.py bin/patched.rom bin $(ELF)
	$(N64BASE)/tools/patch.py $(ELF) $(OUTPUT) $(PATCHFLAGS)
	$(N64BASE)/tools/crc.py -v $(OUTPUT)

run-emu: patch
	mupen64plus --corelib ~/projects/emus/mupen64plus/mupen64plus-core/projects/unix/libmupen64plus.so.2 --emumode 0 $(OUTPUT)

run-ed64: patch
	$(N64BASE)/tools/loader64 -w -f bin/patched.rom
	$(N64BASE)/tools/loader64 -p

run-64drive: patch
	64drive -vvl bin/patched.rom

dirs:
	$(MKDIR) $(DIRS) $(dir $(OBJS))

#$(BIN): dirs $(ELF)
#	$(OBJCOPY) -O binary -j .text -j .data $(ELF) $@

$(ELF): dirs $(OBJS)
	@echo " * Linking: $(notdir $(OBJS))"
	$(call LINK, $(OBJS), $(ELF))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo " * Compiling: $<"
	$(call COMPILE, $<, $@)

$(BUILDDIR)/%.o: $(SRCDIR)/%.s
	@echo " * Assembling: $<"
	$(call ASSEMBLE, $<, $@)

$(BUILDDIR)/lib/%.o: n64/lib/%.c
	@echo " * Compiling: $<"
	$(call COMPILE, $<, $@)
