#include "../include/mk64/mk64.h"
extern "C" {

extern char* strAppend(char *dest, const char *src);
extern char* printHex(char *buf, u32 num, int nDigits);
extern char* printNum(char *buf, u32 num);

void title_main_init() {
    //Called at boot once our code is loaded into RAM.
}


static int optSelected = 0; //currently selected menu option

static const char *onOff[] = {"Off", "On"};
static const char *raceModes[] = {"Mario GP", "Time Trial", "VS", "Battle"};
static const char *cupNames[] = {"Mushroom", "Flower", "Star", "Special"};
static const char *courseNames[] = {
    "Mario Raceway",   "Choco Mountain",  "Bowser's Castle",
    "Banshee Brdwalk", "Yoshi Valley",    "Frappe Snowland",
    "Koopa Beach",     "Royal Raceway",   "Luigi Raceway",
    "Moo Moo Farm",    "Toad's Turnpike", "Kalimari Desert",
    "Sherbet Land",    "Rainbow Road",    "Wario Stadium",
    "Block Fort",      "Skyscraper",      "Double Deck",
    "DK Jungle",       "Big Donut",
};
static const char *playerNames[] = {"1", "2 Up-Down", "2 Left-Right", "3", "4"};
static const char *characterNames[] = {
    "Mario", "Luigi", "Yoshi", "Toad", "DK", "Wario", "Peach", "Bowser"
};
static const char *classNames[] = {"50(", "100(", "150(", "Extra"};// '(' = "cc"

static struct {
    int min, max;
    int value;
    const char *text;
    const char **names;
} options[] = {
    {0,  3, 0, "Race Mode",   raceModes},
    {0,  3, 0, "GP Cup",      cupNames},
    {0, 19, 0, "Course",      courseNames},
    {0,  4, 0, "Players",     playerNames}, //XXX 3-4p crashes
    {0,  7, 0, "Player 1",    characterNames},
    {0,  7, 1, "Player 2",    characterNames},
    {0,  7, 2, "Player 3",    characterNames},
    {0,  7, 3, "Player 4",    characterNames},
    {0,  3, 2, "Class",       classNames}, //default to 150cc
    {0,  1, 0, "Mirror Mode", onOff},
    {0,  1, 1, "Items",       onOff}, //XXX (patch item box function?)
    {0,  1, 1, "Music",       onOff}, //XXX
    {0,  1, 0, "Debug Mode",  onOff},
    //XXX other settings from that competition hack
    //L to reset/quit
    {0,  0, NULL, NULL, NULL}
};

enum {
    OPT_RACE_MODE = 0, //works
    OPT_GP_CUP,
    OPT_COURSE, //only works in TT; pause screen shows Luigi Raceway
    OPT_PLAYERS, //works but 3p/4p crashes
    OPT_DRIVER1, //works
    OPT_DRIVER2,
    OPT_DRIVER3,
    OPT_DRIVER4,
    OPT_CLASS, //works
    OPT_MIRROR, //works
    OPT_ITEMS, //not implemented (need to patch item box loader)
    OPT_MUSIC, //not implemented
    OPT_DEBUG, //works
    NUM_OPTIONS
};


static void drawTitle() {
    //draw our nice new title at the top of the screen.
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


static void drawMenu() {
    //draw the actual menu
    //int x=30, y=48;
    int x=30, y=60;
    for(int i=0; options[i].text; i++) {
        textSetColor(i == optSelected ? TEXT_TRANS2 : TEXT_GREEN);
        textDraw(x, y, options[i].text, 1, 0.8f, 0.8f);

        int val = options[i].value;
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
}


static void startTheGame() {
    //start the game using our current settings
    soundPlay2(SND_MENU_ACCEPT);

    //set numPlayers and screenMode
    switch(options[OPT_PLAYERS].value) {
        case 0: numPlayers = 1; screenMode = SCREEN_MODE_1P;      break;
        case 1: numPlayers = 2; screenMode = SCREEN_MODE_2P_HORZ; break;
        case 2: numPlayers = 2; screenMode = SCREEN_MODE_2P_VERT; break;
        case 3: numPlayers = 3; screenMode = SCREEN_MODE_4P;      break;
        case 4: numPlayers = 4; screenMode = SCREEN_MODE_4P;      break;
    }

    //set other game parameters
    //screenSplitMode     = screenMode;
    raceType            = options[OPT_RACE_MODE].value;
    gpMode_currentCup   = options[OPT_GP_CUP   ].value; //shown at race start
    gpMode_currentCup2  = options[OPT_GP_CUP   ].value; //actually selects cup
    gpMode_currentRound = 0;
    curCourse           = options[OPT_COURSE   ].value;
    playerCharacter[0]  = options[OPT_DRIVER1  ].value;
    playerCharacter[1]  = options[OPT_DRIVER2  ].value;
    playerCharacter[2]  = options[OPT_DRIVER3  ].value;
    playerCharacter[3]  = options[OPT_DRIVER4  ].value;
    raceClass           = options[OPT_CLASS    ].value;
    isMirrorMode        = options[OPT_MIRROR   ].value;
    //XXX items, music
    debugMode           = options[OPT_DEBUG    ].value;
    debugCoordDisplay   = debugMode;
    debugResourceMeters = debugMode;

    //gpMode_currentCup   = 2;
    //gpMode_currentRound = 2;

    mainThreadTask      = 4; //start game
}


static void doButtons() {
    //handle button presses
    /* libultra names
     * 8000 A_BUTTON      0080 unused
     * 4000 B_BUTTON      0040 unused
     * 2000 Z_TRIG        0020 L_TRIG
     * 1000 START_BUTTON  0010 R_TRIG
     * 0800 U_JPAD        0008 U_CBUTTONS
     * 0400 D_JPAD        0004 D_CBUTTONS
     * 0200 L_JPAD        0002 L_CBUTTONS
     * 0100 R_JPAD        0001 R_CBUTTONS
     */
    static u16 prevButtons = 0;
    u16 curButtons = player1_controllerState.buttons;
    u16 buttons = curButtons & ~prevButtons;

    if(buttons & Z_TRIG) {
        //(*(u32*)0xDEADBEEF) = 0xFFFFFFFF; //crash the game to test crash handler
        //asm volatile("syscall"); //for nemu
        debugMenuCursorPos = 2;
    }
    if(buttons & L_JPAD) {
        if(--options[optSelected].value < options[optSelected].min)
            options[optSelected].value = options[optSelected].max;
        soundPlay2(SND_CURSOR_MOVE);
    }
    if(buttons & R_JPAD) {
        if(++options[optSelected].value > options[optSelected].max)
            options[optSelected].value = options[optSelected].min;
        soundPlay2(SND_CURSOR_MOVE);
    }
    if(buttons & U_JPAD) {
        optSelected--;
        if(optSelected < 0) optSelected = NUM_OPTIONS - 1;
        soundPlay2(SND_CURSOR_MOVE);
    }
    if(buttons & D_JPAD) {
        optSelected++;
        if(optSelected >= NUM_OPTIONS) optSelected = 0;
        soundPlay2(SND_CURSOR_MOVE);
    }
    if(buttons & START_BUTTON) startTheGame();

    prevButtons = curButtons;
}


void menu_titleHook() {
    //called when the title screen is being drawn.
    if(mainThreadTask != 0) return; //don't draw if game is starting
    if(debugMenuCursorPos > 1) { //original debug menu active
        titleScreenDraw();
        return;
    }

    //HACK: disable waving flag because it prevents our text from appearing.
    (*(u32*)0x8018DA30) = 0;

    //disable flashing "press start" text
    (*(u32*)0x8018DA58) = 0;

    //prevent demo from starting
    //let it reach at least 4 to play the "welcome to Mario Kart" sound
    if(titleDemoCounter > 4) titleDemoCounter = 4;

    titleScreenDraw(); //draw the original title screen
    drawTitle(); //draw our menu title
    drawMenu(); //draw the menu
    //debugLoadFont();
    //debugPrintStr(10, 10, "butts");
    doButtons(); //handle buttons
}



} //extern "C"
