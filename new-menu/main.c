#include "../include/mk64/mk64.h"
extern "C" {

extern char* strAppend(char *dest, const char *src);
extern char* printHex(char *buf, u32 num, int nDigits);
extern char* printNum(char *buf, u32 num);
extern int raceType, curCourse, numPlayers, screenMode, isHudEnabled, mainThreadTask;

void title_main_init() {
}


static int optSelected = 0;
static int optPlayers = 0;
static int optCharacter = 0;
static int optMirror = 0;
static int optDebug = 0;

static const char *onOff[] = {"Off", "On"};
static const char *raceModes[] = {"Mario GP", "Time Trial", "VS", "Battle"};
static const char *courseNames[] = {
    "Mario Raceway", "Choco Mountain", "Bowser's Castle",
    "Banshee Boardwalk", "Yoshi Valley", "Frappe Snowland",
    "Koopa Beach", "Royal Raceway", "Luigi Raceway",
    "Moo Moo Farm", "Toad's Turnpike", "Kalimari Desert",
    "Sherbet Land", "Rainbow Road", "Wario Stadium",
    "Block Fort", "Skyscraper", "Double Deck",
    "DK Jungle", "Big Donut",
};
static const char *playerNames[] = {"1", "2 Up-Down", "2 Left-Right", "3", "4"};
static const char *characterNames[] = {
    "Mario", "1", "2", "3", "4", "5", "6", "7"
};
static const char *classNames[] = {"50(", "100(", "150("};

static struct {
    int min, max;
    int *value;
    const char *text;
    const char **names;
} options[] = {
    {0,  3, &raceType, "Race Mode", raceModes},
    {0, 19, &curCourse, "Course", courseNames}, //XXX doesn't work
    {0,  4, &optPlayers, "Players", playerNames}, //XXX 3-4p crashes
    {0,  7, &optCharacter, "Driver", characterNames}, //XXX doesn't work
    {0,  1, &raceType, "Class", classNames}, //XXX
    {0,  1, &optMirror, "Mirror Mode", onOff},
    {0,  1, &optDebug, "Items", onOff}, //XXX (patch item box function?)
    {0,  1, &optDebug, "Music", onOff}, //XXX
    {0,  1, &optDebug, "Debug Mode", onOff},
    //XXX other settings from that competition hack
    //L to reset/quit
    {0,  0, NULL, NULL, NULL}
};


static void drawTitle() {
    static int xpos = 0, ypos = 0, width = 320, height = 240;
    dlistBuffer = drawBox(dlistBuffer,
        xpos, ypos, xpos+width, ypos+height, //x1, y1, x2, y2
        0, 0, 0, 128); //r, g, b, a

    //reset the opacity for text
    dlistBuffer = drawBox(dlistBuffer,
        0, 0, 1, 1, //x1, y1, x2, y2
        0, 0, 0, 255); //r, g, b, a

    //dlistBuffer = f_8009BC9C(dlistBuffer, 0x02004E80, 0, 0, 0, 32);
    textSetColor(TEXT_TRANS1);
    textDraw(90, 20, "Mario Kart 64", 0, 1.0f, 1.0f);

    textSetColor(TEXT_YELLOW);
    textDraw(110, 29, "Practice ROM v1.0", 0, 0.5f, 0.5f);
}

void title_menu_draw() {
    drawTitle();

    optCharacter = player1_character; //needs to be int, not s16
    optMirror    = isMirrorMode;
    optDebug     = debugMode;

    //int x=30, y=48;
    int x=30, y=100;
    for(int i=0; options[i].value; i++) {
        textSetColor(i == optSelected ? TEXT_TRANS2 : TEXT_GREEN);
        textDraw(x, y, options[i].text, 1, 0.8f, 0.8f);

        int val = *options[i].value;
        if(options[i].names) {
            textDraw(x+130, y, options[i].names[val], 1, 0.8f, 0.8f);
        }
        else {
            char text[64];
            printNum(text, val);
            textDraw(x+130, y, text, 1, 0.8f, 0.8f);
        }

        y += 13;
    }

    //debugLoadFont();
    //debugPrintStr(10, 10, "butts");

    static u16 prevButtons = 0;
    u16 curButtons = player1_controllerState.buttons;
    u16 buttons = curButtons & ~prevButtons;

    if(buttons & L_JPAD) {
        int val = *options[optSelected].value;
        if(--val < options[optSelected].min)
            val = options[optSelected].max;
        *options[optSelected].value = val;
    }
    if(buttons & R_JPAD) {
        int val = *options[optSelected].value;
        if(++val > options[optSelected].max)
            val = options[optSelected].min;
        *options[optSelected].value = val;
    }
    if(buttons & U_JPAD) {
        optSelected--;
        if(optSelected < 0) {
            while(options[optSelected+1].value) optSelected++;
        }
    }
    if(buttons & D_JPAD) {
        optSelected++;
        if(!options[optSelected].value) optSelected = 0;
    }

    player1_character   = optCharacter;
    isMirrorMode        = optMirror;
    debugMode           = optDebug;
    debugCoordDisplay   = optDebug;
    debugResourceMeters = optDebug;

    if(buttons & START_BUTTON) {
        switch(optPlayers) {
            case 0: numPlayers = 1; screenMode = 0; break; //1p
            case 1: numPlayers = 2; screenMode = 1; break; //2p up/down
            case 2: numPlayers = 2; screenMode = 2; break; //2p left/right
            case 3: numPlayers = 3; screenMode = 3; break; //3p
            case 4: numPlayers = 4; screenMode = 3; break; //4p
        }
        mainThreadTask = 4; //start game
    }

    prevButtons = curButtons;
}

/*
8000 A_BUTTON      0080 unused
4000 B_BUTTON      0040 unused
2000 Z_TRIG        0020 L_TRIG
1000 START_BUTTON  0010 R_TRIG
0800 U_JPAD        0008 U_CBUTTONS
0400 D_JPAD        0004 D_CBUTTONS
0200 L_JPAD        0002 L_CBUTTONS
0100 R_JPAD        0001 R_CBUTTONS
*/


void menu_titleHook() {
    //HACK: disable waving flag because it prevents our text from appearing.
    (*(u32*)0x8018DA30) = 0;

    //disable flashing "press start" text
    (*(u32*)0x8018DA58) = 0;

    titleScreenDraw();
    title_menu_draw();
    titleDemoCounter = -999999999;
}



} //extern "C"
