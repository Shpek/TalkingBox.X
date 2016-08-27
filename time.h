#ifndef TIME_H
#define	TIME_H

#include "config.h"

// Milliseconds resolution timer. Uses TMR0 as soure. Works on 4MHz

void TimeInit(u32 *pTime, u8 initialValue, u8 period, u8 msPerPeriod);
void TimeUpdate(u32 *pTime);

#endif	/* TIME_H */

