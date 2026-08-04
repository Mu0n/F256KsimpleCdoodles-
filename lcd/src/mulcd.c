#include "f256lib.h"
#include "../src/mulcd.h"


//will display a 240x280 image centered on the screen. This assumes a R5G6B5 bitmap file
//converted to a 2 byte per pixel raw binary file, that will be sent to LCD_PIX_LO and LCD_PIX_HI 
//in that order, pixel by pixel
//look for my instructions in https://wiki.f256foenix.com/index.php?title=Use_the_K2_LCD

#pragma clang optimize off
void displayImage(uint32_t addr, uint8_t boost)
{
	uint32_t i;
	uint32_t j;
	uint32_t index = 0;
	uint8_t innerLoop = 240/boost;
	uint8_t skip = 2 * boost;
	bool doubleSpeed = false, quadSpeed = false;
	uint16_t curPixel = 0x0000;
	
	if(boost ==2) doubleSpeed = true;
	if(boost ==4) {
		doubleSpeed = true;	
		quadSpeed = true;
		}
	POKE(LCD_CMD_CMD, LCD_WIN_X);
	POKE(LCD_CMD_DTA, 0); //xstart high
	POKE(LCD_CMD_DTA, 0); //xstart low
	POKE(LCD_CMD_DTA, 0); //xend high
	POKE(LCD_CMD_DTA, 239); //xend low

	POKE(LCD_CMD_CMD, LCD_WIN_Y);
	POKE(LCD_CMD_DTA, 0); //ystart high
	POKE(LCD_CMD_DTA, 20); //ystart low
	POKE(LCD_CMD_DTA, 0x01); //yend high
	POKE(LCD_CMD_DTA, 0x3F); //yend low
	
	POKE(LCD_CMD_CMD, LCD_WRI);
	
	for(j=0;j<280;j++)
	{
		//240 is the default
		for(i=0;i<innerLoop;i++)
		{
			curPixel = FAR_PEEKW(addr + index);
			POKEW(LCD_PIX_LO, curPixel);
			if(doubleSpeed)POKEW(LCD_PIX_LO, curPixel);
			if(quadSpeed)
				{
				POKEW(LCD_PIX_LO, curPixel);
				POKEW(LCD_PIX_LO, curPixel);
				}
			//POKE(LCD_PIX_HI, FAR_PEEK(addr + index + (uint32_t)1));
			index+=skip; //default 2
		}
	}
}
#pragma clang optimize on

}