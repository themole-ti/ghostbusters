#include "vdp.h"
#include "bankswitch.h"
#include "globals.h"

void play_sample(unsigned int bank, unsigned int offset, unsigned int length)
{
	VDP_INT_DISABLE;

	// Set up sound chip for sample playback
	__asm__(
		// Load sound chip address in r5
		"	li r5, 0x8400		 \n"	// Mute channels 1 & 2
		"	li r6, 0x9fbf		 \n"
		"	movb r6, *r5		 \n"
		"	swpb r6				 \n"
		"	movb r6, *r5		 \n"
		"	li r6, 0xdfff		 \n"	// Mute channel 3 & noise channel
		"	movb r6, *r5		 \n"
		"	swpb r6				 \n"
		"	movb r6, *r5		 \n"
		"	li r6, 0x8100		 \n"	// Set channel 1 to max frequency (div by 1)
		"	movb r6, *r5		 \n"
		"	swpb r6				 \n"
		"	movb r6, *r5		 \n"
		:
		:
		: "r5", "r6"
	);

	// Bank accounting
	int oldbank 		= currentbank;										// Save our current logical bank number
#ifndef geneve
	int bank_ref 		= BANKADDRESS + ((bank + _binary_start_bank) * 2);	// Address for bank activation on TI
#else
	int bank_ref 		= PAGELIST + (bank + _binary_start_bank);			// Address of element in pagelist on Geneve
#endif
	int initial_size 	= BANKSIZE - offset;								// How many bytes to play from first bank
	int num_banks 		= ((length + offset) / BANKSIZE) + 1;				// How many banks will we play
	int last_size 		= (length + offset) & 0x1fff;						// How many bytes to play from last bank

	// gcc inline assembly parameters:
	// 		%0: initial_size
	// 		%1: num_banks
	// 		%2: last_size
	// 		%3: offset + BANKADDRESS
	// 		%4: bank_ref

	// Push samples
	__asm__(
		"	li   r5, 0x8400		 \n"	// Sound chip address
		"	mov  %0, r6			 \n"	// number of bytes in first bank
		"	mov  %1, r7			 \n"	// number of banks
		"	mov  %2, r9			 \n"	// number of bytes in last bank
		"	mov  %3, r4			 \n"	// start address in first bank
#ifndef geneve
		"	mov  %4, r8			 \n"	// initial bank reference (as an address on the TI)
		"   mov  r8, *r8 		 \n"	// Activate correct bank
#else
		"	mov  %4, r8			 \n"	// initial bank reference (as an address to the index into the pagelist on the Geneve)
		"	movb *r8,@0x8002 	 \n"	// Activate correct bank (writes reference found at above address to 0x8002)
#endif
		"loop: 			 		 \n"
		"	movb *r4+, *r5		 \n"	// Push samples to sound chip and increase sample index
#ifndef geneve
		"	li   r3, 4      	 \n"	// 4 loops on the TI seems to play back at the right speed	
#else
		"	li   r3, 16      	 \n"	// Geneve is 4 times faster than the TI?
#endif
		"waste:      		 	 \n"
		"	dec  r3         	 \n"	
		"	jne  waste    		 \n"	// More samples to send to chip?
		"	dec  r6      		 \n"	// Wasting time...
		"	jne  loop    		 \n"	// More samples to send to chip?
		"   dec  r7 			 \n"	// Decrement bank counter (r7 needs to be > 0 at the start)
		"   jeq  done 			 \n" 	// If r7 is now zero, we're done
#ifndef geneve
		"   ai   r8, 2 			 \n"	// Increase bank address by two to signify jupming to the next bank
		"   mov  r8, *r8 		 \n"	// Activate correct bank
		"   li   r4, 0x6000 	 \n"    // Restart reading at beginning of bank
#else
		"   ai   r8, 1 			 \n"	// Increase bank address by two to signify jupming to the next bank
		"	movb *r8,@0x8002 	 \n"	// Activate correct bank
		"   li   r4, 0x4000 	 \n"    // Restart reading at beginning of bank
#endif
		"   ci   r7, 1 			 \n"	// If this is the last bank (r7 == 1)
		"   jne  full_bank 		 \n"	// then recall the remaining number of samples from the parameters
		"   mov  r9, r6 		 \n" 	// Remainder stored in r9
		"   jmp  loop 			 \n"
		"full_bank: 			 \n"
		"   li   r6, 0x2000		 \n"    // Not the last bank? Then we read the full 8192 samples from this one
		"   jmp  loop 			 \n"
		"done: 					 \n"
		"	li r6, 0x9fbf		 \n"	// Done playing, mute all channels
		"	movb r6, *r5		 \n"
		"	swpb r6				 \n"
		"	movb r6, *r5		 \n"
		"	li r6, 0xdfff		 \n"
		"	movb r6, *r5		 \n"
		"	swpb r6				 \n"
		"	movb r6, *r5		 \n"
		: /* no outputs */
		: "r"(initial_size) 		/* %0 */,
		  "r"(num_banks) 			/* %1 */, 
		  "r"(last_size) 			/* %2 */, 
		  "r"(offset + BANKADDRESS)	/* %3 */, 
		  "r"(bank_ref)  			/* %4 */
		: "r3", "r4", "r5", "r6", "r7", "r8", "r9"
	);

	// Revert back to current bank for code execution
	switchtobank(oldbank);

	VDP_INT_ENABLE;
}
