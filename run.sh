#!/bin/bash

# for Everdrive64 v3
#./include/n64/tools/loader64 -w -f patched.rom
#./include/n64/tools/loader64 -p

# for 64drive
64drive -vvl patched.rom -c 6102

# for mupen64 (XXX)
#mupen64plus --corelib ~/projects/emus/mupen64plus/mupen64plus-core/projects/unix/libmupen64plus.so.2 --emumode 0 patched.rom
