#include "config.h"
#include "dfplayer.h"
#include "usart.h"
#include "button.h"
#include "stdlib.h"
#include "time.h"
#include "talkingbox.h"

#define NUM_FILES_IDX       0
#define LAST_PLAYED_IDX     1
#define RNG_SEED_IDX        2
#define RANDOM_FILES_START  3
#define ARRLEN(arr) (sizeof(arr)/sizeof(0[arr]))

static u8 InhibitButtonPress = 0;
static u8 ButtonPressed = 0;
static u8 TimerTime = 0;
static u8 Timers[] = {0, 0}; // These will increment each second

static u32 Time = 0;

static u8 LedValues[] = {
    0b01011111, // 0
    0b00000110, // 1
    0b10011011, // 2 
    0b10001111, // 3
    0b11000110, // 4
    0b11001101, // 5
    0b11011101, // 6
    0b00000111, // 7
    0b11011111, // 8
    0b11001111, // 9
};

interrupt void Interrupts() {
    TimeUpdate(&Time);
        
    if (PIR1bits.RCIF) {
        mp3_on_byte_received(RCREG);
        PIR1bits.RCIF = 0;
    }
    
    if (IOCBFbits.IOCBF4) {
        if (!InhibitButtonPress) {
            ButtonPressed = 1;
        }
        
        IOCBFbits.IOCBF4 = 0;
    }
    
//    if (INTCONbits.TMR0IF) {
//        ++ TimerTime;
//        
//        if (TimerTime == 8) {
//            for (u8 i = 0; i < ARRLEN(Timers); ++ i) {
//                ++ Timers[i];
//            }
//            
//            TimerTime = 0;
//        }
//        
//        INTCONbits.TMR0IF = 0;
//    } 
}

void RandomizeFiles(u8 numFiles) {
    u8 seed = eeprom_read(RNG_SEED_IDX);
    seed = seed - 1;
    eeprom_write(RNG_SEED_IDX, seed);
    srand(seed);
    
    for (u8 i = 0; i < numFiles; ++ i) {
        eeprom_write(RANDOM_FILES_START + i, i + 1);
    }
    
    for (u8 i = numFiles - 1; i >= 1; -- i) {
        u8 j = rand() % (i + 1);
        u8 el1 = eeprom_read(RANDOM_FILES_START + i);
        u8 el2 = eeprom_read(RANDOM_FILES_START + j);
        eeprom_write(RANDOM_FILES_START + i, el2);
        eeprom_write(RANDOM_FILES_START + j, el1);
    }
}

u8 GetNextFileToPlay(u8 numFiles) {
    u8 storedNumFiles = eeprom_read(NUM_FILES_IDX);
    
    if (storedNumFiles == numFiles) {
        u8 nextFile = eeprom_read(LAST_PLAYED_IDX) + 1;
        
        if (nextFile == numFiles) {
            RandomizeFiles(numFiles);
            nextFile = 0;
        }
        
        eeprom_write(LAST_PLAYED_IDX, nextFile);
        return eeprom_read(RANDOM_FILES_START + nextFile);
    } else {
        RandomizeFiles(numFiles);
        eeprom_write(NUM_FILES_IDX, numFiles);
        eeprom_write(LAST_PLAYED_IDX, 0);
        return eeprom_read(RANDOM_FILES_START);
    }
}

static u8 DebounceButton() {
    for (u8 i = 0; i < 3; ++ i) {
        __delay_ms(10);
        
        if (PORTBbits.RB4 != 0) {
            return 0;
        }
    }
    
    return 1;
}

void test() {
    // Set the internal oscillator to 4 MHz
    OSCCON = 0b01101000;
    
    // The mp3 player power is on RB0
    TRISB0 = 0;
    LATB0 = 0;
    
    TRISA = 0;
    ANSELA = 0;
    ANSELB = 0;
    
    u8 c = 0;
    
    while (1) {
        LATA = LedValues[c];
        __delay_ms(1000);
        
        ++ c;
        
        if (c == ARRLEN(LedValues)) {
            c = 0;
        }
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
    
    UsartInit();
    TimeInit(&Time, 6, 4, 1);
    
    Button b1, b2;
    ButtonInit(&b1, &PORTB, 4, 0);
    ButtonInit(&b2, &PORTB, 5, 0);
  
    TalkingBoxInit(&b1, &b2);
    
    while (1) {
        TalkingBoxUpdate(Time);
    }
    
 /*   
    u8 cnt = 0;
    u8 currentTrack = 0;
    
    while (1) {
        // Turn off the mp3 player and sleep
        LATB0 = 0;
        
        while (1) {
            SLEEP();
            NOP();
            
            ButtonReset(&b1);
            ButtonReset(&b2);
            
            while (1) {
                if (ButtonUpdate(&b1, Time) && b1.state == Pressed) {
                    // Woke up by the play button - play the current track
                }
                
                if (ButtonUpdate(&b2, Time) && b2.state == Pressed) {
                    // Woke up by the select button - just warm up the player
                }
            }
        }
        
        LATA = LedValues[cnt % 10];
    }
    
    while (1) {
        // Turn off the mp3 player and sleep
        LATB0 = 0;
        LATA = 0;
        
        while (1) {
            SLEEP();
            NOP();
            
            if (DebounceButton()) {
                break;
            }
        }
        
        // Turn on the mp3 player
        LATB0 = 1;
        LATA = LedValues[8];
        
        // Wait for it to initialize
        __delay_ms(1300);
        u8 goToSleep = 0;
        u8 button = PORTBbits.RB4 == 0;
        
        while (!goToSleep) {
            ButtonPressed = 0;
            
            // Start a random track
            u8 numFiles = mp3_get_num_files();

            if (numFiles == 0) {
                goToSleep = 1;
                continue;
            }
            
            __delay_ms(10);
            u8 track = GetNextFileToPlay(numFiles);
            mp3_set_volume(28);
            __delay_ms(10);
            mp3_play_num(track);
            __delay_ms(300);
            
            Timers[0] = 0;
            
            while (1) {
                if (PORTBbits.RB3 == 1) {
                    // If the player stopped wait for some time and
                    // go to sleep
                    mp3_stop();
                    __delay_ms(10);
                    
                    if (Timers[0] >= 60) {    
                        goToSleep = 1;
                        break;
                    }
                } else {
                    Timers[0] = 0;
                }

                if (PORTBbits.RB4 == 0 && DebounceButton()) {
                    if (!button) {
                        button = 1;
                        mp3_stop();
                        __delay_ms(10);
                        break;
                    } else if (Timers[1] >= 3) {
                        mp3_stop();
                        __delay_ms(10);
                        goToSleep = 1;
                        break;
                    }
                } else if (PORTBbits.RB4 == 1) {
                    Timers[1] = 0;
                    button = 0;
                }
            }
        }
    }
*/    
    return;
}

