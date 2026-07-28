#include "f256lib.h"
#include "../src/muUtils.h" //contains helper functions I often use
#include "../src/muMidi.h"
#include "../src/muMidiPlay2.h"
#include "../src/muTimer0Int.h" //contains helper functions I often use
#include "../src/muTextUI.h"

//RGB case LED addresses
uint16_t disp[16] = {0xD6B3, 0xD6B4, 0xD6B5,
					 0xD6AD, 0xD6AE, 0xD6AF, 
					 0xD6AA, 0xD6AB, 0xD6AC, 
					 0xD6A7, 0xD6A8, 0xD6A9,
					 0xD6B3,0xD6AD,0xD6AA,0xD6A7};
//color values to hold
uint8_t vals[16] = {0,0,0,0,
					0,0,0,0,
					0,0,0,0,
					0,0,0,0};
uint8_t shimmerBuffer[16];
struct MIDIParser theOne;

bool midiChip; 

//sends a MIDI event message, either a 2-byte or 3-byte one
void sendAME(uint8_t msg0, uint8_t msg1, uint8_t msg2, uint8_t byteCount, bool wantAlt) {
		uint8_t chan = (msg0)&0x0F;
	POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, msg0);
	POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, msg1);
	
	if(byteCount == 3) {
		POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, msg2);
		
	}
	if((msg0 & 0xF0) == 0x90 && msg2 != 0x00) 
		{
			shimmerBuffer[chan]=21;
			//SET_BIT(shimmerBuffer[chan][bufferLocation],bitLocation);
			//POKE(disp[chan],255);
		}
	else if((msg0 & 0xF0) == 0xC0)
	{
		 updateInstrumentDisplay(msg0 & 0x0F, msg1); 
	}
	else if((msg0 & 0xF0) == 0xC0)
	{
		 updateInstrumentDisplay(msg0 & 0x0F, msg1); 
		 //send instrument change to the other MIDI device as well! Note the reverse lambda writing
		 POKE(wantAlt?MIDI_FIFO:MIDI_FIFO_ALT, msg0);
		 POKE(wantAlt?MIDI_FIFO:MIDI_FIFO_ALT, msg1);
	}
	//for EMWhite, double up the MIDI message towards the UART port
	/*
	if(CLONE_TO_UART_EMWHITE) {
	POKE(1,0);
	POKE(0xD630, msg0);
	POKE(0xD630, msg1);
	if(byteCount == 3) POKE(0xD630, msg2);
	}
	*/
}


	

#pragma clang optimize off
__attribute__((noinline))
uint8_t loadSMFile(char *name, uint32_t targetAddress) {
	volatile uint8_t retVal = 0;
    volatile unsigned char ___mmu = (unsigned char)*(volatile unsigned char *)0x000d;
    *(volatile unsigned char *)0x000d = 8;
    FAR8_loadSMFile(name, targetAddress);
    *(volatile unsigned char *)0x000d = ___mmu;
	return retVal;
}
#pragma clang optimize on
	
	
//Opens the std MIDI file
__attribute__((noinline, section(".block8")))
uint8_t FAR8_loadSMFile(char *name, uint32_t targetAddress) {
	FILE *theMIDIfile;
	uint8_t buffer[255];
	size_t bytesRead = 0;
	uint32_t totalBytesRead = 0;
	uint16_t i=0;

	theMIDIfile = fileOpen(name,"r"); // open file in read mode
	if(theMIDIfile == NULL) {
		return 1;
		}

	while ((bytesRead = fileRead(buffer, sizeof(uint8_t), 250, theMIDIfile))>0) { 
			buffer[0]=buffer[0];	
			//dump the buffer into a special RAM area
			for(i=0;i<bytesRead;i++)
				{
				FAR_POKE((uint32_t)targetAddress+(uint32_t)totalBytesRead+(uint32_t)i,buffer[i]);
				}
			totalBytesRead += (uint32_t) bytesRead;
			if(bytesRead < 250) break;
			}
	fileClose(theMIDIfile);
	return 0;
}


