/* Linker script to create TI99/4A cartridges */

/* Output straight to a flat binary format (i.e. not ELF) */
OUTPUT_FORMAT(binary)
OUTPUT(cartridge.bin)

/* TI memory layout */
MEMORY
{
	cart_rom (rx) : ORIGIN = 0x4000, LENGTH = 0x2000 /* cartridge ROM, read-only */
	lower_exp (wx) : ORIGIN = 0x2000, LENGTH = 0x1F00 /* 8k minus 256b (256b is for stack) */
	higher_exp (wx) : ORIGIN = 0xa000, LENGTH = 0x6000
	scratchpad (wx) : ORIGIN = 0x8342, LENGTH = 0x00bd /* 32b is for gcc workspace, 34b for audio player workspace */
}

/* Where we put sections */
SECTIONS
{
	. = 0x4000;

	.header0 : 
	{ 
		bank0/cart_header.o(.text) 
	} >cart_rom						
	
	_persistent_src = 0x4000 + SIZEOF(.header0);
	
	.persistent	: AT ( _persistent_src ) 
	{ 
		_persistent = . ; 
		persistent/*.a:*(.text); 
		persistent/*.o(.text); 
		_persistent_end = . ;
	} >lower_exp

	.bank0 (LOADADDR(.persistent) + SIZEOF( .persistent )) : 
	{ 
		_text = . ; 
		bank0/*.o(.text); 
		_text_end = . ;
	}

	_scratchpad_src = .;

	.scratchpad	0x8342	: AT ( _text_end ) 
	{ 
		_scratchpad = . ; 
		scratchpad/*.o(.text); 
		_scratchpad_end = . ;
	}

	.header1 0x4000    	: AT ( 0x6000 ) 
	{ 
		bank1/cart_header.o(.text) 
	}	

	.bank1 0x4026   	: AT ( 0x6026 ) 
	{ 
		bank1/*.o(.text); 
	}
	
	.header2 0x4000    	: AT ( 0x8000 ) 
	{ 
		bank2/cart_header.o(.text) 
	}	
	
	.bank2 0x4026    	: AT ( 0x8026 ) 
	{ 
		bank2/*.o(.text); 
	}
	
	.header3 0x4000    	: AT ( 0xa000 ) 
	{ 
		bank3/cart_header.o(.text) 
	}	
	
	.bank3 0x4026    	: AT ( 0xa026 ) 
	{ 
		bank3/*.o(.text); 
	}
	
	.header4 0x4000    	: AT ( 0xc000 ) 
	{ 
		bank4/cart_header.o(.text) 
	}	
	
	.bank4 0x4026    	: AT ( 0xc026 ) 
	{ 
		bank4/*.o(.text); 
	}
	
	.data  0xa000    	: AT ( 0xe000 ) 
	{
		_data = . ; 
		persistent/*.a:*(songDatVars) persistent/*.a:*(sfxDatVars) persistent/*.a:*( .data ) persistent/*.o( .data ) bank0/*.o( .data ) bank1/*.o( .data ) bank2/*.o( .data ) bank3/*.o( .data ); 
		_data_end = . ;
	}
	
	.binary	0x0000   	: AT ( 0x10000 )
	{ 
		resources/*.o( .data ); 
	}
	
	.bss (_data_end) 	: AT ( 0x7a000) 
	{ 
		_bss = . ; 
		persistent/*.a:*(.bss) persistent/*.o( .bss ) bank0/*.o( .bss ) bank1/*.o( .bss ) bank2/*.o( .bss ) bank3/*.o( .bss ); 
		_bss_end = . ;
	}

	.fill  0x81fff   	: AT ( 0x81fff) 
	{ 
		BYTE(0x00); 
	}	/* Pad to 504kb, 8k less than full image size */
	
	.headerend 0x4000	: AT ( 0x82000) 
	{ 
		bankx/cart_header.o(.text) 
	}
	
	.fill2  0x83fff   	: AT ( 0x83fff) 
	{ 
		BYTE(0x00); 
	}	/* Pad to 512kb, change this to your flash size */
}

/* Ensure banks don't call each other's functions directly */
NOCROSSREFS( .bank0 .bank1 .bank2 .bank3 .bank4)
