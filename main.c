#include "config.h"
#include "dfplayer.h"
#include "usart.h"
#include "stdlib.h"

#define NUM_FILES_IDX       0
#define LAST_PLAYED_IDX     1
#define RNG_SEED_IDX        2
#define RANDOM_FILES_START  3
#define ARRLEN(arr) (sizeof(arr)/sizeof(0[arr]))

static u8 InhibitButtonPress = 0;
static u8 ButtonPressed = 0;
static u8 TimerTime = 0;
static u8 Timers[] = {0, 0}; // These will increment each second

interrupt void Interrupts() {
    if (PIR1bits.RCIF) {
        mp3_on_byte_received(RCREG);
        PIR1bits.RCIF = 0;
    }
    
    if (IOCAFbits.IOCAF5) {
        if (!InhibitButtonPress) {
            ButtonPressed = 1;
        }
        
        IOCAFbits.IOCAF5 = 0;
    }
    
    if (INTCONbits.TMR0IF) {
        ++ TimerTime;
        
        if (TimerTime == 8) {
            for (u8 i = 0; i < ARRLEN(Timers); ++ i) {
                ++ Timers[i];
            }
            TimerTime = 0;
        }
        
        INTCONbits.TMR0IF = 0;
    } 
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
        
        if (PORTAbits.RA5 != 0) {
            return 0;
        }
    }
    
    return 1;
}

void main() {
    // Set the internal oscillator to 4 MHz
    OSCCON = 0b01101000;
    
    // The mp3 player power is on RA2
    TRISA2 = 0;
    LATA2 = 0;
    
    // Mp3 player busy pin is on RA4
    ANSA4 = 0;
    TRISA4 = 1;
    
    // Low power sleep mode
    VREGPM0 = 1;
    VREGPM1 = 1;
    
    // Start the timer
    TMR0 = 0;
    TMR0CS = 0;
    PSA = 0;
    PS0 = 1;
    PS1 = 1;
    PS2 = 1;
    TMR0IE = 1;
    
    // Enable interrupts
    PEIE = 1;
    GIE = 1;
    
    // Enable interrupt on falling edge on RA5
    IOCIE = 1;
    IOCAN5 = 1;
    
    USART_Init();
    
    while (1) {
        // Turn off the mp3 player and sleep
        LATA2 = 0;
        
        while (1) {
            SLEEP();
            NOP();
            
            if (DebounceButton()) {
                break;
            }
        }
        
        // Turn on the mp3 player
        LATA2 = 1;
        
        // Wait for it to initialize
        __delay_ms(1300);
        u8 goToSleep = 0;
        u8 button = PORTAbits.RA5 == 0;
        
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
                if (PORTAbits.RA4 == 1) {
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

                if (PORTAbits.RA5 == 0 && DebounceButton()) {
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
                } else if (PORTAbits.RA5 == 1) {
                    Timers[1] = 0;
                    button = 0;
                }
            }
        }
    }
    
    return;
}