//checks the tempo, number of tracks, etc
void detectStructure(uint16_t startIndex, struct midiRecord *rec) {	
    uint32_t trackLength = 0; //size in bytes of the current track
    uint32_t i = startIndex; // #main array parsing index
    //uint32_t j=0;
    uint16_t currentTrack=0; //index for the current track
	  
    i+=4; //skip header tag  
	i+=4; //skip SIZE which is always 6 anyway

    rec->format = 
     	           (uint16_t) (FAR_PEEK(rec->baseAddr+i+1))
    			  |(uint16_t) (FAR_PEEK(rec->baseAddr+i)<<8)
    			  ;
    i+=2;
 
    rec->trackcount = 
     	           (uint16_t)(FAR_PEEK(rec->baseAddr+i+1))  
    			  |(uint16_t)((FAR_PEEK(rec->baseAddr+i)<<8)) 
    			  ;
    i+=2;
    
    rec->tick = 
     	           (uint16_t)(FAR_PEEK(rec->baseAddr+i+1))  
    			  |(uint16_t)((FAR_PEEK(rec->baseAddr+i)<<8)) 
    			  ;
    i+=2;

	currentTrack=0;
     
    while(currentTrack < rec->trackcount)
    	{
			rec->parsers[currentTrack] = 0;
	    	currentTrack++;
	    	i+=4; //skips the MTrk string
	    	
	    	trackLength =  (((uint32_t)(FAR_PEEK(rec->baseAddr+i)))<<24) 
		             | (((uint32_t)(FAR_PEEK(rec->baseAddr+i+1)))<<16)
					 | (((uint32_t)(FAR_PEEK(rec->baseAddr+i+2)))<<8)
					 |  ((uint32_t)(FAR_PEEK(rec->baseAddr+i+3)));
	        i+=4;
	        
	        i+=trackLength;
	       
    	} //end of parsing all tracks
	}
	
	
uint16_t readBigEndian16(uint32_t where) {
    uint8_t bytes[2];
	bytes[0] = FAR_PEEK(where);
	bytes[1] = FAR_PEEK(where+1);
	
    return ((uint16_t)bytes[0] << 8) |
           (uint16_t)bytes[1];
}


uint32_t readBigEndian32(uint32_t where) {
    uint8_t bytes[4];

	bytes[0] = FAR_PEEK(where);
	bytes[1] = FAR_PEEK(where+1);
	bytes[2] = FAR_PEEK(where+2);
	bytes[3] = FAR_PEEK(where+3);
	
    return (((uint32_t)bytes[0]) << 24) |
           (((uint32_t)bytes[1]) << 16) |
           (((uint32_t)bytes[2]) << 8)  |
           (uint32_t)bytes[3];
}

uint32_t divide32by16(uint32_t dividend, uint16_t divisor) {
    uint16_t D_hi = dividend >> 16;
    uint16_t D_lo = dividend & 0xFFFF;

    uint16_t Q_hi = mathUnsignedDivision(D_hi,divisor);
    uint16_t R_hi = D_hi % divisor;

    uint32_t R_combined = ((uint32_t)R_hi << 16) | D_lo;
    uint16_t Q_lo = R_combined / divisor;

    return ((uint32_t)Q_hi << 16) | Q_lo;
}
uint32_t mul32x32_low32(uint32_t a, uint32_t b) {
    uint16_t a_lo = a & 0xFFFF;
    uint16_t a_hi = a >> 16;
    uint16_t b_lo = b & 0xFFFF;
    uint16_t b_hi = b >> 16;

    uint32_t lo_lo = mathUnsignedMultiply(a_lo,b_lo);
    uint32_t hi_lo = mathUnsignedMultiply(a_hi,b_lo);
    uint32_t lo_hi = mathUnsignedMultiply(a_lo,b_hi);

    // Combine partial products
    uint32_t mid = (hi_lo + lo_hi) << 16;
    return lo_lo + mid;
}

