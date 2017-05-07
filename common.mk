cur_makefile := $(abspath $(lastword $(MAKEFILE_LIST)))
cur_dir := $(patsubst %/,%,$(dir $(cur_makefile)))

GAME    ?= mk64
VERSION ?= us
INPUT   := $(cur_dir)/../$(GAME).rom
OUTPUT  := $(cur_dir)/patched.rom
LINKSCRIPT := $(cur_dir)/include/$(GAME)/$(GAME)-$(VERSION).ld
SYMFILE := $(cur_dir)/include/$(GAME)/$(GAME)-$(VERSION).sym

include $(cur_dir)/include/n64/n64.mk
