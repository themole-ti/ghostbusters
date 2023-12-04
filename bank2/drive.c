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

// More quickly render sprites to VRAM, skipping a certain offset
// Can be used when the first 'offset' number of sprites stay 
// static (e.g. when used as a line marker using the 5th sprite flag)
// This version does not do sprite rotation...
#define render_sprites_fast_offset(offset)							\
{																	\
	unsigned char* src = sprite_attribute_list;						\
																	\
	VDP_SET_ADDRESS_WRITE(SAL * 0x80 + ((offset) << 2));			\
																	\
	while (--num_sprites)											\
	{																\
		VDPWD=*(src++);												\
		VDPWD=*(src++);												\
		VDPWD=*(src++);												\
		VDPWD=*(src++);												\
	}																\
																	\
	/* Last byte is 0xd0 to stop the VDP from rendering junk */     \
	VDPWD=0xd0;														\
}

inline void scroll_left(unsigned int from, unsigned int to)
{
	unsigned char frame;

	// This works and is straightforward in it's implementation, but is a bit too slow to my liking...
	while (from < to)
	{
		from++;
		frame = from % 8;

		if (frame == 1)
		{
			// Prepare next batch in backbuffers
			unsigned int row 	 = 3;
			unsigned int col  	 = ((256 - from) >> 3) - 13;

			unsigned int paddr = sitlocations[backbuffer_sit] + (row * 32) + col;

			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
		}
		else if (frame == 0)
		{
			// Prepare next batch in backbuffers
			unsigned int row 	 = 11;
			unsigned int col  	 = ((256 - from) >> 3) - 13;

			unsigned int paddr = sitlocations[backbuffer_sit] + (row * 32) + col;

			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;

			// Swap buffers
			backbuffer_sit = (backbuffer_sit + 1) % 2;
		}
	}
}

inline void scroll_right(unsigned int from, unsigned int to)
{
	unsigned char frame;

	// This works and is straightforward in it's implementation, but is a bit too slow to my liking...
	while (from > to)
	{
		from--;
		frame = from % 8;

		// Move backbuffer one forward in the list of SIT's
		if (frame == 7)
		{
			// Prepare next batch in backbuffers
			unsigned int row 	= 3;
			unsigned int col  	= ((256 - from) >> 3) - 12;

			unsigned int paddr = sitlocations[backbuffer_sit] + (row * 32) + col;

			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;

			// Swap buffers
			backbuffer_sit = (backbuffer_sit + 1) % 2;
		}
		else if (frame == 6)
		{
			// Prepare next batch in backbuffers
			unsigned int row 	= 11;
			unsigned int col  	= ((256 - from) >> 3) - 11;

			unsigned int paddr = sitlocations[backbuffer_sit] + (row * 32) + col;

			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
			vdpmemcpy_8(paddr, &(map[row][19]), 2); paddr += 32; row += 1;
		}
	}
}

void scroll(unsigned int from, unsigned int to)
{
	if (from < to)
		scroll_left(from, to);
	else
		scroll_right(from, to);
}

unsigned int play_music[] =
{
	3,   6,  9, 12,
	15, 18, 21, 24,
	27, 30
};

// Copy first screen from RAM (map) to VRAM
void init_nametable()
{
	int x, y, i = 0;

 	CALL_PLAYER_SN;

	for (x = 0; x < 32; x++)
	{
		if (x == play_music[i]) { CALL_PLAYER_SN; i++; }
		for (y = 0; y < 24; y++)
			vdpchar(sitlocations[0] + (x + (y * 32)), map[y][x]);
	}
 
 	CALL_PLAYER_SN;

 	i = 0;
	for (x = 0; x < 32; x++)
	{
		if (x == play_music[i]) { CALL_PLAYER_SN; i++; }
		for (y = 0; y < 32; y++)
			vdpchar(sitlocations[1] + (x + (y * 32)), map[y][x + 1]);
	}
 
 	CALL_PLAYER_SN;
}

void load_map_by_row(unsigned int bank, unsigned int offset, unsigned int rowlength)
{
	int i;

	for (i = 0; i < 24; i++)
	{
		if (offset >= BANKSIZE)
		{
			bank++;
			offset -= BANKSIZE;
		}

		rom_to_ram(bank, offset, rowlength, map[i]);
		offset += rowlength;
	}

	CALL_PLAYER_SN;
}

