#include "talkingbox.h"
#include "stdlib.h"
#include "dfplayer.h"

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
};

static StateFunc CurrentState = NULL;
static Button *BtnPlay;
static Button *BtnSelect;
static u32 Time;
static u8 TrackNumber = 0; // 0 = play random track
static u8 GoToPlayTrack = 0;
static u8 NumFiles = 0;

static void StateSwitch(State newState) {
    while (StatesMapping[newState] != CurrentState) {
        if (CurrentState != NULL) {
            CurrentState(Exit);
        }
        
        CurrentState = StatesMapping[newState];
        newState = CurrentState(Enter);
    }
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

inline static void Set7SegmentDisplay(u8 number) {
    LATA = LedValues[number];
}

inline static void RotateTrack() {
    ++ TrackNumber;
    
    if (TrackNumber > 9) {
        TrackNumber = 0;
    }
}

static State StateSleep(StateAction action) {
    if (action == Enter) {
        GoToPlayTrack = 0;
        
        // Stop the mp3 player and the 7 segment indicator and
        // put the MCU to sleep
        PowerToPeripherals(0);
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
        wakeUpTime = Time;
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
        
        if (Time - wakeUpTime >= 100) {
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
        Set7SegmentDisplay(TrackNumber);
        playerInitTime = Time + 1300;
        numFilesTimeout = 0;
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
            return StandBy;
        }
        
        if (numFilesTimeout == 0) {
            numFilesTimeout = Time + 1000;
            mp3_get_num_files_async();
            return InitPlayer;
        }
        
        if (mp3_check_for_result()) {
            NumFiles = mp3_get_result();
            
            if (NumFiles == 0) {
                return Sleep;
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
        }
    }
    
    return StandBy;
}

static State StatePlayTrack(StateAction action) {
    return StandBy;
}