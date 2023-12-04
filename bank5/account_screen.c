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

char* fire_to_countinue = "   PRESS FIRE TO CONTINUE...    ";
char* invalid_account = "   -INVALID ACCOUNT NUMBER!-    ";

// Function macro to avoid function call overhead
#define type_string_rom(source, src_line, screen_line)								\
{																					\
	rom_to_ram(source ## _bank, source ## _offset + (src_line * 32), 32, buffer);	\
	type_string(buffer, 0x1800 + (screen_line * 32), 0);							\
}																					\


void type_string(const unsigned char* string, int location, int characteroffset)
{
	int i = 0;
	int len = strlength(string);
	if (len > 32)
		len = 32;

	while (i < len)
	{
		if ((characteroffset + string[i]) > 32)
		{
			vdpchar(location + i, characteroffset + string[i]);
			if (!(i % 4)) 
				StartSfx(levelfx, SOUND_BLIP, i);

			// wait for vsync
			VSYNC_PLAY;
		}

		i++;
	}
	vdpchar(location + i, characteroffset + ' ');
}

unsigned char* get_string(unsigned char* dst, int max_len, int row, int col)
{
	unsigned char key = 0;
	unsigned char prev_key = 0;
	int len = 0;
	int delay = 30;
	int cursor = 1;
	int leave = 0;
	while (!leave)
	{
		key = read_keyboard();

		// reset the screen timeout (it counts UP by 2, so set it low and odd)
		// This avoids running of the built-in screensaver
   		VDP_SCREEN_TIMEOUT = 1;	

		if (key != prev_key)
		{
			// Check for ctrl-s ('backspace')
			if (key & 0x80)
			{
				// ctrl pressed, check underlying key
				if ((key & 0x7f) == 'S')
				{
					if (len > 0)
					{
						vdpchar(0x1800 + (row*32) + col + len, '_');
						dst[len] = '\0';
						len--;
					}
				}
			}
			else
			{
				// Printable character? Let's print it!
				if ((key >= 32) && (key <= 127))
				{
					if (len < 16)
					{
						if (key > 96)
							key -= 32;
						vdpchar(0x1800 + (row*32) + col + len, key);
						dst[len++] = key;
						StartSfx(levelfx, SOUND_BLIP, len);
					}
				}
			}
		}

		// Show cursor
		if (cursor)
			vdpchar(0x1800 + (row*32) + col + len, '@');
		else
			vdpchar(0x1800 + (row*32) + col + len, ' ');

		delay--;
		if (delay == 0)
		{
			cursor = 1 - cursor;
			delay = 30;
		}

		if (key == 13)
		{
			if (len)
				leave = 1;
		}

		// And avoid repeats
		prev_key = key;

		VSYNC_PLAY;
	}
	vdpchar(0x1800 + (11*32) + 6 + len, ' ');  // Delete cursor
	dst[len] = '\0';

	return dst;
}

unsigned char* get_intstring(unsigned char* buffer, int max_len, int row, int col)
{
	int len = 0;
	int key = 0;
	int prev_key = 0;
	int cursor = 1;
	int delay = 30;
	while (key != 13)
	{
		key = read_keyboard();
		if (key != prev_key)
		{
			// Check for ctrl-s ('backspace')
			if (key & 0x80)
			{
				// ctrl pressed, check underlying key
				if ((key & 0x7f) == 'S')
				{
					if (len > 0)
					{
						vdpchar(0x1800 + (row*32) + col + len, '_');
						buffer[len] = '\0';
						len--;
					}
				}
			}
			// Is it a digit?
			else if ((key >= '0') && (key <= '9'))
			{
				if (len < max_len)
				{
					vdpchar(0x1800 + (row*32) + col + len, key);
					buffer[len++] = key;
				}
			}
		}

		// Show cursor
		if (cursor)
			vdpchar(0x1800 + (row*32) + col + len, '@');
		else
			vdpchar(0x1800 + (row*32) + col + len, ' ');

		delay--;
		if (delay == 0)
		{
			cursor = 1 - cursor;
			delay = 30;
		}

		// And avoid repeats
		prev_key = key;

		VSYNC_PLAY;
	}
	vdpchar(0x1800 + (15*32) + 31, ' ');  // Delete cursor

	return buffer;
}

int name_checksum(unsigned char* name)
{
	unsigned char checksum = 0;

	int i = 0;
	while (i < strlength(name))
	{
		checksum = (checksum + name[i++]) & 0xff;
	}

	if (checksum == 0)
		checksum = 1;

	return checksum;
}

int do_checksum(int numloops, int input)
{
	int checksum;
	int addone = 0;

	for (int i = 0; i < numloops; i++) 
	{
		checksum = input;
		input = ((input * 2) & 255) ^ checksum;
		input = ((input * 2) & 255) ^ checksum;
		input = ((input * 4) & 255) ^ checksum;

		if ((input * 2)  > 255 ) 
			addone = 1;
		else 
			addone = 0;

		input = (input * 2) & 255;
		checksum = ((checksum * 2) + addone) & 255;
		input = checksum;
	}

	return input;
}

int decode_account(unsigned char* account_name, unsigned char* accountbuf)
{
	unsigned char reversed_account[9] = { 0 };

	// Reverse the account number in pairs (aabbccdd -> ddccbbaa)
	reversed_account[0] = accountbuf[6];
	reversed_account[1] = accountbuf[7];
	reversed_account[2] = accountbuf[4];
	reversed_account[3] = accountbuf[5];
	reversed_account[4] = accountbuf[2];
	reversed_account[5] = accountbuf[3];
	reversed_account[6] = accountbuf[0];
	reversed_account[7] = accountbuf[1];

	// Turn individual digits into byte values
	unsigned int accountvals[8];
	for (int i = 0; i < 8; i++)
	{
		accountvals[i] = reversed_account[i] - '0';
	}

	// aggregate 8 byte values (0-7) into 3 bytes
	unsigned int bytes[3] = { 0 };

	bytes[0]  =  (accountvals[0] 		 << 5) & 0xe0;
	bytes[0] |=  (accountvals[1] 		 << 2) & 0x1c;
	bytes[0] |=  (accountvals[2] 		 >> 1) & 0x03;

	bytes[1]  = ((accountvals[2] & 0x01) << 7) & 0x80;
	bytes[1] |=  (accountvals[3] 		 << 4) & 0x70;
	bytes[1] |=  (accountvals[4] 		 << 1) & 0x0e;
	bytes[1] |=  (accountvals[5]         >> 2) & 0x01;

	bytes[2]  = ((accountvals[5] & 0x03) << 6) & 0xc0;
	bytes[2] |=  (accountvals[6] 		 << 3) & 0x38;
	bytes[2] |=  (accountvals[7]             ) & 0x07;

	// Bytes 2 and 0 (in that order) contain our account balance in BCD format
	int account_balance = 0;
	account_balance += ((bytes[0] >>   4) * 1000);
	account_balance += ((bytes[0] & 0x0f) *  100);
	account_balance += ((bytes[2] >>   4) *   10);
	account_balance += ((bytes[2] & 0x0f) *    1);

	// Do checksum, to ensure account number and account name belong together
	unsigned char checkbyte = bytes[0] + bytes[2];
	if (!checkbyte)
		checkbyte = 1;

	int itterator = name_checksum(account_name);
 	checkbyte = do_checksum(itterator, checkbyte);

	if (checkbyte != bytes[1])
		return 0;

	return account_balance;
}

void wait_for_enter_fire()
{
	// Loop until enter or fire
	int key = 0;
	int joyst_result = read_joyst(JOYST_1);
	int delay = 30;
	int cursor = 1;
	while ((key != 13) && (!(JOYST_FIRE & joyst_result)))
	{
		key = read_keyboard();
		joyst_result = read_joyst(JOYST_1);

		// reset the screen timeout (it counts UP by 2, so set it low and odd)
		// This avoids running of the built-in screensaver
   		VDP_SCREEN_TIMEOUT = 1;	

		// Show cursor
		if (cursor)
			vdpchar(0x1800 + (19*32) + 30, '@');
		else
			vdpchar(0x1800 + (19*32) + 30, ' ');

		delay--;
		if (delay == 0)
		{
			cursor = 1 - cursor;
			delay = 30;
		}

		// wait for vsync
		VSYNC_PLAY;
	}
}

void load_account_screen()
{
	// Blank screen so we don't show garbage
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_INT);
	VDP_SET_REGISTER(VDP_REG_COL, (COLOR_BLACK << 4 | COLOR_BLACK) );

	// Prep nametable
	vdpwriteinc(0x1800, 0, 768);

	// Load color and pattern generator resources from ROM
	rom_to_vram(unwrap(account_patterns), 0x0000);
	rom_to_vram(unwrap(account_colors), 0x2000);

	// Wipe "screen"
	vdpmemset(0x1800, ' ', 704);

	// Add sprites to final image
	rom_to_vram(unwrap(account_sprite_patterns), 0x3800);
	rom_to_vram(unwrap(account_sprites), 0x1f80);

	// Position tables in VRAM
	VDP_SET_REGISTER(VDP_REG_PDT, 0x03);
	VDP_SET_REGISTER(VDP_REG_CT,  0xFF);
	VDP_SET_REGISTER(VDP_REG_NT,  0x06);
	VDP_SET_REGISTER(VDP_REG_SDT, 0x07);
	VDP_SET_REGISTER(VDP_REG_SAL, 0x3F);

	// Set bitmap mode
	VDP_SET_REGISTER(VDP_REG_MODE0, VDP_MODE0_BITMAP);
	VDP_SET_REGISTER(VDP_REG_MODE1, VDP_MODE1_16K | VDP_MODE1_UNBLANK | VDP_MODE1_INT | VDP_MODE1_SPRMODE16x16);
	VDP_SET_REGISTER(VDP_REG_COL, 0xEE);
}

