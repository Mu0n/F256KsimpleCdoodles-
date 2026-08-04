#include "D:\F256\llvm-mos\code\gfxDemo\.builddir\trampoline.h"

#define F256LIB_IMPLEMENTATION
#include "f256lib.h"
#include "../src/muUtils.h" //contains helper functions I often use

#define PAL_BASE    0x10000
#define BMP_L1      0x10400
#define PAL_THING   0x22A70
#define BMP_L2      0x24000
#define SPR_THING   0x36C00
#define BMP_L3      0x38000
#define SPR_BIRD    0x4AC00
#define PAL_SUNSET  0x4B000
#define TIL_SET     0x4B400
#define TIL_L3_MAP  0x4BF00 //40*17*2 = 1360 bytes

#define SPR_COOKIE 0
#define SPR_DELAY 5
#define SPR_BIRD_COOKIE 1
#define SPR_BIRD_DELAY 3
#define TIL_COOKIE 2
#define TIL_DELAY 2

#define SPR_SIZE  16
#define SPR_SIZE_SQUARED  0x100


#define TIMER_FRAMES 0
#define TIMER_SECONDS 1

#define TEXT_INSTR_STARTY 5


#define PS2_M_MODE_EN 0xD6E0
#define PS2_M_X_LO    0xD6E2
#define PS2_M_X_HI    0xD6E3
#define PS2_M_Y_LO    0xD6E4
#define PS2_M_Y_HI    0xD6E5


#define MEM_TXT_CTRL  0xD300
#define MEM_CRS_CTRL  0xD301
#define MEM_TXT_START_ADDR  0xD304
#define MEM_TXT_COLOR_ADDR  0xD308
#define MEM_TXT_LUT 0xC000 //in io page 4
#define MEM_TXT_FNT 0xC000 //in io page 5




EMBED(pal,    "../assets/gfxDemoL1.pal", 0x10000); //1024 b
EMBED(level1, "../assets/gfxDemoL1.bin", 0x10400); //76800 b
EMBED(gpal, "../assets/grudge.pal", 0x23000);//1kb
EMBED(level2, "../assets/gfxDemoL2.bin", 0x24000); //76800 b
EMBED(thing, "../assets/thing.bin", 0x36C00);//1kb
EMBED(level3, "../assets/gfxDemoL3.bin", 0x38000); //76800 b
EMBED(bird, "../assets/bird.bin", 0x4AC00);//1kb
EMBED(sunset, "../assets/gfxSunset.pal", 0x4B000);//1kb
EMBED(tile3, "../assets/Tiles.bin", 0x4B400);// 2816 b

#define MEM_TXT_CONTENT 0x50000 //9600 bytes = 80 * 60 * 2b
#define MEM_TXT_COLORS  0x60000 //9600 bytes = 80 * 60 * 2b

const uint8_t textFancy[80]={
23, 23, 23, 22,  22, 22, 21, 21,  21, 18,
18, 18, 16, 16,  15, 15, 32, 32,  32, 32,
32, 32, 32, 32,  32, 32, 32, 32,  32, 32,
32, 32, 32, 32,  32, 32, 32, 32,  32, 32,
32, 32, 32, 32,  32, 32, 32, 32,  32, 32,
32, 32, 32, 32,  32, 32, 32, 32,  32, 32,
32, 32, 32, 32,  15, 15, 16, 16,  18, 18,
18, 21, 21, 21,  22, 22, 22, 23,  23, 23
	
};
bool b0 = true, b1 = true, b2 =true; //bitmap toggles
bool t0 = false, t1 = false, t2 = false; //tile map toggles
uint16_t tm2_xf = 0; //fine scroll for tile map 2
uint8_t tm2_x = 0; //crude scroll for tile map 2
bool mEn = false;
bool isSunset = false; //for palette CLUT changes between regular and sunset
struct timer_t sprAnim, sprBirdAnim, tileAnim;
uint8_t sprFrame[3] ={0,1,0};
uint8_t sprBirdFrame[3] ={0,1,3};
int8_t boost; //for mouse boost when flicking hard

