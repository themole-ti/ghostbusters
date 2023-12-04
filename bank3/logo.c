/********************************************/
/* David Crane's Ghostbuster - TI99/4A Port */
/* 2023 - Danny Lousberg   					*/
/********************************************/

#include "globals.h"
#include "bankswitch.h"
#include "input.h"
#include "vdp.h"
#include "trampolines.h"
#include "utils.h"
#include "resource_defs.h"
#include "rnd.h"
#include "map.h"
#include "game_common.h"
#include "TISNPlay.h"
#include "TISfxPlay.h"
#include "sound.h"
#include "resource_copy.h"

inline void enable_laser(int flip)
{
	if (flip)
	{
		int status = VDPST & 0x5f;				// Mask off all bits we don't care about
		while (status != 0x44) 					// 0x44 means 5th sprite flag set, sprite index 4
			status = VDPST & 0x5f ;

		VDP_SET_REGISTER(VDP_REG_COL, 0x22);
		VDP_SET_REGISTER(VDP_REG_NT,  0x07);
		VDP_SET_REGISTER(VDP_REG_COL, 0x33);
		VDP_SET_REGISTER(VDP_REG_COL, 0x33);
		VDP_SET_REGISTER(VDP_REG_COL, 0x22);
		VDP_SET_REGISTER(VDP_REG_COL, 0x11);
	}
	else
	{
		int status = VDPST & 0x5f;				// Mask off all bits we don't care about
		while (status != 0x44) 					// 0x44 means 5th sprite flag set, sprite index 4
			status = VDPST & 0x5f ;

		VDP_SET_REGISTER(VDP_REG_COL, 0x22);
		VDP_SET_REGISTER(VDP_REG_COL, 0x22);
		VDP_SET_REGISTER(VDP_REG_COL, 0x33);
		VDP_SET_REGISTER(VDP_REG_COL, 0x33);
		VDP_SET_REGISTER(VDP_REG_COL, 0x22);
		VDP_SET_REGISTER(VDP_REG_COL, 0x11);
	}
}

void load_logo()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Clear nametable
	vdpmemset(0x1800, 0, 768);
	vdpmemset(0x1C00, 0, 768);

	// Load color, pattern and sprite generator resources from ROM
	rom_to_vram(unwrap(logo_colors), 0xFF * 0x40);
	rom_to_vram(unwrap(logo_patterns), 0x0000);
	rom_to_vram(unwrap(logo_nametable00), 0x1800);

	// Set "THEMOLE" and PRESENTS" text to black color
	vdpmemset((0xFF * 0x40), 0x11, 5);

	// Position tables in VRAM
	VDP_SET_REGISTER(VDP_REG_PDT, 0x00);
	VDP_SET_REGISTER(VDP_REG_CT,  0xFF);
	VDP_SET_REGISTER(VDP_REG_NT,  0x07);
	VDP_SET_REGISTER(VDP_REG_SDT, 0x00);
	VDP_SET_REGISTER(VDP_REG_SAL, 0x5F);

	// Load logo/intro music
	rom_to_ram(unwrap(logo_music), (unsigned char*)levelsongs);
}

