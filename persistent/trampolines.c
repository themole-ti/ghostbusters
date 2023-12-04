/********************************************/
/* Trampoline functions						*/
/* Use these to call functions in other		*/
/* banks. 									*/
/* 2015 - Danny Lousberg   					*/
/********************************************/
#include "bankswitch.h"
#include "title.h"
#include "catch.h"
#include "drive.h"
#include "utils.h"
#include "map.h"
#include "boss_battle.h"
#include "account_screen.h"
#include "shop.h"
#include "outro.h"
#include "logo.h"

extern unsigned int  _data;
extern unsigned int  _data_end;

void breakpoint()
{
	// NOP
}

// Initialize .data section
// Note, this assumes less than 8k of constant data, .bss can be much bigger of course
void init_data_section()
{
	volatile unsigned int *src = (unsigned int*)BANKADDRESS;
	volatile unsigned int *dst = &_data;

	// Switch to bank containing initialization data
	switchtobank(DATA_LOGICAL_BANK);

	// Copy .data from ROM to higher memory expansion
	while (dst < &_data_end)
		*dst++ = *src++;

	// Switch back to bank 0
	switchtobank(0);
}

unsigned char strlength(const unsigned char* string)
{
	unsigned char len = 0;

	while (string[len] != 0)
	{
		len++;
	}

	return len;
}

// In bank 5; called from bank 0 (function: main)
void do_title_screen()
{
	int old = currentbank;

	currentbank = 5;
	switchtobank(currentbank);
	_do_title_screen();

	switchtobank(old);
	currentbank = old;
}

// In bank 1; called from bank 0
void do_catch_screen(int building_id)
{
	int old = currentbank;

	currentbank = 1;
	switchtobank(currentbank);

	_do_catch_screen(building_id);

	switchtobank(old);
	currentbank = old;
}

// In bank 2; called from bank 0
void do_drive_screen()
{
	int old = currentbank;

	currentbank = 2;
	switchtobank(currentbank);

	_do_drive_screen();

	switchtobank(old);
	currentbank = old;
}

// In bank 2; called from bank 0
void do_outro()
{
	int old = currentbank;

	currentbank = 2;
	switchtobank(currentbank);

	_do_outro();

	switchtobank(old);
	currentbank = old;
}

// In bank 5; called from bank 0
void do_account_screen()
{
	int old = currentbank;

	currentbank = 5;
	switchtobank(currentbank);

	_do_account_screen();

	switchtobank(old);
	currentbank = old;
}


// In bank 5; called from bank 0
void do_gameover_screen()
{
	int old = currentbank;

	currentbank = 5;
	switchtobank(currentbank);

	_do_gameover_screen();

	switchtobank(old);
	currentbank = old;
}

// In bank 4; called from bank 0
void do_shop()
{
	int old = currentbank;

	currentbank = 4;
	switchtobank(currentbank);

	_do_shop();

	switchtobank(old);
	currentbank = old;
}

// In bank 3; called from bank 0
void do_logo()
{
	int old = currentbank;

	currentbank = 3;
	switchtobank(currentbank);

	_do_logo();

	switchtobank(old);
	currentbank = old;
}

// In bank 3
int do_map_screen()
{
	int old = currentbank;

	currentbank = 3;
	switchtobank(currentbank);

	int retval = _do_map_screen();

	switchtobank(old);
	currentbank = old;

	return retval;
}


// In bank 5
void do_boss_battle_screen()
{
	int old = currentbank;

	currentbank = 5;
	switchtobank(currentbank);

	_do_boss_battle_screen();

	switchtobank(old);
	currentbank = old;
}
