/* Linker script to fetch cart location of binary resources */

/* Output straight to a flat binary format (i.e. not ELF) */
OUTPUT_FORMAT(binary)
OUTPUT(binaries.bin)

/* Where we put sections */
SECTIONS
{
	.binary	0x0000   	: AT ( 0x6000 ) { resources/*.o( .data ); }
}
