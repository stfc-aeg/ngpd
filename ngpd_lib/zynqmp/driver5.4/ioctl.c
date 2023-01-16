/*
	Generic Driver for use on ZynqMP.
	10/02/2021	Derived from XSPRESS4 Zynq driver

*/
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/types.h>   /* ulong and friends */ 
#include <linux/sched.h>   /* current, task_struct, other goodies */ 
#include <linux/wait.h>   /* current, task_struct, other goodies */ 
#include <linux/fcntl.h>   /* O_NONBLOCK etc. */ 
#include <linux/errno.h>   /* return values */ 
#include <linux/ioport.h>  /* request_region() */ 
/* #include <linux/malloc.h>  kmalloc, kfree */ 
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>    /* char dev */ 
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h> 
#include <asm/io.h>        /* inb() inw() outb() ... */ 
#include <asm/irq.h>       /* unreadable, but useful */ 
#include <linux/string.h>      
#include <asm/string.h>      
#include <net/sock.h>
#include <linux/rwsem.h>

#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"


int copy32_io_to_user(u32 __user *to, const volatile u32 __iomem *from, size_t count);
int copy32_user_to_io(volatile u32 __iomem *to, const u32 __user *from, size_t count);
int read_by_row_len1(DevStatics *dev_stat, ZynqMPPb *param);
int read_by_row_len_n(DevStatics *dev_stat, ZynqMPPb *param);

