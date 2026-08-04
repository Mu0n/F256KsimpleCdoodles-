#ifndef MULCD_H
#define MULCD_H


#define LCD_CMD_CMD 0xDD40  //Write Command Here
#define LCD_RST 0x10        //0 to Reset (RSTn)
#define LCD_BL  0x20        //1 = ON, 0 = OFF 
#define LCD_WIN_X 0x2A    // window X command 
#define LCD_WIN_Y 0x2B    // window Y command
#define LCD_WRI   0x2C    //write command
#define LCD_RD    0x2E    //read  command
#define LCD_MAD   0x36    //MADCTL control register
#define LCD_TE  0x40        //Tear Enable (avoid updating the display while the controller reads its value)

#define LCD_CMD_DTA 0xDD41  //Write Data (For Command) Here
//Always Write in Pairs, otherwise the State Machine will Lock
#define LCD_PIX_LO   0xDD42 //{G[2:0], B[4:0]}
#define LCD_PIX_HI   0xDD43 //{R[4:0], G[5:3]}
#define LCD_CTRL_REG 0xDD44

#define LCD_PURE_RED  0xF800
#define LCD_PURE_GRN  0x07E0
#define LCD_PURE_BLU  0x001F
#define LCD_PURE_WHI  0xFFFF
#define LCD_PURE_BLK  0x0000

#include "f256lib.h"


void displayImage(uint32_t, uint8_t);

#endif // MULCD_H