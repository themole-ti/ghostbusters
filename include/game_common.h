#ifndef GAME_COMMON_H
#define GAME_COMMON_H

extern char* battery_msg;
extern char* bait_msg;
extern char* men_msg;
extern char* traps_msg;
extern char* status_msg;
extern char* crossed_msg;
extern char* battery_msg;
extern char* puftwarning;
extern char* puftpenalty;
extern char* puftaward;
extern char* gotozuul;

// Game logic functions
void increase_pk(unsigned int nt_offset);
void pk_penalty(int penalty);

// Support functions
void show_status_message(int nametable_offset);
void scroll_status_text(int nametable_offset, char* scrollingtext, int len);
void do_scrolling_text(int nametable_offset);
char *uint2str(unsigned int x);
void printnum(int pAddr, int number);
void printstr(int pAddr, const char* str, int cnt);
void put_4x4_sprite_offset(unsigned char pattern, unsigned char color, unsigned char x, unsigned char y);
void render_sprites(int do_flip, int sal_location);

#endif