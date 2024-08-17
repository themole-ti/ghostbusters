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
#include "bankswitch.h"
#include "game_common.h"
#include "sample_playback.h"

#define put_4x4_sprite_offset_fast(pattern, color, x, y)			\
{																	\
	unsigned int loc = (num_sprites << 2); 							\
																	\
	sprite_attribute_list[loc++] = (y);								\
	sprite_attribute_list[loc++] = (x);								\
	sprite_attribute_list[loc++] = ((pattern) << 2);				\
	sprite_attribute_list[loc] = (color);							\
																	\
	num_sprites++;													\
}						

typedef enum
{
	STATE_HIDDEN,
	STATE_TRAP,
	STATE_POSITIONING,
	STATE_WAITING,
	STATE_FIRING,
	STATE_CATCHING,
	STATE_WON,
	STATE_LOST,
	STATE_SLIMED,
	STATE_CROSSED,
	STATE_TOOLATE,
	STATE_LEAVE,
	STATE_GHQ
} states;

typedef struct
{
	int 	x, y;
	int  	hdir, vdir;
	int     direction;
} enemy;

man 	men[2];
enemy 	ghost;
int   	trap_x, trap_y;
int     catch_x, catch_y, catch_yv;
int     ghost_caught = FALSE;
int     left_man = -1, right_man = -1;
int 	counter = START_COUNTER;
int 	dx, dy;
int 	tempx, tempy;
int 	sample_played = 0;

int ghost_col1, ghost_col2;

#define CATCH_YV_UP 	-3
#define CATCH_YV_DOWN 	 2

void load_background(int building_id)
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Prep nametable
	vdpwriteinc(0x1800, 0, 768);

	// Erase all sprites, no junk from last screen
	vdpmemset(SAL * 0x80, 0xd0, 256);
	num_sprites = 0;

	ENABLE_MUSIC_ON_COPY;
		// Load background building into VRAM
		switch(buildings[building_id].type)
		{
			case 0:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(gbhq_bldg_colors), 0x2000);
				rom_to_vram(unwrap(gbhq_bldg_patterns), 0x0000);
				break;
			case 1:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(house_bldg_colors), 0x2000);
				rom_to_vram(unwrap(house_bldg_patterns), 0x0000);
				break;
			case 2:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(office_bldg_colors), 0x2000);
				rom_to_vram(unwrap(office_bldg_patterns), 0x0000);
				break;
			case 3:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(brooklyn_bldg_colors), 0x2000);
				rom_to_vram(unwrap(brooklyn_bldg_patterns), 0x0000);
				break;
			case 4:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(sedgewick_bldg_colors), 0x2000);
				rom_to_vram(unwrap(sedgewick_bldg_patterns), 0x0000);
				break;
			case 5:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(office_bldg_colors), 0x2000);
				rom_to_vram(unwrap(office_bldg_patterns), 0x0000);
				break;
			case 6:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(brooklyn_bldg_colors), 0x2000);
				rom_to_vram(unwrap(brooklyn_bldg_patterns), 0x0000);
				break;
			case 7:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(house_bldg_colors), 0x2000);
				rom_to_vram(unwrap(house_bldg_patterns), 0x0000);
				break;
			case 8:
				// Load color, pattern and sprite generator resources from ROM
				rom_to_vram(unwrap(suburban_bldg_colors), 0x2000);
				rom_to_vram(unwrap(suburban_bldg_patterns), 0x0000);
				break;
		}

		// And load the last two lines of the map screen, this has contains the font
		unsigned int temp_bank   = map_patterns_bank;
		unsigned int temp_offset = map_patterns_offset + FONT_MEM_OFFSET;
		if (temp_offset > BANKSIZE)
		{
			temp_bank++;
			temp_offset -= BANKSIZE;
		}
		unsigned int temp_length = FONT_MEM_LENGTH;
		rom_to_vram(unwrap(temp), 0x0000 + FONT_MEM_OFFSET);

		// Also for pattern color definitions
		temp_bank   = map_colors_bank;
		temp_offset = map_colors_offset + FONT_MEM_OFFSET;
		if (temp_offset > BANKSIZE)
		{
			temp_bank++;
			temp_offset -= BANKSIZE;
		}
		temp_length = FONT_MEM_LENGTH;
		rom_to_vram(unwrap(temp), 0x2000 + FONT_MEM_OFFSET);

		// Make last character black by setting the colors for the last pattern to all black
		vdpmemset(0x2000 + (23*32 + 31) * 8, 0x11, 8);

		// Erase font data from nametable
		vdpmemset(0x1800 + 22*32, 6*32, 32);		// Blue line
		vdpmemset(0x1800 + 23*32, 7*32 + 31, 32); 	// Black line

		// Pre-display the game state in the bar below
		if (scroll_len < 0)
		{
			if (game.account < 9)
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000     $ 00", 32);
			else if (game.account < 99)
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000    $  00", 32);
			else if (game.account < 999)
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000   $   00", 32);
			else
				printstr(0x1800 + 704, " CITY'S PK ENERGY: 0000  $    00", 32);
		}
		else
			printstr(0x1800 + 704, "                                ", 32);

		printnum(0x1800 + 724, game.pk);
		printnum(0x1800 + 731, game.account);

		// Load sprites into VRAM
		rom_to_vram(unwrap(main_sprites), SDT * 0x800);

		// Change up the sprite definitions on each screen a bit
		int i =  rnd() % 4;
		switch (i)
		{
			case 0:
				rom_to_vram(unwrap(glowing_skull_sprites),	SDT * 0x800 + 768);
				ghost_col1 = 2;
				ghost_col2 = 7;
				break;			
			case 1:
				rom_to_vram(unwrap(skull_sprites),	SDT * 0x800 + 768);
				ghost_col1 = 1;
				ghost_col2 = 11;
				break;			
			default:
				ghost_col1 = 15;
				ghost_col2 = 3;
				break;
		}
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

