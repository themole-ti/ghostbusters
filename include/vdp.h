#ifndef VDP_H
#define VDP_H

// Read Data
#define VDPRD	*((volatile unsigned char*)0x8800)
// Read Status
#define VDPST	*((volatile unsigned char*)0x8802)
// Write Address/Register
#define VDPWA	*((volatile unsigned char*)0x8C02)
// Write Data
#define VDPWD	*((volatile unsigned char*)0x8C00)

//*********************
// Inline VDP helpers
//*********************

// Set VDP address for read (no bit added)
inline void VDP_SET_ADDRESS(unsigned int x)							{	VDPWA=((x)&0xff); VDPWA=((x)>>8);			}

// Set VDP address for write (adds 0x4000 bit)
inline void VDP_SET_ADDRESS_WRITE(unsigned int x)					{	VDPWA=((x)&0xff); VDPWA=(((x)>>8)|0x40);	}

// Set VDP write-only register 'r' to value 'v'
inline void VDP_SET_REGISTER(unsigned char r, unsigned char v)		{	VDPWA=(v); VDPWA=(0x80|(r));				}

// get a screen offset for 32x24 graphics mode
inline int VDP_SCREEN_POS(unsigned int r, unsigned int c)			{	return (((r)<<5)+(c));						}

// get a screen offset for 40x24 text mode
inline int VDP_SCREEN_TEXT(unsigned int r, unsigned int c)			{	return (((r)<<5)+((r)<<3)+(c));				}

//*********************
// VDP Console interrupt control
//*********************

// Interrupt counter - incremented each interrupt
#define VDP_INT_COUNTER			*((volatile unsigned char*)0x8379)

// Maximum number of sprites performing automatic motion
#define VDP_SPRITE_MOTION_MAX	*((volatile unsigned char*)0x837a)

// Copy of the VDP status byte. If VDP interrupts are enabled, you should read
// this value, instead of reading it directly from the VDP.
#define VDP_STATUS_MIRROR		*((volatile unsigned char*)0x837b)

// This flag byte allows you to turn parts of the console interrupt handler on and off
// See the VDP_INT_CTRL_* defines below
#define VDP_INT_CTRL			*((volatile unsigned char*)0x83c2)

// Address of a user-defined function to call during the vertical interrupt handler,
// set to 0x0000 if not using
#define VDP_INT_HOOK			*((volatile void**)0x83c4)

// If using KSCAN, you must put a copy of VDP register 1 (returned by the 'set' functions)
// at this address, otherwise the first time a key is pressed, the value will be overwritten.
// The console uses this to undo the screen timeout blanking.
#define VDP_REG1_KSCAN_MIRROR	*((volatile unsigned char*)0x83d4)

// The console counts up the screen blank timeout here. You can reset it by writing 0,
// or prevent it from ever triggering by writing an odd number. Each interrupt, it is
// incremented by 2, and when the value reaches 0x0000, the screen will blank by setting
// the blanking bit in VDP register 1. This value is reset on keypress in KSCAN.
#define VDP_SCREEN_TIMEOUT		*((volatile unsigned int*)0x83d6)

// These values are flags for the interrupt control 
	// disable all processing (screen timeout and user interrupt are still processed)
	#define VDP_INT_CTRL_DISABLE_ALL		0x80
	// disable sprite motion
	#define VDP_INT_CTRL_DISABLE_SPRITES	0x40
	// disable sound list processing
	#define VDP_INT_CTRL_DISABLE_SOUND		0x20
	// disable QUIT key testing
	#define VDP_INT_CTRL_DISABLE_QUIT		0x10

// wait for a vblank (interrupts disabled - will work unreliably if enabled)
// call vdpwaitvint() instead if you want to keep running the console interrupt
// DO NOT USE the non-CRU version - this will miss interrupts.
#define VDP_WAIT_VBLANK  		while (!(VDPST & VDP_ST_INT)) { }
#define VDP_WAIT_VBLANK_CRU	  __asm__( "clr r12\n\ttb 2\n\tjeq -4\n\tmovb @>8802,r12" : : : "r12" );