void spriteGetPosition(byte s, uint16_t *x, uint16_t *y) {
	uint16_t sprite = VKY_SP0_CTRL + (s * 8);

	*x = PEEKW(sprite + OFF_SPR_POS_X_L);
	*y = PEEKW(sprite + OFF_SPR_POS_Y_L);
}

void dealKey(uint8_t raw)
{
	switch(raw)
	{
		case 0x6D: //M ouse
	
			mEn = !mEn;
			POKE(PS2_M_MODE_EN,mEn); //uses bit1= Mode 0 and bit0=enable the mouse
			break;
	
		case 0x62: //B order
			uint16_t r3 = randomRead();
			uint16_t r4 = randomRead();
	
			if((PEEK(0xD004)&0x01)==0x01) POKE(0xD004, 0x00);
			else POKE(0xD004, 0x01); //enable
	
			POKE(0xD005, HIGH_BYTE(r3)); //blue
			POKE(0xD006, LOW_BYTE(r3)); //green
			POKE(0xD007, HIGH_BYTE(r4)); //red

	
			break;
		case 0x31: //1 bitmap layer 0 (topmost)
			if(b0) bitmapSetVisible(0,false);
			else bitmapSetVisible(0,true);
			b0=!b0;
			break;
		case 0x32: //2 bitmap layer 1
			if(b1) bitmapSetVisible(1,false);
			else bitmapSetVisible(1,true);
			b1=!b1;
			break;
		case 0x33: //3 bitmap layer 2
			if(t2==false)b2=!b2;
			else
			{
				b2 = true;
				t2=false;
			}
			bitmapSetVisible(2,b2);
			POKE(VKY_LAYER_CTRL_1, 0b00000010); //tile 2 in layer 2
			break;
		case 0x73: //S prites
			uint8_t cur = PEEK(VKY_MSTR_CTRL_0);
			if((cur & 0x20) == 0x20) POKE(VKY_MSTR_CTRL_0, cur & 0xDF);
			else POKE(VKY_MSTR_CTRL_0, cur | 0x20);
			break;

		case 0x74: //T ext layer
			if((PEEK(VKY_MSTR_CTRL_0)&0x01)==0x01) POKE(VKY_MSTR_CTRL_0, (PEEK(VKY_MSTR_CTRL_0)&0xFE));
			else POKE(VKY_MSTR_CTRL_0, PEEK(VKY_MSTR_CTRL_0)|0x01); //enable
			break;
	
	
		case 0x78: //mem teXt layer
			if((PEEK(VKY_MSTR_CTRL_1)&0xC0)==0xC0) POKE(VKY_MSTR_CTRL_1, (PEEK(VKY_MSTR_CTRL_1)&0x3F));
			else POKE(VKY_MSTR_CTRL_1, PEEK(VKY_MSTR_CTRL_1)|0xC0); //enable
			break;
	
		case 0x67: //G backGround color layer
			uint16_t r1 = randomRead();
			uint16_t r2 = randomRead();
	
			POKE(0xD00D,HIGH_BYTE(r1)); //force black graphics background
			POKE(0xD00E,LOW_BYTE(r1));
			POKE(0xD00F,HIGH_BYTE(r2));
			break;
	
		case 0x75: //U sUnset palette
			isSunset = !isSunset;
			uint8_t palChoice = 0;
			if(isSunset) palChoice = 1;
	
			bitmapSetActive(0);
			bitmapSetCLUT(palChoice);
			bitmapSetActive(1);
			bitmapSetCLUT(palChoice);
			bitmapSetActive(2);
			bitmapSetCLUT(palChoice);
	
			for(uint16_t c=0;c<6;c++)
			{
				POKE(VKY_SP0_CTRL + (c * 8), ((PEEK(VKY_SP0_CTRL + (c * 8)))&0xF9) | (palChoice<<1));
			}

			for(uint16_t y=0;y<17;y++) //straight blue sky
				{
					for(uint16_t x=0;x<40;x++)
					{
						FAR_POKEW(TIL_L3_MAP + (uint32_t)(x*2) + (uint32_t)(y*40*2),
									(FAR_PEEKW((TIL_L3_MAP + (uint32_t)(x*2) + (uint32_t)(y*40*2)))&0x07FF) | isSunset<<11); //clut 0, set 2, tile number 0
					}
				}
	
			break;
		case 0x65: //E Tile Layer 2
		    if(b2==false)t2=!t2;
			else
			{
				t2 = true;
				b2=false;
			}
			POKE(VKY_LAYER_CTRL_1, 0b00000110); //tile 2 in layer 2
			tileSetVisible(2,t2);
			break;
	}
}

