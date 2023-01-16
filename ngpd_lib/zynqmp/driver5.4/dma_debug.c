/*
 * dma_debug.c
 *
 *  Created on: 30 Marh 2015
 *      Author: wih73
 *      Developed from the PowrePC/Virtex-5 version.
 *      This version builds varying length frames to try to break the 10G firmware.
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
#include "xaxidma_hw.h"

int zynqmp_dma_build_debug_desc(DevStatics *dev_stat, ZynqMPPb *param)
{
	u_int64_t num_bytes;
	int first=1, last=0;
	AXIDMADesc *desc;
	u_int32_t ctrl;
	int n = 0;
	u_int64_t chunk_num_bytes;
	int n_frames = 0;
	u_int32_t max_desc_bytes;
	int small_desc_size = 16;
	int packet_desc_size;
	int frame_desc_size;
	int small_inc=0, packet_inc=0, frame_inc=0;
	u_int64_t desc_phys;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	u_int64_t byte_offset, dma_addr;
	int addr_space, seg_num;
//	u_int64_t remaining_bytes_frame;
//	int shortened_frame;
//	enum { FSStart, FSContinue} frame_state; 


	ZYNQMP_DMA_MsgBuildDebugDesc msg;
	

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgBuildDebugDesc)))
		return -EFAULT;

	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA Build debug desc command requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if (debug)
		printk(KERN_WARNING "%s: [INFO] Build Debug Desc for src_stream=%d, flags0x%04X", driver_name, msg.src_stream, msg.flags);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_CONF) == 0)
	{
		printk(KERN_WARNING "[ERROR] Stream %d is not Configured\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}

	if ((dma[stream].state & (ZYNQMP_DMA_STATE_DESC_BUILT | ZYNQMP_DMA_STATE_DESC_DEBUG)) == (ZYNQMP_DMA_STATE_DESC_BUILT | ZYNQMP_DMA_STATE_DESC_DEBUG)  && memcmp(&msg, &(dev_stat->dma_last_build_debug_desc[stream]), sizeof(ZYNQMP_DMA_MsgBuildDebugDesc)) == 0)
	{
		/* Can clean and resend */
		int i;
		DBGLEVEL(MSG_NORMAL, "Cleaning Debug TX Descriptors stream=%d, src stream=%d\n", stream, msg.src_stream);
		desc = dev_stat->dma_priv[stream].desc_virt_base;
		for (i=0; i<dma[stream].defined_desc; i++)
		{
			desc->status = 0;
			desc->app[3] = 0;
			desc++;
		}
	}
	else
	{
		if (msg.src_stream == ZYNQMP_DMA_STREAM_BNUM_DIRECT)
		{
			byte_offset = msg.addr;
			num_bytes = msg.size_bytes;
		}
		else
		{
			if (zynqmp_dma_check_stream(dev_stat, msg.src_stream))
			{
				printk(KERN_WARNING "[ERROR] Src Stream %d is not a Valid\n", msg.src_stream);
				return -ZYNQMP_DMA_ERROR_BAD_STREAM;
			}
			if (!(dma[msg.src_stream].state & ZYNQMP_DMA_STATE_BUFFER_CONF))
			{
				printk(KERN_WARNING "[ERROR] Build Descriptor on function %d, buffer not defined\n", stream);
				return -ZYNQMP_DMA_ERROR_UNCONF_BUFF;
			}
			byte_offset = 0;
			num_bytes =  dma[msg.src_stream].data_size;
			byte_offset += msg.addr;
			if (msg.size_bytes)
				num_bytes = msg.size_bytes;
		}
		dma[stream].transfer_start = byte_offset;
		dma[stream].transfer_size = num_bytes;
		dma[stream].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;
		if (debug)
			printk(KERN_WARNING "[INFO] Building TX Descriptors function %d, Start offset =%010llX, num_bytes=%010llX\n", stream, byte_offset, num_bytes);

		if (msg.packet_size)
			packet_desc_size = msg.packet_size;
		else
			packet_desc_size = 8000-3*8;

		if (msg.frame_size)
			frame_desc_size = msg.frame_size;
		else
			frame_desc_size = dma[stream].max_block_bytes;
		if (debug)
			printk(KERN_WARNING "[INFO] msg.frame_size=%d, hence frame_desc_size=%d\n", msg.frame_size, frame_desc_size);
		dma[stream].state &= ~ZYNQMP_DMA_STATE_DESC_BUILT;

		desc_phys = dma[stream].desc_dma;
		desc = dev_stat->dma_priv[stream].desc_virt_base;

		if (msg.flags & ZYNQMP_DMA_DEBUG_DESC_ALL_SMALL)
			small_desc_size = 32;
		else if (msg.flags & ZYNQMP_DMA_DEBUG_DESC_SHORT_BURSTS)
		{
			frame_desc_size = 8000-80;
			small_desc_size = 800;
		}
		/*** FixMe: This will not fully cope with frame split because we skip from one segment to the next */
		do
		{
			u_int64_t remaining; // max_next_chunk;
			remaining = num_bytes;
			if ( zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_num_bytes, &addr_space, &dma_addr, &seg_num) == NULL)
			{
				printk(KERN_WARNING "[ERROR] Build Ddebug Descriptor on function %d, byte offset %llX does not fit in memory segments\n", stream, byte_offset);
				return -ZYNQMP_DMA_ERROR_DATA_RANGE;
			}

			if (msg.flags & ZYNQMP_DMA_DEBUG_DESC_SHORT_BURSTS)
			{
				if (n % 30 >= 22)
				{
					max_desc_bytes = small_desc_size;
					small_desc_size += 8;
					if (small_desc_size >= 150*8)
						small_desc_size = 800;
				}
				else
				{
					if ((n % 30) == 0)
					{
						frame_desc_size += 8;
						if (frame_desc_size >= 8080)
							frame_desc_size = 8000-80;
					}
					max_desc_bytes = frame_desc_size;
				}
			}
			else if (msg.flags & ZYNQMP_DMA_DEBUG_DESC_ALL_SMALL)
			{
				max_desc_bytes = small_desc_size;
				small_desc_size -= 8;
				if (small_desc_size == 8)
					small_desc_size = 32;
			}
			else
			{
				if (n % 5 == 0 && (msg.flags & ZYNQMP_DMA_DEBUG_DESC_SMALL))
				{
					max_desc_bytes = small_desc_size+8*small_inc;
					small_inc = (small_inc+1) % 3;
				}
				else if ((n%5==1 || n%5==3) && (msg.flags & ZYNQMP_DMA_DEBUG_DESC_NEAR_PACKET))
				{
					max_desc_bytes = packet_desc_size+8*packet_inc;
					packet_inc = (packet_inc+1) % 13;
				}
				else
				{
					max_desc_bytes = frame_desc_size + frame_inc*8;
					if (msg.flags & ZYNQMP_DMA_DEBUG_DESC_INC_FRAME)
						frame_inc = (frame_inc+1) % 29;
				}
			}

			if (chunk_num_bytes > max_desc_bytes)
				chunk_num_bytes = max_desc_bytes;

			if (chunk_num_bytes == num_bytes)
				last =1;
			if (last)
				desc->phys_next = dma[stream].desc_dma;	// Idea is that playback loops forever, others should not get here due to Stop on END
			else
				desc->phys_next = desc_phys+sizeof(AXIDMADesc);

			desc->phys_addr = dma_addr;
			desc->frame_num = n_frames;
			ctrl = chunk_num_bytes;
			if (dma[stream].is_tx)
			{
				switch (dma[stream].frame_rule)
				{
				case AllOneFrame:
					if (first)
						ctrl |= XAXIDMA_BD_CTRL_TXSOF_MASK;
					if (last)
					{
						ctrl |= XAXIDMA_BD_CTRL_TXEOF_MASK;
						n_frames ++;
					}
				break;
				case FramePerDesc:
					ctrl |= XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK;
					n_frames ++;
					break;

				case FrameByBytes:
					printk(KERN_WARNING "[ERROR] Not implemented SOP/EOP by byte count yet\n");
					return -ZYNQMP_DMA_ERROR_UNKNOWN;
				}
			}
			else
			{

			}
			desc->control = ctrl;
			desc->status = 0;

			if ((debug >= MSG_NORMAL && n < 20) || debug >= MSG_VERBOSE)
				printk(KERN_WARNING "Desc. %d, Addr=%010llX, Nxt=%010llX, DMA addr=%010llX, num_bytes=%08X, ctrl=%08X\n", n, desc_phys, desc->phys_next, desc->phys_addr, desc->control & 0x7FFFFF, desc->control);
			desc++;
			desc_phys += sizeof(AXIDMADesc);
			byte_offset += chunk_num_bytes;
			num_bytes -= chunk_num_bytes;
			n++;
			first = 0;
		} while (num_bytes > 0 && n < dma[stream].num_desc);
		if (num_bytes > 0)
		{
			dma[stream].transfer_size -= num_bytes;
			printk(KERN_WARNING "[INFO] Stream %d, Ran out of descriptors, can transfer %lld bytes\n", stream, dma[stream].transfer_size);
		}
//		Xil_DCacheFlushRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*n);
		if (debug >=1 )
			printk(KERN_WARNING "{INFO} Build %d descriptor gving %d frames\n", n, n_frames);
		dma[stream].state |= ZYNQMP_DMA_STATE_DESC_BUILT | ZYNQMP_DMA_STATE_DESC_DEBUG;
		dev_stat->dma_last_build_debug_desc[stream] = msg;
		dma[stream].defined_desc = n;
		dma[stream].num_frames = n_frames;
	}
	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__flush_dcache_area(dev_stat->dma_priv[stream].desc_virt_base, dma[stream].num_desc*sizeof(AXIDMADesc));
	return -ZYNQMP_DMA_ERROR_OK;
}


