.set noreorder # don't rearrange branches
.include ".build/memory.s"

.text
debug_hooks_init:
    addiu $sp, -20
    sw    $ra, 20($sp)
    # hook the main game thread
    lw    $a0, jal_debugHook_base
    li    $a1, 0xA000289C # jal readControllers, runs every frame
    sw    $a0, ($a1)

    lw    $ra, 20($sp)
    j    debug_main_init
        addiu $sp, 20

jal_debugHook_base: jal debugHook_base

debugHook_base:
    addiu $sp, -20
    sw    $ra, 20($sp)
    jal   readControllers # replaced instruction
        nop
    lw    $ra, 20($sp)
    j     debugHook
        addiu $sp, 20