#define USE_COPY_TO_USER 1
#define USE_COPY_FROM_USER 0
long zynqmp_ioctl ( struct file *filp,
                unsigned int cmd,
                unsigned long arg) 
{
  	DevStatics *dev_stat = filp->private_data;
	ZynqMPPb param;
//	volatile u_int32_t * io_base;
//	int i;
	u_int32_t __iomem *ptr, *base;
	u_int32_t d=0;
	int rc=0;
	u_int32_t __user *uptr;
	u_int32_t num;
	u_int64_t remaining, chunk_bytes, byte_offset, num_bytes;
	int addr_space;

	if (debug >= MSG_NORMAL)
		printk("%s: Starting iotcl 0x%08X\n", driver_name, cmd); 

	if (down_interruptible(&dev_stat->sem))
		return -ERESTARTSYS;

	if (_IOC_TYPE(cmd) != IOCTL_ZYNQMP)
	{
		printk(KERN_ALERT "[%d] %s: Inappropriate IOCTL 0x%08X for device, type=0x%X\n, num=%x",
			current->pid, driver_name, cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
		rc = -ENOTTY;
	}
	else
	{
		// If the ioctl is defined an _IOW then copy parameters from user memory space
		// to kernel memory space.
		if ((_IOC_DIR(cmd) & _IOC_WRITE) && (copy_from_user(&param, (ZynqMPPb*)arg, sizeof(ZynqMPPb))))
		{
			rc = -EFAULT;
		}
		else
		{
		    switch (cmd) 
		    {
		    case ZYNQMP_READ:
				if (debug >= MSG_VERBOSE)
					printk("%s: ZYNQMP_READ address_space=%d, offset=0x%08X, num=0x%08X\n", driver_name, param.addr_space, param.start, param.num); 
				if (param.addr_space >  ZYNQMP_ADDR_SPACE_DMA_BUFF && param.addr_space < ZYNQMP_ADDR_SPACE_DMA_BUFF + ZYNQMP_DMA_STREAM_NUM)
				{
					int stream = param.addr_space - ZYNQMP_ADDR_SPACE_DMA_BUFF;
					DMAStream *dma = dev_stat->status_block.stream_def;
					u_int32_t data_size = dma[stream].data_size/4;
					if (debug >= MSG_VERBOSE)
						printk("%s: .... From DMA buffer %d, First segment Phys start 0x%010llx, Segment Size 0x%010llX, virt base %px\n", driver_name, stream, 
							dma[stream].buff_seg[0].cpu_base, dma[stream].buff_seg[0].size, dev_stat->dma_priv[stream].buff_virt_base[0]); 
					if (param.num==0 || param.start >= data_size || param.start+param.num>data_size )
					{
						if (debug < MSG_VERBOSE)
						{
							printk("%s: ZYNQMP_READ from DMA buffer=%d, Lword offset=0x%08X, num=0x%08X is outside buffer size 0x%08X\n", driver_name, param.addr_space, param.start, param.num, data_size); 
							printk("%s: .... Note First segment Phys start 0x%010llx, Segment Size 0x%010llX, virt base %px\n", driver_name, 
								dma[stream].buff_seg[0].cpu_base, dma[stream].buff_seg[0].size, dev_stat->dma_priv[stream].buff_virt_base[0]); 
						}
						rc = -EINVAL;
						break;
					}
					byte_offset = param.start*sizeof(u_int32_t);
					remaining = sizeof(u_int32_t)*param.num;
					uptr = (u_int32_t __user *)param.ptr;
					while (remaining > 0)
					{
						ptr = (u_int32_t __iomem *) zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_bytes, &addr_space, NULL, NULL);
						if (ptr == NULL)
						{
							rc = -EINVAL;
							break;
						}
						if (dev_stat->access_flags[addr_space] & ACCESS_CACHEFLUSH)
							__inval_dcache_area(ptr, chunk_bytes);
#if USE_COPY_TO_USER
						num_bytes= chunk_bytes;
						if (byte_offset & 4)
						{
							/* Not 64 bit aligned in device memory, so do 1 words first */
							d = ZYNQMP_IORead32(ptr);
							put_user(d, uptr);
							ptr++;
							uptr++;
							num_bytes-=sizeof(u_int32_t);
						}
						if (num_bytes > 0 && copy_to_user((void __user *)uptr, (void *) ptr, num_bytes))
						{
							rc = -EFAULT;
							break;
						}
						uptr += num_bytes/sizeof(u_int32_t);
#else
						if (copy32_io_to_user((u_int32_t __user *)uptr,  ptr, chunk_bytes))
						{
							rc = -EFAULT;
							break;
						}
						uptr += chunk_bytes/sizeof(u_int32_t);
#endif
						byte_offset+=chunk_bytes;
					}
					dma_rmb();
				}
				else if (param.addr_space < 0 || param.addr_space >= ZYNQMP_NUM_ADDR_SPACE)
				{
					rc = -EINVAL;
				}
				else
				{
					if (param.num==0 || param.start >= dev_stat->size_bytes[param.addr_space]/4 || param.start+param.num>dev_stat->size_bytes[param.addr_space]/4)
					{
						rc = -EINVAL;
						break;
					}
					ptr = dev_stat->virt_base[param.addr_space]+param.start;
#if USE_COPY_TO_USER
					if (dev_stat->access_flags[param.addr_space] & ACCESS_FORCE32BIT)
					{
						int i;
						uptr = (u_int32_t __user *)param.ptr;
						for (i=0; i<param.num; i++)
						{
							d = ZYNQMP_IORead32(ptr);
							put_user(d, uptr);
							ptr++;
							uptr++;
						}
					}
					else
					{
						if (dev_stat->access_flags[param.addr_space] & ACCESS_CACHEFLUSH)
							__inval_dcache_area(ptr, sizeof(u_int32_t)*param.num);
						if (param.start & 1)
						{
							/* Not 64 bit aligned in device memory, so do 1 words first */
							uptr = (u_int32_t __user *)param.ptr;
							d = ZYNQMP_IORead32(ptr);
							put_user(d, uptr);
							ptr++;
							param.ptr++;
							param.num--;
						}
						if (param.num > 0 && copy_to_user((void __user *)param.ptr, (void *) ptr, sizeof(u_int32_t)*param.num))
						{
							rc = -EFAULT;
						}
						dma_rmb();
					}
#else
	//							xsp4_invalidate_dcache_range((unsigned long)ptr, (unsigned long)(ptr+param.num));
					if (dev_stat->access_flags[param.addr_space] & ACCESS_CACHEFLUSH)
						__inval_dcache_area(ptr, sizeof(u_int32_t)*param.num);
					if (copy32_io_to_user((u_int32_t __user *)param.ptr, ptr, sizeof(u_int32_t)*param.num))
					{
						rc = -EFAULT;
					}
					dma_rmb();
#endif
				}
				break;

		    case ZYNQMP_WRITE:
				if (debug >= MSG_VERBOSE)
					printk("%s: ZYNQMP_WRITE address_space=%d, offset=0x%08X, num=0x%08X\n", driver_name, param.addr_space, param.start, param.num); 
				if (param.addr_space >  ZYNQMP_ADDR_SPACE_DMA_BUFF && param.addr_space < ZYNQMP_ADDR_SPACE_DMA_BUFF + ZYNQMP_DMA_STREAM_NUM)
				{
					int stream = param.addr_space - ZYNQMP_ADDR_SPACE_DMA_BUFF;
					DMAStream *dma = dev_stat->status_block.stream_def;
					if (debug >= MSG_VERBOSE)
						printk("%s: .... To DMA buffer %d, First segment Phys start 0x%010llx, Segment Size 0x%010llX, virt base %px\n", driver_name, stream, 
							dma[stream].buff_seg[0].cpu_base, dma[stream].buff_seg[0].size, dev_stat->dma_priv[stream].buff_virt_base[0]); 
					if (param.num==0 || param.start >= dma[stream].data_size/4 || param.start+param.num>dma[stream].data_size/4)
					{
						rc = -EINVAL;
						break;
					}
					byte_offset = param.start*sizeof(u_int32_t);
					remaining = param.num*sizeof(u_int32_t);
					dma_wmb();	// From io.h note that barriers are used Before WRITE and After read.
					uptr = (u_int32_t __user *)param.ptr;
					while (remaining > 0)
					{
						ptr = (u_int32_t __iomem *) zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_bytes, &addr_space, NULL, NULL);
						if (ptr == NULL)
						{
							rc = -EINVAL;
							break;
						}
						base = ptr;
#if USE_COPY_FROM_USER
						num_bytes = chunk_bytes;
						if (byte_offset & 4)
						{
							/*  For  < 16 byte blocks the copy_template.S will do 64, then 32 bit. If device memory is not aligned, do 1 word first to make next 64 bit words aligned.
								For >= 16 bytes, copy_template.S will do waht is necessary to make it SRC aligned. This cannot be made work with Device memory
								*/
							get_user(d, uptr);
							ZYNQMP_IOWrite32(ptr, d);
							ptr++;
							uptr++;
							num_bytes-=sizeof(u_int32_t);
						}
						if (num_bytes>0 && copy_from_user((void *) ptr, (void __user *)uptr, num_bytes))
						{
							rc = -EFAULT;
							break;
						}
						uptr += num_bytes/sizeof(u_int32_t); 
#else
						if (copy32_user_to_io(ptr, (u_int32_t __user *)uptr, chunk_bytes))
						{
							rc = -EFAULT;
							break;
						}
						uptr += chunk_bytes/sizeof(u_int32_t);
#endif
						if (dev_stat->access_flags[addr_space] & ACCESS_CACHEFLUSH)
							__flush_dcache_area(base, chunk_bytes);
						byte_offset += chunk_bytes;
					}
				}
				else if (param.addr_space < 0 || param.addr_space >= ZYNQMP_NUM_ADDR_SPACE)
				{
					rc = -EINVAL;
				}
				else
				{
					if (param.num==0 || param.start >= dev_stat->size_bytes[param.addr_space]/4 || param.start+param.num>dev_stat->size_bytes[param.addr_space]/4)
					{
						rc = -EINVAL;
						break;
					}
					if (debug >= MSG_VERBOSE)
						printk(KERN_WARNING "%s: Writing to Phys addr %010llX for %X 32 bit words\n", driver_name, dev_stat->phys_base[param.addr_space]+param.start*sizeof(u_int32_t), param.num);
					ptr = dev_stat->virt_base[param.addr_space]+param.start;
					base = ptr;
#if USE_COPY_FROM_USER
					if (dev_stat->access_flags[param.addr_space] & ACCESS_FORCE32BIT)
					{
						int i;
						uptr = (u_int32_t __user *)param.ptr;
						for (i=0; i<param.num; i++)
						{
							get_user(d, uptr);
							ZYNQMP_IOWrite32(ptr, d);
							ptr++;
							uptr++;
						}
					}
					else
					{
						dma_wmb();	// From io.h note that barriers are used Before WRITE and After read.
						num = param.num;
						if (((param.start & 1) && param.num<4) || (param.num>=4 && (src_align ^(param.start&1))) )
						{
							/* Not 64 bit aligned in device memory, so do 1 words first */
							uptr = (u_int32_t __user *)param.ptr;
							get_user(d, uptr);
							ZYNQMP_IOWrite32(ptr, d);
							ptr++;
							param.ptr++;
							num--;
						}
						if (num>0 && copy_from_user((void *) ptr, (void __user *)param.ptr, sizeof(u_int32_t)*num))
						{
							rc = -EFAULT;
						}
						if (dev_stat->access_flags[param.addr_space] & ACCESS_CACHEFLUSH)
							__flush_dcache_area(base, sizeof(u_int32_t)*param.num);
					}
#else

					dma_wmb();	// From io.h note that barriers are used Before WRITE and After read.
					if (copy32_user_to_io( ptr, (u_int32_t __user *)param.ptr, sizeof(u_int32_t)*param.num))
					{
						rc = -EFAULT;
					}
					if (dev_stat->access_flags[param.addr_space] & ACCESS_CACHEFLUSH)
						__flush_dcache_area(ptr, sizeof(u_int32_t)*param.num);
#endif

				}
				break;

		    case ZYNQMP_RMW:
				if (debug >= MSG_VERBOSE)
					printk("%s: ZYNQMP_RMW address_space=%d, offset=0x%08X, and=0x%08X, or=0x%08X\n", driver_name, param.addr_space, param.start, param.and_mask, param.or_mask); 
				if (param.addr_space < 0 || param.addr_space >= ZYNQMP_NUM_ADDR_SPACE)
				{
					rc = -EINVAL;
					break;
				}
				if (param.start >= dev_stat->size_bytes[param.addr_space]/4 || param.start+1>dev_stat->size_bytes[param.addr_space]/4)
				{
					rc = -EINVAL;
					break;
				}
				ptr = dev_stat->virt_base[param.addr_space]+param.start;
//							xsp4_invalidate_dcache_range((unsigned long)ptr, (unsigned long)(ptr+param.num));

				d = readl(ptr);
				d = (d & param.and_mask) | param.or_mask;
				writel(d, ptr);
				if (param.ptr != NULL)
				{
					if (put_user(d, (u_int32_t*)param.ptr))
					{
						printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (u_int32_t *) param.ptr);
						rc = -EFAULT;
					}
				}
				break;

		    case ZYNQMP_READ_ROW:
				if (debug >= MSG_VERBOSE)
					printk("%s: ZYNQMP_READ_ROW address_space=%d, offset=0x%08X, num=0x%08X, row_len=0x%02X, row_stride=0x%02X, to=%px\n", driver_name, param.addr_space, param.start, param.num, param.row_len, param.row_stride, param.ptr); 
				if (param.addr_space < 0 || param.addr_space >= ZYNQMP_NUM_ADDR_SPACE)
				{
					rc = -EINVAL;
				}
				else
				{
					if (param.row_len == 0 || param.row_stride == 0)
						return -EINVAL;
					else if (param.row_len == 1)
						rc = read_by_row_len1(dev_stat, &param);
					else
						rc = read_by_row_len_n(dev_stat, &param);
					dma_rmb();
				}
				break;

		    case ZYNQMP_DMA_V7:
				rc = zynqmp_dma_ioctl(dev_stat, &param);
				break;

		    case ZYNQMP_READ_DMA_STATUS_BLOCK:
				if (param.start >= sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t) || param.start+param.num>sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t))
				{
					rc = -EINVAL;
					break;
				}
				ptr = ((u_int32_t *)&(dev_stat->status_block))+param.start;
				if (copy_to_user((void __user *)param.ptr, (void *) ptr, param.num*sizeof(u_int32_t)))
				{
					rc = -EFAULT;
				}
				break;
				
