#ifndef _LIBVTDC4_H
#define _LIBVTDC4_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>
#include <signal.h>

typedef struct
{
   uint8_t  offsetreg;		/* 01: offset register		       	*/	
   uint8_t  timeoutreg;         /* 03: timeout register		       	*/
   uint8_t  monitorreg;	        /* 07: monitor register		      	*/	
   uint8_t  csrreg;	        /* 13: control and status register	*/
} IORegs;

int vtdc4_open(char *desc);
uint32_t vtdc4_disableSumRejection(int path);
uint32_t vtdc4_displayRegisters(int path);
uint32_t vtdc4_enableSumRejection(int path);
uint32_t vtdc4_getCsr(int path);
uint32_t vtdc4_getOffsetRegister(int path);
uint32_t vtdc4_getOffset(int path);
uint32_t vtdc4_getResolution(int path);
uint32_t vtdc4_getSumRejection(int path);
uint32_t vtdc4_getTimeoutRegister(int path);
uint32_t vtdc4_getMonitorRegister(int path);
uint32_t vtdc4_setCsr(int path, int multiHitOr2D, int enbDistimeStamp, int enbDisVME);
uint32_t vtdc4_setOffsetRegister(int path, uint32_t offset);
uint32_t vtdc4_setOffset(int path, uint32_t ToffsetNS);
uint32_t vtdc4_setLemoBits(int path, uint32_t bits);
uint32_t vtdc4_setTimeoutRegister(int path, uint32_t TimeoutNS);
uint32_t vtdc4_stop(int path);
uint32_t vtdc4_start(int path);
uint32_t vtdc4_getPileupPrecision(int path);
uint32_t vtdc4_setPileupPrecision(int path, uint32_t precision);
uint32_t vtdc4_getHwConfig(int path);
uint32_t vtdc4_getRegisters(int path, IORegs *ioregs);

#endif // LIBVTDC4_H
