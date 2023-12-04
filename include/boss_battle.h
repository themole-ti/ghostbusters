#ifndef BOSS_BATTLE_H
#define BOSS_BATTLE_H

typedef struct
{
	int 	bank;
	int 	offset;
	int 	length;
} frame_address;

#define NFRAMES 43
extern frame_address frame_addresses[NFRAMES];

void _do_boss_battle_screen();

#endif