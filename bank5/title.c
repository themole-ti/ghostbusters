/********************************************/
/* David Crane's Ghostbuster - TI99/4A Port */
/* 2023 - Danny Lousberg   					*/
/********************************************/

#include "vdp.h"
#include "globals.h"
#include "resource_defs.h"
#include "resource_copy.h"
#include "TISNPlay.h"
#include "TISfxPlay.h"
#include "sound.h"
#include "input.h"
#include "utils.h"
#include "rnd.h"
#include "trampolines.h"
#include "sample_playback.h"

int nt_buffers[2];
int buf_id = 0;
int waits[44] = { 0 };
unsigned int ys[16] = { 0, 3, 6, 8, 10, 12, 13, 14, 14, 13, 12, 10, 8, 6, 3, 0 };
int globalcounter;
int slot;


typedef struct
{
	unsigned int when;
	unsigned int velocity;
} bounce_velocity;

bounce_velocity bounce_velocities[] = 
{ 
	{    64 ,   0 },		// Hidden
	{     1 ,   2 },		// Appear
	{    12 ,   0 },
	{     1 ,   1 },		// Shift right
	{     8 ,   0 },
	{     1 ,   3 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{     4 ,   0 },
	{     1 ,   3 },		// If there's
	{     1 ,   3 },		// Something
	{     1 ,   3 },		// Strange
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// In your
	{     1 ,   3 },		// Neighbor
	{     1 ,   4 },		// Hood
	{     1 ,   6 },
	{     4 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     3 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{     3 ,   0 },
	{     1 ,   3 },		// When there's
	{     1 ,   3 },		// Something
	{     1 ,   3 },		// Weird
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// And it
	{     1 ,   3 },		// Don't look
	{     1 ,   4 },		// good
	{     1 ,   6 },
	{     4 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     3 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{    25 ,   0 },
	{     1 ,   3 },		// I ain't
	{     4 ,   1 },		// Afraid of no
	{     1 ,   9 },		// Ghost
	{    23 ,   0 },
	{     1 ,   3 },		// I ain't
	{     4 ,   1 },		// Afraid of no
	{     1 ,   9 },		// Ghost
	{    30 ,   0 },
	{     1 ,   3 },		// If you're
	{     1 ,   3 },		// Seeing
	{     1 ,   3 },		// Things
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// Running
	{     1 ,   3 },		// Through your
	{     1 ,   4 },		// Head
	{     1 ,   6 },
	{     4 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     3 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{     3 ,   0 },
	{     1 ,   3 },		// An 
	{     1 ,   3 },		// invisible
	{     1 ,   3 },		// Man
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// Sleepin
	{     1 ,   3 },		// in your
	{     1 ,   4 },		// bed
	{     1 ,   6 },
	{     4 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     3 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{    25 ,   0 },
	{     1 ,   3 },		// I ain't
	{     4 ,   1 },		// Afraid of no
	{     1 ,   9 },		// Ghost
	{    23 ,   0 },
	{     1 ,   3 },		// I ain't
	{     4 ,   1 },		// Afraid of no
	{     1 ,   9 },		// Ghost
	{    16 ,   0 },
	{     1 ,   3 },		// If you're
	{     1 ,   3 },		// all
	{     1 ,   3 },		// alone
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// Pick
	{     1 ,   3 },		// up the
	{     1 ,   4 },		// phone
	{     1 ,   6 },
	{     3 ,   0 },
	{     1 ,   3 },		// If you're
	{     1 ,   3 },		// all
	{     1 ,   3 },		// alone
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// Pick
	{     1 ,   3 },		// up the
	{     1 ,   4 },		// phone
	{     1 ,   6 },
	{     3 ,   0 },
	{     1 ,   3 },		// and
	{     1 ,   3 },		// call
	{     1 ,   3 },		// ...
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{    18 ,   0 },
	{     1 ,   3 },		// I ain't
	{     4 ,   1 },		// Afraid of no
	{     1 ,   9 },		// Ghost
	{     9 ,   0 },
	{     1 ,   3 },		// I hear
	{     1 ,   3 },		// it like the
	{     1 ,   3 },		// girls
	{     1 ,   7 },
	{     8 ,   0 },
	{     1 ,   3 },		// I ain't
	{     4 ,   1 },		// Afraid of no
	{     1 ,   9 },		// Ghost
	{     3 ,   0 },
	{     1 ,   2 },		
	{     1 ,   2 },		// Yeah
	{     1 ,   0 },		// 
	{     1 ,   3 },		// Yeah
	{     1 ,   0 },		// 
	{     1 ,   2 },		// Yeah
	{     1 ,   0 },		// 
	{     1 ,   3 },		// Yeah
	{     1 ,   4 },		// 
	{    16 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     3 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{     4 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     4 ,   0 },
	{     1 ,   3 },		// Who you
	{     1 ,   3 },		// Gonna
	{     1 ,   3 },		// Call
	{     1 ,   7 },
	{     3 ,   0 },
	{     1 ,   5 },		// Ghost
	{     2 ,   2 },		// Bust-ers
	{     1 ,   7 },		// Jump back to left side of screen
	{    54 ,   0 },
	{     1 ,   3 },		// Let me
	{     4 ,   1 },		// Tell you
	{     1 ,   9 },		// Something
	{     2 ,   0 },
	{     1 ,   3 },		// Bustin' makes
	{     4 ,   1 },		// me feel
	{     1 ,   8 },		// Good
	{     9 ,   0 },
	{  3095 ,   0 },
};

static void load_title()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Prep nametable
	nt_buffers[0] = 0x1800;
	nt_buffers[1] = 0x3C00;
	vdpwriteinc(nt_buffers[0], 0, 768);
	vdpwriteinc(nt_buffers[1], 0, 768);

	// Load color, pattern and sprite generator resources from ROM
	rom_to_vram(unwrap(title_colors), 0x2000);
	rom_to_vram(unwrap(title_patterns), 0x0000);
	rom_to_vram(unwrap(title_sprite_patterns), 0x3800);
	rom_to_vram(unwrap(title_sprite_attributes), 0x1F80);

	// Bouncing ball
	unsigned char ballpatt[32] = 
	{ 
		0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,
		0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,
		0xC0,0x80,0x00,0x00,0x00,0x00,0x00,0x00
	};

	vdpmemcpy(0x3800 + 512, ballpatt, 32);

	// Prepwork for title screen animation
	vdpmemset(0x3200, 0x1f, 0x100);
	vdpmemset(0x3300, 0x1f, 0x100);
	vdpmemset(0x1AA0, 0x00, 64);
	rom_to_vram(font_bank, font_offset, 512, 0x1500);

	// Nice multi-color fonts please!
	unsigned char colors[8] = { 0x61, 0x61, 0x81, 0x91, 0x91, 0x81, 0x61, 0x61 };
	for (int i = 0; i < 512; i++)
	{
		vdpchar(0x3500 + i, colors[i & 0x07]);
	}

	// Position tables in VRAM
	VDP_SET_REGISTER(VDP_REG_PDT, 0x03);
	VDP_SET_REGISTER(VDP_REG_CT,  0xFF);
	VDP_SET_REGISTER(VDP_REG_NT,  0x06);
	VDP_SET_REGISTER(VDP_REG_SDT, 0x07);
	VDP_SET_REGISTER(VDP_REG_SAL, 0x3F);

	// Set bitmap mode
	VDP_SET_REGISTER(VDP_REG_MODE0, VDP_MODE0_BITMAP);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_UNBLANK | VDP_MODE1_INT | VDP_MODE1_SPRMODE16x16);

	// Load timing for karaoke scroll
	for (int i = 0; i < 43; i++)
		waits[i] = 120;

	waits[0] = 720;
	waits[1] = 520;
	waits[11] = 500;					// I ain't 'fraid of no ghosts
	waits[12] = 400;
	waits[13] = 630;
	waits[21] = 500;					// I ain't 'fraid of no ghosts
	waits[22] = 400;
	waits[23] = 360;
	waits[29] = 320;					// I ain't 'fraid of no ghosts
	waits[30] = 260;
	waits[31] = 260;

	waits[33] = 360;
	waits[40] = 710;
}

static int scroll_intro_text(int nametable_offset, unsigned char* scrollingtext, int i)
{
	for (int j = 0; j < 20; j++)
	{
		vdpchar(nt_buffers[buf_id] + nametable_offset + j, scrollingtext[(i/10) + j] + 128);
	}

	i++;

	if (i > ((introscroll_length - 20) * 10))
		i = 0;

	return i;
}

inline void write_str_double32(int vaddr, const char* str)
{
	VDP_SET_ADDRESS_WRITE(vaddr);		// Upper part of double line
	for (int i = 0; i < 32; i++)
		VDPWD = str[i];

	VDP_SET_ADDRESS_WRITE(vaddr + 32);	// Lower part of double line (characters shifted 32 patterns)
	for (int i = 0; i < 32; i++)
		VDPWD = str[i] + 32;
}

static inline void upload_scroll_patterns(int frame)
{
	if (slot)
	{
		rom_to_vram(scrollfont_patterns_bank, scrollfont_patterns_offset       + (frame << 9), 256, 0x1200);	// Upper
		slot = 0;
	}
	else
	{
		rom_to_vram(scrollfont_patterns_bank, scrollfont_patterns_offset + 256 + (frame << 9), 256, 0x1300);	// Lower
		slot = 1;
	}
}

// Display title screen and wait for user to press fire
void _do_title_screen()
{
	unsigned int    joyst_result = 0;

	// Load title music and sound effect into RAM
	MUTE_SOUND();
	rom_to_ram(unwrap(themesong), (unsigned char*)levelsongs);

	// Show title graphics
	load_title();

	play_sample(unwrap(shout));
	SLEEP(60);

	// Play song '0'
	MUTE_SOUND();
	StartSong(levelsongs, 0);

	// Start VRAM address of 20 columns at bottom of screen for scrolling text
	int nametable_offset = ((23 * 32) + 12);

	// Title screen animation variables
	int i = 0;
	unsigned char scrollingtext[200];
	int frame = 7;
	int pos = 0;
	int framecounter = waits[0];
	int swap = 0;
	unsigned char ball_x = 10;
	int current_velocity = 0;
	int current_vel_id   = 0;
	unsigned int when	 = bounce_velocities[0].when << 4;
	globalcounter = 0;
	buf_id = 0;
	slot = 0;

	rom_to_ram(unwrap(introscroll), scrollingtext);

	unsigned char lyric[96];
	rom_to_ram(lyrics_bank, lyrics_offset, 96, lyric);

	// Prep text
	vdpmemset(nt_buffers[buf_id] + (18 * 32), ' ', 32);
	write_str_double32(nt_buffers[buf_id] + (18 * 32), (char*)lyric);
	write_str_double32(nt_buffers[buf_id] + (20 * 32), (char*)lyric + 32);
	upload_scroll_patterns(0);
	upload_scroll_patterns(0);

	// Put ball
	vdpchar(0x1F80 + (17 * 4) + 0, 0xd0);
	vdpchar(0x1F80 + (17 * 4) + 1, ball_x);
	vdpchar(0x1F80 + (17 * 4) + 2,  64);
	vdpchar(0x1F80 + (17 * 4) + 3,  0xf0 + 15);

	while (!(joyst_result & JOYST_FIRE)) 
	{ 
		// reset the screen timeout (it counts UP by 2, so set it low and odd)
		// This avoids running of the built-in screensaver
   		VDP_SCREEN_TIMEOUT = 1;	

		i = scroll_intro_text(nametable_offset, scrollingtext, i);

		if (framecounter)
			framecounter--;

		if (frame < 7)
			frame++;

		if (!framecounter)
		{
			pos++;
			framecounter = waits[pos];

			if (pos > 42)
				pos = 42;
		}

		if (framecounter == 16)
		{
			frame = 0;
			rom_to_ram(lyrics_bank, lyrics_offset + (pos << 5), 96, lyric);						// Get next lyric
			vdpmemset(nt_buffers[(buf_id) ? 0 : 1] + (18 * 32), ' ', 32);						// Wipe upper line
			write_str_double32(nt_buffers[(buf_id) ? 0 : 1] + (19 * 32), (char*)lyric);			// Write lyric line 1
			write_str_double32(nt_buffers[(buf_id) ? 0 : 1] + (21 * 32), (char*)lyric + 32);	// Write lyrice line 2
			swap = 1;																			// Swap buffers
		}

		if (framecounter == 8)
		{
			frame = 0;
			write_str_double32(nt_buffers[(buf_id) ? 0 : 1] + (18 * 32), (char*)lyric);
			write_str_double32(nt_buffers[(buf_id) ? 0 : 1] + (20 * 32), (char*)lyric + 32);
			vdpmemset(nt_buffers[(buf_id) ? 0 : 1] + (22 * 32), ' ', 32);
			swap = 1;
		}

		upload_scroll_patterns(frame);
		if (swap)
		{
			buf_id = (buf_id) ? 0 : 1;
			VDP_SET_REGISTER(VDP_REG_NT, (buf_id) ? 0x0F : 0x06);
			swap = 0;
		}

		globalcounter++;

		if ((globalcounter > when) && (current_velocity < 184))
		{
			current_vel_id++;
			when += (bounce_velocities[current_vel_id].when << 4);
			current_velocity = bounce_velocities[current_vel_id].velocity;
		}

		VSYNC_PLAY;
		ball_x += current_velocity;
		vdpchar(0x1F80 + (17 * 4) + 0, 134 - ys[globalcounter % 16]);
		vdpchar(0x1F80 + (17 * 4) + 1, ball_x);

		joyst_result = read_joyst(JOYST_1);
	}

	// Pressed fire, make noise!
	StopSong(); StopSfx();
}