#define ROWLENGTH 64
void init_level_graphics()
{
	// Load graphics data into VRAM
	ENABLE_MUSIC_ON_COPY;
		switch (game.car_id)
		{
			case CAR_ID_BEETLE:
				rom_to_vram(unwrap(beetle_patt0), 0x0000);
				rom_to_vram(unwrap(beetle_patt1), 0x0800);
				rom_to_vram(unwrap(beetle_patt2), 0x1000);
				rom_to_vram(unwrap(beetle_patt3), 0x1800);
				rom_to_vram(unwrap(beetle_patt4), 0x2000);
				rom_to_vram(unwrap(beetle_patt5), 0x2800);
				rom_to_vram(unwrap(beetle_patt6), 0x3000);
				rom_to_vram(unwrap(beetle_patt7), 0x3800);
				rom_to_vram(unwrap(beetle_colors), GM1_CT * 0x40);
				rom_to_vram(unwrap(beetle_sprites), (GM1_SDT * 0x800) + 0x400);

				// Load map data into RAM
				load_map_by_row(beetle_nametable_bank, beetle_nametable_offset, ROWLENGTH);
				break;
			case CAR_ID_HEARSE:
				rom_to_vram(unwrap(drive_patt0), 0x0000);
				rom_to_vram(unwrap(drive_patt1), 0x0800);
				rom_to_vram(unwrap(drive_patt2), 0x1000);
				rom_to_vram(unwrap(drive_patt3), 0x1800);
				rom_to_vram(unwrap(drive_patt4), 0x2000);
				rom_to_vram(unwrap(drive_patt5), 0x2800);
				rom_to_vram(unwrap(drive_patt6), 0x3000);
				rom_to_vram(unwrap(drive_patt7), 0x3800);
				rom_to_vram(unwrap(drive_colors), GM1_CT * 0x40);
				rom_to_vram(unwrap(drive_sprites), (GM1_SDT * 0x800) + 0x400);

				// Load map data into RAM
				load_map_by_row(drive_nametable_bank, drive_nametable_offset, ROWLENGTH);
				break;
			case CAR_ID_WAGON:
				rom_to_vram(unwrap(wagon_patt0), 0x0000);
				rom_to_vram(unwrap(wagon_patt1), 0x0800);
				rom_to_vram(unwrap(wagon_patt2), 0x1000);
				rom_to_vram(unwrap(wagon_patt3), 0x1800);
				rom_to_vram(unwrap(wagon_patt4), 0x2000);
				rom_to_vram(unwrap(wagon_patt5), 0x2800);
				rom_to_vram(unwrap(wagon_patt6), 0x3000);
				rom_to_vram(unwrap(wagon_patt7), 0x3800);
				rom_to_vram(unwrap(wagon_colors), GM1_CT * 0x40);
				rom_to_vram(unwrap(wagon_sprites), (GM1_SDT * 0x800) + 0x400);

				// Load map data into RAM
				load_map_by_row(wagon_nametable_bank, wagon_nametable_offset, ROWLENGTH);
				break;
			case CAR_ID_SPORTSCAR:
				rom_to_vram(unwrap(sportscar_patt0), 0x0000);
				rom_to_vram(unwrap(sportscar_patt1), 0x0800);
				rom_to_vram(unwrap(sportscar_patt2), 0x1000);
				rom_to_vram(unwrap(sportscar_patt3), 0x1800);
				rom_to_vram(unwrap(sportscar_patt4), 0x2000);
				rom_to_vram(unwrap(sportscar_patt5), 0x2800);
				rom_to_vram(unwrap(sportscar_patt6), 0x3000);
				rom_to_vram(unwrap(sportscar_patt7), 0x3800);
				rom_to_vram(unwrap(sportscar_colors), GM1_CT * 0x40);
				rom_to_vram(unwrap(sportscar_sprites), (GM1_SDT * 0x800) + 0x400);

				// Load map data into RAM
				load_map_by_row(sportscar_nametable_bank, sportscar_nametable_offset, ROWLENGTH);
				break;
		}
	DISABLE_MUSIC_ON_COPY;

	// Erase vacuum if user hasn't bought it in the shop
	if (game.vacuum == 0)
	{
		memcpy(&map[6][25], &map[5][25], 3);
		memcpy(&map[7][25], &map[5][25], 3);
	}

	// // Load song and sfx from rom
	// rom_to_ram(unwrap(themesong), (unsigned char*)levelsongs);
}

