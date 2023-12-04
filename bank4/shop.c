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
#include "account_screen.h"
#include "game_common.h"

typedef struct
{
	int speed;			// Works in reverse, higher numbers are slower (more time acrued on the map)
	int capacity;		// Not used in game, only in the shop
	int price;			// Not used in game, only in the shop. Add two imaginary zeroes to this number for the display price
} car_stats;

const car_stats cars[4] =
{
	{ 50,  5,  20 },	// Compact/Beetle
	{ 37,  9,  48 },	// Hearse
	{ 30, 11,  60 },	// Wagon
	{ 25,  7, 150 }		// High-Performance
};

typedef struct 
{
	int x, y;
	int dx, dy;
	int dir;
	int patterns[2][4];
} forklift_t;

int capacity = 0;

void print(int location, const char* string)
{
	unsigned int cnt = strlength((const unsigned char*)string);
	VDP_SET_ADDRESS_WRITE(location);

	while (cnt--) 
		VDPWD=*(string++);
}

void load_shop(int car_id)
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	rom_to_vram(shop_font_patterns_bank, shop_font_patterns_offset, shop_font_patterns_length, 0x0000 + (32 * 8));
	vdpmemset(sitlocations[1], ' ', 768);

	// Load color and pattern generator resources from ROM
	ENABLE_MUSIC_ON_COPY;
		switch(car_id)
		{
			case 0:
				rom_to_vram(unwrap(beetle_shop_patterns), 0x0000 + (96 * 8));
				rom_to_vram(unwrap(beetle_shop_colors), 0x2400);
				for (int i = 0; i < 18; i++)
					rom_to_vram(beetle_shop_nametable_bank,beetle_shop_nametable_offset + (i * 12), 12, sitlocations[1] + ((i + 3) * 32) + 20);
				break;
			case 1:
				rom_to_vram(unwrap(hearse_shop_patterns), 0x0000 + (96 * 8));
				rom_to_vram(unwrap(hearse_shop_colors), 0x2400);
				for (int i = 0; i < 18; i++)
					rom_to_vram(hearse_shop_nametable_bank,hearse_shop_nametable_offset + (i * 12), 12, sitlocations[1] + ((i + 3) * 32) + 20);
				break;
			case 2:
				rom_to_vram(unwrap(wagon_shop_patterns), 0x0000 + (96 * 8));
				rom_to_vram(unwrap(wagon_shop_colors), 0x2400);
				for (int i = 0; i < 18; i++)
					rom_to_vram(wagon_shop_nametable_bank,wagon_shop_nametable_offset + (i * 12), 12, sitlocations[1] + ((i + 3) * 32) + 20);
				break;
			case 3:
				rom_to_vram(unwrap(sportscar_shop_patterns), 0x0000 + (96 * 8));
				rom_to_vram(unwrap(sportscar_shop_colors), 0x2400);
				for (int i = 0; i < 18; i++)
					rom_to_vram(sportscar_shop_nametable_bank,sportscar_shop_nametable_offset + (i * 12), 12, sitlocations[1] + ((i + 3) * 32) + 20);
				break;
		}

		// Load sprite patterns
		rom_to_vram(unwrap(shop_sprite_patterns), SDT * 0x800);
	DISABLE_MUSIC_ON_COPY;

	// Set registers
	VDP_SET_REGISTER(VDP_REG_MODE0, 0);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT | VDP_MODE1_UNBLANK | VDP_MODE1_SPRMODE16x16 );
	VDP_SET_REGISTER(VDP_REG_PDT, 0);	
	VDP_SET_REGISTER(VDP_REG_CT, GM1_CT);	
	VDP_SET_REGISTER(VDP_REG_NT, GM1_SIT2);
	VDP_SET_REGISTER(VDP_REG_SAL, 0x20);
	VDP_SET_REGISTER(VDP_REG_SDT, SDT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_WHITE << 4 | COLOR_BLACK) );
}


