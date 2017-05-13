.set noreorder # don't rearrange branches
.include ".build/memory.s"

#.equ osPiStartDma, 0x800CDC30

.text
crash_hooks_init:
    # hook the PI thread just before it begins a DMA.
    #make_jal $a0, dmaThreadHook_base
    #li    $a1, 0x800D2FF8
    #sw    $a0, ($a1)

    # hook the "draw time on title screen when R pressed" routine.
    lw    $a0, jal_titleHook_base
    li    $a1, 0x8009F978
    sw    $a0, ($a1)

    # hook crash screen draw routine to print to USB
    lw    $a0, jal_crashHook_base
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

    j    crash_main_init
      nop

# these are copied to the appropriate places in RAM, patching existing code.
# the delay slot doesn't apply since these won't be executed here.
# this wastes a few bytes vs just loading the opcode directly with li,
# but there doesn't seem to be a reliable way to do that.
jal_titleHook_base: jal titleHook_base
jal_crashHook_base: jal crashHook_base


crash_reg_save:
  .word 0, 0, 0, 0, 0, 0, 0, 0


titleHook_base:
    lui   $t1, %hi(RAM_BASE)
    ori   $t1, reg_save
    sw    $ra, 0x00($t1)
    sw    $a0, 0x04($t1)
    sw    $a1, 0x08($t1)
    sw    $a2, 0x0C($t1)
    sw    $a3, 0x10($t1)

    jal   crash_titleHook
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