//reads one midi command, returns how many bytes are needed for the command inside the buffer.
//uint8_t readMIDIEvent(MIDP *theOne, uint8_t track, FILE *fp)
uint8_t readMIDIEvent(uint8_t track) {
	if(theOne.tracks[track].offset >=  theOne.tracks[track].length)
		{
		return 2;
		}
	theOne.tracks[track].delta = readDelta(track);
	return readMIDICmd(track);
}

//reads the time delta
uint32_t readDelta(uint8_t track) {
	uint32_t value = 0;
	uint8_t temp;
	
	//min count needed for describing the delta: 1; max is 4
	temp = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset++);
	while(temp & 0x80) //keep reading after this one
		{
		temp &= 0x7F; //only keep lowest 7 bits
		value += (uint32_t) temp; //add it to the pile
		value <<= 7; //shift the pile 7 spots to make room
		temp = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset++); //get the next one
		}
	value += (uint32_t)temp; //final addition to close it off
	return value;
	
	//old clunky version
	/*
	uint32_t nValue, nValue2, nValue3, nValue4;
	uint8_t temp;
	
	nValue = 0x00000000;
	nValue2 = 0x00000000;
	nValue3 = 0x00000000;
	nValue4 = 0x00000000;
	
	temp = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset++);
	nValue = (uint32_t)temp;
	
	if(nValue & 0x00000080)
		{
		nValue &= 0x0000007F;
		nValue <<= 7;
		
		
		temp = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset++);
		nValue2 = (uint32_t)temp;
		
		if(nValue2 & 0x00000080)
			{
			nValue2 &= 0x0000007F;
			nValue2 <<= 7;
			nValue <<= 7;
			
			temp = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset++);
			nValue3 = (uint32_t)temp;
			
			if(nValue3 & 0x00000080)
				{
				nValue3 &= 0x0000007F;
				nValue3 <<= 7;
				nValue2 <<= 7;
				nValue <<= 7;
				
				temp = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset++);
				nValue4 = (uint32_t)temp;
				} //end of getting to nValue4
			} //end of getting to nValue3
		} //end of getting to nValue2	
	return nValue | nValue2 | nValue3 | nValue4;
	*/
}

uint8_t readMIDICmd(uint8_t track) {
	uint8_t status_byte, extra_byte, extra_byte2; //temporary bytes that fetch data from the file
	uint8_t extra_byte3, extra_byte4, extra_byte5;
	
//status byte or MIDI message reading

//	printf("\nT%d start at: %08lx + %08lx", track, theOne.tracks[track].start, theOne.tracks[track].offset); 
	
	status_byte = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset);
	extra_byte = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset + (uint32_t)1);
	extra_byte2 = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset + (uint32_t)2);
	extra_byte3 = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset + (uint32_t)3);
	extra_byte4 = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset + (uint32_t)4);
	extra_byte5 = FAR_PEEK(theOne.tracks[track].start + theOne.tracks[track].offset + (uint32_t)5);
	theOne.tracks[track].cmd[0] = status_byte;
	theOne.tracks[track].cmd[1] = extra_byte;
	theOne.tracks[track].cmd[2] = extra_byte2;
	
	theOne.tracks[track].offset++; //advance the offset by the first it needs
//first, check for run-on commands that don't repeat the status_byte
	if(status_byte < 0x80) //run-on detected!
		{
		extra_byte2 = extra_byte; //the 2nd parameter of the command was here
		extra_byte = status_byte; //the first parameter of the command was here
		status_byte = theOne.tracks[track].lastCmd; //fetch this from the recorded last command

		theOne.tracks[track].cmd[0] = status_byte;
		theOne.tracks[track].cmd[1] = extra_byte;
		theOne.tracks[track].cmd[2] = extra_byte2;
		
		//printf(" ! %02x", status_byte);
		theOne.tracks[track].offset--; //since there was no command byte, back track by one	
		}