unsigned char pl_patts_l[3][4] =
{
	{ 0, 6, 12, 18 },
	{ 0, 7, 12, 19 },
	{ 2, 8, 14, 20 }
};

unsigned char pl_patts_r[3][4] =
{
	{ 3,  9, 15, 21 },
	{ 3, 10, 15, 22 },
	{ 5, 11, 17, 23 }
};

unsigned char colors[16] = { 0x7, 0xd, 0x2, 0xf };

void put_man(int id)
{
	switch(men[id].state)
	{
		case STATE_TRAP:
		case STATE_WON:
		case STATE_LOST:
		case STATE_POSITIONING:
			{
				if ((men[id].hdir == 0) && (men[id].vdir == 0))
					men[id].framecounter = 1;
				else
				{
					if (men[id].delay == 0)
					{
						if (men[id].framecounter == 1)
							men[id].framecounter = 0;
						else
							men[id].framecounter++;
						men[id].delay = 5;
					}
					else
						men[id].delay--;
				}

				if (men[id].direction == DIR_LEFT)
				{
					put_4x4_sprite_offset_fast(pl_patts_l[men[id].framecounter][0], 0x1, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_l[men[id].framecounter][1], 0x1, men[id].x, men[id].y + 16);
					put_4x4_sprite_offset_fast(pl_patts_l[men[id].framecounter][2], 0x9, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_l[men[id].framecounter][3], 0xa, men[id].x, men[id].y + 16);
				}
				else if (men[id].direction == DIR_RIGHT)
				{
					put_4x4_sprite_offset_fast(pl_patts_r[men[id].framecounter][0], 0x1, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_r[men[id].framecounter][1], 0x1, men[id].x, men[id].y + 16);
					put_4x4_sprite_offset_fast(pl_patts_r[men[id].framecounter][2], 0x9, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_r[men[id].framecounter][3], 0xa, men[id].x, men[id].y + 16);
				}
			}

			if (men[id].state == STATE_TRAP)
			{
				put_4x4_sprite_offset_fast(trap_pattern, 0x1, men[id].x - 15, men[id].y + 15);
			}
			break;
		case STATE_WAITING:
		case STATE_CATCHING:
		case STATE_TOOLATE:
			if (men[id].direction == DIR_LEFT)
			{
				put_4x4_sprite_offset_fast(pl_patts_l[1][0], 0x1, men[id].x, men[id].y +  0);
				put_4x4_sprite_offset_fast(pl_patts_l[1][1], 0x1, men[id].x, men[id].y + 16);
				put_4x4_sprite_offset_fast(pl_patts_l[1][2], 0x9, men[id].x, men[id].y +  0);
				put_4x4_sprite_offset_fast(pl_patts_l[1][3], 0xa, men[id].x, men[id].y + 16);
			}
			else if (men[id].direction == DIR_RIGHT)
			{
				put_4x4_sprite_offset_fast(pl_patts_r[1][0], 0x1, men[id].x, men[id].y +  0);
				put_4x4_sprite_offset_fast(pl_patts_r[1][1], 0x1, men[id].x, men[id].y + 16);
				put_4x4_sprite_offset_fast(pl_patts_r[1][2], 0x9, men[id].x, men[id].y +  0);
				put_4x4_sprite_offset_fast(pl_patts_r[1][3], 0xa, men[id].x, men[id].y + 16);
			}
			break;
		case STATE_SLIMED:
		case STATE_CROSSED:
			put_4x4_sprite_offset_fast(slimed_patt1, 0x1, men[id].x, men[id].y + 24);
			put_4x4_sprite_offset_fast(slimed_patt2, 0xa, men[id].x, men[id].y + 24);
			break;
		case STATE_FIRING:
			{
				unsigned char tmpcolor = 0x2;
				int x = 0;
				if (men[id].direction == DIR_LEFT)
				{
					for (int i = 0; i < 5; i++)
					{
						tmpcolor = colors[fast_rnd(4)];
						x = men[id].x - (i * 8) - 8;
						if (x > 0)
							put_4x4_sprite_offset_fast(beam_pattern_start + fast_rnd(3), tmpcolor, x, men[id].y - (16 * i) - 8);
					}
					put_4x4_sprite_offset_fast(pl_patts_l[2][0], 0x1, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_l[2][1], 0x1, men[id].x, men[id].y + 16);
					put_4x4_sprite_offset_fast(pl_patts_l[2][2], 0x9, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_l[2][3], 0xa, men[id].x, men[id].y + 16);
				}
				else if (men[id].direction == DIR_RIGHT)
				{
					for (int i = 0; i < 5; i++)
					{
						tmpcolor = colors[fast_rnd(4)];
						x = men[id].x + (i * 8) + 16;
						if (x < 256)
							put_4x4_sprite_offset_fast(beam_pattern_start + fast_rnd(3) + 3, tmpcolor, x, men[id].y - (16 * i) - 8);
					}
					put_4x4_sprite_offset_fast(pl_patts_r[2][0], 0x1, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_r[2][1], 0x1, men[id].x, men[id].y + 16);
					put_4x4_sprite_offset_fast(pl_patts_r[2][2], 0x9, men[id].x, men[id].y +  0);
					put_4x4_sprite_offset_fast(pl_patts_r[2][3], 0xa, men[id].x, men[id].y + 16);
				}
			}
			break;
		default:
			break;
	}
}

