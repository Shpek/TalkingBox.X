#include "usart.h"

void UsartInit() {
    // RB2 - TX, RB1 - RX
    // Configure RX and TX ports as digital
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB1 = 0;
    
    // Configure RX as inout and TX as output
    TRISBbits.TRISB2 = 0;
    TRISBbits.TRISB1 = 1;
    
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

void UsartPutc(u8 c) {
    while (!TXSTAbits.TRMT);
    TXREG = c;
}

void UsartPuts(const u8 *s) {
    while (*s) {
        UsartPutc(*s);
        ++ s;
    }
}
