#.include "../lib.s"

.set noreorder # don't rearrange branches

# the assembler doesn't know where our sections are going to be,
# so we need to define these here.
.equ RAM_BASE, 0x80400000 # where our code is in RAM
.equ ROM_BASE, 0xB0C00000 # where our code is in ROM
#.equ osPiStartDma, 0x800CDC30

.macro make_jump reg, dest
    # set register `reg` to the opcode for `j dest`
    # no idea why li doesn't work here.
    # also we have to do some tricky stuff with masking here because
    # for some reason we can't operate directly on .text
    lui \reg, %hi(((\dest - .text) | (RAM_BASE & 0x7FFFFF)) >> 2) | 0x08000000
    ori \reg, %lo(((\dest - .text) | (RAM_BASE & 0x7FFFFF)) >> 2)
.endm

.macro make_jal reg, dest
    # set register `reg` to the opcode for `jal dest`
    lui \reg, %hi(((\dest - .text) | (RAM_BASE & 0x7FFFFF)) >> 2) | 0x0C000000
    ori \reg, %lo(((\dest - .text) | (RAM_BASE & 0x7FFFFF)) >> 2)
.endm

.text
hooks_init:
    # hook the PI thread just before it begins a DMA.
    #make_jal $a0, dmaThreadHook_base
    #li    $a1, 0x800D2FF8
    #sw    $a0, ($a1)

    # hook the "draw time on title screen when R pressed" routine.
    make_jal $a0, titleHook_base
    li    $a1, 0x8009F978
    sw    $a0, ($a1)

    # hook crash screen draw routine to print to USB
    make_jal $a0, crashHook_base
    li    $a1, 0x80004650
    sw    $a0, ($a1)

    # patch crash handler to only require L press
    #li    $a1, 0x800DC6FE
    #li    $a0, 0xFFFF
    #sh    $a0, ($a1)

    # patch crash handler to display without any button press
    li    $a1, 0x800045F0
    li    $a0, 0x08001192 # j 0x80004648
    sw    $a0, ($a1)

    j    main_init
      nop


reg_save:
  .word 0, 0, 0, 0, 0, 0, 0, 0


titleHook_base:
    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    sw    $ra, 0x00($t1)
    sw    $a0, 0x04($t1)
    sw    $a1, 0x08($t1)
    sw    $a2, 0x0C($t1)
    sw    $a3, 0x10($t1)

    jal   titleHook
      move $a1, $s0

    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    lw    $ra, 0x00($t1)
    lw    $a0, 0x04($t1)
    lw    $a1, 0x08($t1)
    lw    $a2, 0x0C($t1)
    j     0x8009FB1C
      lw    $a3, 0x10($t1)


dmaThreadHook_base:
    #800D33A4 jalr $ra, $t9  # a0=0 (direction) a1=devaddr a2=destaddr a3=len
    #800D33A8 nop # t9=osPiRawStartDma

    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    sw    $ra, 0x00($t1)
    sw    $a0, 0x04($t1)
    sw    $a1, 0x08($t1)
    sw    $a2, 0x0C($t1)

    jal dmaThreadHook
      sw    $a3, 0x10($t1)

    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    lw    $ra, 0x00($t1)
    lw    $a0, 0x04($t1)
    lw    $a1, 0x08($t1)
    lw    $a2, 0x0C($t1)
    j     0x800D3830 # back to hooked function
      lw    $a3, 0x10($t1)


crashHook_base:
    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    sw    $ra, 0x00($t1)
    sw    $a0, 0x04($t1)
    sw    $a1, 0x08($t1)
    sw    $a2, 0x0C($t1)
    sw    $a3, 0x10($t1)

    jal   crashHook
      #move $a1, $s0
      nop

    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    lw    $ra, 0x00($t1)
    lw    $a0, 0x04($t1)
    lw    $a1, 0x08($t1)
    lw    $a2, 0x0C($t1)
    j     0x80004298
      lw    $a3, 0x10($t1)