void quantize_trap_location(int *x, int *y)
{
	int tempx = *x;
	int tempy = *y;

	// X - convert to character boundaries
	tempx = tempx >> 3;

	// X - Align to next appropriate character location
	if ((tempx) % 3)
	{
		tempx += 1;
	}

	// // Y - must end up being 17, 18 or 19
	// tempy = tempy >> 3;
	// if (tempy < 18)
		tempy = 18;
	// else if (tempy > 19)
	// 	tempy = 19;

	*x = tempx << 3;
	*y = tempy << 3;
}

void move_ghost()
{
	static int invisibility_counter = 10;
	static int invisible = 0;
	static int orientation = 0;

	// Basic random movement code
	int change_dir = (rnd() > RELUCTANCE);
	if (change_dir)
	{
		int dir = VDP_INT_COUNTER % 2;
		if (dir)
		{
			ghost.hdir = -ghost.hdir;
			if (ghost.hdir > 0)
				orientation = 0;
			else
				orientation = 1;
		}
		else
			ghost.vdir = -ghost.vdir;
	}

	// Do collision detection with beams
	if (men[0].state == STATE_FIRING)
	{
		// Get x position of beams at ghost's y position
		int beam1_x;
		int beam2_x;

		if (men[0].direction == DIR_RIGHT)
			beam1_x = men[0].x + ((men[0].y - (ghost.y + ghost.vdir)) >> 1) + 8;
		else
			beam1_x = men[0].x - ((men[0].y - (ghost.y + ghost.vdir)) >> 1) - 8;

		if (men[1].direction == DIR_LEFT)
			beam2_x = men[1].x - ((men[1].y - (ghost.y + ghost.vdir)) >> 1) - 8;
		else
			beam2_x = men[1].x + ((men[1].y - (ghost.y + ghost.vdir)) >> 1) + 8;

		// If we're about to hit one of the beams, bounce the ghost off of the beam
		// in the opposite direction
		if (ghost.y > 16)	// Make sure spook is not higher than streams
		{
			if (abs((ghost.x + ghost.hdir) - beam1_x) < (BEAM_BOUNCE + 1))
			{
				if ((ghost.x + ghost.hdir) > beam1_x)
					ghost.x += BEAM_BOUNCE;
				else
					ghost.x -= BEAM_BOUNCE;
			}
			else if (abs((ghost.x + ghost.hdir) - beam2_x) < (BEAM_BOUNCE + 1))
			{
				if ((ghost.x + ghost.hdir) > beam2_x)
					ghost.x += BEAM_BOUNCE;
				else
					ghost.x -= BEAM_BOUNCE;
			}
			else
			{
				ghost.x += ghost.hdir;
				ghost.y += ghost.vdir;
			}
		}
		else
		{
			ghost.x += ghost.hdir;
			ghost.y += ghost.vdir;
		}
	}
	// Do collision detection with capture beam from trap
	else if (men[0].state == STATE_CATCHING)
	{
		// Calculate distance between catching sprite and ghost
		int dist = ((ghost.x - catch_x)*(ghost.x - catch_x)) + ((ghost.y - catch_y)*(ghost.y - catch_y));
		if (abs(dist) < 300)
		{
			ghost.x = catch_x;
			ghost.y = catch_y;
			ghost_caught = TRUE;
		}
		else
		{
			int change_dir = (rnd() > RELUCTANCE);
			if (change_dir)
			{
				int dir = VDP_INT_COUNTER % 2;
				if (dir)
					ghost.hdir = -ghost.hdir;
				else
					ghost.vdir = -ghost.vdir;
			}

			ghost.x += ghost.hdir;
			ghost.y += ghost.vdir;
		}
	}
	else if ((men[0].state == STATE_LOST) || (men[left_man].state == STATE_SLIMED))
	{
		// Calculate velocity to get to left man
		if (counter == START_COUNTER)
		{
			tempx = ghost.x << 4;
			tempy = ghost.y << 4;
			dx = ((men[left_man].x - ghost.x) << 4) / counter;
			dy = ((men[left_man].y - ghost.y) << 4) / counter;
			sample_played = 0;
		}

		// Move to left man
		if (counter > 0)
		{
			tempx += dx;
			tempy += dy;

			ghost.x = tempx >> 4;
			ghost.y = tempy >> 4;

			counter--;
		}

		// If we are there, we've slimed him
		if (counter == 0)
		{
			dy = 32;
			dx = 1;
			men[left_man].state = STATE_SLIMED;
		}

		// Move away
		if ((counter <= 0) && (ghost.y > -19))
		{
			tempx -= dx;
			tempy -= dy;

			ghost.x = tempx >> 4;
			ghost.y = tempy >> 4;

			counter--;
		}

		if ((ghost.y <= -19) && !sample_played)
		{
			play_sample(slimed_bank, slimed_offset, slimed_length);
			sample_played = 1;
			men[0].state = STATE_LEAVE;
			pk_penalty(GHOST_LEFT_PK_PENALTY);
		}
	}
	else
	{
		ghost.x += ghost.hdir;
		ghost.y += ghost.vdir;
	}

	if ((!ghost_caught) && !(men[0].state == STATE_LOST) && !(men[left_man].state == STATE_SLIMED))
	{
		if (ghost.x > 240) ghost.x = 240;
		if (ghost.x <   2) ghost.x =   2;
		if (ghost.y >  88) ghost.y =  88;
		if (ghost.y <   8) ghost.y =   8;
	}

	// Do invisibility calculations
	if ((invisibility_counter) && (!game.intensifier))
	{
		invisibility_counter--;

		if (!invisibility_counter)
		{
			invisible = ~invisible;
			invisibility_counter = 10 + (rnd() >> 3);
		}
	}

	// Put actual sprites
	if (men[0].state != STATE_WON)
	{
		if ((!invisible) || (men[0].state == STATE_CATCHING))
		{
			if (!game.intensifier)
			{
				if (gSaveIntCnt & 0x01)
				{
					put_4x4_sprite_offset_fast(24 + orientation, ghost_col1, ghost.x, ghost.y);
				}
				else
				{
					put_4x4_sprite_offset_fast(26 + orientation, ghost_col2, ghost.x, ghost.y);
				}				
			}
			else
			{
				put_4x4_sprite_offset_fast(24 + orientation, ghost_col1, ghost.x, ghost.y);
				put_4x4_sprite_offset_fast(26 + orientation, ghost_col2, ghost.x, ghost.y);
			}
		}
		else
		{
			unsigned char colors = vdpgetch(0x2000 + ((ghost.x + 8) >> 3) + (256 * ((ghost.y + 10) >> 3)));
			if ((colors & 0x0f) < 2)
				colors = colors >> 4;
			else
				colors &= 0x0f;

			if (!(invisibility_counter & 0x10))
			{
				colors = 0x0;
			}

			put_4x4_sprite_offset_fast(24 + orientation, colors, ghost.x, ghost.y);
			put_4x4_sprite_offset_fast(26 + orientation,    0x0, ghost.x, ghost.y);
		}
	}
}