/* 		    case XH_RD_DMA_STATUS:
 				d = in_be32(io_base+XH_DMA_STATUS);
 				break;
 		    case XH_WR_DMA_CONTROL:
 				out_be32(io_base+XH_DMA_CONTROL, arg);
 				break;
*/
		    case ZYNQMP_MEMZERO:
				if (param.addr_space > ZYNQMP_ADDR_SPACE_DMA_BUFF && param.addr_space < ZYNQMP_ADDR_SPACE_DMA_BUFF + ZYNQMP_DMA_STREAM_NUM)
				{
					int stream = param.addr_space - ZYNQMP_ADDR_SPACE_DMA_BUFF;
					DMAStream *dma = dev_stat->status_block.stream_def;
					if (param.num==0 || param.start >= dma[stream].data_size/4 || param.start+param.num>dma[stream].data_size/4)
					{
						rc = -EINVAL;
						break;
					}
					byte_offset = param.start*sizeof(u_int32_t);
					remaining = param.num*sizeof(u_int32_t);
					dma_wmb();	// From io.h note that barriers are used Before WRITE and After read.
					while (remaining > 0)
					{
						ptr = (u_int32_t __iomem *) zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_bytes, &addr_space, NULL, NULL);
						if (ptr == NULL)
						{
							rc = -EINVAL;
							break;
						}
						base = ptr;
						num_bytes = chunk_bytes;
						if (byte_offset & 4)
						{
							ZYNQMP_IOWrite32(ptr, 0);
							ptr++;
							num_bytes -= sizeof(u_int32_t);
						}
						if (num_bytes>0 )
							memset((void *) ptr, 0, num_bytes);
						if (dev_stat->access_flags[addr_space] & ACCESS_CACHEFLUSH)
							__flush_dcache_area(base, chunk_bytes);
						byte_offset += chunk_bytes;
					}
				}
				else if (param.addr_space < 0 || param.addr_space >= ZYNQMP_NUM_ADDR_SPACE)
				{
					rc = -EINVAL;
				}
				else
				{
					if (param.num==0 || param.start >= dev_stat->size_bytes[param.addr_space]/4 || param.start+param.num>dev_stat->size_bytes[param.addr_space]/4)
					{
						rc = -EINVAL;
						break;
					}
					if (debug >= MSG_VERBOSE)
						printk(KERN_WARNING "%s: Zeroing memory to Phys addr %010llX for %08X words\n", driver_name, dev_stat->phys_base[param.addr_space]+param.start*sizeof(u_int32_t), param.num);
					ptr = dev_stat->virt_base[param.addr_space]+param.start;
					base = ptr;
					num = param.num;
	//							xsp4_invalidate_dcache_range((unsigned long)ptr, (unsigned long)(ptr+param.num));
					dma_wmb();	// From io.h note that barriers are used Before WRITE and After read.
					if (param.start &  1)
					{
						ZYNQMP_IOWrite32(ptr, 0);
						ptr++;
						num--;
					}
					if (num>0 )
						memset((void *) ptr, 0, sizeof(u_int32_t)*num);
					if (dev_stat->access_flags[param.addr_space] & ACCESS_CACHEFLUSH)
						__flush_dcache_area(base, sizeof(u_int32_t)*param.num);
				}
				break;

		    case ZYNQMP_SET_READOUT_MODE:
				dev_stat->readout_mode = arg;
				if (debug >= 1 )
					printk(KERN_NOTICE "%s: Setting readout mode = 0x%08X\n", driver_name, dev_stat->readout_mode);
				break;

		    case ZYNQMP_GET_READOUT_MODE:
				d = dev_stat->readout_mode;
				if (debug >= 1 )
					printk(KERN_NOTICE "%s: Getting readout mode = 0x%08X\n", driver_name, d);
				break;

		   	default:
