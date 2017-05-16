extern struct {
    struct { u16 r, g, b; } top, bottom; //why uint16_t? no idea.
    //upper bytes seem to be used for something...
} skyColor[NUM_COURSES];

enum {
    SCREEN_MODE_1P,
    SCREEN_MODE_2P_HORZ, //normal 2P mode
    SCREEN_MODE_2P_VERT, //unused vertical 2P mode
    SCREEN_MODE_4P //also used for 3P, the 4th part shows map
} ScreenModes;

extern int screenMode, screenSplitMode;
extern u32* dlistBuffer; //ptr to current position in buffer
extern u16* frameBuffers[4]; //XXX verify size
extern s16 curDisplayList1; //XXX size, players
extern s16 curDisplayList2; //copied here

extern s16 blackSkyOnSomeCoursesIfZero; //XXX what is this?

/* 800DDEB0 array of 16bit wheel sizes */
