#include "time.h"

static u8 InitialValue = 0;
static u8 Period = 0;
static u8 MsPerPeriod = 0;

void TimeInit(u32 *pTime, u8 initialValue, u8 period, u8 msPerPeriod) {
    InitialValue = initialValue;
    Period = period;
    MsPerPeriod = msPerPeriod;
    TMR0 = initialValue;
    TMR0CS = 0;
    PSA = 1;
    TMR0IE = 1;
    *pTime = 0;
}

void TimeUpdate(u32 *pTime) {
    static u32 timerTime = 0;
    
    if (INTCONbits.TMR0IF) {
        TMR0 = InitialValue;
        ++ timerTime;
        
        if (timerTime == Period) {
            *pTime += MsPerPeriod;
            timerTime = 0;
        }
        
        INTCONbits.TMR0IF = 0;
    }
}
