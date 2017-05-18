typedef struct {
    s16 x, y;
    u16 buttons;
} controllerState;

extern void _InitControllers();
extern void readControllers();
extern controllerState controller1_state; //XXX others?
extern controllerState *controllers[4]; //XXX are all 4 here?

extern controllerState player1_controllerState;
