#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"
#include "defines.h"

typedef struct
{
	unsigned int	x;
	unsigned int	y;
	int				hdir;
	int				vdir;
} player_entity;

typedef struct
{
	unsigned int	x;
	unsigned int	y;
	unsigned int    color;
	unsigned int    destroyed;
	unsigned int    haunted;
	unsigned int    prehaunt;
	unsigned int    toggle;
	unsigned int    type;
} building;

typedef struct
{
	unsigned int	building_id;
	unsigned int	pk_level;
} staypuft_attack;

typedef struct
{
	unsigned long  	pk;
	unsigned int    account;
	unsigned int  	men, battery, traps;
	unsigned int    tick, frames_per_tick, frames_per_tick_reset;
	unsigned int    roamer_tick, frames_per_roamer_tick;
	unsigned int	haunted_tick;
	unsigned int    num_haunted_bldgs, max_haunted_bldgs;
	unsigned int 	max_traps, carspeed, car_id;
	unsigned int    meter, bait, vacuum, intensifier, detector, laser;
	unsigned char   account_name[MAX_NAME_LEN];
	unsigned int    account_start;
	unsigned int    portal_closed;
} game_state;

typedef struct
{
	unsigned int	x, y;
	unsigned int    frozen, distance, suck_x;
	unsigned int    offset_x, offset_y, suck_counter;
	int    			dx, dy;
	unsigned int    sp_patt1, sp_patt2;
	unsigned int    color1, color2;
} roamer;

typedef struct
{
	unsigned char	y, x, patt, color;
} sprite_entry;

typedef struct
{
	int 			frames_per_tick; 		
	int 			frames_per_roamer_tick; 
	int 			max_haunted_bldgs; 			
} progression;

typedef struct
{
	int  	x, y;
	int  	hdir, vdir;
	int     state;
	int     framecounter, delay;
	int     direction;
} man;

extern unsigned char 		f18adetected;
extern unsigned char		vblank_counter;
extern unsigned char 		gSaveIntCnt;
extern unsigned char		prio_counter;
extern unsigned char		levelsongs[MAX_SONG_SIZE];
extern unsigned char		levelfx[MAX_FX_SIZE];
extern unsigned char		sprite_attribute_list[MAX_SAL_SIZE];
extern int 					num_sprites;
extern player_entity 		player;
extern building 			buildings[20];
extern game_state			game;
extern roamer  				roamers[4];
extern unsigned char		map[24][MAX_LEVEL_SIZE / 24];
extern unsigned char 		backbuffer_sit;
extern unsigned char   		backbuffer_counter;
extern int					startrow;
extern int					startcol;
extern int					backscreen;
extern unsigned char 		sitregisters[2];
extern unsigned int  		sitlocations[2];
extern unsigned int			do_flip;
extern int              	distance;
extern int              	disp_pk;
extern const unsigned int 	roamer_homes[4][2];
extern int 					scroll_pos;
extern int 					scroll_len;
extern char* 				scroll_msg;
extern int					max_pk_reached;
extern char 				strbuf[8];
extern int    				strbuf_len;
extern staypuft_attack		staypuft_attacks[5];
extern int  				currentpuft;
extern int 					next_threshold;

#define VSYNC_PLAY 															\
{															 				\
	vdpwaitvint(); 															\
																			\
	/* Play music */														\
	{ 																		\
		CALL_PLAYER_SFX; 													\
		CALL_PLAYER_SN;														\
	} 																		\
																			\
	/* Check for FCTN-= (QUIT) */											\
	check_quit(); 															\
}

#define PRIO 	   (prio_counter++)

#endif
