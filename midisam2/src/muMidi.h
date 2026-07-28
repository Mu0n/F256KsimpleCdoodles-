#ifndef MUMIDI_H
#define MUMIDI_H

//MIDI related meta codes
#define MetaSequence        0x00
#define MetaText            0x01
#define MetaCopyright       0x02
#define MetaTrackName       0x03
#define MetaInstrumentName  0x04
#define MetaLyrics          0x05
#define MetaMarker          0x06
#define MetaCuePoint        0x07
#define MetaChannelPrefix   0x20
#define MetaChangePort      0x21
#define MetaEndOfTrack      0x2F
#define MetaSetTempo        0x51
#define MetaSMPTEOffset     0x54
#define MetaTimeSignature   0x58
#define MetaKeySignature    0x59
#define MetaSequencerSpecific  0x7F

//SAM2695 midi - use all of these for that chip
#define MIDI_CTRL 	   0xDDA0
#define MIDI_FIFO 	   0xDDA1
#define MIDI_RXD 	   0xDDA2
#define MIDI_RXD_COUNT 0xDDA3
#define MIDI_TXD       0xDDA4
#define MIDI_TXD_COUNT 0xDDA5

//VS1053b midi - use the sam's registers for everything, except when it's time to sound on the VS1053b, use MIDI_FIFO_ALT
#define MIDI_CTRL_ALT 	    0xDDB0 //unused?
#define MIDI_FIFO_ALT 	    0xDDB1
#define MIDI_RXD_ALT	    0xDDB2 //unused?
#define MIDI_RXD_COUNT_ALT  0xDDB3 //unused?
#define MIDI_TXD_ALT     	0xDDB4 //unused?
#define MIDI_TXD_COUNT_ALT  0xDDB5 //unused?


//useful to pinpoint exact MIDI events saved into memory somewhere for playback
//offsets used inside an aMIDIEvent struct
#define AME_DELTA 0 //is 4 bytes long
#define AME_BYTECOUNT 4 //is 1 byte long
#define AME_MSG 5 //up to 3 bytes long
#define MIDI_EVENT_FAR_SIZE 8 //total size of an aMIDIEvent


//VS1053b serial bus
#define VS_SCI_CTRL  0xD700
#define VS_SCI_ADDR  0xD701
#define VS_SCI_DATA  0xD702   //2 bytes
#define VS_FIFO_STAT 0xD704   //2 bytes
#define VS_FIFO_DATA 0xD707

#include "f256lib.h"

extern const char *midi_instruments[128];

//keeps track of tempo changes and pre-calculations of usPerTick and usPerTimer0 to make it lighter during parsing
typedef struct tempoChange{
	uint32_t *absTick; //when it occurs
	uint32_t *usPerTick; //microsecond per tick
	uint32_t *usPerTimer0; //microsecond per tick
} tCh;


//struct for holding info about the midi file itself, for info display purposes
typedef struct midiRecord {
	tCh myTempos; //keeps all tempo changes here
	char *fileName;
	uint16_t format; //0: single track, 1: multitrack
	uint16_t trackcount;
	uint32_t tick; //ticks per beat (aka per quarter note). default of 48 can and will probably be replaced by reading a std midi file
	uint32_t fileSize; //number of bytes for the midi file
	double fudge; //conversion factor for time units away from microseconds towards one for the cpu bound timer0, which is 2/3rds of a second long when it goes from 0x000000 to 0xFFFFFF) default is 25.1658
	uint8_t nn, dd, cc, bb; //nn numerator of time signature, dd= denom. cc=nb of midi clocks in metro click. bb = nb of 32nd notes in a beat defaults: nn=4 dd=2 cc=24 bb=8
	uint16_t *parsers; //indices for the various type 1 tracks during playback
	uint32_t totalDuration; //in units to be divded by 125000 and fudge to get seconds
	uint16_t totalSec;
	uint16_t currentSec;
	uint8_t  nbTempoChanges; //count of tempo changes to perform during playback
	uint8_t bpm;
	uint32_t baseAddr;
	uint32_t parsedAddr;
} midiRec, *midiRecPtr;

//the following three structs are for midi event playback. keep every aMIDIEvent's content in far memory
typedef struct aMIDIEvent {
    uint32_t deltaToGo;
    uint8_t bytecount;
    uint8_t msgToSend[3]; //some events are 2 bytes, some are 3, no biggie to use worst-case scenario
    } aME, *aMEPtr;
    
typedef struct aTableOfEvent {
	uint8_t trackno;
	uint16_t eventcount; //keeps track of total events for playback
	uint32_t baseOffset; //where to put the data in far memory
	} aTOE, *aTOEPtr;
	
typedef struct bigParsedEventList {
	bool hasBeenUsed;
	uint16_t trackcount;
	aTOEPtr TrackEventList;
	} bigParsed, *bigParsedPtr;




void resetInstruments(bool wantAlt);
void midiShutAChannel(uint8_t chan, bool wantAlt);
void midiShutAllChannels(bool wantAlt);
void midiShutUp(bool wantAlt);
void prgChange(uint8_t prg, uint8_t chan, bool wantAlt);
void midiNoteOff(uint8_t chan, uint8_t note, uint8_t speed, bool wantAlt);
void midiNoteOn(uint8_t chan, uint8_t note, uint8_t speed, bool wantAlt);

void initVS1053MIDI(void);
void initMidiRecord(struct midiRecord *, uint32_t, uint32_t);
//void initBigList(struct bigParsedEventList *);
uint32_t getTotalLeft(struct bigParsedEventList *);

extern const char *midi_instruments[];
#endif // MUMIDI_H