#include "D:\F256\llvm-mos\code\gfxDemo\.builddir\trampoline.h"

#include "f256lib.h"
#include "../src/muUtils.h"

//will return true if the machine id is detected as a K or K2, enabling peace of mind to use its exclusive features
bool isAnyK(void)
{
	uint8_t value = PEEK(0xD6A7) & 0x1F;
	return (value >= 0x10 && value <= 0x16);
}

bool isK2(void)
{
	uint8_t value = PEEK(0xD6A7) & 0x1F;
	return (value == 0x11);
}

//will return true if the optical keyboard is detected, enabling the case embedded LCD present as well
bool hasCaseLCD(void)
{
	//if the 2nd to last least significant bit is set, it's a mechanical keyboard, LCD not available, don't use it!
	//if it's cleared, then it's an optical keyboard, you can use it!
	return ((PEEK(0xDDC1) & 0x02)==0); //here the bit is cleared, so it's true it's an optical keyboard, it "hasCaseLCD"
}

//returns yes if it's a Jr2 or a K2 in classic mmu mode (ie if it supports a VS1053b or SAM2695)
bool isWave2(void)
{
	uint8_t mid;
	mid = PEEK(0xD6A7)&0x3F;
	return (mid == 0x22 || mid == 0x11); //22 is Jr2 and 11 is K2
}

//graphics background cleardevice
void wipeBitmapBackground(uint8_t blue, uint8_t green, uint8_t red)
{
	byte backup;
	backup = PEEK(MMU_IO_CTRL);
	POKE(MMU_IO_CTRL,0);
	POKE(0xD00D,blue); //force black graphics background
	POKE(0xD00E,green);
	POKE(0xD00F,red);
	POKE(MMU_IO_CTRL,backup);
}
//codec enable all lines
void openAllCODEC()
{
	POKE(0xD620, 0x1F);
	POKE(0xD621, 0x2A);
	POKE(0xD622, 0x01);
	while(PEEK(0xD622) & 0x01);
}
//realTextClear: manually changes to MMU page 2 and covers the whole 80x60 text layer
//blank characters. the f256lib.h's textClear seems to only erase part of the screen only.
void realTextClear()
{
	uint16_t c;
	POKE(MMU_IO_CTRL,0x02);
	for(c=0;c<4800;c++)
	{
		POKE(0xC000+c,0x20);
	}
	POKE(MMU_IO_CTRL,0x00);
}


//Sends a kernel based timer. You must prepare a timer_t struct first and initialize its fields
bool setTimer(const struct timer_t *timer)
{
    *(uint8_t*)0xf3 = timer->units;
    *(uint8_t*)0xf4 = timer->absolute;
    *(uint8_t*)0xf5 = timer->cookie;
    kernelCall(Clock.SetTimer);
	return !kernelError;
}
//getTimerAbsolute:
//This is essential if you want to retrigger a timer properly. The old value of the absolute
//field has a high chance of being desynchronized when you arrive at the moment when a timer
//is expired and you must act upon it.
//get the value returned by this, add the delay you want, and use setTimer to send it off
//ex: myTimer.absolute = getTimerAbsolute(TIMES_SECONDS) + TIMER_MYTIMER_DELAY
uint8_t getTimerAbsolute(uint8_t units)
{
    *(uint8_t*)0xf3 = units | 0x80;
    return kernelCall(Clock.SetTimer);
}

//injectChar: injects a specific character in a specific location on screen.
//position x,y is where it'll be injected in text layer coordinates
//theChar is the byte from 0-255 that will be placed there
//col(umn) should be either 40 (in double character width mode) or 80 (in regular width mode)
void injectChar40col(uint8_t x, uint8_t y, uint8_t theChar, uint8_t col)
{
		POKE(MMU_IO_CTRL,0x02); //set io_ctrl to 2 to access text screen
		POKE(0xC000 + col * y + x, theChar);
		POKE(MMU_IO_CTRL,0x00);  //set it back to default
}

//simple hit space to continue forced modal delay
void hitspace()
{
	bool exitFlag = false;
	
	while(exitFlag == false)
	{
			kernelNextEvent();
			if(kernelEventData.type == kernelEvent(key.PRESSED))
			{
				switch(kernelEventData.key.raw)
				{
					case 148: //enter
					case 32: //space
						exitFlag = true;
						break;
				}
			}
	}
}

void lilpause(uint8_t timedelay)
{
	struct timer_t pauseTimer;
	bool noteExitFlag = false;
	pauseTimer.units = 0; //frames
	pauseTimer.cookie = 213; //let's hope you never use this cookie!
	pauseTimer.absolute = getTimerAbsolute(0) + timedelay;
	setTimer(&pauseTimer);
	noteExitFlag = false;
	while(!noteExitFlag)
	{
		kernelNextEvent();
		if(kernelEventData.type == kernelEvent(timer.EXPIRED))
		{
			switch(kernelEventData.timer.cookie)
			{
			case 213:
				noteExitFlag = true;
				break;
			}
		}
	}
}
