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

#define DEBUG_VALUES 0

void detect_f18a()
{
	unsigned int joyst_result = 0;
	f18adetected = 0;

	joyst_result = read_joyst(JOYST_1);
	if (joyst_result & JOYST_FIRE)	// If fire button is pressed when launching the game, skip F18A detection
		return;

	unsigned char compare = 0;

	VDP_SET_REGISTER(57, 0x1c);		// Write twice to unlock F18A
	VDP_SET_REGISTER(57, 0x1c);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
									// Blank VDP, Disable interupts

	VDP_SET_REGISTER(15, 0x00);		// Prime status register 0 for reading
	f18adetected = VDPST;			// Read status register into variable, this resets bits 7 and 5
									// We need this to ensure the next read from status register 0 
									// remains the same (no interupts, no sprites defined)

	VDP_SET_REGISTER(15, 0x0e);		// Prime status register 14 (0x0e) for reading
	f18adetected = VDPST;			// Read status register into variable, this contains the F18A firmare version

	VDP_SET_REGISTER(15, 0x00);		// Set back to status register 0
	compare = VDPST;				// Re-read VDP status byte into comparison variable

	if ( f18adetected == compare )	// If these two values are identical, we weren't able to change the SR we read from, so it's not an F18A
	{
		f18adetected = 0x00;

		return;
	}
	else
		VDP_SET_REGISTER(30, 0x04);	// Enable 4-sprites-per-row limitation

	if ( f18adetected < 0x16 )		// If the version number is lower than 1.6, we don't support it. Upgrade that shit, chump!
		f18adetected = 0x00;

	return;
}

void init_buildings()
{
	// Initialize building structures
	buildings[ 0].x 		= 5;
	buildings[ 0].y 		= 1;
	buildings[ 0].destroyed = FALSE;
	buildings[ 0].haunted 	= 0;
	buildings[ 0].toggle 	= 0;
	buildings[ 0].type 		= 6;

	buildings[ 1].x 		= 11;
	buildings[ 1].y 		= 1;
	buildings[ 1].destroyed = FALSE;
	buildings[ 1].haunted 	= 0;
	buildings[ 1].toggle 	= 0;
	buildings[ 1].type 		= 7;

	buildings[ 2].x 		= 17;
	buildings[ 2].y 		= 1;
	buildings[ 2].destroyed = FALSE;
	buildings[ 2].haunted 	= 0;
	buildings[ 2].toggle 	= 0;
	buildings[ 2].type 		= 6;

	buildings[ 3].x 		= 23;
	buildings[ 3].y 		= 1;
	buildings[ 3].destroyed = FALSE;
	buildings[ 3].haunted 	= 0;
	buildings[ 3].toggle 	= 0;
	buildings[ 3].type 		= 7;

	buildings[ 4].x 		= 5;
	buildings[ 4].y 		= 3;
	buildings[ 4].destroyed = FALSE;
	buildings[ 4].haunted 	= 0;
	buildings[ 4].toggle 	= 0;
	buildings[ 4].type 		= 1;

	buildings[ 5].x 		= 11;
	buildings[ 5].y 		= 3;
	buildings[ 5].destroyed = FALSE;
	buildings[ 5].haunted 	= 0;
	buildings[ 5].toggle 	= 0;
	buildings[ 5].type 		= 2;

	buildings[ 6].x 		= 17;
	buildings[ 6].y 		= 3;
	buildings[ 6].destroyed = FALSE;
	buildings[ 6].haunted 	= 0;
	buildings[ 6].toggle 	= 0;
	buildings[ 6].type 		= 3;

	buildings[ 7].x 		= 23;
	buildings[ 7].y 		= 3;
	buildings[ 7].destroyed = FALSE;
	buildings[ 7].haunted 	= MAX_HAUNT_VALUE;
	buildings[ 7].toggle 	= 0;
	buildings[ 7].type 		= 4;

	buildings[ 8].x 		= 5;
	buildings[ 8].y 		= 9;
	buildings[ 8].destroyed = FALSE;
	buildings[ 8].haunted 	= 0;
	buildings[ 8].toggle 	= 0;
	buildings[ 8].type 		= 3;

	buildings[ 9].x 		= 11;
	buildings[ 9].y 		= 9;
	buildings[ 9].destroyed = FALSE;
	buildings[ 9].haunted 	= 0;
	buildings[ 9].toggle 	= 0;
	buildings[ 9].type 		= 1;

	buildings[10].x 		= 17;
	buildings[10].y 		= 9;
	buildings[10].destroyed = FALSE;
	buildings[10].haunted 	= 0;
	buildings[10].toggle 	= 0;
	buildings[10].type 		= 5;

	buildings[11].x 		= 23;
	buildings[11].y 		= 9;
	buildings[11].destroyed = FALSE;
	buildings[11].haunted 	= 0;
	buildings[11].toggle 	= 0;
	buildings[11].type 		= 2;

	buildings[12].x 		= 5;
	buildings[12].y 		= 15;
	buildings[12].destroyed = FALSE;
	buildings[12].haunted 	= 0;
	buildings[12].toggle 	= 0;
	buildings[12].type 		= 4;

	buildings[13].x 		= 11;
	buildings[13].y 		= 15;
	buildings[13].destroyed = FALSE;
	buildings[13].haunted 	= 0;
	buildings[13].toggle 	= 0;
	buildings[13].type 		= 2;

	buildings[14].x 		= 17;
	buildings[14].y 		= 15;
	buildings[14].destroyed = FALSE;
	buildings[14].haunted 	= 0;
	buildings[14].toggle 	= 0;
	buildings[14].type 		= 4;

	buildings[15].x 		= 23;
	buildings[15].y 		= 15;
	buildings[15].destroyed = FALSE;
	buildings[15].haunted 	= 0;
	buildings[15].toggle 	= 0;
	buildings[15].type 		= 1;

	buildings[16].x 		= 5;
	buildings[16].y 		= 21;
	buildings[16].destroyed = FALSE;
	buildings[16].haunted 	= 0;
	buildings[16].toggle 	= 0;
	buildings[16].type 		= 8;

	buildings[17].x 		= 11;
	buildings[17].y 		= 21;
	buildings[17].destroyed = FALSE;
	buildings[17].haunted 	= 0;
	buildings[17].toggle 	= 0;
	buildings[17].type 		= 0;

	buildings[18].x 		= 17;
	buildings[18].y 		= 21;
	buildings[18].destroyed = FALSE;
	buildings[18].haunted 	= 0;
	buildings[18].toggle 	= 0;
	buildings[18].type 		= 8;

	buildings[19].x 		= 23;
	buildings[19].y 		= 21;
	buildings[19].destroyed = FALSE;
	buildings[19].haunted 	= 0;
	buildings[19].toggle 	= 0;
	buildings[19].type 		= 8;
}

