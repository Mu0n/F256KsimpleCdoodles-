#include "f256lib.h"
#include "../src/midibouncergfx.h"
#include "../src/muUtils.h"


uint16_t midiSpr_x, midiSpr_y;
int8_t midiSpr_vx, midiSpr_vy;
uint8_t freakoutCoundDown;
bool isFreakingOut;

void drawBouncingBox()
{
bitmapSetActive(0);
bitmapSetColor(2);
bitmapLine(OFF_LEFT, OFF_TOP, OFF_RIGHT, OFF_TOP); //top edge
bitmapLine(OFF_LEFT, OFF_TOP, OFF_LEFT, OFF_BOTTOM); //left edge
bitmapLine(OFF_RIGHT, OFF_TOP, OFF_RIGHT, OFF_BOTTOM); //right edge
bitmapLine(OFF_LEFT, OFF_BOTTOM, OFF_RIGHT, OFF_BOTTOM); //bottom edge
freakoutCoundDown = FREAKOUT_INIT;
isFreakingOut = false;
}
void initMIDISprite()
{
midiSpr_x = OFF_LEFT -SPR_OFF_x;
midiSpr_y = OFF_TOP - SPR_OFF_y;

midiSpr_vx = 2;
midiSpr_vy = 3;

POKE(MMU_IO_CTRL,1);  //MMU I/O to page 1
	// Set up CLUT0.
for(uint16_t c=0;c<1023;c++)  //copy clut0 for regular graphics
	{
		POKE(VKY_GR_CLUT_0+c, FAR_PEEK(SPR_MIDI_PAL+c));
	}
POKE(MMU_IO_CTRL, 0);

graphicsDefineColor(0, 2, 0xFF, 0xFF, 0xFF);

bitmapSetActive(0);
spriteDefine(0, SPR_MIDI, 32, 0, 0);
spriteSetPosition(0, midiSpr_x + 32, midiSpr_y + 32);
randomSeed(getTimerAbsolute(0));
}

void updateMIDISprite()
{
uint8_t bouncecount = 0;
if(isFreakingOut)
	{
	graphicsDefineColor(0, 2, (uint8_t)randomRead(), (uint8_t)randomRead(), (uint8_t)randomRead());
	freakoutCoundDown--;
	if(freakoutCoundDown == 0)
		{
		isFreakingOut = false;
		freakoutCoundDown = FREAKOUT_INIT;
		}
	}
midiSpr_x += midiSpr_vx;
midiSpr_y += midiSpr_vy;	
if(midiSpr_x <= SPR_MIN_X || midiSpr_x >= SPR_MAX_X) 
	{
	midiSpr_vx = -midiSpr_vx;
	bouncecount++;
	}
if(midiSpr_y <= SPR_MIN_Y || midiSpr_y >= SPR_MAX_Y) 
	{
	midiSpr_vy = -midiSpr_vy;
	bouncecount++;
	}
spriteSetPosition(0, midiSpr_x + 32, midiSpr_y + 32);
if(bouncecount==2) 
	{
	isFreakingOut = true;
	}
}



