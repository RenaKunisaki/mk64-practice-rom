#include "../include/mk64/mk64.h"
extern "C" {

extern char* printHex(char *buf, u32 num, int nDigits);
extern char* printNum(char *buf, u32 num);

void debug_main_init() {
    //Called at boot once our code is loaded into RAM.

    //patch display list debug to show in corner of screen.
    //these addresses are part of the instructions that set the text position.
    u32 xpos[] = {0xA00016FA, 0xA00016CE, 0xA0001662, 0xA0001726, 0xA0001732};
    u32 ypos[] = {0xA0001706, 0xA00016DA, 0xA00016B2, 0xA0001752, 0xA0001736};
    for(int i=0; i<5; i++) {
        *(u16*)(xpos[i]) =   0;
        *(u16*)(ypos[i]) = 205;
    }

    //enable title screen debug menu by default
    // *(u16*)0xA00B3F76 = 2;

    screenMode = 0; //not sure why it keeps defaulting to 2p
    numPlayers = 1;
}

void doButton() {
    //called when the debug button is pressed. (either the button on the
    //6drive cartridge of the controller combo.)
    debugMode ^= 1;
    debugCoordDisplay = 1;
    debugResourceMeters = 1;
    debugMenuCursorPos = debugMode + 1;
}

void drawInputDisplay() {
    //static int xpos = 66, ypos = 32, w = 6, h = 6; //below lap counter
    static int xpos = 24, ypos = 1, w = 6, h = 6; //above top 4
    static const char *names[] = {
        "A", "B", "Z", "S",
        NULL, NULL, NULL, NULL, //don't need to draw d-pad
        NULL, NULL, //unused bits
        "L", "R",
        "U", "D", "L", "R" //C buttons
    };
    static const u8 colors[][3] = {
        {  0, 192, 255}, //A: blue
        {  0, 255,   0}, //B: green
        {255, 255, 255}, //Z: white
        {255,   0,   0}, //Start: red
        {128, 128, 128}, {128, 128, 128}, //up, down
        {128, 128, 128}, {128, 128, 128}, //left, right
        {  0,   0,   0}, {  0,   0,   0}, //unused
        {255, 255, 255}, {255, 255, 255}, //L, R
        {255, 255,   0}, {255, 255,   0}, //C up, down
        {255, 255,   0}, {255, 255,   0}  //C left, right
    };
    static const u8 coords[][2] = {
        {1, 2}, //A
        {0, 1}, //B
        {0, 2}, //Z
        {1, 0}, //start
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, //d-pad
        {0, 0}, {0, 0}, //unused
        {0, 0}, //L
        {3, 0}, //R
        {2, 0}, //C up
        {2, 2}, //C down
        {1, 1}, //C left
        {3, 1}  //C right
    };

    dlistBuffer = drawBox(dlistBuffer,
        xpos, ypos, xpos+w*4, ypos+h*3, //x1, y1, x2, y2
        0, 0, 0, 96); //r, g, b, a

    u16 buttons = player1_controllerState.buttons;
    u16 flag = 0x8000;
    for(int i=0; i<16; i++) {
        if(names[i]) {
            int r = colors[i][0], g = colors[i][1], b = colors[i][2];
            int a = (buttons & flag) ? 255 : 128;
            int x = xpos + (coords[i][0] * w);
            int y = ypos + (coords[i][1] * h);

            dlistBuffer = drawBox(dlistBuffer,
                x, y, x+w, y+h, //x1, y1, x2, y2
                r, g, b, a); //r, g, b, a

            //this is still glitchy.
            //the text disappears sometimes and the pause menu breaks.
            //textSetColor(TEXT_RED);
            //textDraw(x + 10, y + 8, names[i], 0, 0.5f, 0.5f);
        }
        flag >>= 1;
    }
    //XXX analog stick
}

void drawPlayerInfo(int which) {
    Player *p = &player[which];
    char text[64];

    static vec3f prevCoords;

    //draw coords
    #if 1
        char *buf = text;
        buf = printHex(buf, p->position.x, 4); *buf++ = ' ';
        buf = printHex(buf, p->position.y, 4); *buf++ = ' ';
        buf = printHex(buf, p->position.z, 4); *buf++ = 0;
        debugPrintStr(170, 205, text);
    #endif

    //draw speed (XXX don't update if paused)
    #if 1
        float dx = p->position.x - prevCoords.x;
        //float dy = p->position.y - prevCoords.y;
        float dz = p->position.z - prevCoords.z;
        float dxz = sqrtf((dx*dx) + (dz*dz));

        //6.4 seems about the right scale for units -> km/h
        buf = text;
        buf = printNum(buf, dxz * 6.4f); *buf++ = 0;
        debugPrintStr(250, 195, text);
    #endif

    //draw camera coords (will overlap speed)
    #if 0
        buf = text;
        buf = printHex(buf, player1_cameraPos.x, 4); *buf++ = ' ';
        buf = printHex(buf, player1_cameraPos.y, 4); *buf++ = ' ';
        buf = printHex(buf, player1_cameraPos.z, 4); *buf++ = 0;
        debugPrintStr(170, 195, text);
    #endif

    //draw race progress
    #if 1
        buf = text;
        buf = printHex(buf, player1_raceProgress, 4); *buf++ = 0;
        debugPrintStr(47, 13, text);
    #endif

    prevCoords.x = p->position.x;
    prevCoords.y = p->position.y;
    prevCoords.z = p->position.z;
}

void (*replaced)() = (void(*)())0x80093E20;
void debugHook() {
    //called every frame
    replaced();
    static int buttonCounter = 0;
    u16 buttons = player1_controllerState.buttons;
    u16 debugBtns = L_TRIG | Z_TRIG;

    if(debugMode) {
        drawInputDisplay();
        debugLoadFont();
        drawPlayerInfo(0);
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