void print_car_stats(int car_id)
{
	// Print account balance
	char* balance = uint2str(game.account);
	print(sitlocations[1] + 32 - 6, "CREDIT");
	print(sitlocations[1] + 64 - 6, "------");
	print(sitlocations[1] + 96 - (strlength((unsigned char*)balance)) - 2, balance);
	print(sitlocations[1] + 96 - (strlength((unsigned char*)balance)) - 3, "$");
	print(sitlocations[1] + 96 - 2, "00");

	// Instructions
	print(sitlocations[1] + 704, "PRESS 1, 2, 3 OR 4 TO CHOOSE CAR");
	print(sitlocations[1] + 736, "HIT 'ENTER' TO BUY");

	switch (car_id)
	{
		case CAR_ID_BEETLE:
			// Car stats
			print(sitlocations[1] + 32 *  0, "1. COMPACT");
			print(sitlocations[1] + 32 *  1, "----------");
			print(sitlocations[1] + 32 *  5, "SPEED________75 MPH");
			print(sitlocations[1] + 32 *  6, "CAPACITY____5 ITEMS");
			print(sitlocations[1] + 32 *  7, "PRICE_________$2000");

			// Flavor text
			print(sitlocations[1] + 32 * 11, "'SLOW AND STEADY");
			print(sitlocations[1] + 32 * 12, "WINS THE RACE, BUT");
			print(sitlocations[1] + 32 * 13, "WILL IT SAVE THE");
			print(sitlocations[1] + 32 * 14, "CITY...  AT LEAST");
			print(sitlocations[1] + 32 * 15, "IT'S CHEAP!'");
			break;
		case CAR_ID_HEARSE:
			// Car stats
			print(sitlocations[1] + 32 *  0, "2. 1963 HEARSE");
			print(sitlocations[1] + 32 *  1, "--------------");
			print(sitlocations[1] + 32 *  5, "SPEED________90 MPH");
			print(sitlocations[1] + 32 *  6, "CAPACITY____9 ITEMS");
			print(sitlocations[1] + 32 *  7, "PRICE_________$4800");

			// Flavor text
			print(sitlocations[1] + 32 * 11, "'SOMETHING ABOUT");
			print(sitlocations[1] + 32 * 12, "THE COLOR SCHEME");
			print(sitlocations[1] + 32 * 13, "STANDS OUT. IT");
			print(sitlocations[1] + 32 * 14, "LOOKS ICONIC, ");
			print(sitlocations[1] + 32 * 15, "DOESN'T IT!'");
			break;
		case CAR_ID_WAGON:
			// Car stats
			print(sitlocations[1] + 32 *  0, "3. STATION WAGON");
			print(sitlocations[1] + 32 *  1, "----------------");
			print(sitlocations[1] + 32 *  5, "SPEED_______110 MPH");
			print(sitlocations[1] + 32 *  6, "CAPACITY___11 ITEMS");
			print(sitlocations[1] + 32 *  7, "PRICE_________$6000");

			// Flavor text
			print(sitlocations[1] + 32 * 11, "'IT MIGHT NOT LOOK ");
			print(sitlocations[1] + 32 * 12, "LIKE MUCH, BUT IT");
			print(sitlocations[1] + 32 * 13, "IS PRACTICAL AND");
			print(sitlocations[1] + 32 * 14, "RELIABLE. YOU CAN'T");
			print(sitlocations[1] + 32 * 15, "GO WRONG'");
			break;
		case CAR_ID_SPORTSCAR:
			// Car stats
			print(sitlocations[1] + 32 *  0, "4. HIGH-PERFORMANCE");
			print(sitlocations[1] + 32 *  1, "-------------------");
			print(sitlocations[1] + 32 *  5, "SPEED_______160 MPH");
			print(sitlocations[1] + 32 *  6, "CAPACITY____7 ITEMS");
			print(sitlocations[1] + 32 *  7, "PRICE________$15000");

			// Flavor text
			print(sitlocations[1] + 32 * 11, "'WHO CARES ABOUT");
			print(sitlocations[1] + 32 * 12, "EQUIPMENT SPACE");
			print(sitlocations[1] + 32 * 13, "WHEN A TRIP BACK TO");
			print(sitlocations[1] + 32 * 14, "HQ ONLY TAKES A FEW");
			print(sitlocations[1] + 32 * 15, "MINUTES'");
			break;
	}
}

void update_credit()
{
	print(sitlocations[1] + 96 - 7, "       ");
	char* balance = uint2str(game.account);
	print(sitlocations[1] + 96 - (strlength((unsigned char*)balance)) - 2, balance);
	print(sitlocations[1] + 96 - (strlength((unsigned char*)balance)) - 3, "$");
	print(sitlocations[1] + 96 - 2, "00");
}

