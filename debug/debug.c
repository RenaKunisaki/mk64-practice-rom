#include "../include/mk64/mk64.h"
extern "C" {

extern char* printHex(char *buf, u32 num, int nDigits);

void debug_main_init() {
    //Called at boot once our code is loaded into RAM.

    //patch display list debug to show in corner of screen
    u32 xpos[] = {0xA00016FA, 0xA00016CE, 0xA0001662, 0xA0001726, 0xA0001732};
    u32 ypos[] = {0xA0001706, 0xA00016DA, 0xA00016B2, 0xA0001752, 0xA0001736};
    for(int i=0; i<5; i++) {
        *(u16*)(xpos[i]) =   0;
        *(u16*)(ypos[i]) = 205;
    }

    //enable title screen debug menu by default
     *(u16*)0xA00B3F76 = 2;

    screenMode = 0; //not sure why it keeps defaulting to 2p
    numPlayers = 1;
}

void doButton() {
    debugMode ^= 1;
    debugCoordDisplay = 1;
    debugResourceMeters = 1;
    debugMenuCursorPos = debugMode + 1;
}

void (*replaced)() = (void(*)())0x80093E20;
void debugHook() {
    //called every frame
    replaced();
    static int buttonCounter = 0;
    u16 buttons = player1_controllerState.buttons;
    u16 debugBtns = L_TRIG | Z_TRIG;

    if(debugMode) {
        //we can draw here...
        //dlistBuffer = drawBox(dlistBuffer,
        //        29, 29, 60, 40, //x1, y1, x2, y2
        //        128, 0, 0, 128); //r, g, b, a
        //textSetColor(TEXT_TRANS1);
        //textDraw(30, 30, "HELLO", 0, 1.0f, 1.0f);

        char text[64];
        char *buf = text;
        buf = printHex(buf, buttons, 4);
        *buf = 0;
        debugLoadFont();
        debugPrintStr(0, -16, text);
    }

    //if 64drive button pressed, or L+R+Z pressed, toggle debug mode
    if(/*(sdrv_isInit && sdrv_isButtonPressed())
    || */ (buttons & debugBtns) == debugBtns) {
        buttonCounter++; //debounce button
        if(buttonCounter == 4) doButton();
    }
    else buttonCounter = 0;
}

} //extern "C"
