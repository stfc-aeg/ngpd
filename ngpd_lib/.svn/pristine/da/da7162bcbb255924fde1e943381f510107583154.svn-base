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
#include <linux/rwsem.h>

#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"

int zynqmp_dma_build_test_pat(DevStatics *dev_stat, ZynqMPPb *param)
{
	int rc = 0;
	int i;
	u8  c, *base, *cp;
	u_int16_t s0, s1, *sptr;
	u_int32_t u, *up;
	u_int32_t prev_bit32, bit32;
	u_int64_t remaining,  chunk_bytes, byte_offset,rem_copy;
	int stream = param->num;
	ZYNQMP_DMA_MsgTestPat msg;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int num_pb_streams = 8;
	int addr_space;
	int pb_stream=0;
	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA Build test pattern command requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgTestPat)))
		return -EFAULT;

	if (stream !=0 && !(dma[stream].state & ZYNQMP_DMA_STATE_BUFFER_CONF))
	{
		printk(KERN_WARNING "%s: [ERROR] Build test pattern on stream %d, Buffer address and size not defined\n", driver_name, stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_BUFF;
	}
	byte_offset = msg.address;
	if (msg.num_bytes == 0)
		remaining = dma[stream].data_size-msg.address;
	else
		remaining = msg.num_bytes;
	printk(KERN_WARNING "%s: [INFO] Build test pattern for stream %d,from start offset 0x%010llX for %08llX bytes, options=0x%04X\n", driver_name, stream, byte_offset, remaining, msg.options);
	u = msg.start_val;
	c  = (u_int8_t) msg.start_val;
	prev_bit32 = 0;
	s0 = msg.start_val & 0xFFFF;
	s1 = msg.start_val >> 16;
	while (remaining>0)
	{
		rem_copy = remaining;
		base = (char *)zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &rem_copy, &chunk_bytes, &addr_space, NULL, NULL);
		if (base == NULL)
			return -ZYNQMP_DMA_ERROR_OUTSIDE_VA;
		if(chunk_bytes > 0x100000)
			chunk_bytes = 0x100000;
		wmb();
		switch (msg.options)
		{
		case ZYNQMP_DMA_MSG_TP_INC32:
			up = (u_int32_t *) base;
			for (i=0; i<chunk_bytes/4; i++)
				*up ++ = u++;
			break;

		case ZYNQMP_DMA_MSG_TP_INC8:
			cp = base;
			for (i=0; i<chunk_bytes; i++)
				*cp ++ = c++;
			break;

		case ZYNQMP_DMA_MSG_TP_SLIDE:
			up = (u_int32_t *) base;
			for (i=0; i<chunk_bytes/4; i++)
			{
				*up ++ = u;
				bit32 = u >>31;
				u = u << 1 | prev_bit32;
				prev_bit32 = bit32;
			}
			break;
		case ZYNQMP_DMA_MSG_TP_PLAYBACK1:
			sptr = (u_int16_t *) (base);

			for (i=0; i<chunk_bytes/sizeof(u_int16_t); i++)
			{
				*sptr++ = s0;
				s0++;
			}
			break;

		case ZYNQMP_DMA_MSG_TP_PLAYBACK2:
		case ZYNQMP_DMA_MSG_TP_PLAYBACK4:
		case ZYNQMP_DMA_MSG_TP_PLAYBACK8:
			if (msg.options == ZYNQMP_DMA_MSG_TP_PLAYBACK2)
				num_pb_streams = 2;
			else if (msg.options == ZYNQMP_DMA_MSG_TP_PLAYBACK4)
				num_pb_streams = 4;
			else 
				num_pb_streams = 8;
			sptr = (u_int16_t *) (base);
			
			s0 = s0 & 0xFFF;
			for (i=0; i<chunk_bytes/sizeof(u_int16_t); i++)
			{
				*sptr++ = s0 | pb_stream<<12;
				pb_stream++;
				if (pb_stream == num_pb_streams)
				{
					pb_stream = 0;
					s0 = (s0+1) & 0xFFF;
				}
			}
			break;

		case ZYNQMP_DMA_MSG_TP_DEADBEEF:
			up = (u_int32_t *) base;
			for (i=0; i<chunk_bytes/4; i++)
			{
				*up ++ = 0xdeadbeef;
			}
			break;


		default:
			DBGLEVEL(MSG_ERROR, "[ERROR] DMA Build test pattern command unknown test pattern options 0x%02X\n", msg.options);
			rc = -ZYNQMP_DMA_ERROR_BAD_COMMAND;
			break;
		}
		wmb();
		if (rc != 0)
			break;
		if (dev_stat->access_flags[addr_space] & ACCESS_CACHEFLUSH)
			__flush_dcache_area(base, chunk_bytes);
		remaining -= chunk_bytes;
		byte_offset += chunk_bytes;
		if (remaining > 0)
			cond_resched();
	}
	return rc;

}