void choose_car()
{
	int done = 0;

	// Show initial car
	load_shop(CAR_ID_BEETLE);

	// Print car stats
	print_car_stats(CAR_ID_BEETLE);

	int selected_car = CAR_ID_BEETLE;
	while (!done)
	{
		int key = 0;
		while (key != 13)
		{
			key = read_keyboard();

			switch(key)
			{
				case '1':
					if (selected_car != CAR_ID_BEETLE)
					{
						load_shop(CAR_ID_BEETLE);
						print_car_stats(CAR_ID_BEETLE);
						selected_car = CAR_ID_BEETLE;
					}
					break;
				case '2':
					if (selected_car != CAR_ID_HEARSE)
					{
						load_shop(CAR_ID_HEARSE);
						print_car_stats(CAR_ID_HEARSE);
						selected_car = CAR_ID_HEARSE;
					}
					break;
				case '3':
					if (selected_car != CAR_ID_WAGON)
					{
						load_shop(CAR_ID_WAGON);
						print_car_stats(CAR_ID_WAGON);
						selected_car = CAR_ID_WAGON;
					}
					break;
				case '4':
					if (selected_car != CAR_ID_SPORTSCAR)
					{
						load_shop(CAR_ID_SPORTSCAR);
						print_car_stats(CAR_ID_SPORTSCAR);
						selected_car = CAR_ID_SPORTSCAR;
					}
					break;
				default:
					break;
			}

			// Mute if done
			if (!isSNPlaying) 
			{
				MUTE_SOUND();
				StartSong(levelsongs, 0);
			}

			VSYNC_PLAY;
		}


		// Player made a choice, let's see if they can afford it.
		if (cars[selected_car].price < game.account)
		{
			game.account -= cars[selected_car].price;
			game.carspeed = cars[selected_car].speed;
			game.car_id   = selected_car;
			capacity      = cars[selected_car].capacity;
			done = 1;
		}
		else
		{
			// PLAY SOUND FX HERE
			int count = 100;
			while (count)
			{
				if (count ==  100)
					print(sitlocations[1] + 96 - 7, "       ");
				if (count ==  80)
					update_credit();
				if (count ==  60)
					print(sitlocations[1] + 96 - 7, "       ");
				if (count ==  40)
					update_credit();
				if (count ==  20)
					print(sitlocations[1] + 96 - 7, "       ");
				if (count ==  1)
					update_credit();

				// Mute if done
				if (!isSNPlaying) 
				{
					MUTE_SOUND();
					StartSong(levelsongs, 0);
				}

				VSYNC_PLAY;

				count--;
			}
		}
	}
}

void erase_text()
{
	vdpmemset(sitlocations[1] +   0, ' ', 24);
	vdpmemset(sitlocations[1] +  32, ' ', 24);
	for (int i = 2; i < 20; i++)
		vdpmemset(sitlocations[1] + (i * 32), ' ', 20);
}

void render_page(int selected_page)
{
	switch(selected_page)
	{
		case 0:
			erase_text();
			print(sitlocations[1] + 32 *  0, "1. MONITORING EQUIPMENT");
			print(sitlocations[1] + 32 *  1, "-----------------------");
			print(sitlocations[1] + 32 *  3, "PK ENERGY METER");
			print(sitlocations[1] + 32 *  4, "              $400");
			print(sitlocations[1] + 32 *  8, "IMAGE INTENSIFIER");
			print(sitlocations[1] + 32 *  9, "              $800");
			print(sitlocations[1] + 32 * 13, "MARSHMALLOW SENSOR ");
			print(sitlocations[1] + 32 * 14, "              $800");

			break;
		case 1:
			erase_text();
			print(sitlocations[1] + 32 *  0, "2. CAPTURE EQUIPMENT");
			print(sitlocations[1] + 32 *  1, "--------------------");
			print(sitlocations[1] + 32 *  3, "GHOST BAIT");
			print(sitlocations[1] + 32 *  4, "              $400");
			print(sitlocations[1] + 32 *  8, "TRAPS  -REQUIRED-");
			print(sitlocations[1] + 32 *  9, "              $600");
			print(sitlocations[1] + 32 * 13, "GHOST VACUUM");
			print(sitlocations[1] + 32 * 14, "              $500");

			break;
		case 2:
			erase_text();
			print(sitlocations[1] + 32 *  0, "3. STORAGE EQUIPMENT");
			print(sitlocations[1] + 32 *  1, "--------------------");
			print(sitlocations[1] + 32 *  3, "PORTABLE LASER");
			print(sitlocations[1] + 32 *  4, "CONFINEMENT SYSTEM");
			print(sitlocations[1] + 32 *  5, "             $8000");

			break;
	}
}

