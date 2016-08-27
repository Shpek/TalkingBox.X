#ifndef USART_H
#define UASRT_H

#include "config.h"

void UsartInit();

void UsartPutc(u8 c);

void UsartPuts(const u8 *s);

#endif	// USART_H