// Wait for the 5th sprite flag to be set on the fifth sprite (index 4 -> 0x04)
inline void wait_for_5th_sprite()
{
	int status = VDPST & 0x5f;				// Mask off all bits we don't care about
	while (status != 0x44) 					// 0x44 means 5th sprite flag set, sprite index 4
		status = VDPST & 0x5f ;
	VDP_SET_REGISTER(VDP_REG_PDT, 0x04);
	VDP_SET_REGISTER(VDP_REG_NT, sitregisters[0]);
	VDP_SET_ADDRESS_WRITE(SAL * 0x80);
	VDPWD = 0xd0;							// Make sure sprites don't render after the screen split
}

// Init all graphics routines (upload patterns, set registers, ...)
void init_drive_section()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);

	// Set all sprites offscreen
	vdpmemset(SAL * 0x80, 0xd0, 128);

	// Load graphics into VRAM and RAM
	init_level_graphics();

	// Copy first screen from RAM to VRAM
	init_nametable();

	// Copy the font to VRAM
	ENABLE_MUSIC_ON_COPY;
		rom_to_vram(font_bank, font_offset, (64*8), 0x2400 + 64*8);		// Load font data into VRAM	
		vdpmemset(0x2410, 0xf4, 16);
		vdpmemset(0x2410, 0x11,  1);
	DISABLE_MUSIC_ON_COPY;

	// And pre-display the game state in the bar below, in both NT buffers
	if (game.account < 9)
		printstr(sitlocations[1] + 704, " CITY'S PK ENERGY: 0000     $ 00", 32);
	else if (game.account < 99)
		printstr(sitlocations[1] + 704, " CITY'S PK ENERGY: 0000    $  00", 32);
	else if (game.account < 999)
		printstr(sitlocations[1] + 704, " CITY'S PK ENERGY: 0000   $   00", 32);
	else
		printstr(sitlocations[1] + 704, " CITY'S PK ENERGY: 0000  $    00", 32);

	if (scroll_len < 1)
	{
		if (game.account < 9)
			printstr(sitlocations[0] + 704, " CITY'S PK ENERGY: 0000     $ 00", 32);
		else if (game.account < 99)
			printstr(sitlocations[0] + 704, " CITY'S PK ENERGY: 0000    $  00", 32);
		else if (game.account < 999)
			printstr(sitlocations[0] + 704, " CITY'S PK ENERGY: 0000   $   00", 32);
		else
			printstr(sitlocations[0] + 704, " CITY'S PK ENERGY: 0000  $    00", 32);
		printnum(sitlocations[0] + 724, game.pk);
		printnum(sitlocations[0] + 731, game.account);
	}
	else
		printstr(sitlocations[0] + 704, "                                ", 32);

	vdpmemset(sitlocations[0] + (23 * 32), 128,  32);
	vdpmemset(sitlocations[1] + (23 * 32), 128,  32);

	// Set registers
	VDP_SET_REGISTER(VDP_REG_MODE0, 0);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT | VDP_MODE1_UNBLANK | VDP_MODE1_SPRMODE16x16 );
	VDP_SET_REGISTER(VDP_REG_PDT, 0);	
	VDP_SET_REGISTER(VDP_REG_CT, GM1_CT);	
	VDP_SET_REGISTER(VDP_REG_NT, GM1_SIT1);
	VDP_SET_REGISTER(VDP_REG_SAL, SAL);
	VDP_SET_REGISTER(VDP_REG_SDT, GM1_SDT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_WHITE << 4 | COLOR_BLACK) );

	// Choose which nametable to render to (backbuffer) 
	backbuffer_sit = 1;
}