// we enable interrupts via the CPU instruction, not the VDP itself, because it's faster
// Note that on the TI interrupts DISABLED is the default state
#define VDP_INT_ENABLE			__asm__("LIMI 2")
#define VDP_INT_DISABLE			__asm__("LIMI 0")

// A small sleep function
#define SLEEP(x) { for (int i = 0; i < (x); i++) vdpwaitvint(); }

//*********************
// Register settings
//*********************

// Bitmasks for the status register
#define VDP_ST_INT				0x80		// interrupt ready
#define VDP_ST_5SP				0x40		// 5 sprites-on-a-line
#define VDP_ST_COINC			0x20		// sprite coincidence
#define VDP_ST_MASK				0x1f		// mask for the 5 bits that indicate the fifth sprite on a line

// these are the actual write-only register indexes
#define VDP_REG_MODE0			0x00		// mode register 0
#define VDP_REG_MODE1			0x01		// mode register 1
#define VDP_REG_NT				0x02		// screen image table address (this value times 0x0400)
#define VDP_REG_CT				0x03		// color table address (this value times 0x0040)
#define VDP_REG_PDT				0x04		// pattern descriptor table address (this value times 0x0800)
#define VDP_REG_SAL				0x05		// sprite attribute list address (this value times 0x0080)
#define VDP_REG_SDT				0x06		// sprite descriptor table address (this value times 0x0800)
#define VDP_REG_COL				0x07		// screen color (most significant nibble - foreground in text, 
											// least significant nibble - background in all modes)

// settings for mode register 0
#define VDP_MODE0_BITMAP		0x02		// set bitmap mode
#define VDP_MODE0_EXTVID		0x01		// enable external video (not connected on TI-99/4A)

// settings for mode register 1
#define VDP_MODE1_16K			0x80		// set 16k mode (4k mode if cleared)
#define VDP_MODE1_UNBLANK		0x40		// set to enable display, clear to blank it
#define VDP_MODE1_INT			0x20		// enable VDP interrupts
#define VDP_MODE1_TEXT			0x10		// set text mode
#define VDP_MODE1_MULTI			0x08		// set multicolor mode
#define VDP_MODE1_SPRMODE16x16	0x02		// set 16x16 sprites
#define VDP_MODE1_SPRMAG		0x01		// set magnified sprites (2x2 pixels) 

// sprite modes for the mode set functions
#define VDP_SPR_8x8				0x00
#define	VDP_SPR_8x8MAG			(VDP_MODE1_SPRMAG)
#define VDP_SPR_16x16			(VDP_MODE1_SPRMODE16x16)
#define VDP_SPR_16x16MAG		(VDP_MODE1_SPRMODE16x16 | VDP_MODE1_SPRMAG)

// VDP colors
#define COLOR_TRANS				0x00
#define COLOR_BLACK				0x01
#define COLOR_MEDGREEN			0x02
#define COLOR_LTGREEN			0x03
#define COLOR_DKBLUE			0x04
#define COLOR_LTBLUE			0x05
#define COLOR_DKRED				0x06
#define COLOR_CYAN				0x07
#define COLOR_MEDRED			0x08
#define COLOR_LTRED				0x09
#define COLOR_DKYELLOW			0x0A
#define COLOR_LTYELLOW			0x0B
#define COLOR_DKGREEN			0x0C
#define COLOR_MAGENTA			0x0D
#define COLOR_GRAY				0x0E
#define COLOR_WHITE				0x0F

// Actual functions
void vdpmemcpy(int pAddr, const unsigned char *pSrc, int cnt);
void vdpmemcpy_sn(int pAddr, const unsigned char *pSrc, int cnt);		// This version periodically polls if music needs to be played
void vdpmemset(int pAddr, int ch, int cnt);
void vdpchar(int pAddr, int ch);
void vdpwriteinc(int pAddr, int nStart, int cnt);
void vdpwaitvint();

#define vdpgetch(pAddr)	({ VDP_SET_ADDRESS(pAddr); VDPRD; })

// asm implementation of an unrolled vdpmemcpy, copying in 32 byte blocks for speed
// extern void vdpmemcpy_line_asm(int addr, const unsigned char *src, int cnt, int offset);
// Extra fast asm 16 byte copy routine
extern void vdpmemcpy_8(int addr, unsigned char *src, int loop);

#endif