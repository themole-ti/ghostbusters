#include "globals.h"
#include "bankswitch.h"
#include "vdp.h"

unsigned char			f18adetected = 0;
volatile unsigned int 	currentbank = 0;
volatile unsigned int 	*bankbase = (unsigned int*)BANKADDRESS;
unsigned char			vblank_counter = 0;
unsigned char			levelsongs[MAX_SONG_SIZE];
unsigned char			levelfx[MAX_FX_SIZE];
unsigned char			sprite_attribute_list[MAX_SAL_SIZE];
int 					num_sprites = 0;
player_entity 			player;
building 				buildings[20];
game_state				game = { 0 };
roamer  				roamers[4];
unsigned char			map[24][MAX_LEVEL_SIZE / 24];
unsigned char 			backbuffer_sit;
unsigned char   		backbuffer_counter;
int						startrow;
int						startcol;
int						backscreen;
unsigned char 			sitregisters[2] = { GM1_SIT1, GM1_SIT2 };
unsigned int  			sitlocations[2] = { GM1_SIT1 * 0x400, GM1_SIT2 * 0x400 };
unsigned int			do_flip = 0;
int 					distance = 0;
int 		            disp_pk = 0;
int 					scroll_pos = 0;
int 					scroll_len = 0;
char* 					scroll_msg = 0;
int						max_pk_reached;
char 					strbuf[8];
int  					strbuf_len = 0;
staypuft_attack      	staypuft_attacks[5];
int						currentpuft = 0;
unsigned char 			prio_counter = 0;
int 					next_threshold = 1;