void init_roamers()
{
	roamers[0].x 			=  56 << 8;
	roamers[0].y 			=   0 << 8;
	roamers[0].frozen		=  FALSE;
	roamers[0].dx			=   2;			// To help guide roamers to Zuul
	roamers[0].dy 			=   2;
	roamers[0].sp_patt1		=  24;
	roamers[0].sp_patt2 	=  26;
	roamers[0].color1		=  15;
	roamers[0].color2		=   2;

	roamers[1].x 			= 247 << 8;
	roamers[1].y 			=   0 << 8;
	roamers[1].frozen		=  FALSE;
	roamers[1].dx			=  -2;			// To help guide roamers to Zuul
	roamers[1].dy 			=   2;
	roamers[1].sp_patt1		=  25;
	roamers[1].sp_patt2 	=  27;
	roamers[1].color1		=  15;
	roamers[1].color2		=   2;

	roamers[2].x 			=  56 << 8;
	roamers[2].y 			= 175 << 8;
	roamers[2].frozen		=  FALSE;
	roamers[2].dx			=   2;			// To help guide roamers to Zuul
	roamers[2].dy 			=  -2;
	roamers[2].sp_patt1		=  24;
	roamers[2].sp_patt2 	=  26;
	roamers[2].color1		=  15;
	roamers[2].color2		=   2;

	roamers[3].x 			= 247 << 8;
	roamers[3].y 			= 175 << 8;
	roamers[3].frozen		=  FALSE;
	roamers[3].dx			=  -2;			// To help guide roamers to Zuul
	roamers[3].dy 			=  -2;
	roamers[3].sp_patt1		=  25;
	roamers[3].sp_patt2 	=  27;
	roamers[3].color1		=  15;
	roamers[3].color2		=   2;
}

void init_staypuft_attacks()
{
	int selected[NUM_BUILDINGS] = { 0 };

	// Disable top and bottom rows for attacks
	for (int i = 0; i < 4; i++)
		selected[i] = 1;
	for (int i = 16; i < 20; i++)
		selected[i] = 1;

	// Disable zuul as well
	selected[ZUUL_ID] = 1;

	for (int i = 0; i < 5; i++)
	{
		int offset = rnd() * 4;
		staypuft_attacks[i].pk_level = 5000 + (i * 1000) + offset;

		// Choose a building
		int building_id = rnd() & 0xf;

		// If this building is already slated for attack, keep selecting a new one
		while (selected[building_id] == 1)
			building_id = rnd() & 0xf;

		staypuft_attacks[i].building_id = building_id;
		selected[building_id] = 1;
	}
	staypuft_attacks[4].pk_level += 750;

#if DEBUG_VALUES
	staypuft_attacks[0].building_id = 4;
	staypuft_attacks[0].pk_level    = 5015;
#endif
}

