#include "talkingbox.h"
#include "stdlib.h"
#include "dfplayer.h"
#include "coroutine.h"

typedef enum {
    Enter,
    Update,
    Exit,
} StateAction;

typedef enum {
    Sleep,
    WakingUp,
    InitPlayer,
    StandBy,
    PlayTrack,

    Restart,      
    Last,
} State;

typedef State (*StateFunc)(StateAction);

static State StateSleep(StateAction action);
static State StateWakingUp(StateAction action); 
static State StateInitPlayer(StateAction action); 
static State StateStandBy(StateAction action);
static State StatePlayTrack(StateAction action);

StateFunc StatesMapping[] = {
    StateSleep,
    StateWakingUp,
    StateInitPlayer,
    StateStandBy,
    StatePlayTrack,
};

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
    0b11010111, // A
    0b11011100, // B
    0b01011001, // C
    0b10011110, // D
    0b11011001, // E
    0b11010001, // F
    0b00000000, // 16 (off)
};

static StateFunc CurrentState = NULL;
static u32 StateWaitUntil = 0;
static Button *BtnPlay;
static Button *BtnSelect;
static u32 Time;
static u8 TrackNumber = 0; // 0 = play random track
static u8 GoToPlayTrack = 0;
static u8 NumTracks = 0;

static void StateSwitch(State newState) {
    do {
        if (CurrentState != NULL) {
            CurrentState(Exit);
        }
        
        if (newState != Restart) {
            CurrentState = StatesMapping[newState];
        }
        
        newState = CurrentState(Enter);
    } while (StatesMapping[newState] != CurrentState);
}

static void StateUpdate() {
    if (CurrentState == NULL) {
        return;
    }
    
    State newState = CurrentState(Update);
    
    if (StatesMapping[newState] != CurrentState) {
        StateSwitch(newState);
    }
}

void TalkingBoxInit(Button *btnPlay, Button *btnSelect)
{
    BtnPlay = btnPlay;
    BtnSelect = btnSelect;
}

void TalkingBoxUpdate(u32 time)
{
    Time = time;
    
    if (CurrentState == NULL) {
        StateSwitch(Sleep);
    }
    
    StateUpdate();
}

inline static void PowerToPeripherals(u8 on) {
    LATB0 = on;
}

inline static void TurnOff7SegmentDisplay() {
    LATA = LedValues[16];
}

inline static void Set7SegmentDisplay(u8 number) {
    LATA = LedValues[number];
}

inline static void RotateTrack() {
    ++ TrackNumber;
    
    if (TrackNumber > 9 || (NumTracks > 0 && TrackNumber > NumTracks)) {
        TrackNumber = 0;
    }
}

static State StateSleep(StateAction action) {
    if (action == Enter) {
        GoToPlayTrack = 0;
        
        // Stop the mp3 player and the 7 segment indicator and
        // put the MCU to sleep
        PowerToPeripherals(0);
        TurnOff7SegmentDisplay();
        SLEEP();
        NOP();
        
        // An interrupt from a button will wake the MCU up - go
        // to waking up state to check what happened
        return WakingUp;
    }
    
    return Sleep;
}

static State StateWakingUp(StateAction action) {
    static u32 wakeUpTime;
    
    if (action == Enter) {
        ButtonReset(BtnPlay);
        ButtonReset(BtnSelect);
        wakeUpTime = Time + 100;
        Set7SegmentDisplay(TrackNumber);
        return WakingUp;
    }
    
    if (action == Update) {
        if (ButtonUpdate(BtnPlay, Time) && BtnPlay->state == Pressed) {
            GoToPlayTrack = 1;
            return InitPlayer;
        }
        
        if (ButtonUpdate(BtnSelect, Time) && BtnSelect->state == Pressed) {
            return InitPlayer;
        }
        
        if (Time >= wakeUpTime) {
            return Sleep;
        }
    }
    
    return WakingUp;
}

static State StateInitPlayer(StateAction action) {
    static u32 playerInitTime;
    static u32 numFilesTimeout;

    if (action == Enter) {
        PowerToPeripherals(1);
        playerInitTime = Time + 1300;
        numFilesTimeout = 0;
        NumTracks = 0;
        return InitPlayer;
    }
    
    if (action == Update) {
        if (ButtonUpdate(BtnPlay, Time) && BtnPlay->state == Pressed) {
            GoToPlayTrack = 1;
        }

        if (ButtonUpdate(BtnSelect, Time) && BtnSelect->state == Pressed) {
            RotateTrack();
            Set7SegmentDisplay(TrackNumber);
        }
        
        if (Time < playerInitTime) {
            return InitPlayer;
        }
        
        if (numFilesTimeout > 0 && Time >= numFilesTimeout) {
            return Sleep;
        }
        
        if (numFilesTimeout == 0) {
            numFilesTimeout = Time + 1000;
            mp3_get_num_files_async();
            return InitPlayer;
        }
        
        if (mp3_check_for_result()) {
            NumTracks = mp3_get_result();
            
            if (NumTracks == 0) {
                return Sleep;
            }
            
            if (TrackNumber > NumTracks) {
                TrackNumber = NumTracks;
                Set7SegmentDisplay(TrackNumber);
            }
            
            if (GoToPlayTrack) {
                return PlayTrack;
            }
            
            return StandBy;
        }
    }
    
    return InitPlayer;
}

static State StateStandBy(StateAction action) {
    if (action == Update) {
        if (ButtonUpdate(BtnPlay, Time) && BtnPlay->state == Pressed) {
            return PlayTrack;
        }

        if (ButtonUpdate(BtnSelect, Time) && BtnSelect->state == Pressed) {
            RotateTrack();
            Set7SegmentDisplay(TrackNumber);
            return StandBy;
        }
    }
    
    return StandBy;
}

#define SleepMs(ms) waitUntil = Time + (ms); seq = seq + 1

static void DumpU32(u32 num) {    
    for (int i = 0; i < 8; ++ i) {
        TurnOff7SegmentDisplay();
        __delay_ms(1000);

        Set7SegmentDisplay(num & 0x0000000F);
        __delay_ms(1000);
        
        num = num >> 4;
    }
}

static State StatePlayTrack(StateAction action) {
    static u8 trackNumber;
    static u8 seq;
    static u32 waitUntil = 0;

    if (action == Enter) {
        if (TrackNumber == 0) {
            // Random track
            trackNumber = 0;
        } else {
            // Predefined track
            trackNumber = TrackNumber - 1;
        }
        
        seq = 0;
        waitUntil = 0;
        return PlayTrack;
    }
    
    if (action == Update) {
        if (ButtonUpdate(BtnSelect, Time) && BtnSelect->state == Pressed) {
            RotateTrack();
            Set7SegmentDisplay(TrackNumber);
        }

        if (Time < waitUntil) {
            return PlayTrack;
        }
        
        switch (seq) {
            case 0:
                SleepMs(10);
                break;
                
            case 1: 
                mp3_set_volume(15); 
                SleepMs(300);
                break;
                
            case 2:
                mp3_play_num(trackNumber);
                SleepMs(10);
                break;
            
            case 3: {                
                if (ButtonUpdate(BtnPlay, Time) && BtnPlay->state == Pressed) {
                    return Restart;
                }
                
                break;
            }  
        }
    }
    
    return PlayTrack;
}
