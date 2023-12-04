#ifndef RESOURCE_COPY_H
#define RESOURCE_COPY_H

// Convenience macro function to increase brevity for plain copy actions
#define unwrap(NAME) NAME##_bank, NAME##_offset, NAME##_length

// Internal copy functions to be used to copy from ROM banks
// 		Use this to determine if the copy loop needs
// 		to play sound while copying
//
//		Defaults should are set to:
//			vdpmemcpy
//			memcpy
extern void (*vdpmemcpy_func)(int pAddr, const unsigned char *pSrc, int cnt);
extern void* (*memcpy_func)(void* dst, const void* src, unsigned int n);

#define DISABLE_MUSIC_ON_COPY { vdpmemcpy_func = &vdpmemcpy; memcpy_func = &memcpy; }
#define ENABLE_MUSIC_ON_COPY { vdpmemcpy_func = &vdpmemcpy_sn; memcpy_func = &memcpy; }

// Copy data from ROM to RAM or VRAM
void rom_to_ram(unsigned int srcbank, unsigned int srcoffset, unsigned int length, unsigned char *dst);
void rom_to_vram(unsigned int srcbank, unsigned int srcoffset, unsigned int length, unsigned int dst);

#endif
