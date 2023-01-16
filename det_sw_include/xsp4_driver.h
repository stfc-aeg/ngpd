
#ifndef XSP4_DRIVER_H
#define XSP4_DRIVER_H
typedef struct 
{
	int addr_space;		/* which memory space (reg, rmda, dma, darm etc		*/
	u_int32_t start, num;	/* For Read/Write to Memory and TestPattern generator 	*/
	u_int32_t *ptr, *ptr2, *ptr3;
	u_int32_t and_mask, or_mask;	
	int t, dt;	/* for scope mode read and others */
} Xsp4Pb;

#define XSP4_NUM_ADDR_SPACE	    	8
#define XSP4_ADDR_SPACE_LOCAL		0
#define XSP4_ADDR_SPACE_REGS		1
#define XSP4_ADDR_SPACE_RDMA		2
#define XSP4_ADDR_SPACE_DMA			3
#define XSP4_ADDR_SPACE_RDRAM		4
#define XSP3M_ADDR_SPACE_PB_SCOPE 	5
#define XSP3M_ADDR_SPACE_HIST     	6
#define XSP3M_ADDR_SPACE_MGT_DRP    7

#define XSP4_ADDR_SPACE_DMA_BUFF 100

#define XSP4_RDRAM_PHYS_BASE 0x90000000
#define XSP4_RDRAM_PHYS_SIZE 0x10000000

#define IOCTL
#define IOCTL_XSP4				0xE1

#define XSP4_WRITE					_IOW(IOCTL_XSP4, 0, Xsp4Pb)
#define XSP4_READ					_IOW(IOCTL_XSP4, 1, Xsp4Pb)
#define XSP4_RMW					_IOW(IOCTL_XSP4, 2, Xsp4Pb)
#define XSP4_DMA_V7					_IOW(IOCTL_XSP4, 3, Xsp4Pb)
#define XSP4_READ_DMA_STATUS_BLOCK	_IOW(IOCTL_XSP4, 4, Xsp4Pb)
#define XSP4_MEMZERO				_IOW(IOCTL_XSP4, 5, Xsp4Pb)

#define XSP4_SET_READOUT_MODE		_IO(IOCTL_XSP4, 6)
#define XSP4_GET_READOUT_MODE		_IOR(IOCTL_XSP4, 7, u_int32_t)

/*
#define XH_WR_DMA_CONTROL		_IO(IOCTL_XSP4, 0x10)

#define XH_RD_DMA_STATUS 		_IOR(IOCTL_XSP4, 0x40, u_int32_t)
*/

#endif

