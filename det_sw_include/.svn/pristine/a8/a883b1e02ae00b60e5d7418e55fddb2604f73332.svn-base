
typedef struct 
{
	u_int32_t value;		/* For Register Write		*/
	u_int32_t start, num;	/* For Read/Write to Memory and TestPattern generator 	*/
	u_int32_t *data;		
} XHPb;


#define IOCTL
#define IOCTL_XH			0xE0
#define XH_SET_DIR	 	_IOW(IOCTL_XH, 0, XHPb)
#define XH_OUTPUT	 	_IOW(IOCTL_XH, 1, XHPb)
