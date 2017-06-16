#ifndef SWSERIAL_H
#define	SWSERIAL_H

#include "config.h" 

void SwSerailInit(i32 baudrate, i8 timerCorrection);
void SwSerialInterrupt();
void SwSerialSend(u8 data);
void SwSerialSendStr(char *str);

#endif	/* SWSERIAL_H */