// Display title screen and wait for user to press fire
void _do_account_screen()
{
	// Show title graphics
	load_account_screen();

	while (game.account_start == 0)
	{
		// Clear screen
		vdpmemset(0x1800, ' ', 704);

		// Display first screen section
		unsigned char buffer[33] = { 0 };
		for (int i = 0; i < 12; i++)
		{
			rom_to_ram(account_text_bank, account_text_offset + (i * 32), 32, buffer);
			type_string(buffer, 0x1800 + (i * 32), 0);
		}

		// Get name
		memset(game.account_name, 0, MAX_NAME_LEN);
		unsigned char* name_ptr = (unsigned char*)game.account_name;
		name_ptr = get_string(name_ptr, 17, 11, 6);

		// Ask if user has an account
		rom_to_ram(account_text_bank, account_text_offset + (12 * 32), 32, buffer);
		type_string(buffer, 0x1800 + (12 * 32), 0);

		int key = 0;
		int delay = 30;
		int cursor = 1;
		while ((key != 'Y') && (key !='N'))
		{
			key = read_keyboard();

			if (key > 96)
				key -= 32;

			// Show cursor
			if (cursor)
				vdpchar(0x1800 + (12*32) + 30, '@');
			else
				vdpchar(0x1800 + (12*32) + 30, ' ');

			delay--;
			if (delay == 0)
			{
				cursor = 1 - cursor;
				delay = 30;
			}

			VSYNC_PLAY;
		}
		vdpchar(0x1800 + (12*32) + 30, key);  // Delete cursor

		unsigned char accountbuf[9] = { 0 };
		unsigned char* accountbuf_ptr = (unsigned char*)accountbuf;

		if (key == 'N')
		{
			// Okay, new account, give out $10 000
			game.account_start = 100;

			// Type rest of screen
			for (int i = 14; i < 20; i++)
			{
				rom_to_ram(account_text_bank, account_text_offset + (i * 32), 32, buffer);
				type_string(buffer, 0x1800 + (i * 32), 0);
			}

			wait_for_enter_fire();
		}
		else
		{
			rom_to_ram(account_text_bank, account_text_offset + (20 * 32), 32, buffer);
			type_string(buffer, 0x1800 + (14 * 32), 0);
			rom_to_ram(account_text_bank, account_text_offset + (21 * 32), 32, buffer);
			type_string(buffer, 0x1800 + (15 * 32), 0);

			accountbuf_ptr = (unsigned char*)get_intstring(accountbuf_ptr, 8, 15, 23);
			// Did we get a valid account? If so, give player the correct amount of money
			if (accountbuf[0]) // <- if we received a number, the first digit has to exist
				game.account_start = decode_account(game.account_name, accountbuf);

			if (!game.account_start)
			{
				type_string((unsigned char*)invalid_account, 0x1800 + (17 * 32), 0);
				wait_for_enter_fire();
			}
		}
	}

	// Pressed fire, make noise!
	StopSfx();
	MUTE_SOUND();
}

