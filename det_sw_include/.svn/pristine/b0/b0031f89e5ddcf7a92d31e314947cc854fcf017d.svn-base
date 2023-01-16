
#ifndef ZYNQMP_DRIVER_H
#define ZYNQMP_DRIVER_H
typedef struct 
{
	int addr_space;		/* which memory space (reg, rmda, dma, DRAM etc		*/
	u_int32_t start, num;	/* For Read/Write to Memory and TestPattern generator 	*/
	u_int32_t row_len, row_stride;
	u_int32_t *ptr, *ptr2, *ptr3;
	u_int32_t and_mask, or_mask;	
	int t, dt;	/* for scope mode read and others */
} ZynqMPPb;

#define ZYNQMP_NUM_ADDR_SPACE	    8	// Extra at end will have no used_on bits set so ignored
#define ZYNQMP_ADDR_SPACE_LOCAL		0
#define ZYNQMP_ADDR_SPACE_REGS		1
#define ZYNQMP_ADDR_SPACE_JESD		2
#define ZYNQMP_ADDR_SPACE_DMA		3
#define ZYNQMP_ADDR_SPACE_PS_LOW	4
#define ZYNQMP_ADDR_SPACE_PS_HIGH	5
#define ZYNQMP_ADDR_SPACE_PL_RAM	6
#define ZYNQMP_ADDR_SPACE_HIST		7

#define ZYNQMP_ADDR_SPACE_DMA_BUFF 100

#define ZYNQMP_PL_DRAM_PHYS_BASE 0x400000000L
#define ZYNQMP_PL_DRAM_PHYS_SIZE 0x080000000L

/* There is 4 Gbytes of DRAM< 2 GByts at 0, 2 GBytes at 0x8_0000_0000
   Although there is 4 GByte SDRAM. The bottom 512 M bytes is controlled by Linux . the rest is at 0x8_0000_0000 and visible from 64 bit addresses */
/* From Vitis platform it appears the top 64 k of the low 2 GBytes is not available */
#define ZYNQMP_PS_DRAM_PHYS_BASE_LOW 0x20000000L
#define ZYNQMP_PS_DRAM_PHYS_SIZE_LOW 0x5FFF0000

#define ZYNQMP_PS_DRAM_PHYS_BASE_HIGH 0x800000000L
#define ZYNQMP_PS_DRAM_PHYS_SIZE_HIGH 0x80000000

#define IOCTL
#define IOCTL_ZYNQMP				0xE1

#define ZYNQMP_WRITE					_IOW(IOCTL_ZYNQMP, 0, ZynqMPPb)
#define ZYNQMP_READ						_IOW(IOCTL_ZYNQMP, 1, ZynqMPPb)
#define ZYNQMP_RMW						_IOW(IOCTL_ZYNQMP, 2, ZynqMPPb)
#define ZYNQMP_DMA_V7					_IOW(IOCTL_ZYNQMP, 3, ZynqMPPb)
#define ZYNQMP_READ_DMA_STATUS_BLOCK	_IOW(IOCTL_ZYNQMP, 4, ZynqMPPb)
#define ZYNQMP_MEMZERO					_IOW(IOCTL_ZYNQMP, 5, ZynqMPPb)

#define ZYNQMP_SET_READOUT_MODE			_IO(IOCTL_ZYNQMP, 6)
#define ZYNQMP_GET_READOUT_MODE			_IOR(IOCTL_ZYNQMP, 7, u_int32_t)

#define NGZMP_LOAD_FILTER				_IOW(IOCTL_ZYNQMP, 8, ZynqMPPb)
#define ZYNQMP_READ_ROW					_IOW(IOCTL_ZYNQMP, 9, ZynqMPPb)

#define ZYNQMP_USE_SOCKETS		0
/*
#define XH_WR_DMA_CONTROL		_IO(IOCTL_XSP4, 0x10)

#define XH_RD_DMA_STATUS 		_IOR(IOCTL_XSP4, 0x40, u_int32_t)
*/

#endif

