#ifndef USART_H
#define UASRT_H

#include "config.h"

void USART_Init();

void USART_Putc(u8 c);

void USART_Puts(const u8 *s);

#endif	// USART_H

