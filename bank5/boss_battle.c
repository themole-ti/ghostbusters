/********************************************/
/* David Crane's Ghostbuster - TI99/4A Port */
/* 2023 - Danny Lousberg   					*/
/********************************************/

#include "globals.h"
#include "bankswitch.h"
#include "input.h"
#include "vdp.h"
#include "TISNPlay.h"
#include "TISfxPlay.h"
#include "sound.h"
#include "trampolines.h"
#include "utils.h"
#include "resource_defs.h"
#include "resource_copy.h"
#include "rnd.h"
#include "trampolines.h"
#include "game_common.h"
#include "boss_battle.h"

#define STAY_PUFT_SPRITE_CNT 24
int sprites_frame[3][STAY_PUFT_SPRITE_CNT] = 
{
	{ 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 15, 16, 17, 18, 23, 24, 25, 26, 31, 32, 63, 63},
	{ 0, 1, 2, 3, 4, 5, 6, 11, 12, 13, 14, 19, 20, 21, 22, 27, 28, 29, 30, 63, 63, 33, 34},
	{ 0, 1, 2, 3, 4, 5, 6, 11, 12,  9, 10, 19, 20, 17, 18, 23, 24, 29, 30, 31, 32, 33, 34}
};

typedef struct
{
	int x, y;
} offset_t;

offset_t base_offsets[3][STAY_PUFT_SPRITE_CNT] =
{
	{
		{  0,  0 }, { 16,  0 }, { 32,  0 },
		{  0, 16 }, { 16, 16 }, { 32, 16 }, { 48, 16 },
		{  0, 32 }, { 16, 32 }, { 32, 32 }, { 48, 32 },
		{  0, 48 }, { 16, 48 }, { 32, 48 }, { 48, 48 },
		{  0, 64 }, { 16, 64 }, { 32, 64 }, { 48, 64 },
		{  0, 80 }, { 16, 80 }, {  0,-64 }, {  0,-64 }
	},
	{
		{  0,  0 }, { 16,  0 }, { 32,  0 },
		{  0, 16 }, { 16, 16 }, { 32, 16 }, { 48, 16 },
		{  0, 32 }, { 16, 32 }, { 32, 32 }, { 48, 32 },
		{  0, 48 }, { 16, 48 }, { 32, 48 }, { 48, 48 },
		{  0, 64 }, { 16, 64 }, { 32, 64 }, { 48, 64 },
		{  0,-64 }, {  0,-64 }, { 32, 80 }, { 48, 80 }
	},
	{
		{  0,  0 }, { 16,  0 }, { 32,  0 },
		{  0, 16 }, { 16, 16 }, { 32, 16 }, { 48, 16 },
		{  0, 32 }, { 16, 32 }, { 32, 32 }, { 48, 32 },
		{  0, 48 }, { 16, 48 }, { 32, 48 }, { 48, 48 },
		{  0, 64 }, { 16, 64 }, { 32, 64 }, { 48, 64 },
		{  0, 80 }, { 16, 80 }, { 32, 80 }, { 48, 80 }
	}
};

offset_t animation_offsets[NFRAMES] = 
{
	{  0,  0  }, {  0, -1  }, {  0, -2  }, {  0, -3  }, {  1, -4  }, {  1, -5  }, {  1, -6  }, 
	{  2, -7  }, {  2, -8  }, {  3, -9  }, {  4, -10 }, {  5, -11 }, {  6, -12 }, {  7, -13 }, 
	{  8, -13 }, {  9, -14 }, { 10, -14 }, { 11,-14  }, { 12, -15 }, { 13, -15 }, { 14, -15 }, 
	{ 15, -15 }, { 16, -15 }, { 17, -15 }, { 18, -15 }, { 19,-15  }, { 20, -14 }, { 21, -14 }, 
	{ 22, -14 }, { 23, -13 }, { 24, -13 }, { 25, -12 }, { 26, -11 }, { 27,-10  }, { 28, -9  }, 
	{ 29, -8  }, { 29, -7  }, { 30, -6  }, { 30, -5  }, { 30, -4  }, { 31, -3  }, { 31, -2  }, 
	{ 31, -1  }
};

// Our player sprite
man runner;

// Know which staypuft animation frame we're showing
int puftframe = 0;

// More quickly render sprites to VRAM, skipping a certain offset
// Can be used when the first 'offset' number of sprites stay 
// static (e.g. when used as a line marker using the 5th sprite flag)
// This version does not do sprite rotation...
inline void render_sprites_fast_offset(int offset)
{
	unsigned char* src = sprite_attribute_list;

	VDP_SET_ADDRESS_WRITE(SAL * 0x80 + (offset << 2));

	while (num_sprites--)
	{
		VDPWD=*(src++);
		VDPWD=*(src++);
		VDPWD=*(src++);
		VDPWD=*(src++);
	}

	// Last byte contains 0xd0 to stop the VDP from rendering junk sprite data from the
	// previous frame
	VDPWD=0xd0;

	// FIXME: figure out why I need to set this explicitely
	num_sprites = 0;
}


