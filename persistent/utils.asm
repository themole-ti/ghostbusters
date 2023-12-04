	def	memcpy
memcpy	
L1		movb	*r2+,*r1+
		dec		r3
		jne		L1
		b		*r11

	def memset
memset
L2		movb	r2,*r1+
		dec		r3
		jne		L1
		b		*r11

	def abs
abs
L3		andi    r1, 0x7FFF
		b		*r11

    def memcpy_sn
memcpy_sn
L6		li 		r4, 128			* How many bytes before we play our music?
L4		movb	*r2+,*r1+		* copy src to dst and increment pointers
		dec 	r4				* decrement music bytecounter
		jeq     L5				* if it hits zero, jump to the song playing routine
		dec		r3				* if not, we decrement to copy counter
		jne		L4				* if this is not zero, we loop back to our copy-and-increment operation
		b		*r11			* if it is, we bail and return to the caller
L5		mov 	r11, r6			* we're about to call the SongLoop routine, so we put our caller's return address in r6 for safekeeping
		bl 		@SongLoop		* Branch and Link to the SongLoop routine
		mov 	r6, r11			* we're back from SongLoop, restore our caller's return address from r6 to r11
		b 		@L6				* reset the music byte counter and continue copying