#define vacuum_radius (40 << 5)
#define vacuum_trap   ( 8 << 5)
#define MAX_SUCK_COUNTER 60
inline int suck_roamer(int scroll_x, int i, int x, int y)
{
	int flip = 0;

	// Where is the vacuum on screen?
	int vacuum_x = (224 - scroll_x -16) << 5;
	int vacuum_y = 54 << 5;

	// What is the distance from roamer to vacuum for each axis?
	int delta_x = x - vacuum_x;
	int delta_y = y - vacuum_y;

	int y_accell = 4;
	if (delta_y > 0)
		y_accell = 3;

	// Both axii within range?
	if ((abs(delta_x) < vacuum_radius) && (abs(delta_y) < vacuum_radius))
	{
		roamers[i].offset_x += -((delta_x) >> 4);
		roamers[i].offset_y += -((delta_y) >> y_accell);
	}

	// Is the roamer on top off the vacuum?
	if ((abs(delta_x) < vacuum_trap) && (abs(delta_y) < vacuum_trap))
	{
		roamers[i].offset_x += -((delta_x) >> 4);
		roamers[i].offset_y += -((delta_y) >> y_accell);

		if (roamers[i].suck_counter == MAX_SUCK_COUNTER)
			StartSfx(levelfx, SOUND_VACUUM, 1);

		// Are we doing the flip animation?
		if (roamers[i].suck_counter > 0)
		{
			flip = (roamers[i].suck_counter >> 1) & 0x01;
			roamers[i].suck_counter--;
		}
		else
		{
			// Yay, sucked up the roamer, great!
			roamers[i].frozen = FALSE;
			roamers[i].offset_x = 0;
			roamers[i].offset_y = 0;
			roamers[i].x = roamer_homes[i][0];
			roamers[i].y = roamer_homes[i][1];
		}
	}
	else
		roamers[i].suck_counter = MAX_SUCK_COUNTER;

	return flip;
}


// If player froze roamers on the map screen, show them here
inline void put_roamers(unsigned int traveled, unsigned int fire_pressed, unsigned int scroll_x)
{
	for (int i = 0; i < MAX_ROAMERS; i++)
	{
		if (roamers[i].frozen)
		{
			if ((roamers[i].distance < traveled) && ((roamers[i].distance - (roamers[i].offset_y >> 5) + 384) > traveled))
			{
				int x = roamers[i].suck_x + roamers[i].offset_x;
				int y = ((traveled - roamers[i].distance) << 4) + roamers[i].offset_y - (16 << 5);
				int flip = 0;

				if (game.vacuum)
				{
					if (fire_pressed)
					{
						flip = suck_roamer(scroll_x, i, x, y);
					}
					else
					{
						if (roamers[i].suck_counter != MAX_SUCK_COUNTER)
						{
							roamers[i].suck_counter = MAX_SUCK_COUNTER;
							StopSfx();
							StartSfx(levelfx, SOUND_SILENT, 0);
						}
					}
				}

				// Roamer escaped?
				if (y > (208 << 5))
				{
					roamers[i].frozen = FALSE;
					roamers[i].offset_x = 0;
					roamers[i].offset_y = 0;
				}

				if (gSaveIntCnt & 0x01)
				{
					put_4x4_sprite_offset_fast(32 + 19 + flip, roamers[i].color1, x >> 5, y >> 5);
				}
				else
				{
					put_4x4_sprite_offset_fast(32 + 21 + flip, roamers[i].color2, x >> 5, y >> 5);
				}
			}
		}
	}
}

// Unrolled and ugly code for speed...
inline void put_roadmarkings(int y, int car_x)
{
	unsigned char second_y = y - 128;
	int cond1 = (car_x > 75);
	int cond2 = (y < car_top);
	int cond3 = (y > car_bottom);
	int cond4 = (second_y < car_top);
	int cond5 = (second_y > car_bottom);
	int cond6 = (car_x < 105);

	if (cond1 || cond2 || cond3)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 82, y);
	if (cond1 || cond4 || cond5)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 82, second_y);
	if (cond6 || cond2 || cond3)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 174, y);
	if (cond6 || cond4 || cond5)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 174, second_y);

	y += 16;
	second_y += 16;
	cond2 = (y < car_top);
	cond3 = (y > car_bottom);
	cond4 = (second_y < car_top);
	cond5 = (second_y > car_bottom);

	if (cond1 || cond2 || cond3)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 82, y);
	if (cond1 || cond4 || cond5)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 82, second_y);
	if (cond6 || cond2 || cond3)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 174, y);
	if (cond6 || cond4 || cond5)
		put_4x4_sprite_offset_fast(roadmarking_pattern, 0xf, 174, second_y);
}

