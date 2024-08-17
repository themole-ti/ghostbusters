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

int bait_layed = 0;

unsigned char bldg_gfx[900];
unsigned char bldg_destroyed[25];

unsigned int stomp_function[] = 
{
	16, 398, 756, 1090, 1400, 1689, 1956, 2203, 2430, 
	2638, 2828, 3000, 3157, 3297, 3423, 3535, 3634, 
	3720, 3795, 3859, 3914, 3959, 3996, 4026, 4049, 
	4066, 4079, 4087, 4092, 4095, 4096, 4096, 4096, 
	4096, 4096, 4095, 4092, 4087, 4077, 4061, 4036, 
	3744, 3694, 3371, 3282, 2912, 2768, 2590, 2373, 
	2110, 1797, 1172, 739, 237, -343, -1264, -856, 
	-448, -1112, -1520, -1112, -960, -1368, -1776
};

void destroy_building(int id)
{
	int vaddr = 0x1800 + ((buildings[id].y) << 5) + (buildings[id].x);
	for (int i = 0; i < 5; i++)
	{
		vdpmemcpy(vaddr, bldg_destroyed + (i * 5), 5);
		vaddr += 32;
	}

	buildings[id].destroyed = TRUE;
}

void destroy_building_row(int id, int row)
{
	int vaddr = 0x1800 + ((buildings[id].y) << 5) + (buildings[id].x) + (row * 32);
	vdpmemcpy(vaddr, bldg_destroyed + (row * 5), 5);
}

// We use a function-like macro here to avoid the function
// call overhead if we don't need to set a new color
#define colorize_building(id, clr)					\
{													\
	if (buildings[(id)].color != (clr))				\
	{												\
		buildings[(id)].color = (clr);				\
		colorize_building_real((id), (clr));		\
	}												\
}

void colorize_building_real(int id, int color)
{
	int iterations = (buildings[id].type > BLDG_TYPE_ZUUL) ? 1 : 5;

	// We have the data link now, let's re-color the building
	int vaddr = 0x1800 + ((buildings[id].y) << 5) + (buildings[id].x);
	for (int i = 0; i < iterations; i++)
	{
		vdpmemcpy(vaddr, bldg_gfx + (color * 225) + (buildings[id].type * 25) + (i * 5), 5);
		vaddr += 32;
	}
}

void load_map()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Set all sprites offscreen
	vdpmemset(SAL * 0x80, 0xd0, 128);

	ENABLE_MUSIC_ON_COPY;
		// Load color, pattern and sprite generator resources from ROM
		rom_to_vram(map_nw_nametable00_bank,	map_nw_nametable00_offset, 	768,  						0x1800);
		rom_to_vram(map_nw_patterns_bank, 		map_nw_patterns_offset, 	map_nw_patterns_length,		0x0000);	// upper third
		rom_to_vram(map_nw_patterns_bank, 		map_nw_patterns_offset, 	map_nw_patterns_length,		0x0800);	// middle third
		rom_to_vram(map_nw_patterns_bank, 		map_nw_patterns_offset, 	map_nw_patterns_length,		0x1000);	// lower third
		rom_to_vram(map_nw_colors_bank, 		map_nw_colors_offset, 		map_nw_colors_length,		0x2000);	// upper third
		rom_to_vram(map_nw_colors_bank, 		map_nw_colors_offset, 		map_nw_colors_length,		0x2800);	// middle third
		rom_to_vram(map_nw_colors_bank, 		map_nw_colors_offset, 		map_nw_colors_length,		0x3000);	// lower third
		rom_to_vram(main_sprites_bank, 			main_sprites_offset, 		main_sprites_length,		SDT * 0x800);

		// Pre-display the game state in the bar below
		if (scroll_len < 1)
		{
			if (game.account < 9)
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000     $ 00", 32);
			else if (game.account < 99)
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000    $  00", 32);
			else if (game.account < 999)
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000   $   00", 32);
			else
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000  $    00", 32);
			printnum(0x1800 + 724, game.pk);
			printnum(0x1800 + 731, game.account);
		}

		// Load building variant templates in RAM
		rom_to_ram(map_nw_nametable01_bank,	map_nw_nametable01_offset      , 900, (void*)bldg_gfx);
		rom_to_ram(map_nw_nametable01_bank,	map_nw_nametable01_offset + 900,  25, (void*)&bldg_destroyed);
	DISABLE_MUSIC_ON_COPY;

	// Pre-color and destroy buildings based on game map state
	for (int i = 0; i < NUM_BUILDINGS; i++)
	{
		if (buildings[i].destroyed)
			destroy_building(i);
	}

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

