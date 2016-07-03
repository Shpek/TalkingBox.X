#include "usart.h"

void USART_Init() {
    // RA0 - TX, RA1 - RX
    // Configure RX and TX portsas digital
    ANSELAbits.ANSA0 = 0;
    ANSELAbits.ANSA1 = 0;
    
    // Configure RX as inout and TX as output
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 1;
    
    // Configure baud rate
    TXSTAbits.BRGH = 1;
    SPBRGL = 25; // 9600
    
    // Enable interrupt on receive, peripheral 
    // interrupts, and global interrupts
    PIE1bits.RCIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;

    // Enable receive, transmit, and USART
    RCSTAbits.CREN = 1;
    TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
}

void USART_Putc(u8 c) {
    while (!TXSTAbits.TRMT);
    TXREG = c;
}

void USART_Puts(const u8 *s) {
    while (*s) {
        USART_Putc(*s);
        ++ s;
    }
}
