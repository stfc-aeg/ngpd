typedef struct
{
   unsigned short arm;
   unsigned short testcount;
   unsigned long scaler[4];
   unsigned short nframes;
   unsigned long start;
   unsigned long num;
   unsigned long *data;
} IOBlock;

/*
 * ioctl() cmd constants
 */
#define IOCTL_MAGIC 0x25
#define IOCTL_RESET             _IO(IOCTL_MAGIC, 0x1)
#define IOCTL_ARM_REGISTERS     _IOW(IOCTL_MAGIC, 0x2, IOBlock)
#define IOCTL_READ_FROM_SCALERS _IOR(IOCTL_MAGIC, 0x3, IOBlock)
#define IOCTL_PRESET_SCALERS    _IOW(IOCTL_MAGIC, 0x4, IOBlock)
#define IOCTL_TESTCOUNT         _IOW(IOCTL_MAGIC, 0x5, IOBlock)
#define IOCTL_ARM_AND_ALLOC     _IOW(IOCTL_MAGIC, 0x6, IOBlock)
#define IOCTL_READ_FROM_BUFFERS _IOW(IOCTL_MAGIC, 0x7, IOBlock)
#define IOCTL_FREE              _IO(IOCTL_MAGIC, 0x8)
