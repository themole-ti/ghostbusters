#include "vdp.h"
#include "bankswitch.h"
#include "globals.h"
#include "rnd.h"
#include "game_common.h"
#include "trampolines.h"

char* battery_msg = "PROTON PACK BATTERIES EMPTY. GO TO GHQ...                                ";
char* bait_msg    = "-NO MARSHMALLOW MAN BAIT!!!-                                             ";
char* men_msg     = "NOT ENOUGH MEN, GO TO GHQ...                                             ";
char* traps_msg   = "NO EMPTY TRAPS, GO TO GHQ...                                             ";
char* status_msg  = "BACKPACK POWER AT XX% OF MAXIMUM... X EMPTY TRAPS... X MEN LEFT...                                    ";
char* crossed_msg = "YOU CROSSED THE STREAMS. LUCKILY YOUR BATTERIES SHORTED OUT IN TIME, GO BACK TO GHQ TO RECHARGE...                                   ";
char* puftwarning = "-MARSHMALLOW ATTACK!-                                ";
char* puftpenalty = "MARSHMALLOW MAN DID $4000 WORTH OF DAMAGE. THAT AMOUNT WILL BE DEDUCTED FROM YOUR ACCOUNT.                                 ";
char* puftaward   = "THE MAYOR HAS AWARDED YOU $2000 FOR AVERTING A MARSHMALLOW MAN CATASTROPHY!                                ";
char* gotozuul    = "GO TO ZUUL! SNEAK PAST THE MARSHMALLOW MAN AND CLOSE THE PORTAL!                                ";

progression progression_table[10] = 
{
	{ GAME_SPEED     , GAME_SPEED         , 1 },
	{ GAME_SPEED / 2 , GAME_SPEED / 2     , 1 },
	{ GAME_SPEED / 4 , GAME_SPEED / 4     , 2 },
	{ GAME_SPEED / 5 , GAME_SPEED / 5     , 2 },
	{ GAME_SPEED / 6 , GAME_SPEED / 6  - 1, 2 },
	{ GAME_SPEED / 7 , GAME_SPEED / 7  - 2, 2 },
	{ GAME_SPEED / 8 , GAME_SPEED / 8  - 3, 3 },
	{ GAME_SPEED / 9 , GAME_SPEED / 9  - 4, 3 },
	{ GAME_SPEED / 11, GAME_SPEED / 11 - 3, 3 },
	{ GAME_SPEED / 13, GAME_SPEED / 13    , 3 }
};

int haunt_values[4] = { MAX_HAUNT_VALUE / 4, MAX_HAUNT_VALUE / 3, MAX_HAUNT_VALUE / 2, MAX_HAUNT_VALUE };

void scroll_status_text(int nametable_offset, char* scrollingtext, int len)
{
	scroll_msg = scrollingtext;
	scroll_pos = 0;
	scroll_len = len;
	printstr(nametable_offset + 1, "                               ", 31);
}

void show_status_message(int sitlocation)
{
	// positions 18&19 = battery
	// position  36    = traps
	// position  53    = men
	if (scroll_len < 0)
	{
		if (game.traps > 9)
			status_msg[36] = '9';
		else
			status_msg[36] = game.traps + '0';
		status_msg[53] = game.men   + '0';

		unsigned int battery = (game.battery / (MAX_BATTERY_CHARGE / 100));
		if (battery > 99)
			battery = 99;

		if (battery < 9)
		{
			status_msg[18] = ' ';
			status_msg[19] = battery + '0';
		}
		else
		{
			status_msg[18] = (battery / 10) + '0';
			status_msg[19] = (battery - ((status_msg[18] - '0') * 10)) + '0';
		}

		scroll_status_text(sitlocation + 704, (char*)status_msg, strlength((unsigned char*)status_msg));
	}
}


void do_scrolling_text(int nametable_offset)
{
	static int ticks = 0;

	nametable_offset++;
	if (scroll_len > 0)
	{
		if (ticks)
			ticks--;
		else
		{
			if (scroll_pos < 31)
				printstr(nametable_offset + 31 - scroll_pos, scroll_msg, scroll_pos);
			else
				printstr(nametable_offset, scroll_msg + (scroll_pos - 31), 31);
			scroll_pos++;
			scroll_len--;
			ticks = SCROLL_SPEED;
		}
	}
	// Re-show game status bar if we're done scrolling
	else if (scroll_len == 0)
	{
		if (game.account < 9)
			printstr(nametable_offset - 1, " CITY'S PK ENERGY: 0000     $ 00", 32);
		else if (game.account < 99)
			printstr(nametable_offset - 1, " CITY'S PK ENERGY: 0000    $  00", 32);
		else if (game.account < 999)
			printstr(nametable_offset - 1, " CITY'S PK ENERGY: 0000   $   00", 32);
		else
			printstr(nametable_offset - 1, " CITY'S PK ENERGY: 0000  $    00", 32);
		printnum(nametable_offset + 19, game.pk);
		printnum(nametable_offset + 26, game.account);
		scroll_len--;
	}
}

