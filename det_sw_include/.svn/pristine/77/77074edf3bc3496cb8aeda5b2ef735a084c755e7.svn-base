#ifndef _VME_IO_H
#define _VME_IO_H

/*
 * Enforce In-order Execution of I/O:
 * Acts as a barrier to ensure all previous I/O accesses have
 * completed before any further ones are issued.
 */
#ifndef iobarrier_rw
extern inline void eieio(void)
{
	__asm__ __volatile__ ("eieio" : : : "memory");
}
#endif
/* Normal VME read/write 8, 16, and 32 bit, with Motorl Byte order and eieio embeded */
extern inline int vme_read8(volatile unsigned char *addr)
{
	int ret;

	__asm__ __volatile__(
		"lbz%U1%X1 %0,%1; eieio" : "=r" (ret) : "m" (*addr));
	return ret;
}
extern inline int vme_read16(volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__(
		"lhz%U1%X1 %0,%1; eieio" : "=r" (ret) : "m" (*addr));
	return ret;
}
extern inline int vme_read32(volatile unsigned int *addr)
{
	int ret;

	__asm__ __volatile__(
		"lwz%U1%X1 %0,%1; eieio" : "=r" (ret) : "m" (*addr));
	return ret;
}
extern inline void vme_write8(int val, volatile unsigned char *addr)
{
	__asm__ __volatile__("stb%U0%X0 %1,%0; eieio" : "=m" (*addr) : "r" (val));
}
extern inline void vme_write16(int val, volatile unsigned short *addr)
{
	__asm__ __volatile__("sth%U0%X0 %1,%0; eieio" : "=m" (*addr) : "r" (val));
}
extern inline void vme_write32(int val, volatile unsigned *addr)
{
	__asm__ __volatile__("stw%U0%X0 %1,%0; eieio" : "=m" (*addr) : "r" (val));
}

/*
 * 8, 16 and 32 bit, big and little endian I/O operations, with barrier.
 *
 * vme_readXXs operations have additional twi & isync to make sure the read
 * is actually performed (i.e. the data has come back) before we start
 * executing any following instructions.
 */

extern inline int vme_read8s(volatile unsigned char *addr)
{
	int ret;

	__asm__ __volatile__(
		"lbz%U1%X1 %0,%1;\n"
		"twi 0,%0,0;\n"
		"isync" : "=r" (ret) : "m" (*addr));
	return ret;
}

extern inline int vme_read16s(volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__("lhz%U1%X1 %0,%1;\n"
			     "twi 0,%0,0;\n"
			     "isync" : "=r" (ret) : "m" (*addr));
	return ret;
}

extern inline unsigned vme_read32s(volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwz%U1%X1 %0,%1;\n"
			     "twi 0,%0,0;\n"
			     "isync" : "=r" (ret) : "m" (*addr));
	return ret;
}

/* Byte reversed version for special case of VME PMC carrier */

extern inline int vme_read16les(volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__("lhbrx %0,0,%1;\n"
			     "twi 0,%0,0;\n"
			     "isync" : "=r" (ret) :
			      "r" (addr), "m" (*addr));
	return ret;
}
extern inline unsigned vme_read32les(volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwbrx %0,0,%1;\n"
			     "twi 0,%0,0;\n"
			     "isync" : "=r" (ret) :
			     "r" (addr), "m" (*addr));
	return ret;
}

extern inline void vme_write16le(int val, volatile unsigned short *addr)
{
	__asm__ __volatile__("sthbrx %1,0,%2; eieio" : "=m" (*addr) :
			      "r" (val), "r" (addr));
}



extern inline void vme_write32le(int val, volatile unsigned *addr)
{
	__asm__ __volatile__("stwbrx %1,0,%2; eieio" : "=m" (*addr) :
			     "r" (val), "r" (addr));
}

#endif /* _VME_IO_H */