void dealTimer(uint8_t cookie)
{
	switch(cookie)
	{
		case SPR_COOKIE:
			for(uint8_t z=0;z<3;z++)
			{
			uint16_t x, y;
			spriteGetPosition(z, &x, &y);
			x=x+5;
			if(x>340) x=0;
			spriteSetPosition(z, x, y);
			if(sprFrame[z]==0) sprFrame[z] =1;
			else sprFrame[z] = 0;
			POKEA(VKY_SP0_CTRL + (z*8)+ OFF_SPR_ADL_L, (SPR_THING + SPR_SIZE_SQUARED * sprFrame[z]));
	
			}
	
			sprAnim.absolute = getTimerAbsolute(TIMER_FRAMES) + SPR_DELAY;
			setTimer(&sprAnim);
			break;
		case SPR_BIRD_COOKIE:
			for(uint8_t z=3;z<6;z++)
			{
			uint16_t x, y;
			spriteGetPosition(z, &x, &y);
			x=x-3;
			if(x<16) x=336;
			spriteSetPosition(z, x, y);
			sprBirdFrame[z-3]++;
			if(sprBirdFrame[z-3]>3) sprBirdFrame[z-3]=0;
			POKEA(VKY_SP0_CTRL + (z*8)+ OFF_SPR_ADL_L, (SPR_BIRD + SPR_SIZE_SQUARED * sprFrame[z-3]));
			}
			sprBirdAnim.absolute = getTimerAbsolute(TIMER_FRAMES) + SPR_BIRD_DELAY;
			setTimer(&sprBirdAnim);
			break;
		case TIL_COOKIE:
			if(t2)
			{
			tm2_xf++;
			if(tm2_xf > 15)
			{
				tm2_xf=0;
				tm2_x++;
				if(tm2_x > 19) tm2_x = 0;
			}
			tileSetScroll(2, tm2_xf, tm2_x, 0, 2);
			}
			tileAnim.absolute = getTimerAbsolute(TIMER_FRAMES) + TIL_DELAY;
			setTimer(&tileAnim);
			break;
	}
}
void setup()
{
//sets the gfx vicky mode
POKE(MMU_IO_CTRL, 0x00);
// DIS GAMMA  SPRITE   TILE  | BITMAP  GRAPH  OVRLY  TEXT
POKE(VKY_MSTR_CTRL_0, 0b00111111); //sprite,graph,overlay,text
// MT_Bk MT_En FON_SET FON_OVLY | MON_SLP DBL_Y  DBL_X  CLK_70
POKE(VKY_MSTR_CTRL_1, 0b00010000); //font overlay, double height text, 320x240 at 70 Hz;

POKE(VKY_LAYER_CTRL_0, 0b00010000); //bitmap 0 in layer 0, bitmap 1 in layer 1
POKE(VKY_LAYER_CTRL_1, 0b00000010); //bitmap 2 in layer 2


POKE(MMU_IO_CTRL,1);  //MMU I/O to page 1
	// Set up CLUT0.
for(uint16_t c=0;c<1023;c++)  //copy clut0 for regular graphics
	{
		POKE(VKY_GR_CLUT_0+c, FAR_PEEK(PAL_BASE+c));
	}

for(uint16_t c=0;c<1023;c++)  //copy clut1 for sunset graphics
	{
		POKE(VKY_GR_CLUT_1+c, FAR_PEEK(PAL_SUNSET+c));
	}
POKE(MMU_IO_CTRL, 0);

//background color layer
POKE(0xD00D,0x00); //force black graphics background
POKE(0xD00E,0x00);
POKE(0xD00F,0x00);


//bitmap layers
bitmapSetAddress(0,BMP_L1);
bitmapSetAddress(1,BMP_L2);
bitmapSetAddress(2,BMP_L3);

bitmapSetActive(0);
bitmapSetCLUT(0);
bitmapSetActive(1);
bitmapSetCLUT(0);
bitmapSetActive(2);
bitmapSetCLUT(0);

bitmapSetVisible(0,true);
bitmapSetVisible(1,true);
bitmapSetVisible(2,true);



//tiles
tileDefineTileMap(2, TIL_L3_MAP, 16, 40, 17); //40x15 map of 16x16, tile map2, making sure it's twice as large as the screen. B[AB]A data repetition for endless scroll
tileDefineTileSet(0, TIL_SET, false); //in a strip graphic data, tile set0

for(uint16_t y=0;y<13;y++) //straight blue sky
{
	for(uint16_t x=0;x<40;x++)
	{
		FAR_POKEW(TIL_L3_MAP + (uint32_t)(x*2) + (uint32_t)(y*40*2), 0x0000 | 0); //clut 0, set 2, tile number 0
	}
}
for(uint16_t y=13;y<17;y++) //crosshatch transparent sky
{
	for(uint16_t x=0;x<40;x++)
	{
		FAR_POKEW(TIL_L3_MAP + (uint32_t)(x*2) + (uint32_t)(y*40*2),  0x0000 | 1); //clut 0, set 2, tile number 1
	}
}
for(uint8_t rep=0;rep<2;rep++)
{
FAR_POKEW(TIL_L3_MAP + (uint32_t)((2+rep*20)*2) + (uint32_t)(8*40*2),  0x0000 | 6); //clut 0, set 2, tile number 2 //1x3 cloud
FAR_POKEW(TIL_L3_MAP + (uint32_t)((3+rep*20)*2) + (uint32_t)(8*40*2),  0x0000 | 7); //clut 0, set 2, tile number 3
FAR_POKEW(TIL_L3_MAP + (uint32_t)((4+rep*20)*2) + (uint32_t)(8*40*2),  0x0000 | 8); //clut 0, set 2, tile number 4

FAR_POKEW(TIL_L3_MAP + (uint32_t)((6+rep*20)*2) + (uint32_t)(4*40*2),  0x0000 | 2); //clut 0, set 2, tile number 2 //2x2 cloud
FAR_POKEW(TIL_L3_MAP + (uint32_t)((7+rep*20)*2) + (uint32_t)(4*40*2),  0x0000 | 3); //clut 0, set 2, tile number 3
FAR_POKEW(TIL_L3_MAP + (uint32_t)((6+rep*20)*2) + (uint32_t)(5*40*2),  0x0000 | 4); //clut 0, set 2, tile number 4
FAR_POKEW(TIL_L3_MAP + (uint32_t)((7+rep*20)*2) + (uint32_t)(5*40*2),  0x0000 | 5); //clut 0, set 2, tile number 5

FAR_POKEW(TIL_L3_MAP + (uint32_t)((12+rep*20)*2) + (uint32_t)(3*40*2),  0x0000 | 9); //clut 0, set 2, tile number 2 //1x2 cloud
FAR_POKEW(TIL_L3_MAP + (uint32_t)((13+rep*20)*2) + (uint32_t)(3*40*2),  0x0000 | 10); //clut 0, set 2, tile number 3

FAR_POKEW(TIL_L3_MAP + (uint32_t)((11+rep*20)*2) + (uint32_t)(8*40*2),  0x0000 | 2); //clut 0, set 2, tile number 2 //2x2 cloud
FAR_POKEW(TIL_L3_MAP + (uint32_t)((12+rep*20)*2) + (uint32_t)(8*40*2),  0x0000 | 3); //clut 0, set 2, tile number 3
FAR_POKEW(TIL_L3_MAP + (uint32_t)((11+rep*20)*2) + (uint32_t)(9*40*2),  0x0000 | 4); //clut 0, set 2, tile number 4
FAR_POKEW(TIL_L3_MAP + (uint32_t)((12+rep*20)*2) + (uint32_t)(9*40*2),  0x0000 | 5); //clut 0, set 2, tile number 5

}

tileSetScroll(2, 0, 10, 0, 0);
//sprites layers
spriteDefine(0, SPR_THING, 16, 0, 1);
spriteSetPosition(0, 100, 216);
spriteSetVisible(0, true);

spriteDefine(1, SPR_THING, 16, 0, 0);
spriteSetPosition(1, 50, 232);
spriteSetVisible(1, true);

spriteDefine(2, SPR_THING, 16, 0, 0);
spriteSetPosition(2, 20, 248);
spriteSetVisible(2, true);


spriteDefine(3, SPR_BIRD, 16, 0, 2);
spriteSetPosition(3, 300, 50);
spriteSetVisible(3, true);

spriteDefine(4, SPR_BIRD, 16, 0, 0);
spriteSetPosition(4, 250, 150);
spriteSetVisible(4, true);

spriteDefine(5, SPR_BIRD, 16, 0, 1);
spriteSetPosition(5, 320, 110);
spriteSetVisible(5, true);

sprAnim.units = TIMER_FRAMES;
sprAnim.cookie = SPR_COOKIE;
sprAnim.absolute = getTimerAbsolute(TIMER_FRAMES) + SPR_DELAY;
setTimer(&sprAnim);

sprBirdAnim.units = TIMER_FRAMES;
sprBirdAnim.cookie = SPR_BIRD_COOKIE;
sprBirdAnim.absolute = getTimerAbsolute(TIMER_FRAMES) + SPR_BIRD_DELAY;
setTimer(&sprBirdAnim);

tileAnim.units = TIMER_FRAMES;
tileAnim.cookie = TIL_COOKIE;
tileAnim.absolute = getTimerAbsolute(TIMER_FRAMES) + TIL_DELAY;
setTimer(&tileAnim);

//border layer
POKE(0xD005, 0xFF); //blue
POKE(0xD006, 0xFF); //green
POKE(0xD007, 0xFF); //red
POKE(0xD008, 0x0F); //4b x size
POKE(0xD009, 0x0F); //4b y size
	
//Text Layer

asm("sei");
POKE(MMU_IO_CTRL,2);  //MMU I/O to page 2
for(uint16_t y=0;y<60;y++)
	{
	for(uint16_t x=0;x<80;x++)
		{
			POKE(0xC000 + x+y*80, textFancy[x]);
		}
	}
	
POKE(MMU_IO_CTRL,3);  //MMU I/O to page 3
for(uint16_t c=0;c<80*60;c++)
{
	POKE(0xC000 + c, 0);
}
POKE(MMU_IO_CTRL,0);  //MMU I/O to page 0
asm("cli");
	

uint8_t ty = TEXT_INSTR_STARTY;

textSetColor(0x0D,0x02);
textGotoXY(22,ty);
textPrint("   Mu0n's VICKY graphics Demo v0.9    ");
ty+=3;
textGotoXY(22,ty++);
textPrint(" M: Mouse                             ");
textGotoXY(22,ty++);
textPrint(" B: Border                            ");
textGotoXY(22,ty++);
textPrint(" T: Text or X: memteXt Layer (buggy)  ");
textGotoXY(22,ty++);
textPrint(" 1: Bitmap Layer 0 or Q: Tile L0(soon)");
textGotoXY(22,ty++);
textPrint(" 2: Bitmap Layer 1 or W: Tile L1(soon)");
textGotoXY(22,ty++);
textPrint(" 3: Bitmap Layer 3 or E: Tile Layer 2 ");
textGotoXY(22,ty++);
textPrint(" G: Change Background Color           ");
ty++;
textGotoXY(22,ty++);
textPrint(" S: Sprites                           ");
textGotoXY(22,ty++);
textPrint(" U: toggle regular/sUnset palette     ");

//mouse
POKE(PS2_M_MODE_EN,mEn); //uses bit1= Mode 0 and bit0=enable the mouse
POKEW(PS2_M_X_LO,0x100); //centers it in both x and y directions
POKEW(PS2_M_Y_LO,0x100);
	
//memtext
POKE(MEM_TXT_CTRL, 0x01); //memtext b0=enable, b1=8x8 mode
POKE(MEM_CRS_CTRL, 0x00); //disable cursor

POKEW(MEM_TXT_START_ADDR  , (uint16_t)(MEM_TXT_CONTENT&0x0000FFFF));
POKE( MEM_TXT_START_ADDR+2, (uint8_t) ((MEM_TXT_CONTENT&0x00FF0000)>>16));

POKEW(MEM_TXT_COLOR_ADDR  , (uint16_t)(MEM_TXT_COLORS&0x0000FFFF));
POKE( MEM_TXT_COLOR_ADDR+2, (uint8_t) ((MEM_TXT_COLORS&0x00FF0000)>>16));
for(uint32_t y=0;y<60;y++)
	{
	for(uint32_t c=0;c<80;c++)
		{
		FAR_POKEW(MEM_TXT_CONTENT + (uint32_t)(c*2) + (uint32_t)(y*160), 0x2000 | 48); //ascii char
		}
	}
	
for(uint16_t y=0;y<60;y++)
	{
	for(uint16_t c=0;c<80;c++)
		{
		FAR_POKEW(MEM_TXT_COLORS + (uint32_t)(c*2) + (uint32_t)(y*160),  0x0300 | 0x0000); //
		}
	}
	


POKE(MMU_IO_CTRL,0x08);  //MMU I/O to page 4  ---- 1-00 = io page 4 LUT TABLES for MEMTEXT
for(uint16_t y=0;y<9;y++)
	{
	POKE(0xC000 +     y*3,     0xFF);
	POKE(0xC000 + (y+1)*3,     0x00);
	POKE(0xC000 + (y+2)*3,     0x00);

	}
/*
POKE(MMU_IO_CTRL,0x09);  //MMU I/O to page 5  ---- 1-01 = io page 5 FONT DEF for MEMTEXT
//to do?
*/
POKE(MMU_IO_CTRL,0x00); //MMU I/O to page 0

}


