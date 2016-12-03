#include "talkingbox.h"
#include "stdlib.h"
#include "dfplayer.h"
#include "coroutine.h"

const u32 PowerOffTime = 60000;

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

    Continue,
    Restart,      
    None,
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

static State CurrentState = None;
static Button *BtnPlay;
static Button *BtnSelect;
static u32 Time;
static u32 StateWaitUntil = 0;
static u8 TrackNumber = 0; // 0 = play random track
static u8 GoToPlayTrack = 0;
static u8 NumTracks = 0;

static void StateSwitch(State newState) {
    do {
        if (CurrentState != None) {
            StatesMapping[CurrentState](Exit);
        }
        
        if (newState != Restart) {
            CurrentState = newState;
        }
        
        newState = StatesMapping[CurrentState](Enter);
    } while (newState != Continue && newState != CurrentState);
}

static void StateUpdate() {    
    if (CurrentState == None) {
        return;
    }
    
    State newState = StatesMapping[CurrentState](Update);
        
    if (newState != Continue && newState != CurrentState) {
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
    ButtonUpdate(BtnPlay, Time);
    ButtonUpdate(BtnSelect, Time);
    
    if (CurrentState == None) {
        StateSwitch(Sleep);
    }
    
    if (BtnPlay->state == Pressed && BtnPlay->stateChangeTime + 2000 < Time) {
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

inline static u8 PlayerPlaying() {
    return PORTBbits.RB3 == 0;
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
        if (BtnPlay->stateChanged && BtnPlay->state == Pressed) {
            GoToPlayTrack = 1;
            return InitPlayer;
        }
        
        if (BtnSelect->stateChanged && BtnSelect->state == Pressed) {
            return InitPlayer;
        }
        
        if (Time >= wakeUpTime) {
            return Sleep;
        }
    }
    
    return WakingUp;
}

#define scrSleep(ms) StateWaitUntil = Time + (ms); scrReturn(Continue)

static State StateInitPlayer(StateAction action) {
    scrDeclare;

    if (action == Enter) {
        PowerToPeripherals(1);
        NumTracks = 0;
        scrReset;
        return Continue;
    }
    
    if (action == Update) {
        if (BtnPlay->stateChanged && BtnPlay->state == Pressed) {
            GoToPlayTrack = 1;
        }

        if (BtnSelect->stateChanged && BtnSelect->state == Pressed) {
            RotateTrack();
            Set7SegmentDisplay(TrackNumber);
        }
        
        if (Time < StateWaitUntil) {
            return Continue;
        }
        
        scrBegin;
        scrSleep(1300);
        static u32 numFilesTimeout;
        numFilesTimeout = Time + 1000;
        mp3_set_volume(15);
        scrSleep(20);
        mp3_get_num_files_async();
        
        
        
        while (1) {
            scrReturn(Continue);
            
            if (Time > numFilesTimeout) {
                scrReturn(Sleep);
            }
            
            if (!mp3_check_for_result()) {
                continue;
            }

            NumTracks = mp3_get_result();
            
            if (TrackNumber > NumTracks) {
                TrackNumber = NumTracks;
                Set7SegmentDisplay(TrackNumber);
            }
            
            
            
            
            if (NumTracks == 0) {
                scrReturn(Sleep);
            }

            if (GoToPlayTrack) {
                scrReturn(PlayTrack);
            }

            scrReturn(StandBy);
        }
        
        // Shouldn't come here
        scrFinish(None);
    }
    
    return Continue;
}

static State StateStandBy(StateAction action) {
    static u32 turnOffTime;
    
    if (action == Enter) {
        turnOffTime = Time + PowerOffTime;
    }
    
    if (action == Update) {
        if (BtnPlay->stateChanged && BtnPlay->state == Pressed) {
            return PlayTrack;
        }

        if (BtnSelect->stateChanged && BtnSelect->state == Pressed) {
            RotateTrack();
            Set7SegmentDisplay(TrackNumber);
            return StandBy;
        }
        
        if (Time > turnOffTime) {
            return Sleep;
        }
    }
    
    return StandBy;
}

static void DumpU32(u32 num) {    
    for (int i = 0; i < 8; ++ i) {
        TurnOff7SegmentDisplay();
        __delay_ms(500);

        Set7SegmentDisplay(num & 0x0000000F);
        __delay_ms(500);
        
        num = num >> 4;
    }
}

static State StatePlayTrack(StateAction action) {
    scrDeclare;
    static u8 trackNumber;

    if (action == Enter) {
        if (TrackNumber == 0) {
            // Random track
            trackNumber = 0;
        } else {
            // Predefined track
            trackNumber = TrackNumber - 1;
        }
        
        scrReset;
        return Continue;
    }
    
    if (action == Update) {
        if (BtnSelect->stateChanged && BtnSelect->state == Pressed) {
            RotateTrack();
            Set7SegmentDisplay(TrackNumber);
        }

        if (Time < StateWaitUntil) {
            return Continue;
        }
        
        scrBegin;
        mp3_play_num(trackNumber);
        
        // Wait for player to start
        static u32 enterStandbyTime;
        enterStandbyTime = Time + 1000;
        
        while (1) {
            if (PlayerPlaying()) {
                break;
            }
            
            if (Time > enterStandbyTime) {
                scrReturn(StandBy);
            }
            
            scrReturn(Continue);
        }

        // Wait for the player to stop
        while (1) {
            if (BtnPlay->stateChanged && BtnPlay->state == Pressed) {
                scrReturn(Restart);
            }
            
            if (!PlayerPlaying()) {
                scrReturn(StandBy);
            }
            
            scrReturn(Continue);
        }
        
        // Shouldn't come here
        scrFinish(None);
    }
    
    return Continue;
}