void pre_render_beetle(int scroll_x)
{
	unsigned char sprite_scroll_x = 256 - scroll_x - 80;
	unsigned char sprite_scroll_x2 = sprite_scroll_x + 64;
	unsigned char carsprite_y = 31;
	for (int i = 32; i < 50; i++)
	{
		put_4x4_sprite_offset_fast(i++, 0x01, sprite_scroll_x , carsprite_y);
		put_4x4_sprite_offset_fast(i  , 0x01, sprite_scroll_x2, carsprite_y);
		carsprite_y += 16;
	}	
}

void pre_render_hearse(int scroll_x)
{
	unsigned char sprite_scroll_x = 256 - scroll_x - 80;
	unsigned char sprite_scroll_x2 = sprite_scroll_x + 64;
	unsigned char carsprite_y = 15;
	for (int i = 32; i < 50; i++)
	{
		put_4x4_sprite_offset_fast(i++, 0x01, sprite_scroll_x , carsprite_y);
		put_4x4_sprite_offset_fast(i  , 0x01, sprite_scroll_x2, carsprite_y);
		carsprite_y += 16;
	}	
}

void pre_render_wagon(int scroll_x)
{
	unsigned char sprite_scroll_x = 256 - scroll_x - 80;
	unsigned char sprite_scroll_x2 = sprite_scroll_x + 64;
	unsigned char carsprite_y = 31;
	for (int i = 32; i < 50; i++)
	{
		put_4x4_sprite_offset_fast(i++, 0x04, sprite_scroll_x , carsprite_y);
		put_4x4_sprite_offset_fast(i  , 0x04, sprite_scroll_x2, carsprite_y);
		carsprite_y += 16;
	}	
}

void pre_render_sportscar(int scroll_x)
{
	unsigned char sprite_scroll_x = 256 - scroll_x - 80;
	unsigned char sprite_scroll_x2 = sprite_scroll_x + 64;
	unsigned char carsprite_y = 31;
	for (int i = 32; i < 50; i++)
	{
		put_4x4_sprite_offset_fast(i++, 0x08, sprite_scroll_x , carsprite_y);
		put_4x4_sprite_offset_fast(i  , 0x08, sprite_scroll_x2, carsprite_y);
		carsprite_y += 16;
	}	
}

