/* This patch adds a new menu at the title screen, from which you can quickly
 * set up the game parameters and start playing. It's similar to the existing
 * debug menu, but looks nicer and provides more features.
 */
#include "../include/mk64/mk64.h"
extern "C" {

void menu_titleHook();

void title_main_init() {
    //Called at boot once our code is loaded into RAM.
    PATCHJAL(0x80094BD0, menu_titleHook);
    PATCH16 (0x800B2190, 0x1000); //disable A/Start button at title
    PATCH16 (0x800A7D64, 0x1000); //disable R button at title
}


static int optSelected = 0; //currently selected menu option
static int fadeFrameCount = 0;
static const int fadeDuration = 30; //frames

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
    u8 dispModes; //bitmask: 01=GP 02=TT 04=VS 08=Battle
    const char *text;
    const char **names;
} options[] = {
    {0,  3, 0, 0xF, "Race Mode",   raceModes},
    {0,  3, 0, 0x1, "GP Cup",      cupNames},
    {0,  3, 0, 0x0, "GP Round",    NULL}, //XXX
    {0, 19, 0, 0xE, "Course",      courseNames},
    {0,  4, 0, 0xD, "Players",     playerNames},
    {0,  7, 0, 0xF, "Player 1",    characterNames},
    {0,  7, 1, 0xD, "Player 2",    characterNames},
    {0,  7, 2, 0xC, "Player 3",    characterNames},
    {0,  7, 3, 0xC, "Player 4",    characterNames},
    {0,  3, 2, 0xF, "Class",       classNames}, //default to 150cc
    {0,  1, 0, 0xF, "Mirror Mode", onOff},
    {0,  1, 1, 0x0, "Items",       onOff}, //XXX (patch item box function?)
    {0,  1, 1, 0x0, "Music",       onOff}, //XXX
    {0,  1, 0, 0xF, "Debug Mode",  onOff},
    //XXX other settings from that competition hack
    //L to reset/quit
    {0,  0, NULL, NULL, NULL}
};

enum {
    OPT_RACE_MODE = 0, //works
    OPT_GP_CUP,   //works
    OPT_GP_ROUND, //works but game doesn't init properly
    OPT_COURSE,   //only works in TT; pause screen shows Luigi Raceway
    OPT_PLAYERS,  //works
    OPT_DRIVER1,  //works
    OPT_DRIVER2,
    OPT_DRIVER3,
    OPT_DRIVER4,
    OPT_CLASS,  //works
    OPT_MIRROR, //works
    OPT_ITEMS,  //not implemented (need to patch item box loader)
    OPT_MUSIC,  //not implemented
    OPT_DEBUG,  //works
    NUM_OPTIONS
};

//#players options are 0=1p, 1=2p UD, 2=2p LR, 3=3p, 4=4p
static u8 gameModeMinPlayers[] = {0, 0, 1, 1}; //GP, TT, VS, Battle
static u8 gameModeMaxPlayers[] = {2, 0, 4, 4}; //GP, TT, VS, Battle


