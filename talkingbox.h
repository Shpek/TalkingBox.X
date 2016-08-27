#ifndef TALKINGBOX_H
#define	TALKINGBOX_H

#include "config.h"
#include "button.h"

void TalkingBoxInit(Button *pPlay, Button *pSelect);
void TalkingBoxUpdate(u32 time);

#endif	/* TALKINGBOX_H */

