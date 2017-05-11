extern struct {
    struct { u16 r, g, b; } top, bottom; //why uint16_t? no idea.
    //upper bytes seem to be used for something...
} skyColor[NUM_COURSES];

extern int screenMode;
extern u32* dlistBuffer; //ptr to current position in buffer
extern u16* frameBuffers[4]; //XXX verify size
extern s16 curDisplayList1; //XXX size, players
extern s16 curDisplayList2; //copied here
extern float viewPort; //XXX details

extern s16 blackSkyOnSomeCoursesIfZero; //XXX what is this?

/* 800DDEB0 array of 16bit wheel sizes */