int is_horizontal_street(int y)
{
	if (y == 11) return 1;
	if (y == 59) return 1;
	if (y == 107) return 1;
	if (y == 155) return 1;

	return 0;
}

int is_vertical_street(int x)
{
	if (x == 28) return 1;
	if (x == 76) return 1;
	if (x == 124) return 1;
	if (x == 172) return 1;
	if (x == 220) return 1;

	return 0;
}

void lay_dot(int x, int y, int pattern)
{
	// Convert pixel coords to character coords
	x = (x >> 3);
	y = (y >> 3);

	// Don't put dots on street names
	if (x < 6)
		if ((y == 1) || (y == 7) || (y == 13) || (y == 19))
			return;

	int addr = 0x1800 + ((y + 1) * 32) + (x + 1);

	// TODO: find a quicker (non-vdpgetch) way to check if we need to lay down a dot...
	if (vdpgetch(addr) != bait_patt)
		vdpchar(addr, pattern);
}

inline int is_next_to_building(int id)
{
	unsigned int x = (player.x >> 3) + 1;
	unsigned int y = (player.y >> 3) + 1;

	if ((x >= (buildings[id].x - 1)) &&
		(x <= (buildings[id].x + 5)))
	{
		if ((y >= (buildings[id].y - 1)) &&
			(y <= (buildings[id].y + 5)))
		{
			return 1;
		}
	}

	return 0;
}

inline void blink_buildings()
{
	for (int i = 0; i < NUM_BUILDINGS; i++)
	{
		// Is building (about to be) haunted
		if (buildings[i].haunted)
		{
			if (buildings[i].prehaunt)
			{
				if (game.meter)
				{
					// Check if player is next to building
					int do_color = is_next_to_building(i);

					if (do_color)
					{
						colorize_building(i, BLDG_MAGENTA);
					}
					else
					{
						colorize_building(i, BLDG_NORMAL);
					}
				}
			}
			else
			{
				int toggleval = (buildings[i].haunted + gSaveIntCnt) & 0x001f;
				if (toggleval == 0x10)
				{
					colorize_building(i, BLDG_RED);
				}
				else if (toggleval == 0)
				{
					colorize_building(i, BLDG_NORMAL);
				}
				if (buildings[i].haunted == 1)	// FIXME: Buildings stay lit after being haunted when you don't leave the map
				{								//        This check is designed to mitigate that...
												//        We can't check for 'zero' here, because all unhaunted buildings are
												//        also zero. But we would stay at 1 for multiple frames
												//        which would kill framerate during those frames. So, we duplicate
												//        the common game logic (including taking the penalty) here.
												//        This is BAD!
					colorize_building(i, BLDG_NORMAL);
					buildings[i].haunted = 0;			// TODO: This parts specifically is what decentralizes the logic
					game.pk += GHOST_LEFT_PK_PENALTY; 	//       as mentioned above.
				}
			}
		}
	}

	// Is next building about to be attacked by stay puft and are we close
	if (game.detector)
	{
		if ((staypuft_attacks[currentpuft].pk_level - game.pk) < MARSHMALLOW_SENSOR_TIME)
		{
			if (buildings[staypuft_attacks[currentpuft].building_id].haunted)
				staypuft_attacks[currentpuft].pk_level += buildings[staypuft_attacks[currentpuft].building_id].haunted + 50;
			else
			{
				int i = staypuft_attacks[currentpuft].building_id;
				if (is_next_to_building(i))
				{
					colorize_building(i, BLDG_WHITE);
				}
				else
				{
					colorize_building(i, BLDG_NORMAL);
				}
			}
		}
	}
}

