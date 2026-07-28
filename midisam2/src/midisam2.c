#define F256LIB_IMPLEMENTATION
#include "f256lib.h"
#include "../src/muUtils.h" //contains helper functions I often use
#include "../src/setup.h" //contains helper functions I often use
#include "../src/muMidi.h"  //contains basic MIDI functions I often use
#include "../src/muMidiPlay2.h"  //contains basic MIDI functions I often use
#include "../src/muVS1053b.h"  //contains basic MIDI functions I often use
#include "../src/muFilePicker.h"  //contains basic MIDI functions I often use
#include "../src/muTimer0Int.h"  //contains basic cpu based timer0 functions I often use
#include "../src/muTextUI.h" //text dialogs and file directory and file picking
#include "../src/mulcd.h"
#include "../src/midibouncergfx.h"

#define MIDI_PARSED 0x50000 //end of ram is 0x7FFFF, gives a nice 256kb of parsed midi
#define BITMAP_BASE 0x12000 
#define MUSIC_BASE 0x50000

#define TIMER_FRAMES 0
#define TIMER_SECONDS 1
#define TIMER_PLAYBACK_COOKIE 0
#define TIMER_DELAY_COOKIE 1
#define TIMER_SHIMMER_COOKIE 2
#define TIMER_SHIMMER_DELAY 3

#define CLONE_TO_UART_EMWHITE 0
#define ACTIVITY_CHAN_X 4
EMBED(cozyLCD, "../assets/cozyLCD", 0x30000);
EMBED(midiinst, "../assets/midi_instruments.bin", 0x25400);
EMBED(midigfxbounce, "../assets/midiLogo.bin", 0x24C00);
EMBED(midigfxpal, "../assets/midiLogo.pal", 0x25000);

//STRUCTS
struct timer_t playbackTimer, shimmerTimer;
struct time_t time_data;
bool isPaused = false;
bool isTrulyDone = false;
bool isLightShow = false; //to enable case RGB lights

struct midiRecord myRecord;

uint8_t fileOpenedErrorCode;
//filePickRecord fpr;
	
bool repeatFlag = false;
//PROTOTYPES
short optimizedMIDIShimmering(void);
void zeroOutStuff(void);
void wipeShimmer(void);

char currentFilePicked[32];


//FUNCTIONS

int readLine(FILE *fp, char *buf, int max) {
    int i = 0;
    char c;

    while (fileRead(&c, 1, 1, fp) == 1) {
		if (c == 0x0a) //line feed control detected
            continue;
			
        if (c == 0x0d) //carriage return detected
            break;

        if (i < max - 1)
            buf[i++] = c;
    }

    if (i == 0 && c != 0x0d)
        return 0; // EOF

    buf[i] = '\0';
    return 1;
}

void fetchNextPLEntry(FILE *playlistFile, uint16_t *curLine, uint16_t nbLines)
{
if(playlistFile == NULL) return; //not normal to not have a playlist when using this function

for(uint8_t z=0;z<MAX_FILENAME_LEN;z++) 
	{
	name[z]=0; //empty out the global extern name
	}
readLine(playlistFile, name, MAX_FILENAME_LEN);
lilpause(1);
*curLine = (*curLine)++;
if(*curLine == nbLines)
	{
	fseek(playlistFile, 0, SEEK_SET);
	*curLine=0;
	}
}


bool isK2(void)
{
	uint8_t value = PEEK(0xD6A7) & 0x1F;
	return (value == 0x11);
}
void K2LCD()
{
if(hasCaseLCD())displayImage(0x30000, 2);
lilpause(1);
}

void wipeShimmer()
{
	for(uint8_t i=0;i<16;i++) //channels
		{
		shimmerBuffer[i]=21;
		textGotoXY(ACTIVITY_CHAN_X,8+i);
		for(uint8_t j=0;j<40;j++)  __putchar(32);
		}
}