static void drawTitle() {
    //draw our nice new title at the top of the screen.
    static int xpos = 0, ypos = 0, width = 320, height = 240;
    dlistBuffer = drawBox(dlistBuffer,
        xpos, ypos, xpos+width, ypos+height, //x1, y1, x2, y2
        0, 0, 0, fadeFrameCount * (128.0f / fadeDuration)); //r, g, b, a

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
    u8 mode = 1 << options[OPT_RACE_MODE].value;

    //int x=30, y=48;
    int x=30, y=60;
    for(int i=0; options[i].text; i++) {
        if(options[i].dispModes & mode) {
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
}


static char* printNumText(char *buf, const char *s, int num) {
    buf = strAppend(buf, s);
    return printNum(buf, num);
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
    screenSplitMode       = screenMode;
    debugMenuScreenMode   = screenMode; //this might be the actual screen mode...
    //numPlayers2           = numPlayers;
    debugMenuNumPlayers   = numPlayers;
    raceType              = options[OPT_RACE_MODE].value;
    gpMode_currentCupDisp = options[OPT_GP_CUP   ].value; //shown at race start
    gpMode_currentCup     = options[OPT_GP_CUP   ].value; //actually selects cup
    gpMode_currentRound   = options[OPT_GP_ROUND ].value;
    curCourse             = options[OPT_COURSE   ].value;
    playerCharacter[0]    = options[OPT_DRIVER1  ].value;
    playerCharacter[1]    = options[OPT_DRIVER2  ].value;
    playerCharacter[2]    = options[OPT_DRIVER3  ].value;
    playerCharacter[3]    = options[OPT_DRIVER4  ].value;
    raceClass             = options[OPT_CLASS    ].value;
    isMirrorMode          = options[OPT_MIRROR   ].value;
    //XXX items, music
    debugMode             = options[OPT_DEBUG    ].value;
    debugCoordDisplay     = debugMode;
    debugResourceMeters   = debugMode;

    for(int i=0; i<numPlayers; i++) playerBalloons[i] = 3;

    char text[2048]; char *buf = text;
    buf = printNumText(buf, "Starting game\n * players    = ", numPlayers);
    buf = printNumText(buf, "\n * screenMode = ", screenMode);
    buf = printNumText(buf, "\n * raceType   = ", raceType);
    buf = printNumText(buf, "\n * cup        = ", gpMode_currentCup);
    buf = printNumText(buf, "\n * round      = ", gpMode_currentRound);
    buf = printNumText(buf, "\n * course     = ", curCourse);
    buf = printNumText(buf, "\n * chars      = ", playerCharacter[0]);
    buf = printNumText(buf, ",", playerCharacter[1]);
    buf = printNumText(buf, ",", playerCharacter[2]);
    buf = printNumText(buf, ",", playerCharacter[3]);
    buf = printNumText(buf, "\n * class      = ", raceClass);
    buf = printNumText(buf, "\n * mirror     = ", isMirrorMode);
    buf = printNumText(buf, "\n * debug      = ", debugMode);
    buf = strAppend(buf, "\n");
    sdrv_dprint(text);

    mainThreadTask = 4; //start game
}


static void doButtons() {
    //handle button presses
    /* ultra64 names
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
    u8 mode = 1 << options[OPT_RACE_MODE].value;

    if(buttons & Z_TRIG) {
        //(*(u32*)0xDEADBEEF) = 0xFFFFFFFF; //crash the game to test crash handler
        //asm volatile("syscall"); //for nemu
        debugMenuCursorPos = 2; //show the original debug menu for testing
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
        do {
            optSelected--;
            if(optSelected < 0) optSelected = NUM_OPTIONS - 1;
        } while(!(options[optSelected].dispModes & mode));
        soundPlay2(SND_CURSOR_MOVE);
    }
    if(buttons & D_JPAD) {
        do {
            optSelected++;
            if(optSelected >= NUM_OPTIONS) optSelected = 0;
        } while(!(options[optSelected].dispModes & mode));
        soundPlay2(SND_CURSOR_MOVE);
    }

    mode = options[OPT_RACE_MODE].value;
    if( options[OPT_PLAYERS].value < gameModeMinPlayers[mode])
        options[OPT_PLAYERS].value = gameModeMaxPlayers[mode];
    if( options[OPT_PLAYERS].value > gameModeMaxPlayers[mode])
        options[OPT_PLAYERS].value = gameModeMinPlayers[mode];

    if(buttons & (START_BUTTON | A_BUTTON)) startTheGame();
    prevButtons = curButtons;
}


void menu_titleHook() {
    //called when the title screen is being drawn.
    if(mainThreadTask != 0) {
        fadeFrameCount = 0;
        return; //don't draw if game is starting
    }
    if(debugMenuCursorPos > 1) { //original debug menu active
        fadeFrameCount = 0;
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

    if(fadeFrameCount < fadeDuration) fadeFrameCount++;
}



} //extern "C"
