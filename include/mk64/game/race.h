extern u8  vsWins[2]; //win count player 1, 2
extern u8  battleWins[2]; //win count player 1, 2
extern s16 playerBalloons[MAX_PLAYERS]; //battle mode

extern u8  gpRankPoints[4]; //points for 1st, 2nd, 3rd, 4th

extern int raceType; //0=GP 1=Time Trial 2=VS 3=Battle
extern int raceClass; //0=50cc 1=100cc 2=150cc 3=Extra
extern s16 curCourse;
extern s16 gpMode_currentCup;
extern int gpMode_currentCupDisp;
extern s16 gpMode_currentRound; //0 to 3; also cup selected in menu
extern s16 isMirrorMode;

//800067C4 set places
extern int player1_raceProgress; //counts up as you progress, down if backward
extern int playerLap[NUM_RACERS];
extern int player1_displayedPosition;
extern s8  characterPlace[NUM_RACERS]; //which character is 1st, 2nd...
extern s8  player1_displayedPosition2;

extern float gameTimer;
extern s16   isRaceTimerRunning;
