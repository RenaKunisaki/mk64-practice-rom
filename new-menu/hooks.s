.set noreorder # don't rearrange branches
.include ".build/memory.s"

.text
title_menu_hooks_init:
    # hook to call our function
    lw    $t0, jal_titleHook
    li    $t1, 0x80094BD0 # jal titleScreenDraw
    sw    $t0, ($t1)

    lui   $t1, 0x800B
    ori   $t0, $zero, 0x1000
    sh    $t0, 0x2190($t1) # disable A/Start button at title
    sw    $t0, 0x829C($t1) # 800A7D64 - disable R button at title

    j     title_main_init
        nop # why sw doesn't work here? macro expands to multiple instructions?
        # but there's no reason sw should be a macro at all...

jal_titleHook: jal menu_titleHook