int main(int argc, char *argv[])
{
	// Detect F18A, we don't use it, but ensure 4-sprite-per-line limit if it is detected
#if DEBUG_VALUES
#else
	detect_f18a();
#endif

	// Initialize current bank to 0
	currentbank = 0;
	DISABLE_MUSIC_ON_COPY;

	// Seed PRNG by randomly executing a few itterations of the generator already
	vblank_counter = VDP_INT_COUNTER;
	while (vblank_counter--)
		rnd();

	// set the interrupt routine, disable unneeded processing for extra performance
	VDP_INT_CTRL = VDP_INT_CTRL_DISABLE_ALL;

	// And install our own user interrupt hook
	VDP_INT_HOOK = 0;

#if DEBUG_VALUES
#else
	do_logo();
#endif

	while (1)
	{
		game.account_start  = 0;
		game.account	    = 0;
		memset(game.account_name, 0, MAX_NAME_LEN);

		// Load song and sfx from rom
		rom_to_ram(unwrap(themesong), 	(unsigned char*)levelsongs);
		rom_to_ram(unwrap(fx), 			(unsigned char*)levelfx);

		// Show intro sequence
#if DEBUG_VALUES
#else
		do_title_screen();
		MUTE_SOUND();
#endif

#if DEBUG_VALUES
		game.account_start = 100;
#else
		do_account_screen();
#endif
		MUTE_SOUND();

		// Transfer money to account
		game.account 		= game.account_start;
		game.max_traps		= 0;
		game.bait 			= 0;
		game.vacuum 		= 0;
		game.intensifier 	= 0;
		game.detector 		= 0;
		game.meter 			= 0;
		game.laser 			= 0;

#if DEBUG_VALUES
		game.account 				= game.account_start + 5;
		game.max_traps 				= 3;
		game.carspeed 				= 37;
		game.car_id   				= CAR_ID_HEARSE;
		game.vacuum   				= 1;
		game.meter 					= 1;
		game.intensifier			= 1;
		game.bait 					= 5;
		memcpy(game.account_name, "THEMOLE", 8);

		game.pk 					= 998;
		game.men 					= 3;
		game.traps 					= game.max_traps;
		game.battery 				= MAX_BATTERY_CHARGE;
		game.num_haunted_bldgs 		= 1;
		game.max_haunted_bldgs 		= 1;
		game.tick 					= FALSE;
		game.haunted_tick			= TICKS_PER_HAUNT_CHECK;
		game.frames_per_tick		= GAME_SPEED;
		game.frames_per_tick_reset	= GAME_SPEED;
		game.frames_per_roamer_tick = GAME_SPEED;
#else
		do_shop();

		MUTE_SOUND();

		// Initialize game state
		game.pk 					= 0;
		game.men 					= 3;
		game.battery 				= MAX_BATTERY_CHARGE;
		game.traps 					= game.max_traps;
		game.num_haunted_bldgs 		= 1;
		game.max_haunted_bldgs 		= 1;
		game.tick 					= FALSE;
		game.haunted_tick			= TICKS_PER_HAUNT_CHECK;
		game.frames_per_tick		= GAME_SPEED;
		game.frames_per_tick_reset	= GAME_SPEED;
		game.frames_per_roamer_tick = GAME_SPEED;
#endif

		// Initialize player info
		player.x 	= 100;
		player.y 	= 155;
		player.hdir = 0;
		player.vdir = 0;

		// Init game state
		init_buildings();
		init_roamers();
		init_staypuft_attacks();

		// Reset game over condition
		max_pk_reached = 0;
		currentpuft = 0;
		next_threshold = 1;

// Print out which buildings will be attacked by the marshmallow man at which pk level
#if 0
		for (int i = 0; i < 5; i++)
		{
			vdpmemcpy(0x0000 + (i * 64)    , (const unsigned char*)(uint2str(staypuft_attacks[i].building_id)), strlength((const unsigned char*)strbuf));
			vdpmemcpy(0x0000 + (i * 64) + 5, (const unsigned char*)(uint2str(staypuft_attacks[i].pk_level)), strlength((const unsigned char*)strbuf));
		}
		int joyst_result = read_joyst(JOYST_1);
		while (!(JOYST_FIRE & joyst_result))
		{
			joyst_result = read_joyst(JOYST_1);

			// wait for vsync
			VSYNC_PLAY;
		}
#endif

		rom_to_ram(unwrap(themesong), 	(unsigned char*)levelsongs);

		// Main game loop (map -> drive -> catch -> back to map)
		int id;
		id = do_map_screen();
		while(!max_pk_reached)
		{
			do_drive_screen();
			do_catch_screen(id);
			id = do_map_screen();
		}

		// PK reached 9999, see if we've made enough money to go to ZUUL
		if (game.account < game.account_start)
		{
			// Nope
			MUTE_SOUND();
			do_gameover_screen();
		}
		else
		{
			// Yes!
			do_boss_battle_screen();

			// Did we close the portal? If so, show the outro animation
			if (game.men > 1)
				do_outro();

			// Give updated account information
			MUTE_SOUND();
			do_gameover_screen();
		}
	}

	return 0;
}
