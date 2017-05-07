/* INCLUDE include/n64/n64.ld */

BOOTPATCH_ROM_ADDR = 0x3034;
BOOTPATCH_RAM_ADDR = 0x80002434;
BOOTSTRAP_ROM_ADDR = 0xBF0000;
BOOTSTRAP_RAM_ADDR = 0x803F0000;
BOOTSTRAP_READ_ADDR = BOOTSTRAP_ROM_ADDR | 0xB0000000;
PATCH_ROM_ADDR = 0xC00000;
PATCH_RAM_ADDR = 0x80400000;
PATCH_READ_ADDR = PATCH_ROM_ADDR | 0xB0000000;

PHDRS
{
    /* these have to be PT_SHLIB or else the VMA becomes zero
     * because reasons.  or it tries to make a 2GB file.
     * or PT_DYNAMIC but then readelf barfs since that's not
     * what PT_DYNAMIC is for.
     * it SHOULD be PT_LOAD (which might also kill the "not in segment"
     * warning) but that doesn't work.
     */
    bootstrap 0x7511   AT(BOOTPATCH_ROM_ADDR) ;
    boot      0x7511   AT(BOOTSTRAP_ROM_ADDR) ;
    text      PT_SHLIB AT(PATCH_ROM_ADDR) ;
}

SECTIONS
{
    . = BOOTPATCH_RAM_ADDR;
    .bootstrap . : { KEEP(*(.bootstrap)); } AT>RAM :bootstrap

    . = BOOTSTRAP_RAM_ADDR;
    .boot . : {
        _sboot = .;
        *(.boot);
        _eboot = .;
        _lboot = _eboot - _sboot;
    } AT>RAM :boot

    . = PATCH_RAM_ADDR;
    .text . : {
        _stext = .;
        *(.text);
        *(.*data*);
        _etext = .;
        _ltext = _etext - _stext;
    } AT>RAM :text

    /* .data . : { _sdata = . ; *(.*data); _edata = . ; } */

    .bss  . : {
        _sbss  = .;
        *(.bss);
         _ebss  = . ;
         _lbss = _ebss - _sbss;
    }

    /* get these out of the way */
    .junk . : {
        *(.reginfo) *(.gnu.*) *(.MIPS.*)
    } :NONE
}