short optimizedMIDIShimmering() { //deals with textchar animations, but ballooned into deal with key presses during playback too

	if(kernelEventData.type == kernelEvent(timer.EXPIRED))
	{
		
		if(!isPaused)updateMIDISprite();
		switch(kernelEventData.timer.cookie)
		{
			case TIMER_SHIMMER_COOKIE:
				
				for(uint8_t i=0;i<16;i++) //channels
					{
					if(shimmerBuffer[i]==0)continue;
					if(shimmerBuffer[i]==14) {
						textGotoXY(ACTIVITY_CHAN_X,8+i);
						shimmerBuffer[i]=0;
						__putchar(32);
						
						if(isLightShow)POKE(disp[i],0);
						continue;
					}
					textSetColor(i+1,0);
					textGotoXY(ACTIVITY_CHAN_X,8+i);
					__putchar(shimmerBuffer[i]);
					if(isLightShow)POKE(disp[i],1<<(shimmerBuffer[i]-14));
					shimmerBuffer[i]--;
					}
				
				shimmerTimer.absolute = getTimerAbsolute(TIMER_FRAMES)+TIMER_SHIMMER_DELAY;
				setTimer(&shimmerTimer);	
				break;
		}
	}
	
	if(kernelEventData.type == kernelEvent(key.PRESSED))
		{	
			if(kernelEventData.key.raw == 0x83) //F3
				{
					
					/*
					if(isPaused==false)
					{
						midiShutAllChannels(true);
						midiShutAllChannels(false);
						isPaused = true;
						textSetColor(1,0);textGotoXY(20,MENU_Y);textPrint("pause");
					}
					destroyTrack();
					zeroOutStuff();
					wipeText();
					if(getTheFile_far(name)!=0) return 0;
					
					setColors();
					textGotoXY(0,25);printf("->Currently Loading file %s...",name);
					
					
					loadSMFile(name, MUSIC_BASE);
					
					
					detectStructure(0, &myRecord);
					initTrack(MUSIC_BASE);
					wipeText();
					initProgress();
					displayInfo(&myRecord);
					superExtraInfo(&myRecord,midiChip);		
					isPaused = false;
					textSetColor(0,0);textGotoXY(20,MENU_Y);textPrint("pause");
					shimmerTimer.absolute = getTimerAbsolute(TIMER_FRAMES)+TIMER_SHIMMER_DELAY;
					wipeShimmer();
					setTimer(&shimmerTimer);
					
					if(isLightShow)
					{textSetColor(1,0);
					textGotoXY(31,MENU_Y);textPrint("Light Show");
					}
					*/
					return 2;
				}
				
			if(kernelEventData.key.raw == 0x93) //tab
				{
					
					return 3;
				}
			if(kernelEventData.key.raw == 146) //esc
				{
				midiShutAllChannels(midiChip);
				isTrulyDone = true;
				return 1;
				}
			if(kernelEventData.key.raw == 32) //space
			{
				if(isPaused==false)
				{
					midiShutAllChannels(midiChip);
					isPaused = true;
					textSetColor(1,0);textGotoXY(20,MENU_Y);textPrint("pause");
				}	
				else
				{
					isPaused = false;
					textSetColor(0,0);textGotoXY(20,MENU_Y);textPrint("pause");				
				}
			}
			if(kernelEventData.key.raw == 129) //F1
			{
				if(isLightShow==true)
					{
					textSetColor(0,0);
					textGotoXY(31,MENU_Y);textPrint("Light Show");
					isLightShow = false;
					
					
					for(uint8_t i=0; i<3; i++)
						{
						POKE(0xD6A7+i,0x00); //power
						POKE(0xD6AA+i,0x00); //SD
						POKE(0xD6AD+i,0x00); //caps
						POKE(0xD6B3+i,0x00); //netw
						}
					POKE(0xD6A8,0x5F); //turn power green like before
						lilpause(1);
					POKE(0xD6A0,PEEK(0xD6A0)& 0x9D); //turn off control 
					}
				else
					{
					textSetColor(1,0);
					textGotoXY(31,MENU_Y);textPrint("Light Show");
					isLightShow = true;
					
					POKE(0xD6A0,PEEK(0xD6A0) | 0x63); //turn on control
					
					for(uint8_t i=0; i<3; i++)
						{
						POKE(0xD6A7+i,0x00); //power
						POKE(0xD6AA+i,0x00); //SD
						POKE(0xD6AD+i,0x00); //caps
						POKE(0xD6B3+i,0x00); //netw
						}
						
					}

					
			}
			if(kernelEventData.key.raw == 0x85) //F5
				{
				midiShutAllChannels(midiChip);
				if(midiChip==true)
					{
					textSetColor(1,0);textGotoXY(57,MENU_Y);textPrint("SAM2695");
					textSetColor(0,0);textGotoXY(65,MENU_Y);textPrint("VS1053b");
					midiChip = false;
					}
				else
					{
					textSetColor(0,0);textGotoXY(57,MENU_Y);textPrint("SAM2695");
					textSetColor(1,0);textGotoXY(65,MENU_Y);textPrint("VS1053b");
					midiChip = true;
					}
				}
			if(kernelEventData.key.raw == 0x72) //r
				{
				repeatFlag = !repeatFlag;
				if(repeatFlag)
				{
				textSetColor(1,0);textGotoXY(3,27);textPrint("[r]");
				}
				else {
				textSetColor(0,0);textGotoXY(3,27);textPrint("[r]");
					}
				}
			if(kernelEventData.key.raw == 0x2E) //.
			{
			for(uint8_t i=0;i<16;i++)
					{
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0xB0 | i); // control change message
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0x5B); // reverb
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0x00); 
						
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0xB0 | i); // control change message
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0x5D); // chorus
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0x00); 
						
					}
			}
			if(kernelEventData.key.raw == 0x2C) //,
			{
			for(uint8_t i=0;i<16;i++)
					{
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0xB0 | i); // control change message
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0x07); // reverb
						POKE(midiChip?MIDI_FIFO_ALT:MIDI_FIFO, 0x7F); 
						
					}
			}
		//printf("%02x",kernelEventData.key.raw);
		} // end if key pressed
