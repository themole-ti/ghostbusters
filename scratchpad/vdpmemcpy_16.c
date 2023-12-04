void vdpmemcpy_8(int pAddr, unsigned char* src, int loops)
{
	// Inline assembly for speed 
	__asm__(
		// "		mov		%0, r1			\n"	// Dest location in VRAM
		// "		mov		%1, r2			\n"	// Src address in RAM
		// "		MOV     %2, r6 			\n"	// Number of loops
		"		MOVB	@>8303,@>8C02	\n"	//
		"		ORI		r1,>4000 		\n"	//
		"		MOVB	r1,@>8C02		\n"	//
		"		LI		r5,>8C00		\n"	//
		"octet	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	MOVB 	*r2+,*r5		\n"	//
		"	   	DEC		r3				\n"	//
		"	   	JNE		octet			\n"	//
		: /* no outputs */
		: // "r"(pAddr) 	/* %0 */,
		  // "r"(src) 		/* %1 */,
		  // "r"(loops) 	/* %2 */
		: "r1", "r2", "r5", "r6"
	);	
}
