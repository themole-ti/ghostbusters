#ifndef BANKSWITCH_H
#define BANKSWITCH_H

#include "globals.h"

#ifndef geneve
	// TI defines
	#define BANKADDRESS 0x6000		// Where banked code/data lives
	#define MAPPERADDR  0x6000		// Base bankswitching address (write increases by 2 to switch to higher banks)
	#define ASM_ADDRESS 6000		// For cart header file
#else
	// Geneve defines
	#define BANKADDRESS 0x4000		// Where banked code/data lives
	#define MAPPERADDR  0x8002		// Address to write the page number to
	#define ASM_ADDRESS 8002		// For cart header file
	#define PAGELIST	0x8208		// Logical to physical page translation table
#endif
	
#define _binary_start_bank 7
#define DATA_LOGICAL_BANK (_binary_start_bank - 1)

#define BANKSIZE	0x2000

// Variable to store our current bank, used by all trampoline functions
extern volatile unsigned int currentbank;

#ifdef geneve
	// Switching banks on the Geneve E.g.
	// 		switchtobank(2);
	// will switch to bank 2
	#define switchtobank(x)	{ *(volatile unsigned char*)MAPPERADDR = *(volatile unsigned char*)(PAGELIST + x); }
#else
	// Switching banks on the TI E.g.
	// 		switchtobank(2);
	// will switch to bank 2
	#define switchtobank(x)	{ *((volatile unsigned char*)MAPPERADDR + (x << 1)) = 1; }
#endif

#endif