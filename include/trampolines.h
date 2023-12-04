#ifndef TRAMPOLINES_H
#define TRAMPOLINES_H

// System utility functions
void breakpoint();
void init_data_section();

// Game sections
void do_title_screen();
void do_catch_screen(int id);
void do_drive_screen();
int do_map_screen();
void do_boss_battle_screen();
void do_account_screen();
void do_gameover_screen();
void do_shop();
void do_outro();
void do_logo();

// Game utility functions
unsigned char strlength(const unsigned char* string);

#endif
