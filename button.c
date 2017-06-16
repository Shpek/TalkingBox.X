#include "button.h"

typedef struct {
    volatile u8 *pPort;
    u8 mask;
    u8 edge;
    u8 debounceCount;
    u8 state;
    u32 nextUpdateTime;
} InternalState;

static InternalState States[BUTTON_MAX_BUTTONS];
static u8 NextState = 0;

void ButtonInit(Button *pb, volatile u8 *pPort, u8 pin, u8 edge) {
    InternalState *ps = &States[NextState];
    ++ NextState;
    ps->pPort = pPort;
    ps->mask = 1 << pin;
    ps->edge = edge;
    ps->state = 1;
    ps->nextUpdateTime = 0;
    
    pb->state = Unknown;
    pb->stateChanged = 0;
    pb->stateChangeTime = 0;
    pb->pState = ps;
}

void ButtonReset(Button *pb) {
    InternalState *ps = &States[NextState];
    ps->state = 1;
    pb->state = Unknown;
    pb->stateChanged = 0;
    pb->stateChangeTime = 0;
}

u8 ButtonUpdate(Button *pb, u32 time) {
    InternalState *ps = (InternalState *) pb->pState;
    pb->stateChanged = 0;
    
    if (time < ps->nextUpdateTime) {
        return 0;
    }
    
    u8 switched = 0;

    switch (ps->state) {        
        case 1: { // Check state
            u8 pressed = !!(*ps->pPort & ps->mask) == ps->edge;
            
            u8 changedState = 
                    (pb->state == Pressed && !pressed) ||
                    (pb->state == Released && pressed) ||
                    pb->state == Unknown;
            
            if (changedState) {
                // Button pressed/released, start debounce
                ps->debounceCount = BUTTON_DEBOUNCE_COUNT;
                ps->state = 2;
                ps->nextUpdateTime = time + BUTTON_DEBOUNCE_TIME;
            }
        }
        break;
            
        case 2: { // Debounce state
            u8 pressed = !!(*ps->pPort & ps->mask) == ps->edge;

            if (pb->state != pressed || pb->state == Unknown) {
                -- ps->debounceCount;
                
                if (ps->debounceCount == 0) {
                    pb->state = (ButtonState) (pressed ? Pressed : Released);
                    pb->stateChanged = 1;
                    pb->stateChangeTime = time;
                    ps->state = 1;
                    switched = 1;
                } else {
                    ps->nextUpdateTime = time + BUTTON_DEBOUNCE_TIME;
                }
            } else {
                ps->state = 1;
            }
        }
        break;
    }
    
    return switched;
}
