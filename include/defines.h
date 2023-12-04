#ifndef DEFINES_H
#define DEFINES_H

#undef geneve

// Array sizes
#define MAX_SONG_SIZE			9516	// Maximum soundlist size in bytes
#define MAX_FX_SIZE				954		// Maximum soundlist size in bytes
#define NUM_BUILDINGS 			20 		// Number of buildings on the map
#define MAX_LEVEL_SIZE			8160	// Maximum level size in bytes
#define MAX_BATTERY_CHARGE      1800	// Battery charge of your proton packs, reduces every frame the beams are firing
#define MAX_ROAMERS				4  		// Number of roamers on the map screen
#define MAX_SAL_SIZE 			256     // Maximum number of bytes in sprite attribute table
#define MAX_NAME_LEN 			17		// Maximum length of account name string

// Sounds
#define SOUND_BLIP 				0
#define SOUND_VACUUM			1
#define SOUND_BEAM				2
#define SOUND_TRAP				3
#define SOUND_BOOM 				4
#define SOUND_BRRR 				5
#define SOUND_SILENT 			6

// tms9918a VRAM layout defines
// These are register values
#define PG 						0x02	// Pattern Generator table
#define NT						0x00	// Points to first of two nametables, one is always used to display, the other two as backbuffers
#define CT						0x90	// Color table
#define SAL						0x7e	// Points to sprite attributes
#define SDT 					0x07	// Sprite pattern generator table

// Defines for car driving sections
#define GM1_SIT1				0x01	// Points to first of two nametables, one is always used to display, the other as backbuffer
#define GM1_SIT2				0x03	// Points to second of two nametables.
#define GM1_CT					0x90	// Color table (always the same)
#define GM1_SAL					0x1e	// Points to sprite attributes
#define GM1_SDT     			0x02

// Locations of graphics in VRAM
#define FONT_MEM_OFFSET 		5632	// (22 * 32 * 8) -> Offset binary blob where font lives in bitmap mode
#define FONT_MEM_LENGTH 		512		// (2 * 32 * 8)  -> how many bytes in font pattern
#define SCROLL_SPEED 			6		// Font scrolling speed for scrolling status bar text

// Sprite and pattern defines
#define trap_pattern 			30		// Sprite pattern for ghost trap
#define beam_pattern_start		31		// Sprite pattern for proton pack
#define slimed_patt1  			51		// Slimer/roamer green sprite pattern
#define slimed_patt2  			52		// Slimer/roamer white sprite outline pattern
#define fptn_offset 			64		// Font pattern definition offset in pattern table
#define fptn_ascii_offset 		32		// Offset in font patterns where ascii characters begin
#define car_top 				20		// top of car vs. screen border in pixels
#define car_bottom 				150		// bottom of car vs. screen border in pixels
#define roadmarking_pattern 	50		// Sprite pattern for road markings
#define dot_patt 				164		// Pattern id depicting the "pacman" style dot you lay down when driving around
#define bait_patt 				168		// Pattern id depicting ghost bait
#define empty_patt 				0 		// Pattern id depicting black empty space
#define ghostlogo_red 			28 		// Ghost logo sprite pattern - red circle
#define ghostlogo_white 		29 		// Ghost logo sprite pattern - white ghost

// Building type definitions
#define BLDG_TYPE_GHQ 			0
#define BLDG_TYPE_TWO_TOWERS	1
#define BLDG_TYPE_SET_BACK		2
#define BLDG_TYPE_COURTYARD		3
#define BLDG_TYPE_TOWER			4
#define BLDG_TYPE_ZUUL			5
#define BLDG_TYPE_SET_BACK_B	6
#define BLDG_TYPE_TOWER_B		7
#define BLDG_TYPE_TWO_TOWERS_B	8

// Building colorization enums
#define BLDG_NORMAL 			0		// Not haunted
#define BLDG_RED 				1		// Haunted
#define BLDG_MAGENTA			2		// Haunting imminent
#define BLDG_WHITE 				3		// Marshmallow man imminent

// Car identifiers
#define CAR_ID_BEETLE 			0
#define CAR_ID_HEARSE 			1
#define CAR_ID_WAGON  			2
#define CAR_ID_SPORTSCAR 		3


// Game management defines
#define DIR_LEFT  				0		// sprite orientation, or position of man on screen
#define DIR_RIGHT 				1		// sprite orientation, or position of man on screen
#define RELUCTANCE 				230 	// Minimum value rnd() needs to reach before ghost changes direction/velocity
#define trap_y_offset 			10 		// Transparent pixels at top of trap sprite
#define START_COUNTER 			60 		// Counts number of frames before/during animations, or before moving to the next screen
#define BBOX 					14		// Collision detection bounding box size for freezing ghosts on map screen
#define ROAMER_PK_PENALTY 		100		// Penalty to PK energy level when roamer reaches ZUUL
#define GHOST_LEFT_PK_PENALTY 	300 	// Penalty for ghosts leaving a haunting building before getting caught
#define GAME_SPEED 				76		// Initial number of frames per game 'tick', speeds up throughout the game
#define GHQ_ID 					17      // Id of GHQ building in buildings array
#define ZUUL_ID					10      // Id of ZUUL building in buildings array
#define TICKS_PER_HAUNT_CHECK   121     // How many vsync ticks between checks for haunted buildings
#define HAUNTING_THRESHOLD		150 	// Threshold for rnd() check to haunt a new building
#define MARSHMALLOW_SENSOR_TIME 200 	// Pre-warning time for marshmallow attacks if sensor is installed
#define MARSHMALLOW_PENALTY		20 		// How much money does the player lose if they fail to foil a marshmallow man attack
#define PRE_HAUNT_VALUE 		10      // How long before a building is haunted should it be detectable by the PK meter
#define MAX_HAUNT_VALUE         90		// Maximum haunting value for buildings, the higher, the longer 
										// they stay haunted and the more money you can win
#define BEAM_BOUNCE				3		// Force with which ghosts bounce off of proton streams
#endif
