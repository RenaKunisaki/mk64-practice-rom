#!/bin/bash
set -e

BUILDDIR=.build
ROMFILE_IN=../mk64.rom
ROMFILE_OUT=patched.rom
SYMFILE_IN=./include/mk64/mk64-us.sym
SYMFILE_OUT=$BUILDDIR/symbols.sym
TOOLS=./include/n64/tools/
ELF_OUT=$BUILDDIR/$1.elf

mkdir -p $BUILDDIR
cp -n $SYMFILE_IN $SYMFILE_OUT
cp -n $ROMFILE_IN $ROMFILE_OUT

unset ENTRY
unset NOLOAD
ROMSIZE=16M
if [ -f $1/patch.cfg ]; then
    source $1/patch.cfg
fi

if [ -f $1/makefile ]; then
    make -C $1
else
    make --eval="NAME=$1" -f elf.mk
fi

PATCH_ARGS=
if [ ! -z ${ENTRY+x} ]; then PATCH_ARGS+="--entry=$ENTRY "; fi
if [ -n "$NOLOAD" ]; then PATCH_ARGS+="--no-load "; fi
PATCH_ARGS+="--pad $ROMSIZE "

nm $ELF_OUT > $BUILDDIR/sym.tmp
$TOOLS/mergesyms.py $SYMFILE_OUT $BUILDDIR/sym.tmp > $SYMFILE_OUT
$TOOLS/patch.py $PATCH_ARGS $ROMFILE_OUT $ELF_OUT
$TOOLS/crc.py -v $ROMFILE_OUT