void do_player_movement(int joyst_result)
{
	// Process joystick input
	if (player.vdir)
	{
		player.vdir = 0;
		player.hdir = 0;

		if ((joyst_result & JOYST_LEFT) && is_horizontal_street(player.y))
			player.hdir = -1;
		else if ((joyst_result & JOYST_RIGHT) && is_horizontal_street(player.y))
			player.hdir =  1;
		else
		{
			if (joyst_result & JOYST_UP)
				player.vdir = -1;
			else if (joyst_result & JOYST_DOWN)
				player.vdir =  1;
		}
	}
	else if (player.hdir)
	{
		player.vdir = 0;
		player.hdir = 0;

		if ((joyst_result & JOYST_UP) && is_vertical_street(player.x))
			player.vdir = -1;
		else if ((joyst_result & JOYST_DOWN) && is_vertical_street(player.x))
			player.vdir =  1;
		else
		{
			if (joyst_result & JOYST_LEFT)
				player.hdir = -1;
			else if (joyst_result & JOYST_RIGHT)
				player.hdir =  1;
		}
	}
	else
	{
		if ((joyst_result & JOYST_LEFT) && is_horizontal_street(player.y))
			player.hdir = -1;
		else if ((joyst_result & JOYST_RIGHT) && is_horizontal_street(player.y))
			player.hdir =  1;

		if ((joyst_result & JOYST_UP) && is_vertical_street(player.x))
			player.vdir = -1;
		else if ((joyst_result & JOYST_DOWN) && is_vertical_street(player.x))
			player.vdir =  1;
	}

	player.x += player.hdir;
	player.y += player.vdir;

	if (player.x > 220) player.x = 220;
	if (player.x <  28) player.x =  28;
	if (player.y > 155) player.y = 155;
	if (player.y <  11) player.y =  11;
}

void calculate_distance()
{
	// Reset previously calculated distance, if any
	distance = 0;

	// Check horizontal streets for dots
	int row = 2;

	for (int i = 0; i < 5; i++)
	{
		for (int col = 7; col < 29; col++)
		{
			unsigned char pattern = vdpgetch(0x1800 + (row * 32) + col);
			if (pattern == dot_patt)
			{
				distance += game.carspeed;
			}
		}
		row += 6;
	}

	// Check horizontal streets for dots
	int col = 4;

	for (int i = 0; i < 5; i++)
	{
		for (int row = 2; row < 21; row++)
		{
			unsigned char pattern = vdpgetch(0x1800 + (row * 32) + col);
			if (pattern == dot_patt)
			{
				distance += game.carspeed;
			}
		}
		col += 6;
	}
}

void check_collisions()
{
	for (int i = 0; i < MAX_ROAMERS; i++)
	{
		if (!roamers[i].frozen)
		{
			int check_x = (roamers[i].x >> 8) + 1;
			int check_y = (roamers[i].y >> 8) + 1;

			if ((abs(player.x - check_x) < BBOX)
				&& (abs(player.y - check_y) < BBOX))
			{
				// Touched a roamer, make it frozen and record position
				roamers[i].frozen = TRUE;
				calculate_distance();
				roamers[i].distance = distance;
			}
		}
	}
}

void goto_building(int id)
{
	(void)id;

	calculate_distance();

	// // Pressed fire, make noise!
	// StopSong(); StopSfx();
	// MUTE_SOUND();
}

inline void calculate_dxdy(int roamerid, int x, int y, int steps)
{
	if (roamers[roamerid].x > x)
		roamers[roamerid].dx = -((roamers[roamerid].x - x) >> steps);
	else
		roamers[roamerid].dx =  ((x - roamers[roamerid].x) >> steps);

	if (roamers[roamerid].y < y)
		roamers[roamerid].dy = ((y - roamers[roamerid].y) >> steps);
	else
		roamers[roamerid].dy = -((roamers[roamerid].y - y) >> steps);
}

inline void position_marshmallow_man(int x, int y)
{
	roamers[0].sp_patt1 = 39;
	roamers[0].sp_patt2 = 41;
	roamers[0].x = x - 0x0800;
	roamers[0].y = y - 0x0800;
	roamers[1].sp_patt1 = 40;
	roamers[1].sp_patt2 = 42;
	roamers[1].x = x + 0x0800;
	roamers[1].y = y - 0x0800;
	roamers[2].sp_patt1 = 37;
	roamers[2].sp_patt2 = 44;
	roamers[2].x = x - 0x0800;
	roamers[2].y = y + 0x0800;
	roamers[3].sp_patt1 = 46;
	roamers[3].sp_patt2 = 48;
	roamers[3].x = x + 0x0800;
	roamers[3].y = y + 0x0800;
	roamers[0].color1 = 0x1;
	roamers[0].color2 = 0xf;
	roamers[1].color1 = 0x1;
	roamers[1].color2 = 0xf;
	roamers[2].color1 = 0x1;
	roamers[2].color2 = 0xf;
	roamers[3].color1 = 0x1;
	roamers[3].color2 = 0xf;
}

