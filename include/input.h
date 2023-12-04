#ifndef INPUT_H
#define INPUT_H

#define JOYST_1		0x0001
#define JOYST_2		0x0002
#define	JOYST_FIRE	0x0100
#define	JOYST_LEFT	0x0200
#define	JOYST_RIGHT	0x0400
#define	JOYST_DOWN	0x0800
#define	JOYST_UP	0x1000

unsigned int 	read_joyst(int joystick_id);
unsigned char 	read_spacebar();
unsigned char 	read_keyboard();

#endif