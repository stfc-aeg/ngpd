
#ifndef XH_DRIEVR_H
#define XH_DRIVER_H
typedef struct 
{
	u_int32_t value;		/* For Register Write		*/
	u_int32_t start, num;	/* For Read/Write to Memory and TestPattern generator 	*/
	u_int32_t *ptr;		
	int t, dt;	/* for scope mode read and others */
} XHPb;





#define IOCTL
#define IOCTL_XH			0xE0
#define XH_GET_MEM_SIZE			_IOR(IOCTL_XH, 0, u_int32_t)
#define XH_OUTPUT	 			_IOW(IOCTL_XH, 1, XHPb)
#define XH_GENERATE_MEM			_IOW(IOCTL_XH, 2, XHPb)
#define XH_READ_MEM				_IOW(IOCTL_XH, 3, XHPb)
#define XH_READ_SCOPE			_IOW(IOCTL_XH, 4, XHPb)

#define XH_RD_REVISION			_IOR(IOCTL_XH, 0x0F, u_int32_t)

#define XH_WR_DMA_CONTROL		_IO(IOCTL_XH, 0x10)
#define XH_WR_DMA_FRAME_NUM 	_IO(IOCTL_XH, 0x11)
#define XH_WR_DMA_BASE_ADDR		_IO(IOCTL_XH, 0x12)
#define XH_WR_DMA_BURSTNO_MAX 	_IO(IOCTL_XH, 0x13)
#define XH_WR_DMA_ADC_CHIP_SEL 	_IO(IOCTL_XH, 0x14)

#define XH_DMA_SCOPE_START		_IOW(IOCTL_XH, 0x20, XHPb)
#define XH_DMA_RUN_START		_IOW(IOCTL_XH, 0x21, XHPb)
#define XH_TRIG_START	 		_IOR(IOCTL_XH, 0x22, u_int32_t)
#define XH_TRIG_STOP	 		_IOR(IOCTL_XH, 0x23, u_int32_t)
#define XH_TRIG_CONTINUE 		_IOR(IOCTL_XH, 0x24, u_int32_t)

#define XH_RD_DMA_STATUS 		_IOR(IOCTL_XH, 0x40, u_int32_t)
#define XH_RD_DMA_FRAME_NUM 	_IOR(IOCTL_XH, 0x41, u_int32_t)
#define XH_RD_DMA_CONTROL		_IOR(IOCTL_XH, 0x42, u_int32_t)
#define XH_RD_DMA_BASE_ADDR		_IOR(IOCTL_XH, 0x43, u_int32_t)
#define XH_RD_DMA_BURSTNO_MAX 	_IOR(IOCTL_XH, 0x44, u_int32_t)
#define XH_RD_DMA_BURSTNO_CNT 	_IOR(IOCTL_XH, 0x45, u_int32_t)
#define XH_RD_DMA_ADC_CHIP_SEL	_IOR(IOCTL_XH, 0x46, u_int32_t)

#define XH_WR_TRIG_CONT			_IO(IOCTL_XH, 0x47)
#define XH_WR_TRIG_ORBIT_DELAY	_IO(IOCTL_XH, 0x48)
#define XH_WR_TRIG_NUM_GROUPS	_IO(IOCTL_XH, 0x49)
#define XH_RD_TRIG_ORBIT_DELAY	_IOR(IOCTL_XH, 0x4A, u_int32_t)
#define XH_RD_TRIG_NUM_GROUPS	_IOR(IOCTL_XH, 0x4B, u_int32_t)
#define XH_WR_XDELAY		 	_IO(IOCTL_XH,  0x4C)
#define XH_RD_XDELAY			_IOR(IOCTL_XH, 0x4D, u_int32_t)
#define XH_RD_DEBUG_STATUS 		_IOR(IOCTL_XH, 0x4E, u_int32_t)
#define XH_RD_DEBUG_BURSTNO_CNT _IOR(IOCTL_XH, 0x4F, u_int32_t)

#define XH_RD_DMA_TP_START 		_IOR(IOCTL_XH, 0x50, u_int32_t)
#define XH_RD_DMA_TP_INC 		_IOR(IOCTL_XH, 0x51, u_int32_t)
#define XH_WR_HV_SUPPLY		 	_IO(IOCTL_XH,  0x52)
#define XH_RD_HV_SUPPLY			_IOR(IOCTL_XH, 0x53, u_int32_t)
/*#define XH_WR_MEMORY_FORMAT	 	_IO(IOCTL_XH,  0x54)
#define XH_RD_MEMORY_FORMAT		_IOR(IOCTL_XH, 0x55, u_int32_t)
*/
#define XH_WR_ADC_CONTROL	_IO(IOCTL_XH,  0x60)
#define XH_RD_ADC_CONTROL	_IOR(IOCTL_XH, 0x61, u_int32_t)
#define XH_RD_ADC_STATUS 	_IOR(IOCTL_XH, 0x64, u_int32_t)
#define XH_RD_ADC_STAGES	_IO(IOCTL_XH,  0x65)
#define XH_RD_ADC_TAP_DEL	_IOW(IOCTL_XH, 0x66, XHPb)
#define XH_RD_ADC_EYE_WID	_IOW(IOCTL_XH, 0x67, XHPb)
#define XH_WR_ADC_TIMERS	_IO(IOCTL_XH,  0x68)
#define XH_RD_ADC_TIMERS	_IOR(IOCTL_XH, 0x69, u_int32_t)
#define XH_RD_ADC_BIT_SLIP	_IOW(IOCTL_XH, 0x6A, XHPb)
#define XH_WR_ADC_PWRDWN 	_IO(IOCTL_XH,  0x6B)
#define XH_RD_ADC_PWRDWN 	_IOR(IOCTL_XH, 0x6C, u_int32_t)

#define XH_RD_TIMING_BRAM 	_IOW(IOCTL_XH, 0x70, XHPb)
#define XH_WR_TIMING_BRAM 	_IOW(IOCTL_XH, 0x71, XHPb)
#define XH_RD_TRIG_STATUS   _IO(IOCTL_XH, 0x72)
#define XH_RD_TRIG_OUT_CONT _IOW(IOCTL_XH, 0x73, XHPb)
#define XH_WR_TRIG_OUT_CONT _IOW(IOCTL_XH, 0x74, XHPb)
#define XH_WR_TRIG_LED_CONT _IO(IOCTL_XH, 0x75)
#define XH_RD_TRIG_LED_CONT _IOR(IOCTL_XH, 0x76, u_int32_t)
#define XH_WR_TIMING_FIXED	_IO(IOCTL_XH, 0x77)
#define XH_RD_TIMING_FIXED	_IOR(IOCTL_XH, 0x78, u_int32_t)
#define XH_RD_ORBIT_STATUS	_IOR(IOCTL_XH, 0x79, u_int32_t)
#define XH_RD_INL_BRAM 		_IOW(IOCTL_XH, 0x7A, XHPb)
#define XH_WR_INL_BRAM 		_IOW(IOCTL_XH, 0x7B, XHPb)

#define XH_WR_INVERT_DATA 	_IO(IOCTL_XH,  0x7C)
#define XH_RD_INVERT_DATA	_IOR(IOCTL_XH, 0x7D, u_int32_t)
#define XH_RD_ADC_FIFO_RD_COUNT _IOW(IOCTL_XH, 0x7E, XHPb)
#endif

void xh_flush_dcache_range(unsigned long start, unsigned long stop);
void xh_invalidate_dcache_range(unsigned long start, unsigned long stop);