//second, deal with MIDI meta-event commands that start with 0xFF
	if(status_byte == 0xFF)
		{
		theOne.tracks[track].cmd[0] = status_byte;
		theOne.tracks[track].cmd[1] = extra_byte;
		theOne.tracks[track].cmd[2] = extra_byte2;	
		if(extra_byte == 0x51) //tempo changes!
			{
			theOne.tracks[track].cmd[3] = extra_byte3;
			theOne.tracks[track].cmd[4] = extra_byte4;
			theOne.tracks[track].cmd[5] = extra_byte5;
			}
		skipWhenFFCmd(track, extra_byte, extra_byte2); //makes the parser offset advance properly for next commands
		return 0;
		}
//Third, deal with regular MIDI commands			
//MIDI commands with only 1 data byte
//MIDI commands with only 1 data byte
//Program change   0xC_
//Channel Pressure 0xD_		
	else if(status_byte >= 0xC0 && status_byte <= 0xDF)
		{
		theOne.tracks[track].offset++; //complete the 2 byte advance in the offset (or 1 if run-on)
		theOne.tracks[track].is2B = true;
		theOne.tracks[track].lastCmd = status_byte; //preserve this in case a run-on command happens next
		
		return 0;
		}
				
//MIDI commands with 2 data bytes
// Note off 0x8_
// Note on  0x9_
// Polyphonic Key Pressure 0xA_ (aftertouch)
// Control Change 0xB_
// (0xC_ and 0xD_ have been taken care of above already)
// Pitch Bend 0xE_
	else if((status_byte >= 0x80 && status_byte <= 0xBF) || (status_byte >= 0xE0 && status_byte <= 0xEF))
		{
		theOne.tracks[track].lastCmd = status_byte; //preserve this in case a run-on command happens next				
		
		
		if((status_byte & 0xF0) == 0x90 && extra_byte2==0x00) 
			{
			status_byte = status_byte & 0x8F;	//sometimes note offs are note ons with 0 velocity, quirk of some midi sequencers
			extra_byte2 = 0x7F;	
			}
	
					
		theOne.tracks[track].offset+=2; //complete the 3 byte advance in the offset (or 2 if run-on)		
		theOne.tracks[track].is2B = false;
		return 0;
		}
	else
		{
		//printf("\n ---Unrecognized event sb= %02x",status_byte);
		}
		return 0;
}

uint8_t skipWhenFFCmd(uint8_t track, uint8_t meta_byte, uint8_t data_byte) {	
	theOne.tracks[track].offset+=(uint32_t)(2+data_byte);
	/*
	if(meta_byte == MetaSequence || meta_byte == MetaChannelPrefix || meta_byte == MetaChangePort)
		{
		theOne.tracks[track].offset+=(uint32_t)3;
		}
	else if(meta_byte == MetaText || meta_byte == MetaCopyright || meta_byte == MetaTrackName || meta_byte == MetaInstrumentName
			 || meta_byte == MetaLyrics || meta_byte == MetaMarker || meta_byte == MetaCuePoint)
		{
		theOne.tracks[track].offset+=(uint32_t)2+(uint32_t)data_byte;
		}
	else if(meta_byte == MetaEndOfTrack)
		{
		theOne.tracks[track].offset+=(uint32_t)2;
		}
	else if(meta_byte == MetaSetTempo)
		{
		theOne.tracks[track].offset+=(uint32_t)5;
		}
	else if(meta_byte == MetaSMPTEOffset)
		{
		theOne.tracks[track].offset+=(uint32_t)7;
		}
	else if(meta_byte == MetaTimeSignature)
		{

		theOne.tracks[track].offset+=(uint32_t)6;
		}
	else if(meta_byte == MetaKeySignature)
		{
		theOne.tracks[track].offset+=(uint32_t)4;
		}
	else if(meta_byte == MetaSequencerSpecific)
		{
		}
		*/
		
		
	return 0;
}

//get the next event in this track. do check out if it is "isDone" first, then get into this if it is!
void chainEvent(uint8_t track)
{
	bool quitRefresh = false; //keep looking until we get a case 0, important MIDI event we shouldn't skip, or a case 2 =end of track
	for(;;)
		{
		switch(readMIDIEvent(track))
			{
			case 1: //a skippable 0xFF event was detected, go to the next
				continue;
			case 0: //a regular event was detected, was not skipped
				quitRefresh = true;
				break;
			case 2: //a 0xFF 0x2F event was detected, end of track
				theOne.tracks[track].isDone = true;
				theOne.isMasterDone++;
				quitRefresh = true;
				break;
			}
		if(quitRefresh) break;
		}
}