#if HAVE_EXTRA_IOTCL
				rc = zynqmp_extra_ioctl ( cmd, dev_stat, &param);
#else
		        rc = -EINVAL; /* for Silly Billy */
#endif
		    }
			if (rc == 0)
			{
				if (_IOC_DIR(cmd) & _IOC_READ)
				{
//						printk(KERN_ALERT "Xsp4: Returning value %#X from command num %#x, _IOC_SIZE=%d\n", d, _IOC_NR(cmd), _IOC_SIZE(cmd));
					if (_IOC_SIZE(cmd) == sizeof(uint32_t))
					{
						if (put_user(d, (uint32_t*)arg))
						{
							printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (uint32_t *)arg);
							rc = -EFAULT;
						}
					}
					else if (_IOC_SIZE(cmd) == sizeof (ZynqMPPb))
					{
						if ( copy_to_user((ZynqMPPb*)arg, &param, sizeof(ZynqMPPb)))
							rc = -EFAULT;
					}
					else
					{
						printk(KERN_WARNING "%s: IOC_READ with unknown block size %#x, IOC=%#x\n", driver_name, _IOC_SIZE(cmd), cmd);
						rc = -EINVAL;
					}
				}
			}
		}
	}
	up(&dev_stat->sem); 
	if (rc < 0 || debug >= 3)
	{
		printk(KERN_WARNING "%s: Function %X returns err 0x%X=%d\n", driver_name, cmd, -rc, -rc);
	}
	return rc;
}