void victory_jump()
{
	if (counter >= START_COUNTER - 2)
	{
		// do nothing for two frames
	}
	else if (counter == (START_COUNTER - 3))
	{
		play_sample(shout_bank, shout_offset, shout_length);
	}
	else if (counter > 0)
	{
		if ((counter >> 2) % 2)
		{
			men[0].y -= 1;
			men[1].y += 1;
		}
		else
		{
			men[0].y += 1;
			men[1].y -= 1;
		}
	}
	else
	{
		men[0].state = STATE_LEAVE;
	}

	counter--;
}


void crossed_streams()
{
	if (counter == START_COUNTER)
		pk_penalty(GHOST_LEFT_PK_PENALTY);
	else if (counter == 0)
		men[0].state = STATE_LEAVE;

	counter--;
}


void drop_trap(int id)
{
	int framecounter = 10;

	int from_x = men[id].x - 15;
	int from_y = men[id].y + 15;

	int to_x = from_x - 10;
	int to_y = from_y + 10;

	quantize_trap_location(&to_x, &to_y);

	int dx = (to_x - from_x) / framecounter;
	int dy = (to_y - from_y) / framecounter;

	while (framecounter--)
	{
		put_4x4_sprite_offset_fast(pl_patts_l[1][0], 0x1, men[id].x, men[id].y +  0);
		put_4x4_sprite_offset_fast(pl_patts_l[1][1], 0x1, men[id].x, men[id].y + 16);
		put_4x4_sprite_offset_fast(pl_patts_l[1][2], 0x9, men[id].x, men[id].y +  0);
		put_4x4_sprite_offset_fast(pl_patts_l[1][3], 0xa, men[id].x, men[id].y + 16);

		put_4x4_sprite_offset_fast(trap_pattern, 0x1, from_x, from_y);

		from_x += dx;
		from_y += dy;

		move_ghost();

		render_sprites(FALSE, SAL);

		// wait for vsync
		VSYNC_PLAY;
	}

	// render trap to patterns
	const unsigned char tpcd[16] = 
	{ 
		0x00, 0x00, 0x3f, 0x66, 0x4c, 0x7f, 0x00, 0xff,
		0x3e, 0x20, 0xf0, 0x68, 0xc8, 0xf8, 0x00, 0xff
	};

	// Render pattern defined above to appropriate name table location
	int vdp_addr = 0x0000 + ((to_y >> 3) * 256) + ((to_x >> 3) * 8);
	vdpmemcpy(vdp_addr, tpcd, 16);
	// Render black/grey color combination, with white line, to appropriate color table location
	vdp_addr += 0x2000;
	vdpmemset(vdp_addr, 0x1e, 6);
	vdpchar(vdp_addr + 6, 0xff);
	vdpchar(vdp_addr + 7, 0x11);
	vdpmemset(vdp_addr + 8, 0x1e, 6);
	vdpchar(vdp_addr + 14, 0xff);
	vdpchar(vdp_addr + 15, 0x11);

	trap_x = to_x;
	trap_y = to_y;
}


