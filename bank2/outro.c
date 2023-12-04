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

void load_outro()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Set all sprites offscreen
	vdpmemset(SAL * 0x80, 0xd0, 128);

	// Load color, pattern and sprite generator resources from ROM
	ENABLE_MUSIC_ON_COPY;
		rom_to_vram(zuul_nametable00_bank,	zuul_nametable00_offset + 4320, 768, 0x1800);
		VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );
	DISABLE_MUSIC_ON_COPY;
	rom_to_vram(unwrap(zuul_patterns), 0x0000);	// upper third
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );
	rom_to_vram(unwrap(zuul_patterns), 0x0800);	// middle third
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_WHITE << 4 | COLOR_WHITE) );
	rom_to_vram(unwrap(zuul_patterns), 0x1000);	// lower third
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );
	rom_to_vram(unwrap(zuul_colors), 0x2000);	// upper third
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_WHITE << 4 | COLOR_WHITE) );
	rom_to_vram(unwrap(zuul_colors), 0x2800);	// middle third
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );
	rom_to_vram(unwrap(zuul_colors), 0x3000);	// lower third
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

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

unsigned char men_patts_l[3][4] =
{
	{ 0, 6, 12, 18 },
	{ 0, 7, 12, 19 },
	{ 2, 8, 14, 20 }
};

unsigned char men_patts_r[3][4] =
{
	{ 3,  9, 15, 21 },
	{ 3, 10, 15, 22 },
	{ 5, 11, 17, 23 }
};

unsigned char beam_colors[4] = { 0x7, 0xd, 0x2, 0xf };