/*
 * Copy data from IO memory space to "user" memory space.
 * Made from arch/arm64/include/asm/uaccess.h for user side and arch/arm64/include/io.h for hardware side
 */
int copy32_io_to_user(u32 __user *to, const volatile u32 __iomem *from, size_t count)
{
	u32 l;
	u32 __user *p32 = to;
	u64 q;
	u64 __user *p64;
	int err=0;
	if (!access_ok((void  __user *)to, count))
		return -EFAULT;

	uaccess_enable_not_uao();
	while (count && !IS_ALIGNED((unsigned long)from, 8)) 
	{
		l = __raw_readl(from);
		__put_user_asm("str", "sttr", "%w", l, (p32), (err), ARM64_HAS_UAO);
		from++;
		p32++;
		count-=4;
	}
	p64 = (u64 __user *)p32;
	while (count >= 8) 
	{
		q = __raw_readq(from);
		__put_user_asm("str", "sttr", "%x", q, (p64), (err), ARM64_HAS_UAO);
		from += 2;
		p64 ++;
		count -= 8;
	}
	p32 = (u32  __user *) p64;
	while (count)
	{
		l = __raw_readl(from);
		__put_user_asm("str", "sttr", "%w", l, (p32), (err), ARM64_HAS_UAO);
		from++;
		p32++;
		count-=4;
	}
	uaccess_disable_not_uao();
	return err;
}

