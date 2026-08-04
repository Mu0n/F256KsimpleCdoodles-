#include "D:\F256\llvm-mos\code\lcd\.builddir\trampoline.h"

#define F256LIB_IMPLEMENTATION

#include "f256lib.h"
#include "../src/muUtils.h" //contains helper functions I often use
#include "../src/mulcd.h" //contains functions for the K2 LCD

EMBED(mac, "../assets/IDLM.bin", 0x10000);

int main(int argc, char *argv[]) {
	
	textGotoXY(1,10);
	if(hasCaseLCD() && isAnyK())
	{
    textPrint("K2 detected. custom LCD image loading on the screen case.");
	displayImage(0x10000,1);
	}
	else
	{
    textPrint("K2 is not detected. can't load a custom LCD picture on the screen case.");
	textGotoXY(1,11);
    textPrint("Program stalled. Hit Reset.");
	}
	while(true);
	return 0;}