#define BEAM_START 0
void _do_logo()
{
	load_logo();

	// Start music playback
	StartSong(levelsongs, 0);

	int y = BEAM_START;
	int counter = 780;
	int dir = 1;

	// Set bitmap mode
	VDP_SET_REGISTER(VDP_REG_MODE0, 0);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_UNBLANK | VDP_MODE1_INT);

	while(counter)
	{ 
		int joyst_result = read_joyst(JOYST_1);
		if (joyst_result & JOYST_FIRE)
			counter = 1;

		VSYNC_PLAY;

		if (counter > (780 - 239))
		{
			if (dir > 0)
				VDP_SET_REGISTER(VDP_REG_NT, 0x06);

			for (int i = 0; i < 5; i++)
				put_4x4_sprite_offset(0, 0, 10, y);

			render_sprites(FALSE, 0x5F);
			y += dir;

			if (y == 120)
				dir = -1;
			if (y == BEAM_START)
				dir = 1;

			enable_laser((dir > 0) ? 1 : 0);

			// Set logo to red color
			vdpchar((0xFF * 0x40) + 0, 0x60);
			vdpchar((0xFF * 0x40) + 1, 0x06);
			vdpchar((0xFF * 0x40) + 2, 0x60);
		}
		else
		{
			if (counter == (780 - 239))
			{
				VDP_SET_REGISTER(VDP_REG_NT, 0x06);
			}

			if (counter == (600 - 285))
			{
				// Set logo to white color
				StartSfx(levelsongs, 1, PRIO);
				vdpchar((0xFF * 0x40) + 0, 0xf0);
				vdpchar((0xFF * 0x40) + 1, 0x0f);
				vdpchar((0xFF * 0x40) + 2, 0xf0);	
			}

			if (counter == (600 - 289))
			{
				// Set logo to red color
				vdpchar((0xFF * 0x40) + 0, 0x60);
				vdpchar((0xFF * 0x40) + 1, 0x06);
				vdpchar((0xFF * 0x40) + 2, 0x60);				
			}

			if (counter == (600 - 345))
			{
				// Set logo to white color
				StartSfx(levelsongs, 1, PRIO);
				vdpchar((0xFF * 0x40) + 0, 0xf0);
				vdpchar((0xFF * 0x40) + 1, 0x0f);
				vdpchar((0xFF * 0x40) + 2, 0xf0);	
			}

			if (counter == (600 - 349))
			{
				// Set logo to red color
				vdpchar((0xFF * 0x40) + 0, 0x60);
				vdpchar((0xFF * 0x40) + 1, 0x06);
				vdpchar((0xFF * 0x40) + 2, 0x60);				
			}

			if (counter == (600 - 405))
			{
				// Set logo to white color
				StartSfx(levelsongs, 1, PRIO);
				vdpchar((0xFF * 0x40) + 0, 0xf0);
				vdpchar((0xFF * 0x40) + 1, 0x0f);
				vdpchar((0xFF * 0x40) + 2, 0xf0);	
			}

			if (counter == (600 - 409))
			{
				// Set logo to red color
				vdpchar((0xFF * 0x40) + 0, 0x60);
				vdpchar((0xFF * 0x40) + 1, 0x06);
				vdpchar((0xFF * 0x40) + 2, 0x60);				
			}

			if (counter == (600 - 415))
			{
				// Set logo to white color
				StartSfx(levelsongs, 1, PRIO);
				vdpchar((0xFF * 0x40) + 0, 0xf0);
				vdpchar((0xFF * 0x40) + 1, 0x0f);
				vdpchar((0xFF * 0x40) + 2, 0xf0);	
			}

			if (counter == (600 - 419))
			{
				// Set logo to red color
				vdpchar((0xFF * 0x40) + 0, 0x60);
				vdpchar((0xFF * 0x40) + 1, 0x06);
				vdpchar((0xFF * 0x40) + 2, 0x60);				
			}

			if (counter == (600 - 425))
			{
				// Set logo to white color
				StartSfx(levelsongs, 1, PRIO);
				vdpchar((0xFF * 0x40) + 0, 0xf0);
				vdpchar((0xFF * 0x40) + 1, 0x0f);
				vdpchar((0xFF * 0x40) + 2, 0xf0);	
			}

			if (counter == 120)
				vdpchar((0xFF * 0x40) + 4, 0x40);
			if (counter == 110)
				vdpchar((0xFF * 0x40) + 4, 0x50);
			if (counter == 100)
				vdpchar((0xFF * 0x40) + 4, 0x70);
			if (counter == 90)
				vdpchar((0xFF * 0x40) + 4, 0xf0);
		}

		if ((!isSNPlaying) && (!isSFXPlaying))
			MUTE_SOUND();

		counter--;
	}

	StopSong();
	StopSfx();
	MUTE_SOUND();

	// Animation done/stopped, make sure all characters are full white
	vdpchar((0xFF * 0x40) + 0, 0xf0);
	vdpchar((0xFF * 0x40) + 1, 0x0f);
	vdpchar((0xFF * 0x40) + 2, 0xf0);	
	vdpchar((0xFF * 0x40) + 4, 0xf0);
	VDP_SET_REGISTER(VDP_REG_NT, 0x06);
	
	// And fade to black
	counter = 70;
	while (counter)
	{
		if (counter == 40)
		{
			vdpchar((0xFF * 0x40) + 0, 0x70);
			vdpchar((0xFF * 0x40) + 1, 0x07);
			vdpchar((0xFF * 0x40) + 2, 0x70);	
			vdpchar((0xFF * 0x40) + 4, 0x70);
		}
		if (counter == 30)
		{
			vdpchar((0xFF * 0x40) + 0, 0x50);
			vdpchar((0xFF * 0x40) + 1, 0x05);
			vdpchar((0xFF * 0x40) + 2, 0x50);	
			vdpchar((0xFF * 0x40) + 4, 0x50);
		}
		if (counter == 20)
		{
			vdpchar((0xFF * 0x40) + 0, 0x40);
			vdpchar((0xFF * 0x40) + 1, 0x04);
			vdpchar((0xFF * 0x40) + 2, 0x40);	
			vdpchar((0xFF * 0x40) + 4, 0x40);
		}
		if (counter == 10)
		{
			vdpchar((0xFF * 0x40) + 0, 0x10);
			vdpchar((0xFF * 0x40) + 1, 0x01);
			vdpchar((0xFF * 0x40) + 2, 0x10);	
			vdpchar((0xFF * 0x40) + 4, 0x10);
		}


		VSYNC_PLAY;
		counter--;
	}
}
