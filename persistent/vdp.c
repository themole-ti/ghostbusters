/********************************/
/* VDP access routines	   		*/
/* Copied from Tursi's libti99  */
/********************************/
#include "vdp.h"
#include "bankswitch.h"
#include "globals.h"
#include "TISNPlay.h"
#include "TISfxPlay.h"
#include "utils.h"

unsigned char gSaveIntCnt;	// console interrupt count byte

void vdpwaitvint() 
{
	VDP_INT_ENABLE; 

	while ( VDP_INT_COUNTER == gSaveIntCnt ) { } 
	gSaveIntCnt = VDP_INT_COUNTER; 

	VDP_INT_DISABLE; 
}

void vdpmemcpy(int pAddr, const unsigned char *pSrc, int cnt) 
{
	VDP_SET_ADDRESS_WRITE(pAddr);

	while (cnt--) 
		VDPWD=*(pSrc++);
}

#define BYTES_PER_FRAME 128
void vdpmemcpy_sn(int pAddr, const unsigned char *pSrc, int cnt) 
{
	int counter = BYTES_PER_FRAME;
	VDP_SET_ADDRESS_WRITE(pAddr);

	while (cnt--) 
	{
		counter--;
		if (!counter)
		{
			counter = BYTES_PER_FRAME;
			VSYNC_PLAY;
		}
		VDPWD=*(pSrc++);
	}
}

void vdpmemset(int pAddr, int ch, int cnt)
{
	VDP_SET_ADDRESS_WRITE(pAddr);

	while (cnt--) 
		VDPWD = ch;
}

void vdpchar(int pAddr, int ch) 
{
	VDP_SET_ADDRESS_WRITE(pAddr);

	VDPWD=ch;
}

void vdpwriteinc(int pAddr, int nStart, int cnt) 
{
	VDP_SET_ADDRESS_WRITE(pAddr);

	while (cnt--) 
		VDPWD=nStart++;
}