inline void migrate_roamers(int framecounter)
{
	if (framecounter != 0)
	{
		for (int i = 0; i < 4; i++)
		{
			roamers[i].x += roamers[i].dx;
			roamers[i].y += roamers[i].dy;
		}
	}
}

typedef struct
{
	unsigned int x, y;
} point2d;

#define PFT_STATE_WAITING    0
#define PFT_STATE_GATHERING  1
#define PFT_STATE_DESTROYING 2
#define PFT_STATE_LURING     3
#define PFT_STATE_DESTROYED  4
#define PFT_STATE_LURED      5
int check_staypuft(int bait)
{
	// Don't handle more than 5 puft attacks
	if (currentpuft > 4)
		return 0;

	static int state = PFT_STATE_WAITING;
	static int framecounter = 0;
	static int ys[4] = { 0 };

	static point2d origins[4];

	if (bait && game.bait && (bait_layed == 0))
	{
		if (state == PFT_STATE_GATHERING)
		{
			framecounter = 64;
			state = PFT_STATE_LURING;
		}

		game.bait--;
		if (is_horizontal_street(player.y))
		{
			if (player.x < 128)
				lay_dot(player.x + 12, player.y, bait_patt);
			else
				lay_dot(player.x - 12, player.y, bait_patt);
		}
		else
		{
			if (player.y < 96)
				lay_dot(player.x, player.y + 12, bait_patt);
			else
				lay_dot(player.x, player.y - 12, bait_patt);
		}
		bait_layed = 120;
		StartSfx(levelfx, SOUND_BLIP, 1);
	}
	if (bait && (game.bait == 0) && (bait_layed == 0))
	{
		StartSfx(levelfx, SOUND_BRRR, 1);
		scroll_status_text(0x1800 + 704, (char*)bait_msg, strlength((unsigned char*)bait_msg));
	}

	switch(state)
	{
		case PFT_STATE_WAITING:
			if ((staypuft_attacks[currentpuft].pk_level < game.pk) 
				&& (buildings[staypuft_attacks[currentpuft].building_id].haunted == 0)
				&& (buildings[staypuft_attacks[currentpuft].building_id].destroyed == 0))
			{
				// Calculate new dx and dy for all roamers
				int x = ((buildings[staypuft_attacks[currentpuft].building_id].x * 8) + 12) << 8;
				int y = ((buildings[staypuft_attacks[currentpuft].building_id].y * 8) - 12) << 8;

				for (int i = 0; i < 4; i++)
				{
					int xx, yy;
					switch (i)
					{
						case 0:
							xx = x - 0x0800;
							yy = y - 0x0800;
							break;
						case 1:
							xx = x + 0x0800;
							yy = y - 0x0800;
							break;
						case 2:
							xx = x - 0x0800;
							yy = y + 0x0800;
							break;
						case 3:
							xx = x + 0x0800;
							yy = y + 0x0800;
							break;
					}

					calculate_dxdy(i, xx, yy, 6);		// 6 -> 2^6 = 64 steps

					origins[i].x = roamers[i].x;
					origins[i].y = roamers[i].y;
				}

				// Show warning message
				scroll_status_text(0x1800 + 704, (char*)puftwarning, strlength((unsigned char*)puftwarning));

				// Set number of expected frames in next state
				framecounter = 64;

				// Go to next state
				state = PFT_STATE_GATHERING;
				return 1;
			}
			break;
		case PFT_STATE_GATHERING:
			framecounter--;

			int x = ((buildings[staypuft_attacks[currentpuft].building_id].x * 8) + 12) << 8;
			int y = ((buildings[staypuft_attacks[currentpuft].building_id].y * 8) - 12) << 8;

			migrate_roamers(framecounter);

			if (framecounter == 0)
			{
				position_marshmallow_man(x, y);

				// Record y coords
				for (int i = 0; i < 4; i++)
				{
					ys[i] = roamers[i].y;
				}

				// Go to destroying state
				state = PFT_STATE_DESTROYING;
				framecounter = 256;	// ~4 seconds
			}

			return 1;
			break;
		case PFT_STATE_DESTROYING:
			framecounter--;

			if (framecounter > 192)
			{
				// Destroying layer 1
				int offset = framecounter - 193;

				roamers[0].y = ys[0] - stomp_function[offset] - (14 << 8);
				roamers[1].y = ys[1] - stomp_function[offset] - (14 << 8);
				roamers[2].y = ys[2] - stomp_function[offset] - (14 << 8);
				roamers[3].y = ys[3] - stomp_function[offset] - (14 << 8);

				if (framecounter == 193)
				{
					for (int i = 0; i < 4; i++)
						ys[i] += 8 << 8;

					roamers[2].sp_patt1 = 45;
					roamers[2].sp_patt2 = 47;
					roamers[3].sp_patt1 = 38;
					roamers[3].sp_patt2 = 50;

					StartSfx(levelfx, SOUND_BOOM, 1);
					destroy_building_row(staypuft_attacks[currentpuft].building_id, 0);
				}
			}
			else if (framecounter > 128)
			{
				// Destroying layer 2
				int offset = framecounter - 129;

				roamers[0].y = ys[0] - stomp_function[offset] - (14 << 8);
				roamers[1].y = ys[1] - stomp_function[offset] - (14 << 8);
				roamers[2].y = ys[2] - stomp_function[offset] - (14 << 8);
				roamers[3].y = ys[3] - stomp_function[offset] - (14 << 8);

				if (framecounter == 129)
				{
					for (int i = 0; i < 4; i++)
						ys[i] += 8 << 8;

					roamers[2].sp_patt1 = 37;
					roamers[2].sp_patt2 = 44;
					roamers[3].sp_patt1 = 46;
					roamers[3].sp_patt2 = 48;

					StartSfx(levelfx, SOUND_BOOM, 1);
					destroy_building_row(staypuft_attacks[currentpuft].building_id, 1);
				}
			}
			else if (framecounter > 64)
			{
				// Destroying layer 3
				int offset = framecounter - 65;

				roamers[0].y = ys[0] - stomp_function[offset] - (14 << 8);
				roamers[1].y = ys[1] - stomp_function[offset] - (14 << 8);
				roamers[2].y = ys[2] - stomp_function[offset] - (14 << 8);
				roamers[3].y = ys[3] - stomp_function[offset] - (14 << 8);

				if (framecounter == 65)
				{
					for (int i = 0; i < 4; i++)
						ys[i] += 8 << 8;

					roamers[2].sp_patt1 = 45;
					roamers[2].sp_patt2 = 47;
					roamers[3].sp_patt1 = 38;
					roamers[3].sp_patt2 = 50;

					StartSfx(levelfx, SOUND_BOOM, 1);
					destroy_building_row(staypuft_attacks[currentpuft].building_id, 2);
				}
			}
			else if (framecounter > 0)
			{
				// Destroying layer 4
				int offset = framecounter - 1;

				roamers[0].y = ys[0] - stomp_function[offset] - (14 << 8);
				roamers[1].y = ys[1] - stomp_function[offset] - (14 << 8);
				roamers[2].y = ys[2] - stomp_function[offset] - (14 << 8);
				roamers[3].y = ys[3] - stomp_function[offset] - (14 << 8);

				if (framecounter == 1)
				{
					for (int i = 0; i < 4; i++)
						ys[i] += 8 << 8;

					roamers[3].sp_patt1 = 46;
					roamers[3].sp_patt2 = 48;

					StartSfx(levelfx, SOUND_BOOM, 1);
					destroy_building_row(staypuft_attacks[currentpuft].building_id, 3);
					destroy_building_row(staypuft_attacks[currentpuft].building_id, 4);
				}
			}
			else
			{
				// Go to next state
				state = PFT_STATE_DESTROYED;
				framecounter = 128;
			}
			return 1;
			break;
		case PFT_STATE_LURING:
			framecounter--;

			if (framecounter == 63)
			{
				int y_target = (player.y - 24) << 8;
				if (player.y < 24)
					y_target = (player.y + 24) << 8;
				for (int i = 0; i < 4; i++)
					calculate_dxdy(i, (player.x) << 8, y_target, 6);		// 6 -> 2^6 = 64 steps
			}

			migrate_roamers(framecounter);
			if (framecounter == 0)
			{
				int y_target = (player.y - 24) << 8;
				if (player.y < 24)
					y_target = (player.y + 24) << 8;
				position_marshmallow_man((player.x) << 8, y_target);
				state = PFT_STATE_LURED;
				framecounter = 256;
			}

			return 1;
			break;
		case PFT_STATE_DESTROYED:
			framecounter--;
			if (framecounter == 127)
			{
				scroll_status_text(0x1800 + 704, (char*)puftpenalty, strlength((unsigned char*)puftpenalty));

				for (int i = 0; i < 4; i++)
					calculate_dxdy(i, origins[i].x, origins[i].y, 6);
			}

			if (framecounter == 64)
			{
				roamers[0].sp_patt1	=  24;
				roamers[0].sp_patt2 =  26;
				roamers[1].sp_patt1	=  25;
				roamers[1].sp_patt2 =  27;
				roamers[2].sp_patt1	=  24;
				roamers[2].sp_patt2 =  26;
				roamers[3].sp_patt1	=  25;
				roamers[3].sp_patt2 =  27;

				roamers[0].color1 = 0xf;
				roamers[0].color2 = 0x2;
				roamers[1].color1 = 0xf;
				roamers[1].color2 = 0x2;
				roamers[2].color1 = 0xf;
				roamers[2].color2 = 0x2;
				roamers[3].color1 = 0xf;
				roamers[3].color2 = 0x2;
			}

			if (framecounter < 64)
			{
				for (int i = 0; i < 4; i++)
				{
					roamers[i].x += roamers[i].dx;
					roamers[i].y += roamers[i].dy;
				}
			}

			if (framecounter == 0)
			{
				// Mark building as destroyed, take penalty
				buildings[staypuft_attacks[currentpuft].building_id].destroyed = 1;
				if (game.account > (MARSHMALLOW_PENALTY << 1))
					game.account -= (MARSHMALLOW_PENALTY << 1);
				else
					game.account = 0;

				// Reset roamer dx/dy parameters...
				roamers[0].dx =   2;
				roamers[0].dy =   2;
				roamers[1].dx =  -2;
				roamers[1].dy =   2;
				roamers[2].dx =   2;
				roamers[2].dy =  -2;
				roamers[3].dx =  -2;
				roamers[3].dy =  -2;

				// Prepare for the next attack
				currentpuft++;
				state = PFT_STATE_WAITING;

				// Make sure puft attacks aren't back-to-back
				if ((staypuft_attacks[currentpuft].pk_level - 200) > game.pk)
					staypuft_attacks[currentpuft].pk_level = game.pk + 450;
			}

			return 1;
			break;
		case PFT_STATE_LURED:
			framecounter--;
			if (framecounter == 255)
			{
				scroll_status_text(0x1800 + 704, (char*)puftaward, strlength((unsigned char*)puftaward));
				game.account += MARSHMALLOW_PENALTY;

				for (int i = 0; i < 4; i++)
				{
					calculate_dxdy(i, origins[i].x, origins[i].y, 6);
				}
			}

			if (framecounter == 63)
			{
				roamers[0].sp_patt1	=  24;
				roamers[0].sp_patt2 =  26;
				roamers[1].sp_patt1	=  25;
				roamers[1].sp_patt2 =  27;
				roamers[2].sp_patt1	=  24;
				roamers[2].sp_patt2 =  26;
				roamers[3].sp_patt1	=  25;
				roamers[3].sp_patt2 =  27;

				roamers[0].color1 = 0xf;
				roamers[0].color2 = 0x2;
				roamers[1].color1 = 0xf;
				roamers[1].color2 = 0x2;
				roamers[2].color1 = 0xf;
				roamers[2].color2 = 0x2;
				roamers[3].color1 = 0xf;
				roamers[3].color2 = 0x2;
			}

			if (framecounter < 64)
				migrate_roamers(framecounter);

			if (framecounter == 0)
			{
				// Reset roamer dx/dy parameters...
				roamers[0].dx =   2;
				roamers[0].dy =   2;
				roamers[1].dx =  -2;
				roamers[1].dy =   2;
				roamers[2].dx =   2;
				roamers[2].dy =  -2;
				roamers[3].dx =  -2;
				roamers[3].dy =  -2;

				currentpuft++;
				state = PFT_STATE_WAITING;

				// Make sure puft attacks aren't back-to-back
				if ((staypuft_attacks[currentpuft].pk_level - 200) > game.pk)
					staypuft_attacks[currentpuft].pk_level = game.pk + 450;
			}

			return 1;
			break;
	}

	return 0;
}

