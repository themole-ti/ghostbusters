/********************************************/
/* Resource copy routines - ROM to (V)RAM	*/
/* 2015 - Danny Lousberg   					*/
/********************************************/

#include "utils.h"
#include "vdp.h"
#include "bankswitch.h"
#include "trampolines.h"
#include "globals.h"
#include "TISNPlay.h"
#include "TISfxPlay.h"
#include "sound.h"
#include "resource_copy.h"

void (*vdpmemcpy_func)(int pAddr, const unsigned char *pSrc, int cnt);
void* (*memcpy_func)(void* dst, const void* src, unsigned int n);

// Copy from cartridge ROM to a section in VRAM
void rom_to_vram(unsigned int srcbank, unsigned int srcoffset, unsigned int length, unsigned int dst)
{
	srcbank += _binary_start_bank;

	// This allows cherrypicking data from within the binary blob without 
	// having to fiddle with the location/offset/length parameters
	while(srcoffset >= BANKSIZE)
	{
		srcoffset -= BANKSIZE;
		srcbank++;
	}

	// Do we need to read across different banks?
	while ( (srcoffset + length) > BANKSIZE )
	{
		unsigned int templength = BANKSIZE - srcoffset;

		// activate source bank
		switchtobank(srcbank);

		// Copy to VRAM
		(*vdpmemcpy_func)(dst, (void*)(BANKADDRESS + srcoffset), templength);

		// update bank, offset and remaining length
		length -= templength;
		srcbank++;				// prime for next bank
		srcoffset = 0;			// start at start of bank
		dst += templength;		// Move destination pointer
	}

	// Copy contents of one (last) bank
	switchtobank(srcbank);

	(*vdpmemcpy_func)(dst, (void*)(BANKADDRESS + srcoffset), length);

	// Restore bank
	switchtobank(currentbank);
}

// Copy from cartridge ROM to a section in RAM
void rom_to_ram(unsigned int srcbank, unsigned int srcoffset, unsigned int length, unsigned char *dst)
{
	srcbank += _binary_start_bank;

	// This allows cherrypicking data from within the binary blob without 
	// having to fiddle with the location/offset/length parameters
	while(srcoffset >= BANKSIZE)
	{
		srcoffset -= BANKSIZE;
		srcbank++;
	}

	// Do we need to read across different banks?
	while ( (srcoffset + length) > BANKSIZE )
	{
		unsigned int templength = BANKSIZE - srcoffset;

		// activate source bank
		switchtobank(srcbank);

		// copy contents until end of bank
		(*memcpy_func)(dst, (void*)(BANKADDRESS + srcoffset), templength);

		// update bank, offset and remaining length
		length -= templength;
		srcbank++;				// prime for next bank
		srcoffset = 0;			// start at start of bank
		dst += templength;		// Move destination pointer
	}

	// Copy contents of one (last) bank
	switchtobank(srcbank);
	(*memcpy_func)(dst, (void*)(BANKADDRESS + srcoffset), length);

	// Restore bank
	switchtobank(currentbank);
}
