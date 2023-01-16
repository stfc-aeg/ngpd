/*
	05/05/2015	Ported code to run as Linux/ARM device driver

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
#include <net/sock.h>

#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"

int zynqmp_dma_print_data(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i;
	u_int32_t *up;
	int rc = 0;
	ZYNQMP_DMA_MsgPrint msg;
	int stream;
	int addr_space, seg_num;
	u_int64_t byte_offset, remaining, chunk_bytes, dma_addr;
	DMAStream *dma = dev_stat->status_block.stream_def;
	
	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgPrint)))
		return -EFAULT;

	stream = param->num;

	if (stream == 0)
	{
		byte_offset = msg.offset;
		remaining = msg.num_bytes;
	}
	else if (stream >= ZYNQMP_DMA_STREAM_NUM)
	{
		printk(KERN_WARNING "%s: [ERROR] Display command, stream %d is out of range\n", driver_name, stream);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}
	else
	{
		if (!(dma[stream].state & ZYNQMP_DMA_STATE_BUFFER_CONF))
		{
			printk(KERN_WARNING "%s:[ERROR] Display command on stream %d, Buffer address and size not defined\n", driver_name, stream);
			return -ZYNQMP_DMA_ERROR_UNCONF_BUFF;
		}
		byte_offset = msg.offset;

		remaining= 1000;
		if (msg.num_bytes != 0)
		{
			if (msg.num_bytes < dma[stream].data_size)
				remaining = msg.num_bytes;
			else
				remaining = dma[stream].data_size;
		}
	}

	while (remaining > 0)
	{
		up = zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_bytes, &addr_space, &dma_addr, &seg_num);
		if (up == NULL)
		{
			printk(KERN_WARNING "%s [ERROR] Requested region from physical byte offset 0x%010llX for 0x%08llX bytes does not lie in mapped area of remote DRAM \n", driver_name, byte_offset, remaining);
			return -ZYNQMP_DMA_ERROR_OUTSIDE_VA;
		}
		printk(KERN_WARNING "%s: Displaying data for stream %d, segment=%d: From byte offset address 0x%010llX DMA phys addr=%010llX, for %08llX bytes\n", driver_name, stream, seg_num, 
						byte_offset, dma_addr, chunk_bytes);

		rmb();
		switch (msg.options & 3)
		{
		case ZYNQMP_DMA_MSG_PRINT_1COL | ZYNQMP_DMA_MSG_PRINT_DEC:
			for (i=0; i<chunk_bytes/4; i++)
			{
				printk(KERN_WARNING "%s: %lld\t%d\n", driver_name, dma_addr, *up);
				up++;
				dma_addr += sizeof(u_int32_t);
			}
			break;

		case ZYNQMP_DMA_MSG_PRINT_1COL:
			for (i=0; i<chunk_bytes/4; i++)
			{
				printk(KERN_WARNING "%s: %010llX\t%08X\n", driver_name, dma_addr, *up);
				up++;
				dma_addr += sizeof(u_int32_t);
			}
			break;

		case  ZYNQMP_DMA_MSG_PRINT_DEC:
			for (i=0; i<chunk_bytes/4; i++)
			{
				if (i % 4 == 0)
					printk(KERN_WARNING "%s: %lld", driver_name, dma_addr);
				printk(KERN_WARNING "\t%d", *up);
				if (i % 4 == 3)
					printk(KERN_WARNING "\n");
				up++;
				dma_addr += sizeof(u_int32_t);
			}
			break;

		case 0:
			for (i=0; i<chunk_bytes/4; i++)
			{
				if (i % 4 == 0)
					printk(KERN_WARNING "%s: %010llX", driver_name, dma_addr);
				printk(KERN_WARNING "\t%08X", *up);
				if (i % 4 == 3)
					printk(KERN_WARNING "\n");
				up++;
				dma_addr += sizeof(u_int32_t);
			}
			break;

		}
		rmb();
		byte_offset += chunk_bytes;
	}
	printk(KERN_WARNING "\n");

	return rc;

}