// Ghost catching game logic
void _do_catch_screen(int building_id)
{
	unsigned int joyst_result, active_man = 0, leave = 0, has_ghost = 0;

	// Initial men data
	men[0].x 			= 200;
	men[0].y 			= 120;
	men[0].hdir 		= 0;
	men[0].vdir 		= 0;
	men[0].state 		= STATE_TRAP;
	men[0].framecounter =  0;
	men[0].delay   		= 10;
	men[0].direction    = DIR_LEFT;
	men[1].x 			= 200;
	men[1].y 			= 120;
	men[1].hdir 		= 0;
	men[1].vdir 		= 0;
	men[1].state 		= STATE_HIDDEN;
	men[1].framecounter =  0;
	men[1].delay   		= 10;
	men[1].direction    = DIR_LEFT;

	// No trap placed yet
	trap_x = 0;
	trap_y = 0;
	ghost_caught = FALSE;

	counter = START_COUNTER;
	sample_played = 0;


	// // Load title music and sound effect into RAM
	// MUTE_SOUND();
	// rom_to_ram(unwrap(themesong), (unsigned char*)levelsongs);

	// Show background graphics
	load_background(building_id);

	VDP_SET_REGISTER( 1, VDP_MODE1_16K | VDP_MODE1_UNBLANK | VDP_MODE1_INT | VDP_MODE1_SPRMODE16x16);		// GM1 / No blanking | Interupt enabled

	// // Play song '0'
	// MUTE_SOUND();
	// StartSong(levelsongs, 0);

	// Prepare ghost
	if ((buildings[building_id].haunted) && !(buildings[building_id].prehaunt))
	{
		has_ghost = 1;
		ghost.x = (rnd() % 220) + 128;
		ghost.y = (rnd() % 100) + 32;
		ghost.hdir = (rnd() % 2) ? -1 : 1;
		ghost.vdir = (rnd() % 2) ? -1 : 1;
		ghost.direction = DIR_LEFT;
	}

	// Title screen animation
	joyst_result = 0;
	int fire_pressed = FALSE;
	int catch_framecounter = 180;

	int ghq_animation_man = 0;
	if (building_id == GHQ_ID)
	{
		men[1].x = 128;
		men[1].y = 100;
		men[1].direction = DIR_RIGHT;
		men[1].state = STATE_POSITIONING;
		men[1].hdir = 1;
	}

	while (!leave)
	{
		// reset the screen timeout (it counts UP by 2, so set it low and odd)
		// This avoids running of the built-in screensaver
		VDP_SCREEN_TIMEOUT = 1;

		// Mute if done
		if (!isSNPlaying) 
		{
			MUTE_SOUND();
			StartSong(levelsongs, 0);
		}

		// If we are at Ghostbusters HQ, don't do any game logic, just animate
		if (building_id == GHQ_ID)
		{
			if (ghq_animation_man < 3)
			{
				put_man(1);

				if (men[1].x < 254)
				{
					men[1].x += 2;
					men[1].y  = 100 + ((men[1].x - 128) >> 2);
				}
				else
				{
					men[1].x = 128;
					men[1].y = 100;
					ghq_animation_man++;
				}
			}
			else
			{
				game.battery = MAX_BATTERY_CHARGE;
				game.men     = 3;
				game.traps   = game.max_traps;
				leave = 1;
			}
		}
		else
		{
			joyst_result = read_joyst(JOYST_1);

			// Move your men
			switch(men[active_man].state)
			{
				case STATE_TRAP:
					men[active_man].direction = DIR_LEFT;
					if (joyst_result & JOYST_LEFT)
					{
						men[active_man].hdir = -1;
					}
					if (joyst_result & JOYST_RIGHT)
					{
						men[active_man].hdir =  1;
					}
					if (joyst_result & JOYST_UP)
					{
						men[active_man].vdir = -1;
					}
					if (joyst_result & JOYST_DOWN)
					{
						men[active_man].vdir =  1;
					}
					break;
				case STATE_POSITIONING:
					if (joyst_result & JOYST_LEFT)
					{
						men[active_man].direction = DIR_LEFT;
						men[active_man].hdir = -1;
					}
					if (joyst_result & JOYST_RIGHT)
					{
						men[active_man].direction = DIR_RIGHT;
						men[active_man].hdir =  1;
					}
					if (joyst_result & JOYST_UP)
					{
						men[active_man].vdir = -1;
					}
					if (joyst_result & JOYST_DOWN)
					{
						men[active_man].vdir =  1;
					}
					break;
				case STATE_FIRING:
					// Check for crossed streams
					{
						int distance = 0;
						if (men[0].x < men[1].x)
						{
							if ((men[0].direction == DIR_RIGHT) && (men[1].direction == DIR_LEFT))
							{
								// Calculate distance between streams, taking into account difference in heights of men
								distance = (men[1].x - men[0].x) + (abs(men[0].y - men[1].y) >> 1);
							}
							else
								distance = 255;
						}
						else
						{
							if ((men[1].direction == DIR_RIGHT) && (men[0].direction == DIR_LEFT))
							{
								// Calculate distance between streams, taking into account difference in heights of men
								distance = (men[0].x - men[1].x) + (abs(men[1].y - men[0].y) >> 1);
							}
							else
								distance = 255;
						}

						if (distance < 96)
						{
							men[0].state = STATE_CROSSED;
							men[1].state = STATE_CROSSED;
							scroll_status_text(0x1800 + 704, (char*)crossed_msg, strlength((unsigned char*)crossed_msg));
							StopSfx();
							StartSfx(levelfx, SOUND_BOOM, 1);
							game.battery = 0;
							breakpoint();
							counter = 120;
						}

						// If battery is depleted, automatically deploy the trap
						game.battery--;
						if (game.battery == 0)
						{
							scroll_status_text(0x1800 + 704, (char*)battery_msg, strlength((unsigned char*)battery_msg));
							StopSfx();
							StartSfx(levelfx, SOUND_TRAP, 1);
							men[0].state = STATE_CATCHING;
							men[1].state = STATE_CATCHING;
							catch_x = trap_x;
							catch_y = trap_y - trap_y_offset;
							catch_yv = CATCH_YV_UP;
						}
					}

					if (joyst_result & JOYST_LEFT)
					{
						men[right_man].hdir = -1;
					}
					if (joyst_result & JOYST_RIGHT)
					{
						men[left_man].hdir = 1;
					}
					break;
			}

			// Handle fire button
			if (joyst_result & JOYST_FIRE)
			{
				if (!fire_pressed)
				{
					fire_pressed = TRUE;

					// Drop trap
					if (men[active_man].state == STATE_TRAP)
					{
						men[active_man].state = STATE_POSITIONING;
						drop_trap(active_man);
					}
					else if (men[active_man].state == STATE_POSITIONING)
					{
						if (active_man == 0)
						{
							men[active_man].state = STATE_WAITING;
							active_man = 1;
							men[active_man].state = STATE_POSITIONING;
						}
						else
						{
							men[0].state = STATE_FIRING;
							men[1].state = STATE_FIRING;

							if (men[0].x > men[1].x)
							{
								left_man = 1;
								right_man = 0;
							}
							else
							{
								left_man = 0;
								right_man = 1;
							}
						}
					}
					else if (men[0].state == STATE_FIRING)
					{
						men[0].state = STATE_CATCHING;
						men[1].state = STATE_CATCHING;
						catch_x = trap_x;
						catch_y = trap_y - trap_y_offset;
						catch_yv = CATCH_YV_UP;
						StopSfx();
						StartSfx(levelfx, SOUND_TRAP, 1);
					}
				}
			}
			else
				fire_pressed = FALSE;

			if (men[0].state == STATE_LEAVE)
				leave = 1;

			// Render player sprites
			put_man(0);
			put_man(1);

			// Handle ghost logic
			if (has_ghost)
				move_ghost();

			// Catching sequence
			if (men[0].state == STATE_CATCHING)
			{
				(void)catch_framecounter;

				catch_x = trap_x;
				catch_y = catch_y + catch_yv;

				if (catch_y < 20)
					catch_yv = CATCH_YV_DOWN;	// slower on the way down!

				if (catch_y > (trap_y - trap_y_offset))
				{
					if (ghost_caught)
					{
						// Reset framecounter for victory animation
						counter = START_COUNTER;
						men[0].state = STATE_WON;
						men[1].state = STATE_WON;
						game.account += ((buildings[building_id].haunted / 10) + 1);
						game.traps--;
					}
					else
					{
						men[0].state = STATE_LOST;
						men[1].state = STATE_LOST;
						game.men--;
					}
				}

				put_4x4_sprite_offset_fast(43, colors[rnd() % 4], catch_x, catch_y);
				if (catch_y < ((trap_y - trap_y_offset) - 16))
					put_4x4_sprite_offset_fast(49, colors[rnd() % 4], catch_x, catch_y + 16);
				if (catch_y < ((trap_y - trap_y_offset) - 32))
					put_4x4_sprite_offset_fast(49, colors[rnd() % 4], catch_x, catch_y + 32);
				if (catch_y < ((trap_y - trap_y_offset) - 48))
					put_4x4_sprite_offset_fast(49, colors[rnd() % 4], catch_x, catch_y + 48);
				if (catch_y < ((trap_y - trap_y_offset) - 64))
					put_4x4_sprite_offset_fast(49, colors[rnd() % 4], catch_x, catch_y + 64);
				// if (catch_y < ((trap_y - trap_y_offset) - 96))
				// 	put_4x4_sprite_offset_fast(49, 0x2, catch_x, catch_y + 96);
			}

			if (men[0].state == STATE_FIRING)
			{
				if (!isSFXPlaying)
					StartSfx(levelfx, SOUND_BEAM, 1);
			}

			if (men[0].state == STATE_WON)
			{
				victory_jump();
			}

			if (men[0].state == STATE_CROSSED)
			{
				crossed_streams();
			}

			men[0].x += men[0].hdir;
			men[0].y += men[0].vdir;
			men[1].x += men[1].hdir;
			men[1].y += men[1].vdir;

			// Bounds checks
			if (men[0].y < 106) men[0].y = 106;
			if (men[0].y > 128) men[0].y = 128;
			if (men[0].x <   2) men[0].x =   2;
			if (men[0].x > 238) men[0].x = 238;
			if (men[1].y < 106) men[1].y = 106;
			if (men[1].y > 128) men[1].y = 128;
			if (men[1].x <   2) men[1].x =   2;
			if (men[1].x > 238) men[1].x = 238;

			men[0].hdir = 0;
			men[0].vdir = 0;
			men[1].hdir = 0;
			men[1].vdir = 0;
		}

		// Did the player request the status message?
		if (read_spacebar())
			show_status_message(0x1800);

		// Scrolling text
		do_scrolling_text(0x1800 + 704);

		// Do we need to increase PK energy?
		increase_pk(0x1800);

		// wait for vsync
		VSYNC_PLAY;

		// Only cycle sprites when catching
		render_sprites(TRUE, SAL);		

		// Check to see if building was/is still haunted, if not, we leave
		if (men[0].state < STATE_WON)
		{
			if ((buildings[building_id].haunted == 0 || buildings[building_id].prehaunt != 0) && (building_id != GHQ_ID))
			{
				men[0].state = STATE_TOOLATE;
				men[1].state = STATE_TOOLATE;
				StopSfx();

				for (int i = 0; i < 120; i++)
				{
					// Scroll statusbar if needed
					do_scrolling_text(0x1800 + 704);

					increase_pk(0x1800);

					put_man(0);
					put_man(1);

					if (has_ghost)
					{
						if (ghost.y > -17)
							ghost.y--;

						put_4x4_sprite_offset_fast(24, ghost_col1, ghost.x, ghost.y);
						put_4x4_sprite_offset_fast(26, ghost_col2, ghost.x, ghost.y);
					}

					VSYNC_PLAY;

					render_sprites(TRUE, SAL);
				}

				leave = 2;
				break;
			}
		}
	}

	// If this was a haunted building, it now no longer is
	if ((buildings[building_id].haunted) && (leave == 1) )
		buildings[building_id].haunted = 0;

	VDP_SET_REGISTER( 1, VDP_MODE1_16K | VDP_MODE1_UNBLANK);		// GM1 / No blanking | Interupt enabled

	StopSfx();
}
