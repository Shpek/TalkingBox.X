#include "swserial.h"

static u8 TimerValue = 0;
static u8 SendingState = 0;
static u8 SendBuffer = 0;
static u8 SendBitsRemaining = 0;

#define SwSerialTxPin LATBbits.LATB7 // Output Tx  
#define SwSerialRxPin PORTBbits.RB6	// Input  Rx 

void SwSerailInit(i32 baudRate, i8 timerCorrection)
{
    TRISB6 = 1;
    TRISB7 = 0;
    TMR2ON = 1;
    TimerValue = (u8) (((_XTAL_FREQ / 4) / baudRate) + timerCorrection);
    SwSerialTxPin = 1;
    TMR2 = 0;
    PR2 = TimerValue;
    TMR2IE = 1;
}

void SwSerialInterrupt()
{
    if (TMR2IF) {
        TMR2 = 0;
        PR2 = TimerValue;
        TMR2IF = 0;

        switch (SendingState) {
            case 0: break; // Idle
            
            case 1: // Send start
                SwSerialTxPin = 0;
                SendingState = 2;
                break;
                
            case 2: // Send bit
                SwSerialTxPin = (u8) (SendBuffer & 1);
                SendBuffer >>= 1;
                -- SendBitsRemaining;
                
                if (SendBitsRemaining == 0) {
                    SendingState = 3;
                }
                break;
                
            case 3: // Send stop bit
                SwSerialTxPin = 1;
                SendingState = 0;
                break;
        }
    }
}

void SwSerialSend(u8 data)
{
    while (SendingState != 0) {
        // Wait for another send operation
    }
    
    SendBuffer = data;
    SendBitsRemaining = 8;
    SendingState = 1;
}

void SwSerialSendStr(char *str)
{
    char *c = str;
    
    while (*c) {
        SwSerialSend(*c);
        ++ c;
    }
}