void put_forklift(forklift_t* forklift)
{
	if (forklift->dir == DIR_LEFT)
	{
		put_4x4_sprite_offset(forklift->patterns[0][0], 0x0d, (forklift->x >> 4) + 16, (forklift->y >> 4));
		put_4x4_sprite_offset(forklift->patterns[0][1], 0x04, (forklift->x >> 4) + 16, (forklift->y >> 4));
		put_4x4_sprite_offset(forklift->patterns[0][2], 0x04, (forklift->x >> 4)     , (forklift->y >> 4) + 16);
		put_4x4_sprite_offset(forklift->patterns[0][3], 0x04, (forklift->x >> 4) + 16, (forklift->y >> 4) + 16);
	}
	else
	{
		put_4x4_sprite_offset(forklift->patterns[1][0], 0x0d, (forklift->x >> 4) - 17, (forklift->y >> 4));
		put_4x4_sprite_offset(forklift->patterns[1][1], 0x04, (forklift->x >> 4) - 17, (forklift->y >> 4));
		put_4x4_sprite_offset(forklift->patterns[1][2], 0x04, (forklift->x >> 4) -  1, (forklift->y >> 4) + 16);
		put_4x4_sprite_offset(forklift->patterns[1][3], 0x04, (forklift->x >> 4) - 17, (forklift->y >> 4) + 16);
	}
}

void put_items(int page, int offset_id, int offset_x, int offset_y)
{
	int offsets[3][2];

	for (int i = 0; i < 3; i++)
	{
		if (i == offset_id)
		{
			offsets[i][0] = offset_x;
			offsets[i][1] = offset_y;
		}
		else
		{
			offsets[i][0] = 0;
			offsets[i][1] = 0;
		}
	}


	switch(page)
	{
		case 0:
			// PK Energy meter
			if (!game.meter)
			{
				put_4x4_sprite_offset( 4, 0x07, 16 + offsets[0][0],  40 + offsets[0][1]);
				put_4x4_sprite_offset(11, 0x04, 16 + offsets[0][0],  40 + offsets[0][1]);
			}
			if (!game.intensifier)
			{
				put_4x4_sprite_offset( 6, 0x02, 16 + offsets[1][0],  80 + offsets[1][1]);
				put_4x4_sprite_offset(13, 0x04, 16 + offsets[1][0],  80 + offsets[1][1]);
			}
			if (!game.detector)
			{
				put_4x4_sprite_offset( 1, 0x0f, 16 + offsets[2][0], 120 + offsets[2][1]);
				put_4x4_sprite_offset( 8, 0x04, 16 + offsets[2][0], 120 + offsets[2][1]);
			}
			break;
		case 1:
			if (!game.bait)
			{
				put_4x4_sprite_offset( 2, 0x0b, 16 + offsets[0][0],  40 + offsets[0][1]);
				put_4x4_sprite_offset( 9, 0x04, 16 + offsets[0][0],  40 + offsets[0][1]);
			}
		
				put_4x4_sprite_offset( 5, 0x0b, 16 + offsets[1][0],  80 + offsets[1][1]);
				put_4x4_sprite_offset(12, 0x04, 16 + offsets[1][0],  80 + offsets[1][1]);
		
			if (!game.vacuum)
			{
				put_4x4_sprite_offset( 3, 0x0e, 16 + offsets[2][0], 120 + offsets[2][1]);
				put_4x4_sprite_offset(10, 0x04, 16 + offsets[2][0], 120 + offsets[2][1]);
			}
			break;
		case 2:
			if (!game.laser)
			{
				put_4x4_sprite_offset( 0, 0x0d, 16 + offsets[0][0],  40 + offsets[0][1]);
				put_4x4_sprite_offset( 7, 0x04, 16 + offsets[0][0],  40 + offsets[0][1]);
			}
			break;
	}
}

void wait_for_no_input()
{
	int joyst = read_joyst(JOYST_1);

	while (joyst & JOYST_FIRE)
	{
		// Mute if done
		if (!isSNPlaying) 
		{
			MUTE_SOUND();
			StartSong(levelsongs, 0);
		}

		VSYNC_PLAY;		
		joyst = read_joyst(JOYST_1);
	}
}

#define SHOP_STATE_SELECTING 0
#define SHOP_STATE_BRINGING  1
#define SHOP_STATE_DROPPING  2

