#ifndef _LIBVVHIST_H
#define _LIBVVHIST_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>
#include <signal.h>

int vvhist_open(char *desc);
int vvhist_disable(int path);
int vvhist_enable(int path);
int vvhist_getStatus(int path);
int vvhist_getMemorySize(int path);
int vvhist_setResolution(int path, long r_x, long r_y, long r_m);
int vvhist_rdmem (int path, uint32_t *buff, uint32_t npts, uint32_t offset);
int vvhist_wrmem (int path, uint32_t *buff, uint32_t npts, uint32_t offset);
int vvhist_read3d(int path, uint32_t x, uint32_t y, uint32_t t, uint32_t dx, uint32_t dy, uint32_t dt,
		       uint32_t *buff, uint32_t nx,  uint32_t ny);

#endif // LIBVVHIST_H