return 0;
}

void machineDependent()
{
	
	uint8_t machineCheck=0;
	//VS1053b
	machineCheck=PEEK(0xD6A7)&0x3F;
	/*
	if(machineCheck == 0x22 || machineCheck == 0x11) //22 is Jr2 and 11 is K2
	{
		*/
	boostVSClock();
	initRTMIDI();
/*
	}
	
	setupSerial();
*/
}

void zeroOutStuff()
{
	for(uint8_t k=0;k<32;k++) currentFilePicked[k]=0;
	
	for(uint8_t i=0;i<16;i++)
	{
		for(uint8_t j=0;j<8;j++)
		{
			shimmerBuffer[i]=14;
		}
	}
	free(myRecord.fileName);
	myRecord.fileName = NULL;
	initMidiRecord(&myRecord, MUSIC_BASE, MUSIC_BASE);
}

int main(int argc, char *argv[]) {

	bool cliFile = false; //first time it loads, next times it keeps the cursor position
	bool isPlayListing = false; //becomes true if a .spl simple playlist file has been opened and is being dealt with
	
	FILE *playlistFile;

	K2LCD();
	
	midiChip = false;
	playbackTimer.units = TIMER_SECONDS;
	playbackTimer.cookie = TIMER_PLAYBACK_COOKIE;
		
	shimmerTimer.units = TIMER_FRAMES;
	shimmerTimer.cookie = TIMER_SHIMMER_COOKIE;
	
	//wipeBitmapBackground(0x2F,0x2F,0x2F);
	POKE(MMU_IO_CTRL, 0x00);
	// XXX GAMMA  SPRITE   TILE  | BITMAP  GRAPH  OVRLY  TEXT
	POKE(VKY_MSTR_CTRL_0, 0b00101111); //sprite,graph,overlay,text
	// XXX XXX  FON_SET FON_OVLY | MON_SLP DBL_Y  DBL_X  CLK_70
	POKE(VKY_MSTR_CTRL_1, 0b00000100); //font overlay, double height text, 320x240 at 60 Hz;
	
	POKE(VKY_LAYER_CTRL_0, 0b00010000); //bitmap 0 in layer 0, bitmap 1 in layer 1
	POKE(VKY_LAYER_CTRL_1, 0b00000010); //bitmap 2 in layer 2

	machineDependent();
	zeroOutStuff();
	setColors();		
	//background color layer
POKE(0xD00D,0x00); //force black graphics background
POKE(0xD00E,0x00);
POKE(0xD00F,0x00);

	bitmapSetActive(0);
	bitmapSetCLUT(0);
	bitmapSetColor(0);
	
	bitmapClear();
	
	bitmapSetVisible(0,false);
	bitmapSetVisible(1,false);
	bitmapSetVisible(2,false);
	
	initMIDISprite();
	
	drawBouncingBox();
	
	/*
	uint8_t v =0;
	for(;;) 
	{
		printf("%c %02x\n", argv[1][v],argv[1][v]);
		v++;
		hitspace();
	}
	*/
	


	if(argc == 3)
		{
		uint8_t i=0, j=0,m=0;
		char *lastSlash;
		size_t dirLen=0;
		char cliPath[MAX_PATH_LEN];
		
		
		while(argv[1][i] != '\0') //copy the directory
			{
				cliPath[i] = argv[1][i];
				i++;
			}	
		cliPath[i] = '\0';
		
		while(argv[1][j] != '\0') //copy the filename
			{
				
				if(argv[1][j]=='.') m++;
				if(m==1 && (argv[1][j]=='m' || argv[1][j]=='M')) m++;
				else m=0;
				if(m==2 && (argv[1][j]=='i' || argv[1][j]=='I')) m++;
				else m=0;
				if(m==3 && (argv[1][j]=='d' || argv[1][j]=='D')) m++;
				else m=0;
				name[j] = argv[1][j];
				j++;
				if(m==4){name[j] = '\0'; break;}
			}		
		lastSlash = strrchr(name, '/');
		dirLen = lastSlash - name; // include '/'
		
		memset(cliPath, 0, sizeof(cliPath));
		strncpy(cliPath, name, dirLen);
		cliPath[dirLen] = '\0';
		fpr_set_currentPath(cliPath);
		
		setColors();textGotoXY(0,25);printf("->Currently Loading file %s...",name);
		loadSMFile(name, MUSIC_BASE);
		lilpause(1);
		
		
		//sprintf(name, "%s%s%s", fpr.currentPath,"/", fpr.selectedFile);
		
		//printf("\nfinal form is: ");
		//printf("%s",name);
		
	cliFile = true;
		}
	else 
		{
	//check if the midi directory is here, if not, target the root
		char *dirOpenResult = fileOpenDir("media/midi");
		if(dirOpenResult != NULL) 
			{
			fpr_set_currentPath("media/midi");
			fileCloseDir(dirOpenResult);
			}
		else fpr_set_currentPath("0:");
		}
	

	
uint16_t nbLines = 538; //these 2 params are to keep track of current playlist position
uint16_t curLine = 0;

	while(!isTrulyDone)
	{
	bool foundOne = false;
	fileOpenedErrorCode = 1; //assume an error opening a file
	
	wipeText();
	resetInstruments(midiChip); //resets all channels to piano, for sam2695
	midiShutUp(true); //ends trailing previous notes if any, for sam2695
	midiShutUp(false); //ends trailing previous notes if any, for sam2695
	midiShutAllChannels(true);
	midiShutAllChannels(false);
	
		bitmapSetVisible(0,false);
		spriteSetVisible(0,false);
	while(!foundOne)
		{
		if(cliFile == false) //force ea file pick if the command line args were not setColors
			{
			if(isPlayListing) //will not enter this block the first time around since a playlist has to be chosen during runtime to get to one
				{
				fetchNextPLEntry(playlistFile, &curLine, nbLines); //next .mid fetched from an active opened playlist file
				}
			else 
				{
				if(getTheFile_far(name) !=0) return 0; //calls the file picker and force the user to pick something, either .mid or .spl
				}
			//update the text UI
			setColors();textGotoXY(0,26);printf("->Currently Loading file %s...",name);
							//at this point, must determine if it's a .VGM or a .SPL
			const char *extension = name + strlen(name) - 3; //gets last 3 characters
			lilpause(10);
			if(strcmp(extension,"spl") == 0) //it's a playlist that has been picked for the first time, feed a .mid into the name string
				{
				isPlayListing = true;
				playlistFile = fileOpen(name,"r");
				
				fetchNextPLEntry(playlistFile, &curLine, nbLines); //next .mid gets fetched from an active opened playlist file
				}
			fileOpenedErrorCode = loadSMFile(name, MUSIC_BASE); //reaching this point, name WILL carry a .mid file name and it must be opened.
			//textGotoXY(0,3);printf("Error Code %d...",fileOpenedErrorCode);
			
			lilpause(1);
			} //end of cliFile not found
		else loadSMFile(name, MUSIC_BASE); //if there was a CL arg, use fetched name to load the mid file
		if(fileOpenedErrorCode == 0) foundOne = true;
		else { //not supposed to happen
			textGotoXY(0,2);printf("error opening");
			hitspace();
			}	
		} //end of while foundOne

		//reset stuff in between file plays, stuff to do after a .mid has been selected
		wipeStatus();
		//textGotoXY(0,3);printf(".W");
		detectStructure(0, &myRecord);
		//textGotoXY(2,3);printf(".D");
		displayInfo(&myRecord);
		initTrack(MUSIC_BASE);
		
		//textGotoXY(4,3);printf(".i");
			//find what to do and exhaust all zero delay events at the start
		for(uint16_t i=0;i<theOne.nbTracks;i++)	exhaustZeroes(i);
		
		
		superExtraInfo(&myRecord, midiChip);		
	
		initProgress();
		setColors();
		
		
		bitmapSetVisible(0,true);
		spriteSetVisible(0, true);
		//file playing
		isPaused = false;
		
		
		resetTimer0();			
		shimmerTimer.absolute = getTimerAbsolute(TIMER_FRAMES)+TIMER_SHIMMER_DELAY;
		setTimer(&shimmerTimer);
		
			for(;;)
				{
				kernelNextEvent();
				int8_t loopStatus = optimizedMIDIShimmering(); //animations and does keypresses
				if(loopStatus == 1) break; //ESC quits the program
				if(loopStatus == 2) //F3 load was pressed
					{
					isPlayListing = false; //break out of playlist playing mode and load something
					break;
					}

				if(!isPaused)
					{

					if(PEEK(INT_PENDING_0)&0x10) //when the timer0 delay is up, go here
						{
						POKE(INT_PENDING_0,0x10); //clear the timer0 delay
						playMidi(); //play the next chunk of the midi file, might deal with multiple 0 delay stuff
						}
					
					if(theOne.isWaiting == false) 
						{
						sniffNextMIDI(); //find next event to play, will cue up a timer0 delay
						}
					
					}	
				if(theOne.isMasterDone >= theOne.nbTracks || loopStatus==3) //end of song or tab pressed
					{
					if(repeatFlag) 
						{
				
					midiShutAllChannels(true);
					midiShutAllChannels(false);
					
					destroyTrack();
					zeroOutStuff();	
					
					detectStructure(0, &myRecord);
					initTrack(MUSIC_BASE);
					textSetColor(1,0);textGotoXY(3,27);textPrint("[r]");

					
					resetTimer0();
					shimmerTimer.absolute = getTimerAbsolute(TIMER_FRAMES)+TIMER_SHIMMER_DELAY;
					setTimer(&shimmerTimer);
						}
					else 
						{
						//isPaused = true;
						break; //really quit no matter what
						}
					}
				
				} //end main playing loop
	}
	return 0;
}
	