void load_boss_battle_screen()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Set all sprites offscreen
	vdpmemset(SAL * 0x80, 0xd0, 128);

	// Prep nametable
	vdpwriteinc(0x1800, 0, 768);

	// Load color, pattern and sprite generator resources from ROM
	ENABLE_MUSIC_ON_COPY;
		rom_to_vram(unwrap(boss_battle_patterns), 0x0000);
		rom_to_vram(unwrap(boss_battle_colors),	0x2000);
		rom_to_vram(unwrap(stay_puft_sprite_patterns), SDT * 0x800);
	DISABLE_MUSIC_ON_COPY;

	// Set bitmap mode
	VDP_SET_REGISTER(VDP_REG_MODE0, VDP_MODE0_BITMAP);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_UNBLANK | VDP_MODE1_INT | VDP_MODE1_SPRMODE16x16);

	// Position tables in VRAM
	VDP_SET_REGISTER(VDP_REG_PDT, 0x03);
	VDP_SET_REGISTER(VDP_REG_CT,  0xFF);
	VDP_SET_REGISTER(VDP_REG_NT,  0x06);
	VDP_SET_REGISTER(VDP_REG_SDT, SDT);
	VDP_SET_REGISTER(VDP_REG_SAL, SAL);
}

#define puft_offset_x (((13 * 8) + 1) + 6)
#define puft_offset_y (( 6 * 8) - 2)

void update_sprite_locs(int framecounter)
{
	int vdp_loc = SAL * 0x80;
	for (int i = 0; i < STAY_PUFT_SPRITE_CNT; i++)
	{
		VDP_SET_ADDRESS_WRITE(vdp_loc);

		if (framecounter < 23)
		{
			if (framecounter == 0)
				StartSfx(levelfx, SOUND_BOOM, 1);
			puftframe = 0;
		}
		else if (framecounter < 29)
		{
			puftframe = 2;
		}
		else
		{
			if (framecounter == NFRAMES-1)
				StartSfx(levelfx, SOUND_BOOM, 1);
			puftframe = 1;
		}
		VDPWD = puft_offset_y + base_offsets[puftframe][i].y + animation_offsets[framecounter].y;
		VDPWD = puft_offset_x + base_offsets[puftframe][i].x + animation_offsets[framecounter].x;
		VDPWD = sprites_frame[puftframe][i] * 4;

		vdp_loc += 4;
	}
}

void put_runner()
{
	int toppatt = 35;
	int patt = 37;

	if (runner.state == 0)
	{
		if (runner.hdir || runner.vdir)
		{
			if (runner.delay == 0)
			{
				if (runner.framecounter == 0)
					runner.framecounter = 1;
				else
					runner.framecounter = 0;

				runner.delay = 5;
			}
			else
				runner.delay--;

			if (runner.framecounter == 0)
				patt = 39;
			else
				patt = 41;
		}

		put_4x4_sprite_offset(patt++, 0x01, runner.x, runner.y + 16);
		put_4x4_sprite_offset(patt, 0x0a, runner.x, runner.y + 16);
	}
	else if (runner.state == 1)
	{
		toppatt = 43;
	}
	put_4x4_sprite_offset(toppatt++, 0x01, runner.x, runner.y);
	put_4x4_sprite_offset(toppatt, 0x0a, runner.x, runner.y);
}

#define foot_width 14
void puft_runner_collission(int framecounter)
{
	offset_t left, right;

	// Determine leg locations
	left.x  = puft_offset_x + animation_offsets[framecounter].x;
	left.y  = puft_offset_y + animation_offsets[framecounter].y;
	right.x = puft_offset_x + animation_offsets[framecounter].x;
	right.y = puft_offset_y + animation_offsets[framecounter].y;
	switch (puftframe)
	{
		case 0:
			left.x  += 14;
			left.y  += 91;
			right.x += 43;
			right.y += 80;
			break;
		case 1:
			left.x  +=  7;
			left.y  += 80;
			right.x += 36;
			right.y += 91;
			break;
		default:
			left.x  += 14;
			left.y  += 91;
			right.x += 36;
			right.y += 91;
			break;
	} 

	// Check left leg
	if (runner.y <= left.y)
	{
		int check_x = runner.x + 8;
		if ((check_x > left.x) && (check_x < (left.x + foot_width)))
		{
			runner.state =  1;
			runner.hdir  = -2;
			runner.vdir  =  1;
			runner.framecounter = 20;
		}
	}

	// Check right leg
	if (runner.y <= right.y)
	{
		int check_x = runner.x + 8;
		if ((check_x > right.x) && (check_x < (right.x + foot_width)))
		{
			runner.state =  1;
			runner.hdir  =  2;
			runner.vdir  =  1;
			runner.framecounter = 20;
		}
	}
}