int _do_map_screen()
{
	load_map();

	// while(1) { };

	distance = 0;
	bait_layed = 0;

	// Unfreeze roamers
	for (int i = 0; i < 4; i++)
	{
		roamers[i].offset_x = 0;
		roamers[i].offset_y = 0;
		roamers[i].frozen = 0;
	}

	// // Play song '0'
	// MUTE_SOUND();
	// StartSong(levelsongs, 0);

	while(1)
	{
		// reset the screen timeout (it counts UP by 2, so set it low and odd)
		// This avoids running of the built-in screensaver
		VDP_SCREEN_TIMEOUT = 1;

		int joyst_result = read_joyst(JOYST_1);

		// Player movement
		do_player_movement(joyst_result);

		lay_dot(player.x, player.y, dot_patt);

		// Does the marshmallow man need to attack?
		int key = read_keyboard();
		int pufting = 0;
		pufting = check_staypuft((key == 'B'));

		// Check for collisions between roamers and player
		if (!pufting)
			check_collisions();

		// Check for building selection, only up and down are supported
		if (joyst_result & JOYST_FIRE)
		{
			if (!pufting)
			{
				if (joyst_result & JOYST_UP)
				{
					for (int i = 0; i < NUM_BUILDINGS; i++)
					{
						if ((player.x > ((buildings[i].x - 1) * 8))  &&
							(player.x < ((buildings[i].x + 5) * 8))  &&

							(player.y < ((buildings[i].y + 5) * 8))  &&
							(player.y > ((buildings[i].y - 1) * 8)))
						{
							if ((game.battery && (game.men > 1) && game.traps) || i == GHQ_ID)
							{
								goto_building(i);
								return i;
							}
							else if (!game.battery)
								scroll_status_text(0x1800 + 704, (char*)battery_msg, strlength((unsigned char*)battery_msg));
							else if (game.men < 2)
								scroll_status_text(0x1800 + 704, (char*)men_msg, strlength((unsigned char*)men_msg));
							else if (!game.traps)
								scroll_status_text(0x1800 + 704, (char*)traps_msg, strlength((unsigned char*)traps_msg));
						}
					}
				}
				else if (joyst_result & JOYST_DOWN)
				{
					for (int i = 0; i < NUM_BUILDINGS; i++)
					{
						if ((player.x > ((buildings[i].x - 1) * 8))  &&
							(player.x < ((buildings[i].x + 5) * 8))  &&

							(player.y < ( buildings[i].y      * 8))  &&
							(player.y > ((buildings[i].y - 2) * 8)))
						{
							if ((game.battery && (game.men > 1) && game.traps) || i == GHQ_ID)
							{
								goto_building(i);
								return i;
							}
							else if (!game.battery)
								scroll_status_text(0x1800 + 704, (char*)battery_msg, strlength((unsigned char*)battery_msg));
							else if (game.men < 2)
								scroll_status_text(0x1800 + 704, (char*)men_msg, strlength((unsigned char*)men_msg));
							else if (!game.traps)
								scroll_status_text(0x1800 + 704, (char*)traps_msg, strlength((unsigned char*)traps_msg));
						}
					}
				}
			}
		}

		// Render hidden sprites to make sure roamer sprites appear only on the map
		put_4x4_sprite_offset(50, 0x00, 0,  -9);
		put_4x4_sprite_offset(50, 0x00, 0,  -9);
		put_4x4_sprite_offset(50, 0x00, 0,  -9);
		put_4x4_sprite_offset(50, 0x00, 0,  -9);
		put_4x4_sprite_offset(50, 0x00, 0, 175);
		put_4x4_sprite_offset(50, 0x00, 0, 175);
		put_4x4_sprite_offset(50, 0x00, 0, 175);
		put_4x4_sprite_offset(50, 0x00, 0, 175);

		// Render player sprite
		put_4x4_sprite_offset(ghostlogo_white, 0xf, player.x, player.y);
		put_4x4_sprite_offset(ghostlogo_red,   0x8, player.x, player.y);

		// Calculate roamer tickover speed
		if (game.roamer_tick)
			game.roamer_tick--;
		else
			game.roamer_tick = game.frames_per_roamer_tick;

		if ((key != 'B') && bait_layed)
			bait_layed--;

		// Render roamers
		for (int i = 0; i < 4; i++)
		{
			if (gSaveIntCnt & 0x01)
			{
				put_4x4_sprite_offset(roamers[i].sp_patt2, roamers[i].color2, roamers[i].x >> 8, roamers[i].y >> 8);
				if (pufting)
					put_4x4_sprite_offset(roamers[i].sp_patt1, roamers[i].color1, roamers[i].x >> 8, roamers[i].y >> 8);
			}
			else
			{
				put_4x4_sprite_offset(roamers[i].sp_patt1, roamers[i].color1, roamers[i].x >> 8, roamers[i].y >> 8);
				if (pufting)
					put_4x4_sprite_offset(roamers[i].sp_patt2, roamers[i].color2, roamers[i].x >> 8, roamers[i].y >> 8);
			}

			// Update roamer positions
			if (!pufting)
			{
				if ((roamers[i].frozen == 0) && !game.roamer_tick)
				{
					roamers[i].x += roamers[i].dx << 7;
					roamers[i].y += roamers[i].dy << 7;

					// Ensure roamers don't move beyond zuul, increase PK speed when they reach zuul
					if (roamers[0].y > (88 << 8))
					{
						pk_penalty(ROAMER_PK_PENALTY);
						roamers[0].x  	=  56 << 8;
						roamers[0].y 	=   0 << 8;
					}
					if (roamers[1].y > (88 << 8))
					{
						pk_penalty(ROAMER_PK_PENALTY);
						roamers[1].x 	= 247 << 8;
						roamers[1].y 	=   0 << 8;
					}
					if (roamers[2].y < (88 << 8))
					{
						pk_penalty(ROAMER_PK_PENALTY);
						roamers[2].x 	=  56 << 8;
						roamers[2].y 	= 175 << 8;
					}
					if (roamers[3].y < (88 << 8))
					{
						pk_penalty(ROAMER_PK_PENALTY);
						roamers[3].x 	= 247 << 8;
						roamers[3].y 	= 175 << 8;
					}
				}
			}
		}

		// Show scrolling message if needed
		do_scrolling_text(0x1800 + 704);

		// Update PK energy and roamer positions
		increase_pk(0x1800);

		// Did the player request the status message?
		if (key == ' ')
			show_status_message(0x1800);

		// Mute if done
		if (!isSNPlaying) 
		{
			MUTE_SOUND();
			StartSong(levelsongs, 0);
		}

		// Show haunted buildings
		if (!pufting)
		{
			blink_buildings();
		}

		// wait for vsync
		VSYNC_PLAY;

		// Render sprites, no flickering
		render_sprites(pufting, SAL);

		// Bail if PK energy reaches 9999
		if (game.pk >= 9999)
		{
			int counter;

			if (game.account > game.account_start)
			{
				scroll_status_text(0x1800 + 704, (char*)gotozuul, strlength((unsigned char*)gotozuul));
				counter = 520;
			}
			else
				counter = 320;

			if (game.account > game.account_start)
			{
				int zuul_x = 148;
				int zuul_y = 88;
				while (counter--)
				{
					// Show scrolling message if needed
					do_scrolling_text(0x1800 + 704);

					if (counter & 0x01)
					{
						if (player.x != zuul_x)
							player.x += (player.x > zuul_x) ? -1 : 1;

						if (player.y != zuul_y)
							player.y += (player.y > zuul_y) ? -1 : 1;
					}

					// Render player sprite
					put_4x4_sprite_offset(ghostlogo_white, 0xf, player.x, player.y);
					put_4x4_sprite_offset(ghostlogo_red,   0x8, player.x, player.y);

					// Render sprites
					render_sprites(FALSE, SAL);
					VSYNC_PLAY;
				}
			}

			max_pk_reached = 1;
			return 0;
		}
	}
	
	// // Pressed fire, make noise!
	StopSfx();
	VSYNC_PLAY;
}