void haunt_buildings()
{
	if (game.num_haunted_bldgs < game.max_haunted_bldgs)
	{
		int haunt = rnd();

		// Do we haunt a new building this tick?
		if (haunt > HAUNTING_THRESHOLD)
		{
			int building_id = 0;

			// Choose a building
			building_id = rnd() & 0xf;
			if (building_id == GHQ_ID)
				building_id++;
			if (building_id == ZUUL_ID)
				building_id = 19;

			if (!(buildings[building_id].haunted) 
				&& !(buildings[building_id].destroyed)
				&& ((staypuft_attacks[currentpuft].building_id != building_id) || game.pk < 5000)	// make sure we don't haunt
				)																					// the upcoming marshmallow
			{																						// target!
				// Select value of ghostbusting job "randomly" based on vint counter
				buildings[building_id].haunted = haunt_values[gSaveIntCnt & 0x03] + PRE_HAUNT_VALUE;
				buildings[building_id].prehaunt = PRE_HAUNT_VALUE;
			}
		}
	}
}

void check_progression()
{
	if (game.pk > (next_threshold * 1000))
	{
		game.frames_per_tick 		= progression_table[next_threshold].frames_per_tick;
		game.frames_per_roamer_tick = progression_table[next_threshold].frames_per_roamer_tick;
		game.max_haunted_bldgs 		= progression_table[next_threshold].max_haunted_bldgs;

		next_threshold++;
	}
	else
		game.frames_per_tick 		= progression_table[next_threshold - 1].frames_per_tick;
}

void pk_penalty(int penalty)
{
	game.pk += penalty;
}

void increase_pk(unsigned int nt_offset)
{
	// reset the screen timeout (it counts UP by 2, so set it low and odd)
	// This avoids running of the built-in screensaver
		VDP_SCREEN_TIMEOUT = 1;	

	// Haunting logic tickover speed
	if (game.haunted_tick)
		game.haunted_tick--;
	else
	{
		game.num_haunted_bldgs = 0;
		for (int i = 0; i < NUM_BUILDINGS; i++)
		{
			if (buildings[i].haunted)
			{
				game.num_haunted_bldgs++;
				if (buildings[i].prehaunt)
					buildings[i].prehaunt--;

				buildings[i].haunted--;

				// Are we too late? -> If building just turned NOT haunted, ghost left, take PK penalty
				if (buildings[i].haunted == 0)
					game.pk += GHOST_LEFT_PK_PENALTY;
			}
		}

		haunt_buildings();
		game.haunted_tick = TICKS_PER_HAUNT_CHECK;
	}

	// PK energy tickover speed
	if (game.frames_per_tick)
	{
		game.frames_per_tick--;
		game.tick = FALSE;
	}
	else
	{
		game.pk += 1;
		game.tick = TRUE;

		if (game.pk > (9999))
		{
			game.pk = (9999);
			max_pk_reached = 1;
		}

		if (scroll_len < 1)
		{
			disp_pk = game.pk;
			uint2str(disp_pk);
			strbuf_len = strlength((const unsigned char*)strbuf);
			printstr(nt_offset + 724 + (4 - strbuf_len) - 1, strbuf, strbuf_len);
		}

		check_progression();
	}
}

void printstr(int pAddr, const char* str, int cnt)
{
	VDP_SET_ADDRESS_WRITE(pAddr);

	while (cnt--) 
		VDPWD=*(str++) + 160;
}

// unsigned int
char *uint2str(unsigned int x) 
{
	// we just populate and return strbuf
	int 	unzero = 0;
	char 	*out   = strbuf;
	int 	div    = 10000;
	int 	tmp;
	int 	offset = 0;

	while (div > 0) 
	{
		tmp = x / div;
		x = x % div;
		if ((tmp > 0) || (unzero)) 
		{
			unzero   = 1;
			*(out++) = tmp + '0' + offset;
		}
		div /= 10;
	}

	if (!unzero) 
		*(out++) = '0' + offset;

	*out = '\0';
	return strbuf;
}

void printnum(int pAddr, int num)
{
	uint2str(num);
	strbuf_len = strlength((const unsigned char*)strbuf);
	printstr(pAddr + (4 - strbuf_len) - 1, strbuf, strbuf_len);
}

void render_sprites(int do_flip, int sal_location)
{
	// For sprite flickering purposes, we push the SAL buffer to VDP RAM
	// in ascending or descending order, depending on whether the vblank_counter
	// is odd or even (effectively reversing the order of the sprites every frame)
	if ((gSaveIntCnt % 2) || (!do_flip))
	{
		unsigned char* src = sprite_attribute_list;
	
		VDP_SET_ADDRESS_WRITE(sal_location * 0x80);

		while (num_sprites--)
		{
			VDPWD=*(src++);
			VDPWD=*(src++);
			VDPWD=*(src++);
			VDPWD=*(src++);
		}
	}
	else
	{
		unsigned char* src = sprite_attribute_list + ((num_sprites - 1) * 4);
	
		VDP_SET_ADDRESS_WRITE(sal_location * 0x80);

		while (num_sprites--)
		{
			VDPWD=*(src++);
			VDPWD=*(src++);
			VDPWD=*(src++);
			VDPWD=*(src++);

			src -=8;
		}
	}		

	// Last byte contains 0xd0 to stop the VDP from rendering junk sprite data from the
	// previous frame
	VDPWD=0xd0;

	// FIXME: figure out why I need to set this explicitely
	num_sprites = 0;
}

void put_4x4_sprite_offset(unsigned char pattern, unsigned char color, unsigned char x, unsigned char y)
{
	unsigned char loc = (num_sprites << 2); 

	sprite_attribute_list[loc++] = y;
	sprite_attribute_list[loc++] = x;
	sprite_attribute_list[loc++] = (pattern << 2);
	sprite_attribute_list[loc] = color;
	
	num_sprites++;
}