void (*render_car_func)(int);
void drive_loop()
{
	unsigned int    joyst_result = 0, fire_pressed = 0, traveled = 0, speed = 0;
	unsigned int	scroll_x, prev_scroll_x;
	int	markings_y = 1;

	// Scrolling variables initialization (pixels)
	prev_scroll_x = scroll_x = 4;
	do_flip = 0;

	// Calculate roamer positions
	for (int i = 0; i < MAX_ROAMERS; i++)
	{
		if (roamers[i].frozen)
		{
			unsigned char suck_x = rnd();

			// Avoid middle section
			if (suck_x > 90 && suck_x < 160)
			{
				suck_x += 128; 
			}

			// Avoid screen edges as well
			if (suck_x < 20)
				suck_x += 48;

			// Avoid screen edges as well
			if (suck_x > 220)
				suck_x -= 48;

			roamers[i].suck_x = suck_x << 5;
			roamers[i].offset_x = 0;
			roamers[i].offset_y = 0;
			roamers[i].suck_counter = MAX_SUCK_COUNTER;
		}
	}

	// // Play song '0'
	// MUTE_SOUND();
	// StartSong(levelsongs, 0);

	// Put 5 transparent sprites at bottom of the screen
	#define SCANLINE 173
	put_4x4_sprite_offset_fast(1, 0x00, 0, SCANLINE);
	put_4x4_sprite_offset_fast(1, 0x00, 0, SCANLINE);
	put_4x4_sprite_offset_fast(1, 0x00, 0, SCANLINE);
	put_4x4_sprite_offset_fast(1, 0x00, 0, SCANLINE);
	put_4x4_sprite_offset_fast(1, 0x00, 0, SCANLINE);
	render_sprites(FALSE, SAL);

	// Pre-render car sprites to buffer
	// In the main loop, we will only update the x positions and num_sprites
	// This speeds up the rendering of these 18 sprites considerable
	int left_offset = 80;
	// int right_offset = 64;
	switch (game.car_id)
	{
		case CAR_ID_BEETLE:
			pre_render_beetle(scroll_x);
			render_car_func = pre_render_beetle;
			break;
		case CAR_ID_HEARSE:
			pre_render_hearse(scroll_x);
			render_car_func = pre_render_hearse;
			break;
		case CAR_ID_WAGON:
			pre_render_wagon(scroll_x);
			render_car_func = pre_render_wagon;
			break;
		case CAR_ID_SPORTSCAR:
			pre_render_sportscar(scroll_x);
			render_car_func = pre_render_sportscar;
			break;
	}

	while(traveled < distance)
	{
		int flip_buffer = (backbuffer_sit + 1) & 0x01;

		// Did the player request the status message?
		if (read_spacebar())
			show_status_message(sitlocations[0]);

		// Show scrolling message if needed
		do_scrolling_text(sitlocations[0] + 704);

		// Increase PK energy
		increase_pk(sitlocations[0]);

		// Flip buffers
		VDP_SET_REGISTER(VDP_REG_PDT, (scroll_x % 8));
		VDP_SET_REGISTER(VDP_REG_NT, sitregisters[flip_buffer]);

		render_sprites_fast_offset(5);			// Skip first 5 sprites for speed, they remain static

		// Scan keys and do movement
		joyst_result = read_joyst(JOYST_1);

		// RIGHT pressed
		if ( joyst_result & JOYST_LEFT )
		{
			if (scroll_x < 168)
			scroll_x++;
		}
		// LEFT pressed
		else if ( joyst_result & JOYST_RIGHT )
		{
			if (scroll_x > 4)
				scroll_x--;
		}

		// Fire button mashed
		if (joyst_result & JOYST_FIRE)
		{
			fire_pressed = TRUE;
		}
		else 
		{
			fire_pressed = FALSE;
		}

		// Update distance and speed
		if (traveled < 120)
			speed = (traveled / 20) + 1;
		if ((distance - traveled) < 240)
		{
			speed = (distance - traveled) / 40;
			if (prev_scroll_x > 4)
				scroll_x = prev_scroll_x - 1;
			else
			{
				prev_scroll_x = 4;
				scroll_x = 4;
			}
		}
		traveled++;

		// The following relies heavily on the order in which sprites are added to the render buffer
		// We can potentially have too many sprites on a line, so we manage which ones are guaranteed
		// to be visible:
		// 		Priority 1: roamers
		//		Priority 2: car outline sprites
		// 		Priority 3: road markings

		// 1. Render roamers, if any...
		put_roamers(traveled, fire_pressed, scroll_x);

		// 2. Render car 'edge' sprites
		int sprite_scroll_x = 256 - scroll_x - left_offset;
		render_car_func(scroll_x);

		// 3. Render road markings
		put_roadmarkings(markings_y, sprite_scroll_x);
		markings_y += speed;

		if (markings_y > 255)
			markings_y = 1;

		// Call scrolling routine
		scroll(prev_scroll_x, scroll_x);		// Push scrolling data to VDP and update registers
		prev_scroll_x = scroll_x;

		// Wait for the 5th sprite flag to be set on the fifth sprite
		wait_for_5th_sprite();

		// wait for vsync
		// Play music and SFX
		VSYNC_PLAY;
		vdpchar(SAL * 0x80, 173);				// Re-position the first (hidden) sprite back at line 173

		// Loop at end of song
		if (!isSNPlaying) 
		{
			StartSong(levelsongs, 0);
		}
	}
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
}

void _do_drive_screen()
{
	// Skip this segment if the distance is too small
	if (distance > 180)
	{
		// load level data
		init_drive_section();

		// Do drive segment
		drive_loop();
	}

	StopSfx();
	StartSfx(levelfx, SOUND_SILENT, 0);
}