#define STATE_SCROLLING  			0
#define STATE_POSITIONING_MAN_1		1
#define STATE_POSITIONING_MAN_2		2
#define STATE_FIRING				3
#define STATE_WAITING 				6
#define STATE_DONE 					7
void _do_outro()
{
	int nt_buffers[2];
	nt_buffers[0] = 0x1800;
	nt_buffers[1] = 0x3C00;

	int currentbuffer = 1;
	int nt_offset = 4320;
	int sprites_loaded = 0;
	int state = STATE_SCROLLING;
	int counter = 120;

	int framecounter = 0;
	int x = 255;

	load_outro();

	while (state != STATE_DONE)
	{ 
		if (nt_offset > 0)
		{
			nt_offset -= 32;

			rom_to_vram(zuul_nametable00_bank,	zuul_nametable00_offset + nt_offset      , 384, nt_buffers[currentbuffer]);
			VSYNC_PLAY;
			rom_to_vram(zuul_nametable00_bank,	zuul_nametable00_offset + nt_offset + 384, 384, nt_buffers[currentbuffer] + 384);
			VSYNC_PLAY; 
			currentbuffer = !currentbuffer;
			VDP_SET_REGISTER(VDP_REG_NT, (currentbuffer) ? 0x06 : 0x0F);
		}
		else
		{
			if (!sprites_loaded)
			{
				rom_to_vram(zuul_nametable00_bank,	zuul_nametable00_offset + nt_offset      , 384, nt_buffers[currentbuffer]);
				VSYNC_PLAY;
				rom_to_vram(zuul_nametable00_bank,	zuul_nametable00_offset + nt_offset + 384, 384, nt_buffers[currentbuffer] + 384);
				VSYNC_PLAY; 
				currentbuffer = !currentbuffer;
				VDP_SET_REGISTER(VDP_REG_NT, (currentbuffer) ? 0x06 : 0x0F);
				rom_to_vram(unwrap(main_sprites), SDT * 0x800);
				sprites_loaded = 1;
			}
			else
			{
				switch (state)
				{
					case STATE_SCROLLING:
						{
							counter = 100;
							state = STATE_POSITIONING_MAN_1;
							framecounter = 0;
							x = 255;
						}
						break;
					case STATE_POSITIONING_MAN_1:
						{
							if (counter == 0)
							{
								put_4x4_sprite_offset(men_patts_r[2][0], 0x1, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_r[2][1], 0x1, x, 140 + 16);
								put_4x4_sprite_offset(men_patts_r[2][2], 0x9, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_r[2][3], 0xa, x, 140 + 16);
								state = STATE_POSITIONING_MAN_2;
								counter = 40;
								x = 255;
								framecounter = 0;
							}
							else
							{
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][0], 0x1, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][1], 0x1, x, 140 + 16);
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][2], 0x9, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][3], 0xa, x, 140 + 16);
								framecounter++;
								if (framecounter > (1 << 4))
									framecounter = 0;

								x -= 1;
								counter--;
							}
						}
						break;
					case STATE_POSITIONING_MAN_2:
						{
							put_4x4_sprite_offset(men_patts_r[2][0], 0x1, 155, 140 +  0);
							put_4x4_sprite_offset(men_patts_r[2][1], 0x1, 155, 140 + 16);
							put_4x4_sprite_offset(men_patts_r[2][2], 0x9, 155, 140 +  0);
							put_4x4_sprite_offset(men_patts_r[2][3], 0xa, 155, 140 + 16);

							if (counter == 0)
							{
								put_4x4_sprite_offset(men_patts_l[2][0], 0x1, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_l[2][1], 0x1, x, 140 + 16);
								put_4x4_sprite_offset(men_patts_l[2][2], 0x9, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_l[2][3], 0xa, x, 140 + 16);
								counter = 180;
								state = STATE_FIRING;
							}
							else
							{
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][0], 0x1, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][1], 0x1, x, 140 + 16);
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][2], 0x9, x, 140 +  0);
								put_4x4_sprite_offset(men_patts_l[(framecounter & 0x08) ? 0 : 1][3], 0xa, x, 140 + 16);
								framecounter++;
								if (framecounter > (1 << 4))
									framecounter = 0;

								x -= 1;
								counter--;
							}
						}
						break;
					case STATE_FIRING:
						{
							int tmpcolor;
							if (counter < 120)
							{
								if (counter == 119)
									StartSfx(levelfx, SOUND_BEAM, 0);

								for (int i = 0; i < 5; i++)
								{
									tmpcolor = beam_colors[fast_rnd(4)];
									int x = 215 - (i * 8) - 8;
									put_4x4_sprite_offset(beam_pattern_start + fast_rnd(3), tmpcolor, x, 140 - (16 * i) - 8);
								}

							}
							if (counter < 60)
							{
								for (int i = 0; i < 5; i++)
								{
									tmpcolor = beam_colors[fast_rnd(4)];
									int x = 179 + (i * 8) - 8;
									put_4x4_sprite_offset(beam_pattern_start + fast_rnd(3) + 3, tmpcolor, x, 140 - (16 * i) - 8);
								}

							}

							put_4x4_sprite_offset(men_patts_r[2][0], 0x1, 155, 140 +  0);
							put_4x4_sprite_offset(men_patts_r[2][1], 0x1, 155, 140 + 16);
							put_4x4_sprite_offset(men_patts_r[2][2], 0x9, 155, 140 +  0);
							put_4x4_sprite_offset(men_patts_r[2][3], 0xa, 155, 140 + 16);
							put_4x4_sprite_offset(men_patts_l[2][0], 0x1, 215, 140 +  0);
							put_4x4_sprite_offset(men_patts_l[2][1], 0x1, 215, 140 + 16);
							put_4x4_sprite_offset(men_patts_l[2][2], 0x9, 215, 140 +  0);
							put_4x4_sprite_offset(men_patts_l[2][3], 0xa, 215, 140 + 16);

							counter--;
							if (counter == 0)
							{
								StartSfx(levelfx, SOUND_BOOM, 1);
								vdpmemset(nt_buffers[!currentbuffer] + 288 + 24, 11, 3);
							}
							if (counter == -20)
							{
								StartSfx(levelfx, SOUND_BOOM, 2);
								vdpmemset(nt_buffers[!currentbuffer] + 320 + 24, 11, 3);
							}
							if (counter == -40)
							{
								StartSfx(levelfx, SOUND_BOOM, 3);
								vdpmemset(nt_buffers[!currentbuffer] + 352 + 24, 11, 3);
							}
							if (counter == -60)
							{
								StartSfx(levelfx, SOUND_BOOM, 4);
								vdpmemset(nt_buffers[!currentbuffer] + 384 + 24, 11, 3);
							}
							if (counter == -80)
							{
								StartSfx(levelfx, SOUND_BOOM, 5);
								vdpmemset(nt_buffers[!currentbuffer] + 416 + 24, 11, 3);
							}
							if (counter == -90)
							{
								StopSfx();
								StartSfx(levelfx, SOUND_BEAM, 6);
							}
							if (counter == -140)
							{
								StopSfx();
								state = STATE_WAITING;
								counter = 60;
							}
						}						
						break;
					case STATE_WAITING:
						counter--;
						put_4x4_sprite_offset(men_patts_r[2][0], 0x1, 155, 140 +  0);
						put_4x4_sprite_offset(men_patts_r[2][1], 0x1, 155, 140 + 16);
						put_4x4_sprite_offset(men_patts_r[2][2], 0x9, 155, 140 +  0);
						put_4x4_sprite_offset(men_patts_r[2][3], 0xa, 155, 140 + 16);
						put_4x4_sprite_offset(men_patts_l[2][0], 0x1, 215, 140 +  0);
						put_4x4_sprite_offset(men_patts_l[2][1], 0x1, 215, 140 + 16);
						put_4x4_sprite_offset(men_patts_l[2][2], 0x9, 215, 140 +  0);
						put_4x4_sprite_offset(men_patts_l[2][3], 0xa, 215, 140 + 16);

						if (counter == 50)
							VDP_SET_REGISTER(VDP_REG_COL, 0x11);

						if (counter == 0)
							state = STATE_DONE;
						break;
				}

				render_sprites(FALSE, SAL);
				VSYNC_PLAY;
			}
		}
	}
}