int main(int argc, char *argv[]) {
	
setup();

while(true)
	{
		kernelNextEvent();
		if(kernelEventData.type == kernelEvent(key.PRESSED))
			{
			dealKey(kernelEventData.key.raw);
			}
		if(kernelEventData.type == kernelEvent(timer.EXPIRED))
			{
			dealTimer(kernelEventData.timer.cookie);
			}
		if(kernelEventData.type == kernelEvent(mouse.DELTA))
			{
	
				int16_t newX, newY;

				if((int8_t)kernelEventData.mouse.delta.x > 4 || (int8_t)kernelEventData.mouse.delta.x < -4) boost = 2;
				else boost = 1;
				newX = PEEKW(PS2_M_X_LO)+boost*(int8_t)kernelEventData.mouse.delta.x;
				if((int8_t)kernelEventData.mouse.delta.y > 4 || (int8_t)kernelEventData.mouse.delta.y < -4) boost = 2;
				else boost = 1;
				newY = PEEKW(PS2_M_Y_LO)+boost*(int8_t)kernelEventData.mouse.delta.y;
	
				if(newX<0) newX=0; if(newX>640-16) newX=640-16;
				if(newY<0) newY=0; if(newY>480-16) newY=480-16;
				POKEW(PS2_M_X_LO,newX);
				POKEW(PS2_M_Y_LO,newY);
			}
	}

	return 0;
} //extra brace, a bug of llvm-mos