/*
 * Copy data from user  memory space to IO memory space, 32  bit aligned.
 */
int copy32_user_to_io(volatile u32 __iomem *to, const u32 __user *from, size_t count)
{
	u32 l;
	const u32 __user *p32 = from;
	u64 q;
	const u64 __user *p64;
	int err=0;

	if (!access_ok((void  __user *)from, count))
		return -EFAULT;

	uaccess_enable_not_uao();

	while (count && !IS_ALIGNED((unsigned long)to, 8)) 
	{
		__get_user_asm("ldr", "ldtr", "%w", l, (p32),(err), ARM64_HAS_UAO);	
		__raw_writel(l, to);
		p32++;
		to++;
		count-=4;
	}
	
	p64 = (u64 __user *)p32;
	while (count >= 8)
	{
		__get_user_asm("ldr", "ldtr", "%x", q, (p64), (err), ARM64_HAS_UAO);
		__raw_writeq(q, to);
		p64 ++;
		to += 2;
		count -= 8;
	}
	p32 = (u32 __user *)p64;
	while (count)
	{
		__get_user_asm("ldr", "ldtr", "%w", l, (p32),(err), ARM64_HAS_UAO);	
		__raw_writel(l, to);
		p32++;
		to ++;
		count-=4;
	}
	uaccess_disable_not_uao();
	return err;
}


/*
 * Copy data from IO memory space to "user" memory space. The data in IO space is not contgous, to suit reading interleaved histogram streams
 * Made from arch/arm64/include/asm/uaccess.h for user side and arch/arm64/include/io.h for hardware side
 */