unsigned char hexstr2uint8(char* str)
{
	return ((str[0] - '0') << 4) + (str[1] - '0');
}

void get_account_num(unsigned char* name, unsigned int balance, unsigned char* result)
{
	int itterator = name_checksum(name);
	char* hexpair = uint2str(balance);

	char pair1[3] = { 0 };
	char pair2[3] = { 0 };
	if (strlength((unsigned char*)hexpair) > 3)
	{
		pair1[0] = hexpair[0];
		pair1[1] = hexpair[1];
		pair2[0] = hexpair[2];
		pair2[1] = hexpair[3];
	}
	else
	{
		pair1[0] = '0';
		pair1[1] = hexpair[0];
		pair2[0] = hexpair[1];
		pair2[1] = hexpair[2];
	}

	pair1[2] = 0;
	pair2[2] = 0;

	unsigned char byte1 = hexstr2uint8(pair1);
	unsigned char byte2 = hexstr2uint8(pair2);

	unsigned char checkbyte = byte1 + byte2;
	checkbyte = do_checksum(itterator, checkbyte);

	unsigned char digits[9] = { 0 };
	digits[0] = '0' + ((byte1 & 0xe0) >> 5);
	digits[1] = '0' + ((byte1 & 0x1c) >> 2);
	digits[2] = '0' + (((byte1 & 0x03) << 1) + (checkbyte >> 7));
	digits[3] = '0' + ((checkbyte & 0x70) >> 4);
	digits[4] = '0' + ((checkbyte & 0x0e) >> 1);
	digits[5] = '0' + (((checkbyte & 0x01) << 2) + ((byte2 & 0xc0) >> 6));
	digits[6] = '0' + ((byte2 & 0x38) >> 3);
	digits[7] = '0' + ((byte2 & 0x07));

	result[0] = digits[6];
	result[1] = digits[7];
	result[2] = digits[4];
	result[3] = digits[5];
	result[4] = digits[2];
	result[5] = digits[3];
	result[6] = digits[0];
	result[7] = digits[1];
	result[8] = '\0';

	return;
}

