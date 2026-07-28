#include "f256lib.h"
#include "../src/muMidi.h"

//shut one channel in particular from 0 to 15
void midiShutAChannel(uint8_t chan, bool wantAlt)
{
	POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0xB0 | chan); // control change message
	POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x7B); // all notes off command
	POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x00); 
}
//shut all channels from 0 to 15
void midiShutAllChannels(bool wantAlt)
{
	uint8_t i=0;
	for(i=0;i<16;i++)
	{
	POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0xB0 | i); // control change message
	POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x7B); // all notes off command
	POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x00); 
	}
}

//Stop all ongoing sounds (ie MIDI panic button)
void midiShutUp(bool wantAlt)
	{
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0xFF);
	}

//Reset all instruments for all 16 channels to acoustic grand piano 00
void resetInstruments(bool wantAlt)
	{
		uint8_t i;
		for(i=0;i<16;i++)
		{
			POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0xC0 | i);
			POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x00);
			
			POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0xB0 | i); // control change message
			POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x79); // all notes off command
			POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x00); 
			
		}
		midiShutUp(wantAlt);
	}
//Set an instrument prg to channel chan
void prgChange(uint8_t prg, uint8_t chan, bool wantAlt)
	{
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0xC0 | chan);
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, prg);
	}
//Cuts an ongoing note on a specified channel
void midiNoteOff(uint8_t channel, uint8_t note, uint8_t speed, bool wantAlt)
	{
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x80 | channel);
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, note);
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, speed);
	}
//Plays a note on a specified channel
void midiNoteOn(uint8_t channel, uint8_t note, uint8_t speed, bool wantAlt)
	{
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, 0x90 | channel);
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, note);
		POKE(wantAlt?MIDI_FIFO_ALT:MIDI_FIFO, speed);
	}

void initMidiRecord(struct midiRecord *rec, uint32_t baseAddr, uint32_t parsedAddr)
{
	rec->totalDuration=0;
	if(rec->fileName == NULL) rec->fileName = malloc(sizeof(char) * 64);
	rec->format = 0;
	rec->trackcount =0;
	rec->tick = 48;
	rec->fileSize = 0;
	rec->fudge = 25.1658;
	rec->nn=4;
	rec->dd=2;
	rec->cc=24;
	rec->bb=8;
	rec->currentSec=0;
	rec->totalSec=0;
	rec->baseAddr=baseAddr;
	rec->parsedAddr=parsedAddr;
}
/*
void initBigList(struct bigParsedEventList *list)
{
	list->hasBeenUsed = false;
	list->trackcount = 0;
	list->TrackEventList = (aTOEPtr)NULL;
}
*/
//gets a count of the total MIDI events that are relevant and left to play	
uint32_t getTotalLeft(struct bigParsedEventList *list)
	{
	uint32_t sum=0;
	uint16_t i=0;

	for(i=0; i<list->trackcount; i++)
		{
		sum+=list->TrackEventList[i].eventcount;
		}
	return sum;
	}	
	

