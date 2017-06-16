#include "config.h"
#include "dfplayer.h"
#include "usart.h"
#include "button.h"
#include "time.h"
#include "swserial.h"
#include "talkingbox.h"

static u32 Time = 0;

interrupt void Interrupts() {
    SwSerialInterrupt();
    TimeUpdate(&Time);
    
    if (PIR1bits.RCIF) {
        mp3_on_byte_received(RCREG);
        PIR1bits.RCIF = 0;
    }
    
    if (IOCBFbits.IOCBF4) {
        IOCBFbits.IOCBF4 = 0;
    }
    
    if (IOCBFbits.IOCBF5) {
        IOCBFbits.IOCBF5 = 0;
    }
}

void main() {
    // Set the internal oscillator to 4 MHz
    OSCCON = 0b01101000;
    
    // Disable analog input
    ANSELA = 0;
    ANSELB = 0;
    
    // 7 segment indicator on RA
    TRISA = 0;
    
    // The mp3 player power is on RB0
    TRISB0 = 0;
    LATB0 = 0;
    
    // Mp3 player busy pin is on RB3
    TRISB3 = 1;

    // Enable interrupts
    PEIE = 1;
    GIE = 1;
    
    // Enable interrupt on falling edge on RB4 (play button) and RB5 (select button)
    IOCIE = 1;
    IOCBN4 = 1;
    IOCBN5 = 1;
    
    UsartInit();
    SwSerailInit(9600, -18);
    TimeInit(&Time, 6, 1, 2);
    
    Button b1, b2;
    ButtonInit(&b1, &PORTB, 4, 0);
    ButtonInit(&b2, &PORTB, 5, 0);  
    TalkingBoxInit(&b1, &b2);
    
//    while (1) {
//        SwSerialSend(0b01010101);
//        SwSerialSend(0b01010101);
//        SwSerialSendStr("Hello World!\n");
//        __delay_ms(500);
//    }
    
    while (1) {
        TalkingBoxUpdate(Time);
    }
}