void _do_gameover_screen()
{
	unsigned char buffer[33] = { '\0' };	// Needed for type_string_rom macro

	// Stop all sounds
	StopSfx();
	StopSong();
	VSYNC_PLAY;

	// Show title graphics
	load_account_screen();

	if (game.account >= game.account_start)
	{
		if (game.men > 1)
		{
			// Enough money and beat stay puft
			type_string_rom(wonwon, 0, 0);
			type_string_rom(wonwon, 1, 2);
			type_string(game.account_name, 0x1800 + (2 * 32) + 15, 0);
			type_string_rom(wonwon, 2, 3);
			type_string_rom(wonwon, 3, 4);
			type_string_rom(wonwon, 4, 5);
			type_string_rom(wonwon, 5, 7);
			type_string_rom(wonwon, 6, 8);
			type_string_rom(wonwon, 7, 9);

			// account balance
			game.account += 50;
			type_string((unsigned char*)"$", 0x1800 + (10 * 32), 0);
			type_string((const unsigned char*)(uint2str(game.account)),0x1800 + (10 * 32) + 1, 0);
			type_string((unsigned char*)"00", 0x1800 + (10 * 32) + strlength((const unsigned char*)strbuf) + 1, 0);

			type_string_rom(wonwon, 8, 12);

			// new account number
			unsigned char result[8];
			get_account_num(game.account_name, game.account, result);
			type_string(result, 0x1800 + (13 * 32), 0);

			type_string_rom(wonwon, 9, 15);
			type_string_rom(wonwon, 10, 16);
		}
		else
		{
			// Enough money, but lost from stay puft
			type_string_rom(won, 0, 0);
			type_string(game.account_name, 0x1800 + ( 1 * 32), 0);
			type_string_rom(won, 1, 3);
			type_string_rom(won, 2, 4);
			type_string_rom(won, 3, 5);

			type_string_rom(won, 4, 7);
			type_string_rom(won, 5, 8);
			type_string_rom(won, 6, 9);
			// account balance
			type_string((unsigned char*)"$", 0x1800 + (10 * 32), 0);
			type_string((const unsigned char*)(uint2str(game.account)),0x1800 + (10 * 32) + 1, 0);
			type_string((unsigned char*)"00", 0x1800 + (10 * 32) + strlength((const unsigned char*)strbuf) + 1, 0);

			type_string_rom(won, 7, 12);

			// new account number
			unsigned char result[12];
			get_account_num(game.account_name, game.account, result);
			type_string(result, 0x1800 + (13 * 32), 0);
		}
	}
	else
	{
		// Not enough money
		type_string_rom(lost, 0, 0);
		type_string_rom(lost, 1, 2);
		type_string_rom(lost, 2, 3);
		type_string_rom(lost, 3, 5);
		type_string_rom(lost, 4, 6);
		type_string_rom(lost, 5, 7);
	}

	type_string((unsigned char*)fire_to_countinue, 0x1800 + (19 * 32), 0);

	// Wait for keypress before restarting
	wait_for_enter_fire();
}