void _do_boss_battle_screen()
{
	load_boss_battle_screen();
	int success = 0;

	// Pre-load sprites attributes into VRAM
	VDP_SET_ADDRESS_WRITE(SAL * 0x80);
	for (int i = 0; i < STAY_PUFT_SPRITE_CNT; i++)
	{
		VDPWD = puft_offset_y + base_offsets[0][i].y;
		VDPWD = puft_offset_x + base_offsets[0][i].x;
		VDPWD = sprites_frame[0][i] * 4;
		VDPWD = 0x0f;
	}

	game.men = 3;

	while ((game.men > 1) && (success < 2))
	{
		int loop = 1;

		runner.x 		= 112;
		runner.y 		= 158;
		runner.hdir 	= 0;
		runner.vdir 	= 0;
		runner.state 	= 0;

		while(loop)
		{
			static int framecounter = 0;
			static int dir = 1;

			int joyst_result = read_joyst(JOYST_1);
			if (runner.state == 0)
			{
				if (joyst_result & JOYST_LEFT)
				{
					runner.hdir = -1;
				}
				else if (joyst_result & JOYST_RIGHT)
				{
					runner.hdir =  1;
				}
				if (joyst_result & JOYST_UP)
				{
					runner.vdir = -1;
				}
				else if (joyst_result & JOYST_DOWN)
				{
					runner.vdir =  1;
				}
			}

			if (runner.state == 0)
			{
				// Normal vertical limits: between 158 and 128
				if ((runner.y + runner.vdir) > 158)
				{
					runner.y    = 158;
					runner.vdir = 0;
				}
				else if ((runner.y + runner.vdir) < 128)
				{
					// Check if we're within the door area, there we can go higher up the screen
					if ((runner.x > 136) && (runner.x < 161))
					{
						if ((runner.y + runner.vdir) < 110)
						{
							// You're in!
							runner.state = 2;
							success++;
							StartSfx(levelfx, SOUND_BLIP, 1);
						}
					}
					else
					{
						runner.y    = 128;
						runner.vdir = 0;
					}
				}

				// Horizontal limits, if we're already higher up the screen, the limits are tigher
				if ((runner.y + runner.vdir) < 128)
				{
					if ((runner.x + runner.hdir) > 160)
					{
						runner.x 	= 160;
						runner.hdir = 0;
					}
					else if ((runner.x + runner.hdir) <  130)
					{				
						runner.x 	= 130;
						runner.hdir = 0;
					}
				}
				else
				{
					if ((runner.x + runner.hdir) > 230)
						runner.hdir = 0;
					else if ((runner.x + runner.hdir) <  20)
						runner.hdir = 0;
				}
			}

			runner.x += runner.hdir;
			runner.y += runner.vdir;

			if (runner.state == 0)
				puft_runner_collission(framecounter >> 1);

			put_runner();

			if (runner.state == 0)
			{
				runner.hdir = 0;
				runner.vdir = 0;
			}
			else if (runner.state == 1)
			{
				if (runner.framecounter)
				{
					runner.framecounter--;
				}
				else
				{
					runner.hdir = 0;
					runner.vdir = 0;
					game.men--;
					loop = 0;
				}
			}
			else if (runner.state == 2)
			{
				if (runner.framecounter)
				{
					runner.framecounter--;
				}
				else
				{
					runner.hdir = 0;
					runner.vdir = 0;
					loop = 0;
				}
			}

			framecounter += dir;
			if ((framecounter >= ((NFRAMES - 1) << 1)) || (framecounter == 0)) 
				dir = -dir;

			VSYNC_PLAY;
			update_sprite_locs(framecounter >> 1);
			render_sprites_fast_offset(STAY_PUFT_SPRITE_CNT);
		}
	}

	int counter = 120;
	if (success > 1)
	{
		while (counter)
		{
			if (counter == 120)
			{
				// Sprite pattern offset for smile = 144 bytes
				unsigned char left[] = { 0xE0, 0xC0, 0x80, 0x00, 0x3F, 0x7F, 0xFF, 0x7F };
				unsigned char right[] = { 0x07, 0x03, 0x01, 0x00, 0xFC, 0xFE, 0xFF, 0xFE };
				vdpmemcpy(0x3800 + 144, left, 8);
				vdpmemcpy(0x3800 + 160, right, 8);
				StopSong(); MUTE_SOUND();
			}

			if (counter == 60)
			{
				VDP_SET_REGISTER(VDP_REG_COL, 0xff);
				StartSfx(levelfx, SOUND_BOOM, 1);
			}

			if (counter == 55)
				VDP_SET_REGISTER(VDP_REG_COL, 0x11);

			if (counter == 30)
			{
				VDP_SET_REGISTER(VDP_REG_COL, 0xff);
				StartSfx(levelfx, SOUND_BOOM, 1);
			}

			if (counter == 25)
				VDP_SET_REGISTER(VDP_REG_COL, 0x11);

			if (counter == 20)
			{
				VDP_SET_REGISTER(VDP_REG_COL, 0xff);
				StartSfx(levelfx, SOUND_BOOM, 2);
			}

			if (counter == 15)
				VDP_SET_REGISTER(VDP_REG_COL, 0x11);

			if (counter == 10)
			{
				StartSfx(levelfx, SOUND_BOOM, 1);
			}

			counter--;
			VSYNC_PLAY;
		}
	}
}
