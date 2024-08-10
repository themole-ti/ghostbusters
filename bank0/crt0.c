// crt0.c - C runtime initialization code
#include "trampolines.h"
#include "vdp.h"

extern unsigned int  _text;
extern unsigned int  _text_end;
extern unsigned int  _bss;
extern unsigned int  _bss_end;
extern unsigned int  _persistent;
extern unsigned int  _persistent_src;
extern unsigned int  _persistent_end;
extern unsigned int  _scratchpad;
extern unsigned int  _scratchpad_src;
extern unsigned int  _scratchpad_end;
extern unsigned int  _bss_end;

// Don't use variables here, they need the 32k!
// This means no loops either...
#define MEMEXP	*((volatile unsigned int*)0xa000)
inline void detect_32k()
{
	// define variable in scratchpad, we don't have a stack yet
	#define i *((volatile unsigned int*)0x8342)
	const char *msg = "32K MEM REQUIRED!";

	MEMEXP = 0xdead;
	if (MEMEXP != 0xdead)
	{
		VDP_SET_REGISTER( 7, 0x44);
		VDP_SET_ADDRESS_WRITE(0x0380);
		for (i = 0; i < 0xf; i = i + 1)
			VDPWD = 0xf4;

		VDP_SET_ADDRESS_WRITE(0x0188);
		for (i = 0; i < 0x11; i = i + 1)
			VDPWD = msg[i];

		while (1)
		{
		}
	}
}

// Linker will look for _start symbol as the entry point of our program
void _start()
{
	// Start by turning off interupts, and setting the workspace pointer
	__asm__
	(
		"limi	0	\n\t"
		"lwpi	0x8300\n\t"
	);

	// This program requires the 32k memory expansion
	detect_32k();

	// Set stack
	__asm__
	(
		"li	sp, 0x3ffe\n\t"
	);

	// The symbols starting with '_' are defined in the linker script
	// They point to the ROM locations for each section
	unsigned int *src = &_persistent_src;
	unsigned int *dst = &_persistent;

	// Copy persistent code (ie, non-bankswitchable) to lower memory expansion
	while (dst < &_persistent_end)
		*dst++ = *src++;

	// Copy scratchpad code (ie, non-bankswitchable super duper fast stuff) to lower memory expansion
	unsigned int *src2 = &_scratchpad_src;
	unsigned int *dst2 = &_scratchpad;
	while (dst2 < &_scratchpad_end)
		*dst2++ = *src2++;

	// Zero BSS
	for (dst = &_bss; dst < &_bss_end; dst++)
		*dst = 0;

	// Copy initial data from ROM to higher memory expansion
	init_data_section();

	// Start executing C program from main function
	__asm__
	(
		"b	@main\n\t"
	);
}