void performMIDICmd(uint8_t track)
{
	if(theOne.tracks[track].cmd[0] == 0xFF)
		{
		if(theOne.tracks[track].cmd[1] == MetaEndOfTrack) theOne.tracks[track].isDone = true;
		if(theOne.tracks[track].cmd[1] == MetaSetTempo) 
			{
			uint32_t usPerBeat = ( ((uint32_t)theOne.tracks[track].cmd[3])<<16 ) | 
								 ( ((uint32_t)theOne.tracks[track].cmd[4])<<8  ) | 
								 (  (uint32_t)theOne.tracks[track].cmd[5]);
							 
			
			//if you divide usPerBeat by tick per beat, 
			//you get the duration in microseconds per tick, ready to be multiplied	by the events' deltaTimes to get delays in us
	/*
			uint32_t usPerTick = (uint32_t)usPerBeat/((uint32_t)theOne.ticks);
			theOne.timer0PerTick = (uint32_t)(usPerTick * FUDGE); //convert to the units of timer0
			*/
			
			uint32_t usPerTick = (uint32_t)usPerBeat/(uint32_t)theOne.ticks;
			theOne.timer0PerTick = (usPerTick<<3)+(usPerTick<<2); //convert to the units of timer0	
			}
		return;
		}
/*
	POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, theOne.tracks[track].cmd[0]);
	POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, theOne.tracks[track].cmd[1]);

	if(theOne.tracks[track].is2B==false)
		{
		POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, theOne.tracks[track].cmd[2]);
		}
		*/
	sendAME(theOne.tracks[track].cmd[0], theOne.tracks[track].cmd[1], theOne.tracks[track].cmd[2], theOne.tracks[track].is2B?2:3, midiChip);
}

void exhaustZeroes(uint8_t track)
{
	/*
	for(uint16_t i=0;i < theOne.nbTracks; i++)
		{
			*/
			if(theOne.tracks[track].isDone) return;
			if(theOne.tracks[track].delta > 0) return;
			for(;;)
				{
				performMIDICmd(track);
						
				chainEvent(track);
				if(theOne.tracks[track].isDone) break;
				if(theOne.tracks[track].delta > (uint32_t)0) break;
				}
			/*
		}
		*/
}

void playMidi()
{
	//POKE(INT_PENDING_0,0x10);
//play stuff
	if(theOne.cuedDelta > 0x00FFFFFF) //0x00FFFFFF is the max value of the timer0 we can do
		{
			//delay up to maximum of 0x00FFFFFF = 2/3rds of a second
		theOne.cuedDelta -= 0x00FFFFFF; //reduce the max value one by one until there is a remainder smaller than the max amount
		
		setTimer0(theOne.cuedDelta);
		return;
		}
	//do the last delay that's under 2/3rds of a second
	if(theOne.cuedDelta > 0)	
		{
		setTimer0(theOne.cuedDelta);
		theOne.cuedDelta = 0;
		return;
		}
	performMIDICmd(theOne.cuedIndex);

	//
	//perform this after an event with a non-zero delay has been played, lower the other tracks' deltas by that amount, and refresh next event
	//
	for(uint16_t i = 0; i < theOne.nbTracks; i++)
		{
		if(theOne.tracks[i].isDone) continue; //this track is done
		if(i==theOne.cuedIndex) continue; //don't modify itself
		theOne.tracks[i].delta -= theOne.tracks[theOne.cuedIndex].delta;
		if(theOne.tracks[i].delta == 0)	exhaustZeroes(i);
		//textGotoXY(0,15+i);
		//printf("%d %02x %02x %02x %08lx", i, theOne.tracks[i].cmd[0], theOne.tracks[i].cmd[1],theOne.tracks[i].cmd[2], theOne.tracks[i].delta);
		}
	
	//renew the one spent
	chainEvent(theOne.cuedIndex);
	exhaustZeroes(theOne.cuedIndex);

	theOne.isWaiting = false;	
}

