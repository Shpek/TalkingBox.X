#ifndef BUTTON_H
#define	BUTTON_H

#include "config.h"

#define BUTTON_MAX_BUTTONS 5
#define BUTTON_DEBOUNCE_COUNT 5
#define BUTTON_DEBOUNCE_TIME 10

typedef enum  {
    Unknown = 0,
    Pressed = 1,
    Released = 2,
} ButtonState;

typedef struct {
    ButtonState state;
    u8 stateChanged;
    u32 stateChangeTime;
    void *pState;
} Button;

void ButtonInit(Button *pb, volatile u8 *pPort, u8 pin, u8 buttonEdge);
void ButtonReset(Button *pb);
u8 ButtonUpdate(Button *pb, u32 time);

#endif	/* BUTTON_H */
