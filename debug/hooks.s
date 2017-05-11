.set noreorder # don't rearrange branches
.include ".build/memory.s"

.text
debug_hooks_init:
    addiu $sp, -0x18
    sw    $ra, 0x14($sp)

    # hook the main game thread
    lw    $a0, jal_debugHook

    # this runs during gameplay and title screen,
    # but we can only draw on title screen.
    #li    $a1, 0xA00028A4 # jal 0x80001ECC in game thread

    # this one runs only during gameplay.
    # it's before the HUD is drawn, so that will overlap our drawings.
    #li    $a1, 0xA0001608 # jal 0x802A59A4 in game thread

    # this one runs after drawing the HUD. it's responsible for drawing
    # the pause screen, but runs even when not paused.
    li    $a1, 0xA0001E74 # jal 0x80093E20 in game thread

    # it's important to hook a good place here. otherwise we can end up in a
    # state where the display list buffer isn't set up yet, and the game will
    # hang or crash when we try to draw anything.

    sw    $a0, ($a1)
    lw    $ra, 0x14($sp)
    j    debug_main_init
        addiu $sp, 0x18

jal_debugHook: jal debugHook
