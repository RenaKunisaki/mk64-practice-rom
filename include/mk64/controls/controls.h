typedef struct {
    s16 x, y;
    u16 buttons;
} controllerState;

extern controllerState controller1_state; //XXX others?
extern controllerState *controllers[4]; //XXX are all 4 here?

extern controllerState player1_controllerState;