void initTrack(uint32_t BASE_ADDR){
	uint32_t pos;	
	
	//read number of tracks
	theOne.nbTracks = readBigEndian16(BASE_ADDR+(uint32_t)10);
	
	if(theOne.tracks != NULL) free(theOne.tracks);
	theOne.tracks = NULL;
	theOne.tracks = (MIDTrackP *)malloc(sizeof(MIDTrackP) * theOne.nbTracks);
	
	theOne.isWaiting = false;
	theOne.timer0PerTick = 500000;
	theOne.cuedDelta = 0xFFFFFFFF;
	theOne.cuedIndex = 0;
	theOne.ticks = 48;
	theOne.isMasterDone=0;
	theOne.progTime = 0;
	
	//read tick
	theOne.ticks = readBigEndian16(BASE_ADDR+(uint32_t)12);
	
	for(uint16_t i=0; i<theOne.nbTracks; i++)
	{
	theOne.tracks[i].length = 0;
	theOne.tracks[i].offset = 0;
	theOne.tracks[i].start = 0;
	theOne.tracks[i].delta = 0;
	theOne.tracks[i].cmd[0] = theOne.tracks[i].cmd[1] = theOne.tracks[i].cmd[2] = 0;
	theOne.tracks[i].cmd[3] = theOne.tracks[i].cmd[4] = theOne.tracks[i].cmd[5] = 0;
	theOne.tracks[i].lastCmd = 0;
	theOne.tracks[i].is2B = true;
	theOne.tracks[i].isDone = false;
	}
	
		//go to every track
	pos=14;
	
	//find the start positions of every track
	for (uint16_t i = 0; i < theOne.nbTracks; i++) {
		pos+=4; //skip header string
		
		uint32_t length  = readBigEndian32(BASE_ADDR + pos); // read track byte length
		theOne.tracks[i].length = length;
		pos+=4;
		theOne.tracks[i].start = BASE_ADDR + pos; //know where to begin
		pos +=length;
		}
		

//first pass, just get one event per track, renew if it's non important event

	for(uint16_t i = 0; i < theOne.nbTracks; i++)
		{
		if(theOne.tracks[i].isDone) continue; //skip finished track, go to next
		if(theOne.tracks[i].offset >= theOne.tracks[i].length) //it's already done, go to next
			{
			theOne.tracks[i].isDone = true; //mark it as finished if we reach the end
			continue;
			}
		textGotoXY(0, 15 + i);
		
		chainEvent(i);
		}	
}

void destroyTrack(){
	if(theOne.tracks != NULL) free(theOne.tracks);
	theOne.tracks = NULL;

}
// Multiply x by a constant multiplier using shifts and adds
uint32_t shift_add_mul(uint32_t x, uint32_t multiplier) {
    uint32_t result = 0;
    for (int bit = 0; bit < 32; ++bit) {
        if ((multiplier >> bit) & 1) {
            result += x << bit;
        }
    }
    return result;
}

//find what to do and cue up the lowest non-zero	
void sniffNextMIDI(){
			uint32_t lowest = 0xFFFFFFFF;
			uint16_t lowestIndex = 0xFFFF;
			for(uint16_t i = 0; i < theOne.nbTracks; i++)
				{
				if(theOne.tracks[i].isDone) continue;

				if(theOne.tracks[i].delta < lowest) //otherwise find the event planned for execution the soonest
					{
					lowest = theOne.tracks[i].delta;
					lowestIndex = i;
					theOne.isWaiting = true;
					theOne.cuedDelta = lowest;
					theOne.cuedIndex = lowestIndex;
					}
				}

			if(theOne.cuedDelta > 0) theOne.cuedDelta = theOne.cuedDelta * theOne.timer0PerTick; 
			setTimer0(theOne.cuedDelta);
			
}