void buy_equipment()
{
	int selected_page = 0;
	int state = SHOP_STATE_SELECTING;

	// Load sprites into memory
	forklift_t forklift;

	forklift.x = 17 << 4;
	forklift.y = 40 << 4;
	forklift.dx = 0;
	forklift.dy = 0;
	forklift.patterns[0][0] = 18;
	forklift.patterns[0][1] = 19;
	forklift.patterns[0][2] = 21;
	forklift.patterns[0][3] = 20;
	forklift.patterns[1][0] = 14;
	forklift.patterns[1][1] = 15;
	forklift.patterns[1][2] = 17;
	forklift.patterns[1][3] = 16;
	forklift.dir = DIR_LEFT;

	// Prep screen
	erase_text();
	print(sitlocations[1] + 704   , "PRESS 1, 2 OR 3 FOR MORE OPTIONS");
	print(sitlocations[1] + 736   , "PRESS 'ENTER' TO END            ");
	update_credit();

	render_page(selected_page);

	int key = read_keyboard();
	int joyst_result = 0;
	int move_timer = 0;
	int max_y = 120 << 4;
	int page = 0;
	int item_y = forklift.y;
	int offset_id = 999;
	int offset_x = 0;
	int offset_y = 0;
	int leave = 0;
	int show_msg = -1;
	while (key) { key = read_keyboard(); };
	while (!leave)
	{
		// reset the screen timeout (it counts UP by 2, so set it low and odd)
		// This avoids running of the built-in screensaver
   		VDP_SCREEN_TIMEOUT = 1;	
		
		render_sprites(TRUE, 0x20);
		put_forklift(&forklift);
		put_items(page, offset_id, offset_x, offset_y);
		if (!move_timer)
		{
			key = read_keyboard();
			joyst_result = read_joyst(JOYST_1);
		}
		else
		{
			if (move_timer % 4)
				StartSfx(levelfx, SOUND_BRRR, PRIO);
			joyst_result = 0;
		}

		switch (state)
		{
			case SHOP_STATE_SELECTING:
				{
					if ((key >= '1') && (key <= '3'))
					{
						int newpage = key - '1';
						if (newpage != page)
						{
							page = newpage;
							render_page(page);

							if (page == 2)
							{
								max_y = 40 << 4;
								if (forklift.y > max_y)
									forklift.y = max_y;
							}
							else
								max_y = 120 << 4;

							while (key == '1' || key == '2' || key == '3')
							{
								// Mute if done
								if (!isSNPlaying) 
								{
									MUTE_SOUND();
									StartSong(levelsongs, 0);
								}

								put_items(page, offset_id, offset_x, offset_y);
								render_sprites(TRUE, 0x20);

								VSYNC_PLAY;		
								key = read_keyboard();
							}
						}
					}

					if (joyst_result & JOYST_DOWN)
					{
						if (forklift.y < max_y)
						{
							forklift.dx = 0;
							forklift.dy = 1 << 4;
							move_timer = 40;
						}
					}
					else
					if (joyst_result & JOYST_UP)
					{
						if (forklift.y > (40 << 4))
						{
							forklift.dx = 0;
							forklift.dy = -1 << 4;
							move_timer = 40;
						}
					}

					if (move_timer)
					{
						forklift.x += forklift.dx;
						forklift.y += forklift.dy;
						move_timer--;
					}

					if (joyst_result & JOYST_FIRE)
					{
						item_y = forklift.y;
						int item_id = ((item_y >> 4) / 40) - 1;

						switch(page)
						{
							case 0:
								{
									switch(item_id)
									{
										case 0:
											if (!game.meter)
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
										case 1:
											if (!game.intensifier)
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
										case 2:
											if (!game.detector)
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
									}									
								}
								break;
							case 1:
								{
									switch(item_id)
									{
										case 0:
											if (!game.bait)
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
										case 1:
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
										case 2:
											if (!game.vacuum)
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
									}									
								}
								break;
							case 2:
								{
									switch(item_id)
									{
										case 0:
											if (!game.laser)
											{
												state = SHOP_STATE_BRINGING;
												forklift.dir = DIR_RIGHT;
												wait_for_no_input();
											}
											break;
									}									
								}
								break;
						}
					}
				}
				break;
			case SHOP_STATE_BRINGING:
				{
					// Picked up an item, now joystick should be moved
					// left and right only to move the thing to the car
					// Pressing fire will move the state machine to DROPPING
					if (joyst_result & JOYST_FIRE)
					{
						if (forklift.x < (20 << 4))
						{
							state = SHOP_STATE_SELECTING;
							forklift.dir = DIR_LEFT;
							wait_for_no_input();
							offset_id = 999;
						}
						else
						{
							// Okay, we've dropped something at the car, let's do
							// some bookkeeping
							int selected_item = ((item_y >> 4) / 40) - 1;

							if (capacity)
							{
								switch(page)
								{
									case 0:
										if (selected_item == 0)
										{
											// PK Energy meter
											if (game.account > 4)
											{
												game.meter = 1;
												game.account -= 4;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										else if (selected_item == 1)
										{
											// Image intensifier
											if (game.account > 8)
											{
												game.intensifier = 1;
												game.account -= 8;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										else if (selected_item == 2)
										{
											// Marshmallow sensor
											if (game.account > 8)
											{
												game.detector = 1;
												game.account -= 8;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										break;
									case 1:
										if (selected_item == 0)
										{
											// Ghost bait
											if (game.account > 4)
											{
												game.bait = 5;
												game.account -= 4;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										else if (selected_item == 1)
										{
											// Trap
											if (game.account > 6)
											{
												game.max_traps++;
												game.account -= 6;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										else if (selected_item == 2)
										{
											// Ghost Vacuum
											if (game.account > 5)
											{
												game.vacuum = 1;
												game.account -= 5;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										break;
									case 2:
										if (selected_item == 0)
										{
											// Containment system
											if (game.account > 80)
											{
												game.laser = 1;
												game.max_traps += 9;
												game.account -= 80;
												offset_id = 999;
												update_credit();
												wait_for_no_input();
												forklift.dx = -2 << 4;
												forklift.dy = -(((140 << 4) - item_y) / 75);
												move_timer = 75;
												forklift.dir = DIR_LEFT;
												state = SHOP_STATE_SELECTING;
											}
										}
										break;
								}

								capacity--;
							}
							if (!capacity)
							{
								print(sitlocations[1] + 32 * 21, "        -- CAR IS FULL! --       ");
								print(sitlocations[1] + 32 * 22, "PRESS 'ENTER' TO START           ");
								print(sitlocations[1] + 32 * 23, "                                 ");
							}

						}
					}
					if (joyst_result & JOYST_RIGHT)
					{
						if (forklift.x < (140 << 4))
						{
							forklift.dx = 2 << 4;
							forklift.dy = (((140 << 4) - item_y) / 75);
							offset_id = ((item_y >> 4) / 40) - 1;
							move_timer = 75;
						}
					}

					if (joyst_result & JOYST_LEFT)
					{
						if (forklift.x > (20 << 4))
						{
							forklift.dx = -2 << 4;
							forklift.dy = -(((140 << 4) - item_y) / 75);
							move_timer = 75;
						}
					}

					if (move_timer)
					{
						forklift.x += forklift.dx;
						forklift.y += forklift.dy;

						if (offset_id != 999)
						{
							offset_x = (forklift.x >> 4) - 17;
							offset_y = (forklift.y >> 4) - (40 * (offset_id + 1));
						}

						move_timer--;
					}
				}
				break;
			case SHOP_STATE_DROPPING:
				{
					// Okay, drop the item and return back to the right hand
					// side of the screen
				}
				break;
		}

		if (key == 13)
		{
			// Check if you have at least one trap
			if (!game.max_traps)
			{
				show_msg = 120;
			}
			else
				leave = 1;
		}

		if (show_msg == 120)
		{
			print(sitlocations[1] + 32 * 21, "-- AT LEAST ONE TRAP REQUIRED --");
		}

		if (show_msg > 0)
		{
			show_msg--;
		}

		if (show_msg == 0)
		{
			print(sitlocations[1] + 32 * 21, "                                ");
			show_msg--;
		}

		// Mute if done
		if (!isSNPlaying) 
		{
			MUTE_SOUND();
			StartSong(levelsongs, 0);
		}

		VSYNC_PLAY;
	}
}

// Display title screen and wait for user to press fire
void _do_shop()
{
	// Load song and sfx from rom
	rom_to_ram(unwrap(maintheme), (unsigned char*)levelsongs);
	rom_to_ram(unwrap(fx), (unsigned char*)levelfx);

	// Play song '0'
	MUTE_SOUND();
	StartSong(levelsongs, 0);

	// First pick a car
	choose_car();

	// Now buy equipment
	buy_equipment();

	// Pressed fire, make noise!
	StopSong(); StopSfx();
	MUTE_SOUND();
	// StartSfx(levelfx, 1, PRIO);
}
