# This file contains the hook that loads the rest of our code into memory.
# It patches the video thread's entry point, copies some code from ROM to RAM,
# calls that, then returns to the video thread.
# This gets our code loaded into RAM early in the boot process, even before the
# game logic thread is running.
# From there, our loaded code can make other patches to call the functions
# we've loaded from various points in the game.

.set noreorder # don't rearrange branches

# the assembler doesn't know where our sections are going to be,
# so we need to define these here.
.equ RAM_BASE, 0x803F0000 # where our code is in RAM
.equ ROM_BASE, 0xB0BF0000 # where our code is in ROM
.equ rom_patchList, 0xB0BF4000 # where our list of patches begins
.equ PATCH_RAM_ADDR, 0x80400000 # where patches go in RAM

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


#.text
.section .boot, "awx", @progbits
loader:
    bootstrap:
        # Loaded into RAM and called by our patch at boot.

        lui   $t1, %hi(RAM_BASE)
        ori   $t1, reg_save
        sw    $ra, 0x00($t1)
        sw    $v0, 0x04($t1)
        sw    $v1, 0x08($t1)

        # zero BSS
        lui   $a1, %hi(RAM_BASE)
        ori   $a1, _sbss
        li    $a0, _lbss
        beq   $a0, $zero, end_bss$ # skip if no BSS
        add   $a0, $a0, $a1
        1:
            addiu $a1, 4
            bne   $a1, $a0, 1b
              sw    $zero, -4($a1)

        end_bss$:

        #jal hooks_init
        #  nop

        # load patches
        li    $t7, rom_patchList
        next_patch$:
            lw    $a0, 0x00($t7) # get size
            beq   $a0, $zero, end_patches$
            # delay slot doesn't matter
            lw    $a1, 0x04($t7) # get ROM addr
            lw    $a2, 0x08($t7) # get RAM addr
            lw    $a3, 0x0C($t7) # get entry point

            jal   load_patch
                sw $t7, 0x10($t1) # save T7

            # restore T1 in case it was clobbered
            # XXX use the stack instead...
            lui   $t1, %hi(RAM_BASE)
            ori   $t1, reg_save
            lw    $t7, 0x10($t1) # restore T7
            j     next_patch$
                addiu $t7, 0x10      # to next entry

        end_patches$: # end of load-patch loop

        lui   $t1, %hi(RAM_BASE)
        ori   $t1, reg_save
        lw    $ra, 0x00($t1)
        lw    $v0, 0x04($t1)
        lw    $v1, 0x08($t1)

    original: # repeat the code we replaced from 0x80002444-2488
        # original code actually loops back here, re-clearing the registers
        # on every iteration of the loop. this is silly, so let's not do that.
        addiu $t2, $zero, 0x0000
        addiu $t3, $zero, 0x0000
        addiu $t4, $zero, 0x0000
        addiu $t5, $zero, 0x0000
        addiu $t6, $zero, 0x0000
        addiu $t7, $zero, 0x0000
        addiu $t0, $zero, 0x0000
        addiu $t1, $zero, 0x0000
        # instead, we'll jump back to here to save a bit of time.
        # we could do even better and just use $zero, but it's best to leave
        # things as close to the game's expected state as possible.
        # MIPS ABI says nothing *should* depend on these registers being set
        # after a function call, but you never know.
    loop$:
        addiu $v0, $v0,   0x0020
        sw    $t7, -4 ($v0)
        sw    $t6, -8 ($v0)
        sw    $t5, -12 ($v0)
        sw    $t4, -16 ($v0)
        sw    $t3, -20 ($v0)
        sw    $t2, -24 ($v0)
        sw    $t1, -28 ($v0)
        bne   $v0, $v1, loop$
          sw    $t0, -32 ($v0)

        # and back to the game code.
        j thread3_main_return
          nop


    reg_save:
        .word 0, 0, 0, 0, 0, 0, 0, 0

    next_free:
        .word PATCH_RAM_ADDR


    load_patch:
        # a0=size a1=romaddr a2=ramaddr a3=entry

        addiu $t1, $zero, 0 # set flags to 0
        addiu $t0, $a3, 1
        beql  $t0, $zero, noentry$ # if entry == 0xFFFFFFFF,
            addiu $t1, $zero, 1    # set flags to 1
        noentry$:

        # if dest is zero, use next free addr
        bne   $a2, $zero, checksrc$
            nop

            lw    $a2, (next_free)
            add   $t0, $a2, $a0
            sw    $t0, (next_free)

        checksrc$:
        # if source is zero, clear dest
        add   $a3, $a2 # get absolute entry point
        # use uncached destination so that it executes correctly.
        # otherwise code is stuck in dcache and not fetched.
        lui $t2, 0x2000
        beq   $a1, $zero, clear$
            or  $a2, $a2, $t2

        # copy source to dest
        1:  # loop until all data is copied.
            lw    $t0, ($a1)
            sw    $t0, ($a2)
            addiu $a0,  -4
            addiu $a1,  4
            bgezl  $a0, 1b
                addiu $a2, 4

        # if entry point is valid, jump to it
        done$:
            beq   $t1, $zero, doentry$
                nop
            jr    $ra # no entry point
                nop

        doentry$:
            jr    $a3 # branch to entry
                nop

        clear$: # no source, just clear destination
            sw    $zero, ($a2)
            addiu $a0,  -4
            bgezl  $a0, clear$
                addiu $a2, 4
            j     done$
                nop




    .align 4 # loader_size must be multiple of 4


# This is the code that actually replaces thread3_main in the ROM.
# Since that begins with a needlessly large memory-clear loop, we have lots of
# room to load and call our new code from here.
# Execution begins at RAM address 0x80002434.
.section .bootstrap, "awx", @progbits # 80002434
thread3_main:
    # these 4 instructions are the same as the original game.
    # I'll leave them here for clarity's sake.
    sw    $s0, 0x001C ($sp)          # 2434
    sw    $a0, 0x0060 ($sp)          # 2438
    addiu $v0, $v0, 0x4F80           # 243C
    addiu $v1, $v1, 0xA780           # 2440

    # read our new code into RAM.
    # we could use DMA here, but the PI manager isn't ready yet.
    # anyway, it's only at boot, so taking a few extra microseconds
    # is no big deal.
    lui   $a0, %hi(BOOTSTRAP_READ_ADDR) # 2444
    ori   $a0, %lo(BOOTSTRAP_READ_ADDR) # 2448
    li    $a2, _lboot                # 244C
    add   $a0, $a0, $a2              # 2450
    li    $a1, RAM_BASE | 0xA0000000 # 2454  use uncached address
    add   $a1, $a1, $a2              # 2458

    1:  # loop until all data is copied.
        lw    $a3, ($a0)             # 245C
        sw    $a3, ($a1)             # 2460
        addiu $a0,  $a0, -4          # 2464
        addiu $a2,  $a2, -4          # 2468
        bgezl  $a2, 1b               # 246C
          addiu $a1, $a1, -4         # 2470

    j bootstrap                      # 2474
      nop                            # 2478

    # pad out the rest of the patched area
    nop                              # 247C
    nop                              # 2480
    nop                              # 2484
    nop                              # 2488

    thread3_main_return:             # 248C
