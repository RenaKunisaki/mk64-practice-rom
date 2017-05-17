typedef enum {
    SONG_NONE = 0,
    SONG_TITLE_SCREEN,
    SONG_MENU,
    SONG_RACEWAY,
    SONG_FARM,
    SONG_CHOCO,
    SONG_BEACH,
    SONG_BOARDWALK,
    SONG_SNOW,
    SONG_BOWSER_CASTLE,
    SONG_DESERT,
    SONG_GP_START,
    SONG_FINAL_LAP,
    SONG_FIRST_PLACE,
    SONG_SECOND_PLACE,
    SONG_YOU_LOSE,
    SONG_RACE_RESULTS,
    SONG_STAR_POWER,
    SONG_RAINBOW_ROAD,
    SONG_DK_JUNGLE,
    SONG_UNKNOWN,
    SONG_TURNPIKE,
    SONG_VS_START,
    SONG_VS_RESULTS,
    SONG_RETRY_QUIT,
    SONG_BIG_DONUT,
    SONG_TROPHY1,
    SONG_TROPHY2,
    SONG_CREDITS,
    SONG_TROPHY_LOSE,
    NUM_SONGS
} SongID;

enum { //XXX figure out how these work
    SOUND_MARIO_THROW_SHELL  = 0x29008000,
    SND_DK_VOICE             = 0x2900804D,

    //Various sound effects, mostly in menus
    SND_CURSOR_MOVE          = 0x49008000, //cursor moving in menus
    SND_MENU_OPEN            = 0x49008001,
    SND_MENU_CANCEL          = 0x49008002,
    SND_MENU_ACCEPT          = 0x4900801A, //pressing Start at title, menus
    SND_MUSIC_ADJUST         = 0x4900801C, //pressing L to change volume
    SND_EXPLOSION            = 0x4900801D, //erase save data

    //Mario voice clips
    SND_WELCOME_TO_MARIOKART = 0x49009009,
    SND_MARIO_GP             = 0x4900900A,
    SND_TIME_TRIAL           = 0x4900900B,
    SND_VERSUS               = 0x4900900C,
    SND_BATTLE               = 0x4900900D,
    SND_SELECT_A_LEVEL       = 0x4900900E,
    SND_OK                   = 0x4900900F, //"OK?"
    SND_MENU_OPTION          = 0x49009010, //"option" in Mario voice
    SND_MENU_DATA            = 0x49009011, //"data" in Mario voice
    SND_SELECT_YOUR_PLAYER   = 0x49009012,
    SND_SELECT_MAP           = 0x49009013,

    SND_NINTENDO_LOGO        = 0x49018008,
} SoundID;

/* 803B03CB lock at 0 to disable race start sounds */
extern s8 musicSpeed;
extern s8 soundMode;

void soundPlay(u32 soundID, float* A1, u32 A2, float* A3, float* sp10, float *sp14);
void soundPlay2(u32 soundID);
