#ifndef UTILS_H
#define UTILS_H

void* memcpy(void* dst, const void* src, unsigned int n);
void* memcpy_sn(void* dst, const void* src, unsigned int n);
void* memset(void* dst, int v, unsigned int n);
int abs(int v);
void check_quit();

#endif