int read_by_row_len1(DevStatics *dev_stat, ZynqMPPb *param)
{
	u_int32_t __user *p32 = param->ptr;
	volatile u_int32_t __iomem *from;
	int err=0;
	u_int32_t l;
	u_int32_t num = param->num;

	if (param->num==0 || param->start >= dev_stat->size_bytes[param->addr_space]/4 || param->start+(param->num-1)*param->row_stride >= dev_stat->size_bytes[param->addr_space]/4)
		return -EINVAL;

	if (!access_ok((void  __user *)p32, param->num*sizeof(u_int32_t)))
		return -EFAULT;

	from = dev_stat->virt_base[param->addr_space]+param->start;
	if (dev_stat->access_flags[param->addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area((void *)from, sizeof(u_int32_t)*((param->num-1)*param->row_stride+1));

	uaccess_enable_not_uao();
	while (num)
	{
		l = __raw_readl(from);
		__put_user_asm("str", "sttr", "%w", l, (p32), (err), ARM64_HAS_UAO);
		from+=param->row_stride;
		p32++;
		num--;
	}
	uaccess_disable_not_uao();
	return err;
}

int read_by_row_len_n(DevStatics *dev_stat, ZynqMPPb *param)
{
	u_int32_t __user *p32 = param->ptr;
	u_int64_t __user *p64;
	volatile u_int32_t __iomem *from;
	volatile u_int64_t __iomem *from64;
	int err=0;
	u_int32_t remaining = param->num, num;
	u_int32_t start_col, num_flush, full_rows;
	int col;
	if (param->num==0 || param->start >= dev_stat->size_bytes[param->addr_space]/4)
		return -EINVAL;

	if (!access_ok((void  __user *)p32, param->num*sizeof(u_int32_t)))
		return -EFAULT;

	from = dev_stat->virt_base[param->addr_space]+param->start;

	start_col = param->start%param->row_len;
	full_rows = (start_col+param->num-1)/param->row_len;
	num_flush = full_rows*param->row_stride+param->num-full_rows*param->row_len;

	if (param->start+num_flush > dev_stat->size_bytes[param->addr_space]/sizeof(u_int32_t))
		return -EINVAL;

	if (dev_stat->access_flags[param->addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area((void *)from, sizeof(u_int32_t)*num_flush);

	if (start_col > 0 || remaining <= param->row_len)
	{
		/* Read first  (partial row) */
		num = param->row_len-start_col;
		if (num > remaining)
			num = remaining;
		if (param->start+num > dev_stat->size_bytes[param->addr_space]/4)
			return -EINVAL;
		if ((err=copy32_io_to_user(p32, from, num*sizeof(u_int32_t))) != 0)
			return err;
		remaining -= num;
		if (remaining == 0)
			return 0;
		p32 += num;
		from -= start_col;
		from += param->row_stride;
	}
	full_rows = remaining/param->row_len;
	if (debug >= MSG_VERBOSE && 0)
		printk("%s: .... start_col=%d, full_rows=%d\n", driver_name, start_col, full_rows); 
	if (full_rows > 0)
	{
		if ((param->row_len&1) == 0 && (param->row_stride&1) == 0)
		{
			/* All 64 bit aligned */
			uaccess_enable_not_uao();
			p64 = (u_int64_t __user *)p32;
			while (full_rows > 0)
			{
				from64 = (volatile u_int64_t __iomem *)from;
				for (col=0; col<param->row_len/2; col++)
				{
					u_int64_t q;
					q = __raw_readq(from64);
					__put_user_asm("str", "sttr", "%x", q, (p64), (err), ARM64_HAS_UAO);
					from64 ++;
					p64 ++;
				}
				remaining -=param->row_len;
				from += param->row_stride;
				full_rows--;
			}
			uaccess_disable_not_uao();
			p32 = (u32 __user *)p64;
			if (err)
				return err;
		}
		else
		{
			while (full_rows > 0)
			{
				if ((err=copy32_io_to_user(p32, from, param->row_len*sizeof(u_int32_t))) != 0)
					return err;
				remaining -= param->row_len;
				p32 += param->row_len;
				from += param->row_stride;
				full_rows--;
			}
		}
	}
	if (remaining > 0)
	{
		if (debug >= MSG_VERBOSE && 0)
			printk("%s: .... Last partial row remaining=%d to=%px, from=%px, data=%08X\n", driver_name, remaining, p32, from, readl(from)); 
		if ((err=copy32_io_to_user(p32, from, remaining*sizeof(u_int32_t))) != 0)
			return err;
	}
	return err;
}

