/*
	10//02/2021	Ported code to run as Linux/ARM64 device driver.
	Refactored slightly to try to ease reuse.
	zynqmp_ function are intened for reuse.
	ngzmp_ fucntions are expected to be specific for Neutron Gamma Pulse Discriminator

*/
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/types.h>   /* ulong and friends */ 
#include <linux/sched.h>   /* current, task_struct, other goodies */ 
#include <linux/wait.h>   /* current, task_struct, other goodies */ 
#include <linux/fcntl.h>   /* O_NONBLOCK etc. */ 
#include <linux/errno.h>   /* return values */ 
#include <linux/ioport.h>  /* request_region() */ 
#include <linux/iopoll.h>  /* request_region() */ 
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

#define XSP3M_HW_NUM_SCALERS 8
#define XSP4_MAX_NUM_CHAN 16
#include "xaxidma_hw.h"

int ngzmp_dma_config_memory(DevStatics *dev_stat, int layout);
int zynqmp_dma_get_desc_status(DevStatics *dev_stat, ZynqMPPb *param);


int zynqmp_dma_ioctl(DevStatics *dev_stat, ZynqMPPb *param)
{
	int rc;
	u_int32_t dma_status=0;

	if (debug >= MSG_NORMAL)
		printk("%s: Calling DMA command 0x%08X\n", driver_name, param->start); 

#if ZYNQMP_USE_SOCKETS
	if (param->start == ZYNQMP_DMA_CMD_READ_SEND_LIST)
	{
		int dma_lock, sock_lock;
		u_int32_t d, *ptr;
		int stream = param->num;
		if ((dma_lock = down_read_trylock(&dev_stat->dma_sem)))	/* 1 if successsful, from https://elixir.bootlin.com/linux/v4.9/source/kernel/locking/rwsem.c, unlike other web pages ? */
			up_read(&dev_stat->dma_sem);
		if (stream == XSP3M_DMA_STREAM_BNUM_HIST_FRAMES)
		{
			if ((sock_lock = mutex_trylock(&dev_stat->hist_frame_mutex)))
				mutex_unlock(&dev_stat->hist_frame_mutex);
		}
		else if (stream == XSP3_DMA_STREAM_BNUM_SCALERS)
		{
			if ((sock_lock = mutex_trylock(&dev_stat->scalar_mutex)))
				mutex_unlock(&dev_stat->scalar_mutex);
		}
		else
		{
			if ((sock_lock = mutex_trylock(&dev_stat->hist_list_mutex)))
				mutex_unlock(&dev_stat->hist_list_mutex);
			stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
		}
		ptr = (u_int32_t *)(param->ptr);
		d = dev_stat->status_block.stream_def[stream].desc;
		put_user(d, ptr); ptr++;
		d = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4);
		put_user(d, ptr); ptr++;
		d = ZYNQMP_IORead32(NGPD_MAX_ERROR_MESSAGE[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4);
		put_user(d, ptr); ptr++;
		d = ZYNQMP_IORead32(NGPD_MAX_ERROR_MESSAGE[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
		put_user(d, ptr); ptr++;
		d = ZYNQMP_IORead32(NGPD_MAX_ERROR_MESSAGE[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
		put_user(d, ptr); ptr++;
		d = dev_stat->readout_cur_desc[stream];
		put_user(d, ptr); ptr++;
		put_user(dma_lock, ptr); ptr++;
		put_user(sock_lock, ptr); ptr++;
		d= dev_stat->readout_error_flags[stream];
		put_user(d, ptr); ptr++;
		d = (u_int32_t)dev_stat->last_time_frame[stream];
		put_user(d, ptr); ptr++;
		rc = 0;
		return rc;
	}
#endif

	down_write(&dev_stat->dma_sem);
	switch (param->start)
	{
	case ZYNQMP_DMA_CMD_RESET:
		rc = zynqmp_dma_reset(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_CONFIG_MEMORY:
		rc = zynqmp_dma_config_mem(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_BUILD_DESC:
		rc = zynqmp_dma_build_desc(dev_stat, param, &dma_status);
		break;

	case ZYNQMP_DMA_CMD_START:
		rc = zynqmp_dma_start(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_BUILD_AND_START:
		rc = zynqmp_dma_build_desc(dev_stat, param, &dma_status);
		rc = zynqmp_dma_start(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_BUILD_TEST_TEST_PAT:
		rc = zynqmp_dma_build_test_pat(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_PRINT_DATA:
		rc = zynqmp_dma_print_data(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_PRINT_SCOPE:
		rc = zynqmp_dma_print_scope(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_RESEND:
		rc = zynqmp_dma_resend(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_READ_STATUS:
		rc = zynqmp_dma_read_status(dev_stat, param, &dma_status);
		break;

	case ZYNQMP_DMA_CMD_BUILD_DEBUG_DESC:
		rc = zynqmp_dma_build_debug_desc(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_PRINT_DESC:
		rc = zynqmp_dma_print_desc(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_CHECK_RX_DESC:
		rc = zynqmp_dma_check_rx_desc(dev_stat, param, &dma_status);
		break;

	case ZYNQMP_DMA_CMD_SET_MSG_SERVER:
		rc = -1;
		break;

	case ZYNQMP_DMA_CMD_SET_MSG_DRIVER:
		debug = param->num;
		rc = 0;
		break;

	case ZYNQMP_DMA_CMD_GET_DESC_STATUS:
		rc = zynqmp_dma_get_desc_status(dev_stat, param);
		break;

#if 0
	case ZYNQMP_DMA_CMD_REUSE:
		rc = zynqmp_dma_reuse(dev_stat, param);
		break;
	case ZYNQMP_DMA_CMD_CHECK_RX_DESC_CIRCULAR:
		rc = zynqmp_dma_check_rx_desc_circular(dev_stat, param);
		break;		
	case ZYNQMP_DMA_CMD_READ_TF_STATUS:
		rc = zynqmp_dma_read_tf_status(dev_stat, param);
		break;

	case ZYNQMP_DMA_CMD_READ_TF_MARKERS:
		rc = zynqmp_dma_read_markers(dev_stat, param);
		break;
#endif
	case ZYNQMP_DMA_CMD_STOP:
		rc = zynqmp_dma_stop(dev_stat, param, &dma_status);
		break;

	default:
		rc = -1;
		break;
	}
	up_write(&dev_stat->dma_sem);
	if (param->ptr2 != NULL)
	{
		if (put_user(dma_status, (int32_t*)(param->ptr2)))
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr2));
			rc = -EFAULT;
		}
	}
	return rc;
}

#define MAX_BLOCK_BYTES 0x7FFFC0



int zynqmp_dma_reset(DevStatics *dev_stat, ZynqMPPb *param)
{
	u_int32_t stat, busy;
	int stream, i, n;
	int found_it;
	u_int32_t *base_addr[ZYNQMP_DMA_STREAM_NUM];
	int reset_tx[ZYNQMP_DMA_STREAM_NUM];
	u_int32_t stream_mask = param->num;

	for (i=0; i<ZYNQMP_DMA_STREAM_NUM; i++)
		base_addr[i] = 0;

	DBGLEVEL(MSG_NORMAL, "%s : xsp3_dma_reset entered, stream mask=%02X\n", driver_name, stream_mask);
	n=0;
	for (stream=0; stream<ZYNQMP_DMA_STREAM_NUM; stream++)
	{
		DBGLEVEL(MSG_VERBOSE, "%s : zynqmp_dma_reset reset trying stream %d, base_address=%08llX, mask=%02X\n", driver_name, stream, dev_stat->status_block.stream_def[stream].base_addr, stream_mask);
		if (dev_stat->status_block.stream_def[stream].base_addr != 0 && (stream_mask & 1 << stream))
		{
			DBGLEVEL(MSG_VERBOSE,"%s : zynqmp_dma_reset reset for stream %d, mask=%02X\n", driver_name, stream, stream_mask);
			found_it = 0;
			dev_stat->last_checked[stream] = -1;
			for (i=0; i<n; i++)
				if (dev_stat->dma_priv[stream].virt_base == base_addr[i])
					found_it = 1;
			if (!found_it)
			{
				base_addr[n] = dev_stat->dma_priv[stream].virt_base;
				reset_tx[n++] = dev_stat->status_block.stream_def[stream].is_tx;
			}
		}
	}
	for (i=0; i<n && base_addr[i] != 0; i++)
	{
		DBGLEVEL(MSG_VERBOSE, "%s : zynqmp_dma_reset at base address %px, address=%px\n", driver_name, base_addr[i], base_addr[i]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
		if (reset_tx[i])
			ZYNQMP_IOWrite32(base_addr[i]+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RESET_MASK);
		else		
			ZYNQMP_IOWrite32(base_addr[i]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RESET_MASK);		
	}
	do
	{
		busy = 0;
		for (i=0; i<n && base_addr[i] != 0; i++)
		{
			DBGLEVEL(MSG_VERBOSE, "Poll reset num %d, pointer %px\n", i, base_addr[i]);
			if (reset_tx[i])
				stat = ZYNQMP_IORead32(base_addr[i]+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			else
				stat = ZYNQMP_IORead32(base_addr[i]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			busy |= stat & XAXIDMA_CR_RESET_MASK;
		}
	} while (busy);

	return 0;
}

	
int zynqmp_dma_build_desc(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *num_desc)
{
	u_int64_t num_bytes;
	int first=1, last=0;
	AXIDMADesc *desc;
	u_int64_t desc_phys;
	u_int64_t byte_offset, dma_addr;
	u_int32_t ctrl;
	int n = 0;
	u_int64_t chunk_num_bytes;
	int n_frames=0;
	u_int32_t max_block_bytes;
	int stream = param->num;
	ZYNQMP_DMA_MsgBuildDesc msg;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int addr_space, seg_num;
	u_int64_t remaining_bytes_frame;
	int shortened_frame;
	enum { FSStart, FSContinue} frame_state; 

	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA Build desc command requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}
	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgBuildDesc)))
		return -EFAULT;

	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_CONF) == 0)
	{
		printk(KERN_WARNING "[ERROR] Stream %d is not Configured\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}

	DBGLEVEL(MSG_VERBOSE, "Build desc src_stream=%d, msg.addr=%lld, msg.size=%lld\n", msg.src_stream, msg.addr, msg.size_bytes);
	if ((dma[stream].state & (ZYNQMP_DMA_STATE_DESC_BUILT | ZYNQMP_DMA_STATE_DESC_DEBUG)) == ZYNQMP_DMA_STATE_DESC_BUILT && memcmp(&msg, &(dev_stat->dma_last_build_desc[stream]), sizeof(ZYNQMP_DMA_MsgBuildDesc)) == 0)
	{
		/* Can clean and resend */
		int i;
		DBGLEVEL(MSG_NORMAL, "Cleaning TX Descriptors stream=%d, src stream=%d\n", stream, msg.src_stream);
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
				printk(KERN_WARNING "%s: [ERROR] Src Stream %d is not a Valid\n", driver_name, msg.src_stream);
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
		max_block_bytes = dma[stream].max_block_bytes;
		if (msg.block_size > 0)
			max_block_bytes = msg.block_size;

		dma[stream].transfer_start = byte_offset;
		dma[stream].transfer_size  = num_bytes;
		dma[stream].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;

		desc_phys = dma[stream].desc_dma;
		desc = dev_stat->dma_priv[stream].desc_virt_base;

		DBGLEVEL(MSG_NORMAL, "Building TX Descriptors stream=%d, src stream=%d\nDescriptor Vaddr=%px, Byte offset=%08llX\nStart address=%08llX, num_bytes=%08llX, max_block=%08X\n", 
								stream, msg.src_stream, desc, desc_phys, byte_offset, num_bytes, max_block_bytes);

		dma[stream].state &= ~ZYNQMP_DMA_STATE_DESC_BUILT & ~ZYNQMP_DMA_STATE_DESC_DEBUG;

	//	Xil_DCacheInvalidateRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*dma[stream].num_desc);
		remaining_bytes_frame = max_block_bytes;
		frame_state = FSStart;
		do
		{
			u_int64_t remaining, max_next_chunk;
			remaining = num_bytes;
			if ( zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_num_bytes, &addr_space, &dma_addr, &seg_num) == NULL)
			{
				printk(KERN_WARNING "[ERROR] Build Descriptor on function %d, byte offset %llX does not fit in memory segments\n", stream, byte_offset);
				return -ZYNQMP_DMA_ERROR_DATA_RANGE;
			}

			if (chunk_num_bytes >= max_block_bytes)
				chunk_num_bytes = max_block_bytes;
			if (dma[stream].frame_rule == FramePerDesc)
			{
				if (chunk_num_bytes > remaining_bytes_frame)
					chunk_num_bytes = remaining_bytes_frame;
			}
			if (chunk_num_bytes == num_bytes)
				last =1;
			if (last)
			{
				desc->phys_next = dma[stream].desc_dma;	// Idea is that playback loops forever, others should not get here due to Stop on END. Also used by hist-list function.
			}
			else
			{
				desc->phys_next = desc_phys+sizeof(AXIDMADesc);
			}
			max_next_chunk = dma[stream].buff_seg[seg_num].size-(dma_addr-dma[stream].buff_seg[seg_num].dma_base);
			if (stream == ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM && num_bytes != chunk_num_bytes && (num_bytes - chunk_num_bytes <= 16 || max_next_chunk <= 16))
			{
				/* Special case is trapped at sending end so that no packet is too short (16 bytes = 2 beats (SOF/EOF)) */
				chunk_num_bytes -= 16;
				shortened_frame = 1;
			}
			else
				shortened_frame = 0;

			desc->phys_addr = dma_addr;
			desc->reserved1[0] = 0;
			desc->reserved1[1] = 0;
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
					if (frame_state == FSStart)
					{
						if (chunk_num_bytes == num_bytes || shortened_frame || chunk_num_bytes == max_block_bytes)
						{
							ctrl |= XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK;
							n_frames++;
						}
						else
						{
							ctrl |= XAXIDMA_BD_CTRL_TXSOF_MASK;
							frame_state = FSContinue;
							remaining_bytes_frame -= chunk_num_bytes;
						}

					}
					else
					{
						if (chunk_num_bytes == num_bytes || shortened_frame || chunk_num_bytes == remaining_bytes_frame)
						{
							ctrl |=  XAXIDMA_BD_CTRL_TXEOF_MASK;
							frame_state = FSStart;
							remaining_bytes_frame = max_block_bytes;
							n_frames++;
						}
						else
						{
							remaining_bytes_frame -= chunk_num_bytes;
						}
					}
					break;

				case FrameByBytes:
					printk(KERN_WARNING "[ERROR] Not implemented SOP/SOP by byte count yet\n");
					return -ZYNQMP_DMA_ERROR_UNKNOWN;
				}
			}
			else
			{

			}
	//		if (last && stream != ZYNQMP_DMA_STREAM_BNUM_PLAYBACK)
	//			ctrl |= XLLDMA_BD_STSCTRL_SOE_MASK;
			desc->control  = ctrl;
			desc->status = 0;
	//		desc->app[1] = 0;

			DBGLEVEL(MSG_VERBOSE, "Desc. %d, Vaddr=%px, Paddr=%010llX, Nxt=%010llX\n.. DMA addr=%010llX, num_bytes=%08X, ctrl=%08X\n", n, desc, desc_phys, desc->phys_next, desc->phys_addr, desc->control & 0x7FFFFF, desc->control);
			if (stream == ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G && (chunk_num_bytes&7) )
			{
				DBGLEVEL(MSG_ERROR, "ERROR: None 64 bit aligned Frame size in 10 G TX Descriptor\nDesc. %d, Addr=%px, Nxt=%010llX, DMA addr=%010llX, num_bytes=%08X, ctrl=%08X\n", n, desc, 
																										desc->phys_next, desc->phys_addr, desc->control & 0x7FFFFF, desc->control);
			}
			desc++;
			desc_phys += sizeof(AXIDMADesc);
			byte_offset += chunk_num_bytes;
			num_bytes -= chunk_num_bytes;
			n++;
			first = 0;
		} while (num_bytes > 0 && n < dma[stream].num_desc);
		if (num_bytes > 0)
		{
			printk(KERN_WARNING "[ERROR] Stream %d, Ran out of descriptors. Used %d of %d with %lld bytes remaining\n", stream, n, dma[stream].num_desc, num_bytes );
			return -ZYNQMP_DMA_ERROR_DESC_RANGE;
		}
	//	Xil_DCacheFlushRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*n);

		dma[stream].state |= ZYNQMP_DMA_STATE_DESC_BUILT;
		dma[stream].defined_desc = n;
		dma[stream].num_frames = n_frames;
		dev_stat->dma_last_build_desc[stream] = msg;
	}

	DBGLEVEL(MSG_NORMAL, "Built %d Descriptors stream=%d, addr_space=%d, access_flags=%08X\n", dma[stream].defined_desc, stream, dma[stream].desc_addr_space, dev_stat->access_flags[dma[stream].desc_addr_space]); 
	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__flush_dcache_area(dev_stat->dma_priv[stream].desc_virt_base, dma[stream].num_desc*sizeof(AXIDMADesc));

	*num_desc = dma[stream].defined_desc;

	dev_stat->last_checked[stream] = -1;
	dev_stat->num_good[stream] = 0;
	dev_stat->num_completed[stream] = 0;
	dev_stat->next_time_frame[stream] = 0;
	dma[stream].readout_error_flags = 0;
	dma[stream].readout_error_data = 0;
	return -ZYNQMP_DMA_ERROR_OK;
}

int zynqmp_dma_stop(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *haltedP)
{
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream_mask = param->num;
	int stream;
	u_int32_t sr, halted_mask = 0;

	for (stream=0; stream < ZYNQMP_DMA_STREAM_NUM; stream++)
	{
		if (dma[stream].base_addr != 0 && (stream_mask & 1 << stream))
		{
			if (dma[stream].is_tx)
				ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, 1 << 16); // Force Run stop to 0
			else
				ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, 1 << 16); // Force Run stop to 0
		}
	}
	/* Now check that they have all stopped */
	for (stream=0; stream < ZYNQMP_DMA_STREAM_NUM; stream++)
	{
		if (dma[stream].base_addr != 0 && (stream_mask & 1 << stream))
		{
			if (dma[stream].is_tx)
			{
				if (!readl_poll_timeout_atomic(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4, sr, sr & XAXIDMA_HALTED_MASK, 0, 1000000))
					halted_mask |= 1 << stream;
			}
			else
			{
				if (!readl_poll_timeout_atomic(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, sr, sr & XAXIDMA_HALTED_MASK, 0, 1000000))
					halted_mask |= 1 << stream;
			}
		}
	}
	*haltedP = halted_mask;
	return -ZYNQMP_DMA_ERROR_OK;
}


int zynqmp_dma_start(DevStatics *dev_stat, ZynqMPPb *param )
{
	u_int32_t first_desc, num_desc;
	u_int64_t desc_phys;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream_mask = param->num;
	ZYNQMP_DMA_MsgStart msg;
	AXIDMADesc *desc;
	u_int32_t cr;
	int stream;
	int i;
	u_int64_t byte_offset, remaining, chunk_bytes;
	int addr_space, seg_num;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgStart)))
		return -EFAULT;

	for (stream=0; stream < ZYNQMP_DMA_STREAM_NUM; stream++)
	{
		if (dma[stream].base_addr != 0 && (stream_mask & 1 << stream))
		{

			if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
			{
				printk(KERN_WARNING "[ERROR] Stream %d has no DMA descriptors\n", stream);
				return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
			}
			if (msg.options & ZYNQMP_DMA_START_FRAME_NUM)
			{
				desc = dev_stat->dma_priv[stream].desc_virt_base;
				for (i=0; i<dma[stream].defined_desc; i++)
				{
					if ((desc->control & XAXIDMA_BD_CTRL_TXSOF_MASK) && desc->frame_num == msg.first_desc)
						break;
					desc++;
				}
				first_desc = i;
				if (msg.num_desc > 0)
				{
					num_desc = 0;
					while (i<dma[stream].defined_desc)
					{
						if ((desc->control & XAXIDMA_BD_CTRL_TXEOF_MASK) && desc->frame_num == msg.first_desc+msg.num_desc-1)
						{
							num_desc = 1+i-first_desc;
							break;
						}
						i++;
						desc++;
					}
				}
				else
				{
					num_desc = dma[stream].defined_desc-first_desc;
				}
				if (first_desc >= dma[stream].defined_desc || num_desc == 0)
				{
					printk(KERN_WARNING "[ERROR] Start: Cannot find descriptors to resend First Frame=%d and num frames=%d out of descriptorrange 0..%d, num_frames=%d\n", msg.first_desc, msg.num_desc, dma[stream].defined_desc-1, dma[stream].num_frames);
					return -ZYNQMP_DMA_ERROR_DESC_RANGE;
				}
			}
			else
			{
				first_desc = msg.first_desc;
				if (msg.num_desc == 0)
				{
					num_desc = dma[stream].defined_desc-first_desc;
				}
				else 
					num_desc = msg.num_desc;

				if (first_desc >= dma[stream].defined_desc || first_desc+num_desc > dma[stream].defined_desc)
				{
					printk(KERN_WARNING "[ERROR] Start: First Descriptor=%d and num=%d out of range 0..%d\n", first_desc, num_desc, dma[stream].defined_desc-1);
					return -ZYNQMP_DMA_ERROR_DESC_RANGE;
				}
			}
			desc = dev_stat->dma_priv[stream].desc_virt_base+first_desc;
			desc_phys = dma[stream].desc_dma+first_desc*sizeof(AXIDMADesc);
			DBGLEVEL(MSG_NORMAL, "Starting DMA Stream %d, first=%d num=%d , all=%d descriptors\n", stream, first_desc, num_desc, dma[stream].defined_desc);
			DBGLEVEL(MSG_NORMAL, ".... First Desc phys address=0x%010llX, virt address=%px\n", desc_phys, desc);
			DBGLEVEL(MSG_NORMAL, ".... Transfer start=%010llX, size=%010llX, end=%010llX\n", dma[stream].transfer_start, dma[stream].transfer_size, dma[stream].transfer_size+dma[stream].transfer_start-1);
			DBGLEVEL(MSG_NORMAL, ".... First desc start=%010llX, Control=%08X, Status=%08X\n", desc->phys_addr, desc->control, desc->status);

			if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
			{
				DBGLEVEL(MSG_NORMAL, ".... Flushing descritors start=%px, Size=%08lX\n", dev_stat->dma_priv[stream].desc_virt_base, dma[stream].num_desc*sizeof(AXIDMADesc));
				__flush_dcache_area(dev_stat->dma_priv[stream].desc_virt_base, dma[stream].num_desc*sizeof(AXIDMADesc));
			}
			byte_offset = dma[stream].transfer_start;
			remaining = dma[stream].transfer_size;
			while (remaining > 0)
			{
				char * va;
				va = (char *) zynqmp_dma_buffer_ptr(dev_stat, stream, byte_offset, &remaining, &chunk_bytes, &addr_space, NULL, &seg_num);
				if (va == NULL)
					break;

				if (dev_stat->access_flags[addr_space] & ACCESS_CACHEFLUSH)
				{
					if (dma[stream].is_tx)
					{
						DBGLEVEL(MSG_NORMAL, ".... Flushing data seg=%d, start=%px, Size=%08llX, buffer Va=%px\n", seg_num, va, chunk_bytes, dev_stat->dma_priv[stream].buff_virt_base[seg_num]);
						__flush_dcache_area(va, chunk_bytes);
					}
					else
					{
						DBGLEVEL(MSG_NORMAL, ".... Invalidating data seg=%d, start=%px, Size=%08llX, buffer Va=%px\n", seg_num, va, chunk_bytes, dev_stat->dma_priv[stream].buff_virt_base[seg_num]);
						__inval_dcache_area(va, chunk_bytes);
					}
				}
				byte_offset += chunk_bytes;
			}
			dev_stat->last_checked[stream] = -1;
			dev_stat->num_good[stream] = 0;
			dev_stat->num_completed[stream] = 0;
			dev_stat->next_time_frame[stream] = 0;
			if (dma[stream].is_tx)
			{
				ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, 1 << 16); // Force Run stop to 0, though this should be done before chaning the descriptors.
				for (i=0;i<100; i++)
				{
					if (ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4) & XAXIDMA_HALTED_MASK)
						break;
				}
				// Xil_DCacheFlushRange((u_int32_t) (dma[stream].transfer_start),  dma[stream].transfer_size);
				ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CDESC_OFFSET)/4, desc_phys);
				if (msg.options & ZYNQMP_DMA_START_CIRCULAR)
					ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RUNSTOP_MASK | XAXIDMA_CR_CYCLIC_MASK | 1 << 16);
				else
					ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RUNSTOP_MASK | 1 << 16);

				if (msg.options & ZYNQMP_DMA_START_CIRCULAR)	/* Deliberately off end of desc list so should not stop */
					ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys+(num_desc)*sizeof(AXIDMADesc));
				else
					ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys+(num_desc-1)*sizeof(AXIDMADesc));
			}
			else
			{
				// Xil_DCacheInvalidateRange((u_int32_t) (dma[stream].transfer_start),  dma[stream].transfer_size);
				ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, 1 << 16); // Force Run stop to 0, though this should be done before chaning the descriptors.
				for (i=0;i<100; i++)
				{
					if (ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4) & XAXIDMA_HALTED_MASK)
						break;
				}
				cr = XAXIDMA_CR_RUNSTOP_MASK | 1 << 16;
#if 0
		/* By using TAILDESC, we should not get a wrap round error, instead to firmware will drop events. */
					/* Could add Start msg option to enable IRS so driver does not need to know */
						cr |= XAXIDMA_IRQ_IOC_MASK;

					if (stream == XSP3M_DMA_STREAM_BNUM_HIST_LIST)
						cr |= XAXIDMA_IRQ_ERROR_MASK;	// If wrap round, clear error and contiue.
#endif
				ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4, desc_phys);
				ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, cr);
				if (msg.options & ZYNQMP_DMA_START_CIRCULAR)
					ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys+(num_desc)*sizeof(AXIDMADesc));	// Deliberate off end of list so cycles round
				else
					ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys+(num_desc-1)*sizeof(AXIDMADesc));
			}
			dma[stream].readout_cur_desc = 0;
			dma[stream].readout_error_flags = 0;
			dma[stream].readout_error_data = 0;
		}
	}
	return -ZYNQMP_DMA_ERROR_OK;
}

int zynqmp_dma_resend(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i;
	AXIDMADesc *desc, *first_desc;
	u_int64_t first_phys, last_phys;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	ZYNQMP_DMA_MsgResend msg;
	int num_desc=0;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgResend)))
		return -EFAULT;
	if (stream < 0)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA resend  command requires 1 and only 1 DMA stream at a time, not 0x%X\n", param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	DBGLEVEL(MSG_NORMAL, "Re-send Stream=%d, first=%d, num=%d\n", stream, msg.first_desc, msg.num_desc);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}
	if (msg.first_desc >= dma[stream].defined_desc || msg.first_desc+msg.num_desc > dma[stream].defined_desc)
	{
		printk(KERN_WARNING "[ERROR] Resend: First Descriptor=%d and num=%d out of range 0..%d\n", msg.first_desc, msg.num_desc, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}
	first_desc = NULL;
	first_phys = 0;
	if (msg.options & ZYNQMP_DMA_RESEND_FRAME_NUM)
	{
		desc=dev_stat->dma_priv[stream].desc_virt_base;
		for (i=0; i<dma[stream].defined_desc; i++)
		{
			if ((desc->control & XAXIDMA_BD_CTRL_TXSOF_MASK) && desc->frame_num == msg.first_desc)
			{
				first_desc = desc;
				first_phys = dma[stream].desc_dma+i*sizeof(AXIDMADesc);
			}
			if ((desc->control & XAXIDMA_BD_CTRL_TXEOF_MASK) && desc->frame_num == msg.first_desc+msg.num_desc-1)
			{
				num_desc = desc-first_desc;
				num_desc++;
				last_phys = dma[stream].desc_dma+i*sizeof(AXIDMADesc);
				break;
			}
			desc++;
		}
		if (first_desc == NULL || num_desc == 0)
		{
			printk(KERN_WARNING "[ERROR] Resend: Cannot find descriptors to resend First Frame=%d and num frames=%d out of descriptorrange 0..%d, num_frames=%d\n", msg.first_desc, msg.num_desc, dma[stream].defined_desc-1, dma[stream].num_frames);
			return -ZYNQMP_DMA_ERROR_DESC_RANGE;

		}
	}
	else
	{
		first_desc = dev_stat->dma_priv[stream].desc_virt_base+msg.first_desc;
		if (!(first_desc->control & XAXIDMA_BD_CTRL_TXSOF_MASK) || !(first_desc[msg.num_desc-1].control & XAXIDMA_BD_CTRL_TXEOF_MASK))
		{
			printk(KERN_WARNING "[ERROR] Resend: First Descriptor=%d and num=%d Descriptor should start and end Frame\n", msg.first_desc, msg.num_desc);
			return -ZYNQMP_DMA_ERROR_DESC_RANGE;
		}
		num_desc = msg.num_desc;
		first_phys = dma[stream].desc_dma+msg.first_desc*sizeof(AXIDMADesc);
		last_phys = first_phys+(msg.num_desc-1)*sizeof(AXIDMADesc);
	}

//	Xil_DCacheInvalidateRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*dma[stream].defined_desc);

	desc = first_desc; 
	for (i=0; i<num_desc; i++)
	{
		desc->status = 0;
		desc->app[3] = 0;
		desc++;
	}
	dma_wmb();
	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__flush_dcache_area(first_desc, num_desc*sizeof(AXIDMADesc));
	if (debug >= MSG_VERBOSE)
	{
		u_int64_t desc_dma;
		desc = first_desc;
		desc_dma = first_phys;
		for (i=0; i<num_desc && i < 10; i++)
		{
			printk(KERN_WARNING "Resend desc/frame. %ld, PAddr=%010llX, Next=%010llX, DMA addr=%010llX, num_bytes=0x%08X=%d, ctrl=%08X, status=%08X\n", i+(first_desc-desc), desc_dma, desc->phys_next, desc->phys_addr, desc->control & 0x7fffff, desc->control & 0x7fffff, desc->control, desc->status);
			desc++;
			desc_dma+=sizeof(AXIDMADesc);
		}
	}
	desc = first_desc;

	dev_stat->last_checked[stream] = -1;
	dev_stat->num_good[stream] = 0;
	dev_stat->num_completed[stream] = 0;
	dev_stat->next_time_frame[stream] = 0;
	if (dma[stream].is_tx)
	{
//		Xil_DCacheFlushRange((u_int32_t) (dma[stream].transfer_start),  dma[stream].transfer_size); // Assume if wanted this we have flushed it.
				// If it is from the other CPU we want to invalidate this. Hope the other has flushed it

		ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, 0 | 1 << 16); // Stop DMA
		while (!(ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4) & XAXIDMA_HALTED_MASK ));
		DBGLEVEL(MSG_VERBOSE, "Finished halting DMA, status=%08X\n", ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4) );
		ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CDESC_OFFSET)/4, first_phys );
		if (msg.options & ZYNQMP_DMA_START_CIRCULAR) // Not clear how/whether resend would be used with circular TX as it should just keep on running.
			ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RUNSTOP_MASK | XAXIDMA_CR_CYCLIC_MASK | 1 << 16);
		else
			ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RUNSTOP_MASK | 1 << 16);

		ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, last_phys);
	}
	else
	{
//		Xil_DCacheInvalidateRange((u_int32_t) (dma[stream].data_start),  dma[stream].data_size);
		ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, 0 | 1 << 16); // Stop DMA
		while (!(ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4) & XAXIDMA_HALTED_MASK ));
		ZYNQMP_IOWrite64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4, first_phys);
		ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RUNSTOP_MASK | 1 << 16);
		ZYNQMP_IOWrite32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, last_phys);
	}
	return -ZYNQMP_DMA_ERROR_OK;
}

int zynqmp_dma_read_status(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *statusP)
{
	u_int32_t stat=0xFFFFFFFF;
	int stream;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream_mask = param->num;

	for (stream=1; stream < ZYNQMP_DMA_STREAM_NUM; stream++)
	{
		if (dma[stream].base_addr != 0 && (stream_mask & 1 << stream))
		{
			if (stream == 0)
				return -ZYNQMP_DMA_ERROR_BAD_STREAM;
			if (dma[stream].is_tx)
			{
				stat = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4);
			}
			else
			{
				stat = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
			}
			dev_stat->status_block.status[stream] = stat;
			DBGLEVEL(MSG_NORMAL,"Stream %d, Status=0x%08X\n", stream, stat);
		}
	}
	if (statusP != NULL)
		*statusP=stat;
	return 0;
}

int zynqmp_dma_print_desc(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i;
	AXIDMADesc *desc;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	ZYNQMP_DMA_MsgPrintDesc msg;
	u_int64_t desc_phys, cur_desc;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgPrintDesc)))
		return -EFAULT;
	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM )
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA print desc command requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	printk(KERN_WARNING "[INFO] Print Desc Stream=%d, first=%d, num=%d\n", stream, msg.first_desc, msg.num_desc);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Print Desc: Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}
	if (msg.first_desc >= dma[stream].defined_desc || msg.first_desc+msg.num_desc > dma[stream].defined_desc)
	{
		printk(KERN_WARNING "[ERROR] Print Desc: First Descriptor=%d and num=%d out of range 0..%d\n", msg.first_desc, msg.num_desc, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}
//	Xil_DCacheInvalidateRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*dma[stream].defined_desc);
	desc = dev_stat->dma_priv[stream].desc_virt_base+msg.first_desc;
	desc_phys = dma[stream].desc_dma+msg.first_desc*sizeof(AXIDMADesc);

	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area(desc, msg.num_desc*sizeof(AXIDMADesc));

	for (i=0; i<msg.num_desc; i++)
	{
		printk(KERN_WARNING "Desc=%d, Addr=%010llX, Nxt=%010llX, DMA addr=%010llX, num_bytes=0x%08X=%d, ctrl=%08X, status=%08X\n",
				i+msg.first_desc, desc_phys, desc->phys_next, desc->phys_addr, desc->control & 0x7FFFFF, desc->control & 0x7FFFFF, desc->control, desc->status );
		desc++;
		desc_phys += sizeof(AXIDMADesc);
	}
	if (dma[stream].is_tx)
		cur_desc = ZYNQMP_IORead64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CDESC_OFFSET)/4);
	else
		cur_desc = ZYNQMP_IORead64(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4);
	printk(KERN_WARNING "Current Desc phys addr=%010llX\n", cur_desc);

	return -ZYNQMP_DMA_ERROR_OK;
}

int zynqmp_dma_check_rx_desc(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *num_complete)
{
	int i;
	AXIDMADesc *desc;
	int num_desc;
	int checks=0;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	ZYNQMP_DMA_MsgCheckDesc msg;
	u_int32_t status;
	u_int64_t desc_phys;
	int error_desc=-1;
	u_int64_t total_bytes=0;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgCheckDesc)))
		return -EFAULT;

	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM )
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA check desc command requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	DBGLEVEL(MSG_NORMAL, "Check Desc Stream=%d, first=%d, num=%d, options=%04X\n", stream, msg.first_desc, msg.num_desc, msg.options);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Check Desc: Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}
	if (msg.first_desc >= dma[stream].defined_desc || msg.first_desc+msg.num_desc > dma[stream].defined_desc)
	{
		printk(KERN_WARNING "[ERROR] Check Desc: First Descriptor=%d and num=%d out of range 0..%d\n", msg.first_desc, msg.num_desc, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}
	if (msg.options == 0)
	{
		switch (stream)
		{
		case ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM:
			checks = ZYNQMP_DMA_MSG_CHECK_10GRX | ZYNQMP_DMA_MSG_CHECK_LENGTH | ZYNQMP_DMA_MSG_CHECK_FRAME_PER_DESC;
			break;


		case NGZMP_DMA_STREAM_BNUM_SCOPE0:
		case NGZMP_DMA_STREAM_BNUM_SCOPE1:
		case NGZMP_DMA_STREAM_BNUM_SCOPE2:
			checks = ZYNQMP_DMA_MSG_CHECK_1_FRAME;
			break;

		default:
			checks = 0;
			break;
		}
	}
	else
		checks = msg.options;

//	Xil_DCacheInvalidateRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*dma[stream].defined_desc);
	desc = dev_stat->dma_priv[stream].desc_virt_base+msg.first_desc;
	num_desc = msg.num_desc;
	if (msg.num_desc == 0)
		num_desc = dma[stream].defined_desc-msg.first_desc;

	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area(desc, num_desc*sizeof(AXIDMADesc));

	for (i=0; i<num_desc; i++)
	{
		status = desc->status;
		if (!(status & XAXIDMA_BD_STS_COMPLETE_MASK))
			break;
		total_bytes += desc->status & 0x7FFFFF;
		if (error_desc == -1)
		{
			if (status & XAXIDMA_BD_STS_ALL_ERR_MASK)
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Error detected by AXI DMA, Status=%08X\n", i+msg.first_desc, status);
				error_desc = i;
			}

			if ((checks & ZYNQMP_DMA_MSG_CHECK_FRAME_PER_DESC) && (status & (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK))
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Missing SOF or EOF Status=%08X\n", i+msg.first_desc, status);
				error_desc = i;
			}
			if (checks & ZYNQMP_DMA_MSG_CHECK_1_FRAME)
			{
				if (dma[stream].defined_desc == 1)
				{
					// Special case of single desc
					if ((status & (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXEOF_MASK))
					{
						printk(KERN_WARNING "Check_DESC: Desc %d: Missing SOF or EOF Status=%08X\n", i+msg.first_desc, status);
						error_desc = i;
					}
				}
				else
				{
					if (i+msg.first_desc == 0)
					{
						if ( (status & (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXSOF_MASK ))
						{
							printk(KERN_WARNING "Check_DESC: Desc %d: Missing SOF or extra EOF on first descriptor Status=%08X\n", i+msg.first_desc, status);
							error_desc = i;
						}
					}
					else if (i+msg.first_desc == dma[stream].defined_desc-1)
					{
						if ( (status & (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXEOF_MASK ))
						{
							printk(KERN_WARNING "Check_DESC: Desc %d: Extra SOF or missing EOF on last descriptor Status=%08X\n", i+msg.first_desc, status);
							error_desc = i;
						}
					}
					else
					{
						if ( (status & (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXEOF_MASK)) != 0)
						{
							printk(KERN_WARNING "Check_DESC: Desc %d: Extra SOF or EOF on intermediate descriptor Status=%08X\n", i+msg.first_desc, status);
							error_desc = i;
						}
					}
				}
			}
			if ((checks & ZYNQMP_DMA_MSG_CHECK_LENGTH) && (desc->control & 0x7FFFFF) != (status & 0x7FFFFF))
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Length mismatch. Requested=%08X, received=%08X\n", i+msg.first_desc, desc->control, status);
				error_desc = i;
			}
			if ((checks & ZYNQMP_DMA_MSG_CHECK_TIMEFRAME) && desc->app[0] != i)
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Time Frame mismatch, found time frame %d\n", i+msg.first_desc, desc->app[0]);
				error_desc = i;
			}
		}
		desc++;
	}
	desc--;
	if (param->ptr3 != NULL)
	{
		if (put_user(i, (int32_t*)(param->ptr3))) // Return the total number of frames with the completed bit set 
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3));
			return -EFAULT;
		}
		if (stream == ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM)
		{
			if (put_user(total_bytes, (int32_t*)&(param->ptr3[1]))) // Return total bytes for 10G to DRAM stream
			{
				printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+1));
				return -EFAULT;
			}
		}
		else
		{
			if (i > 0)
			{
				if (put_user(desc->app[0], (int32_t*)&(param->ptr3[1]))) // Return app[0], which is often the frame number
				{
					printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+1));
					return -EFAULT;
				}
			}
			else
			{
				// No Valid descriptors to read frames from
				int32_t dummy_fnum=0;
				if (put_user(dummy_fnum, (int32_t*)&(param->ptr3[1]))) // Return app[0], which is often the frame number
				{
					printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+1));
					return -EFAULT;
				}
			}
		}
	}

	if (error_desc != -1)
	{
		desc = dev_stat->dma_priv[stream].desc_virt_base+msg.first_desc+error_desc;
		desc_phys = dma[stream].desc_dma + sizeof(AXIDMADesc)*(error_desc+msg.first_desc);
		printk(KERN_WARNING "Desc=%d, Addr=%010llX, Nxt=%010llX, DMA addr=%010llX, num_bytes=0x%08X=%d, ctrl=%08X, status=%08X\n",
				error_desc+msg.first_desc, desc_phys, desc->phys_next, desc->phys_addr, desc->control & 0x7FFFFF, desc->control & 0x7FFFFF, desc->control, desc->status );
		DBGLEVEL(MSG_NORMAL, ".... Check desc found %d good descriptors out of %d\n", error_desc, i);
		if (param->ptr3 != NULL)
		{
			if (put_user(desc->status, (param->ptr3+2)))
			{
				printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+2));
				return -EFAULT;
			}
		}
		if (num_complete != NULL)
			*num_complete = error_desc;
		return 0;
	}
	else
	{
		if (param->ptr3 != NULL)
		{
			if (i > 0)
			{
				if (put_user(desc->status, (int32_t*)(param->ptr3+2)))
				{
					printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+2));
					return -EFAULT;
				}
			}
			else
			{
				// No Valid descriptors to read status from
				int32_t dummy_status=0;
				if (put_user(dummy_status, (int32_t*)(param->ptr3+2)))
				{
					printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+2));
					return -EFAULT;
				}
			}
		}
	}
	DBGLEVEL(MSG_NORMAL, ".... Check desc found %d all good descriptors\n", i);
	if (num_complete != NULL)
		*num_complete = i;
	return 0;
}


int zynqmp_dma_check_stream(DevStatics *dev_stat, int stream)
{
	if (stream < NGZMP_DMA_STREAM_BNUM_PLAYBACK0 || stream >= ZYNQMP_DMA_STREAM_NUM)
		return -1;
	if (dev_stat->status_block.stream_def[stream].base_addr == 0)
		return -1;	

	return 0;
} 

int zynqmp_dma_get_desc_status(DevStatics *dev_stat, ZynqMPPb *param)
{
	AXIDMADesc *desc;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	ZYNQMP_DMA_MsgGetDescStatus msg;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgGetDescStatus)))
		return -EFAULT;

	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM )
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA get desc status requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	DBGLEVEL(MSG_NORMAL, "Get Desc Status Stream=%d, first=%d\n", stream, msg.first_desc);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Get Desc Status: Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}
	if (msg.first_desc >= dma[stream].defined_desc)
	{
		printk(KERN_WARNING "[ERROR] Get Desc Status: First Descriptor=%d out of range 0..%d\n", msg.first_desc, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}

//	Xil_DCacheInvalidateRange((u_int32_t)dma[stream].desc, sizeof(AXIDMADesc)*dma[stream].defined_desc);
	desc = dev_stat->dma_priv[stream].desc_virt_base+msg.first_desc;

	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area(desc, sizeof(AXIDMADesc));

	if (param->ptr3 != NULL)
	{
		if (put_user(desc->status, (int32_t*)(param->ptr3))) // Return the status word from teh specified descriptor. 
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3));
			return -EFAULT;
		}
	}
	return 0;
}
#if 0

int zynqmp_dma_reuse(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i, last, desc_num;
	AXIDMADesc *desc;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	ZYNQMP_DMA_MsgReuse msg;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgReuse)))
		return -EFAULT;

	if (stream < 0 || stream >= ZYNQMP_DMA_STREAM_NUM )
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA reuse command requires stream in range 0 to %d, not %dX\n", ZYNQMP_DMA_STREAM_NUM-1, param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if (stream != ZYNQMP_DMA_STREAM_BNUM_SCALERS && stream != XSP3M_DMA_STREAM_BNUM_HIST_FRAMES)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA reuse command should be used only on Scalars or Hist Frames streams  not %d\n", stream);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	DBGLEVEL(MSG_VERBOSE, "Re-send Stream=%d, first=%d, num=%d\n", stream, msg.first_desc, msg.num_desc);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}
	if (msg.first_desc >= dma[stream].defined_desc)	// Allow to wrap round.
	{
		printk(KERN_WARNING "[ERROR] Resend: First Descriptor=%d and num=%d out of range 0..%d\n", msg.first_desc, msg.num_desc, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}


	last = (msg.first_desc+msg.num_desc-1) % dma[stream].defined_desc;
	desc = dev_stat->dma_priv[stream].desc_virt_base+last;

	if ((desc->status & (XAXIDMA_BD_STS_COMPLETE_MASK | XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_COMPLETE_MASK | XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXEOF_MASK))
	{
		printk(KERN_WARNING "[ERROR] Resend: Last descriptor=%d does not have correct completed status=0x%08X\n", last, desc->status);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}
	
	desc = dev_stat->dma_priv[stream].desc_virt_base+msg.first_desc;
	desc_num = msg.first_desc;
	for (i=0; i<msg.num_desc; i++)
	{
		desc->status = 0;
		desc->app[3] = 0;
		desc++;
		desc_num++;
		if (desc_num == dma[stream].defined_desc)
		{
			desc = dev_stat->dma_priv[stream].desc_virt_base;
			desc_num = 0;
		}
	}
	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__flush_dcache_area(dev_stat->dma_priv[stream].desc_virt_base, dma[stream].num_desc*sizeof(AXIDMADesc));
	return -ZYNQMP_DMA_ERROR_OK;
}


int zynqmp_dma_check_rx_desc_circular(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i;
	AXIDMADesc *desc;
	int num_desc;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = param->num;
	u_int32_t dma_status;
	u_int32_t status;
	int error_desc=-1;
	u_int64_t cur_desc_phys;
	int desc_num, cur_desc;
	u_int64_t num_good, num_completed, next_time_frame, cur_time_frame=0;
	u_int32_t l, h;
	int error_code=0;

	if (stream < 0)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA check desc command requires 1 and only 1 DMA stream at a time, not 0x%X\n", param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	DBGLEVEL(MSG_NORMAL, "Check Desc Circular Stream=%d\n", stream);
	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Check Desc: Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}
	 
	if (dma[stream].is_tx)
	{
		dma_status = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4);
		cur_desc_phys = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_TX_OFFSET+XAXIDMA_CDESC_OFFSET)/4);
	}
	else
	{
		dma_status = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
		cur_desc_phys = ZYNQMP_IORead32(dev_stat->dma_priv[stream].virt_base+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4);
	}
	dev_stat->status_block.status[stream] = dma_status;

	cur_desc = cur_desc_phys - dma[stream].desc_dma;

/*	Possible cases:
	For easy backwards compatible there is the return code which gets back to libxspress3 via the num_ops field and 3 off 32 bits words which get back via the extra1,2,3 fields
	But loses info so now looking to return 3 off 64 bit numbers plus various 32 bit status words.	

	Not done any

	All OK
		Number of correct frames
		Number of compete frames (same)
		Time fame of last frame (num_done-1)
	
	Stopped with over run -- quiet possible

	Some other error.

*/

	desc_num = dev_stat->last_checked[stream]+1;
	num_desc = dma[stream].defined_desc;
	num_good = dev_stat->num_good[stream];
	num_completed = dev_stat->num_completed[stream];
	next_time_frame = dev_stat->next_time_frame[stream];

	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area(dev_stat->dma_priv[stream].desc_virt_base, dma[stream].num_desc*sizeof(AXIDMADesc));
	for (i=0; i<num_desc; i++)
	{
		desc_num %= num_desc;
		if (desc_num == cur_desc)
			break;

		desc = dev_stat->dma_priv[stream].desc_virt_base+desc_num;
		status = desc->status;
		if (!(status & XAXIDMA_BD_STS_COMPLETE_MASK))
			break;

		cur_time_frame = *(u_int64_t *)(desc->app);
		if (error_desc == -1)
		{
			if (status & XAXIDMA_BD_STS_ALL_ERR_MASK)
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Error detected by AXI DMA, Status=%08X\n", desc_num, status);
				error_desc = desc_num;
				error_code = -ZYNQMP_DMA_ERROR_FIRMWARE_DETECTED;
			}

			if ((status & (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK))
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Missing SOF or EOF Status=%08X\n", desc_num, status);
				error_desc = desc_num;
				error_code = -ZYNQMP_DMA_ERROR_DESC_SOF_EOF;
			}

			if ((desc->control & 0x7FFFFF) != (status & 0x7FFFFF))
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Length mismatch. Requested=%08X, received=%08X\n", desc_num, desc->control, status);
				error_desc = desc_num;
				error_code = -ZYNQMP_DMA_ERROR_DESC_LENGTH;
			}
			if (cur_time_frame != next_time_frame)
			{
				printk(KERN_WARNING "Check_DESC: Desc %d: Time Frame mismatch, found time frame %ju\n", desc_num, cur_time_frame);
				error_desc = desc_num;
				error_code = ZYNQMP_DMA_ERROR_DESC_TIME_FRAME;
			}
		}
		if (error_desc == -1 && num_good == num_completed)
		num_good++;
		num_completed++;
		next_time_frame++;
		desc_num++;
	}
	desc_num--;
	if (desc_num == -1)
		desc_num = num_desc-1;
	dev_stat->last_checked[stream] = desc_num;
	dev_stat->num_good[stream] = num_good;
	dev_stat->num_completed[stream] = num_completed;
	dev_stat->next_time_frame[stream] = next_time_frame;
	desc = dev_stat->dma_priv[stream].desc_virt_base+desc_num;

	if (param->ptr3 != NULL)
	{
		DBGLEVEL(MSG_NORMAL, "zynqmp_dma_check_rx_desc_circular: returning num_good=%d, num_completed=%d, cur_time_frame=%d\n", (int)num_good, (int)num_completed, (int)desc->app[0]);
		l = num_good & 0xFFFFFFFF;
		h = num_good >> 32;
		if (put_user(l, (param->ptr3)) || put_user(h, (param->ptr3+1))) // Return the Number of good frames with the completed bit set and time frame matching from the start
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (param->ptr3));
			return -EFAULT;
		}
		l = num_completed & 0xFFFFFFFF;
		h = num_completed >> 32;
		if (put_user(l, (param->ptr3+2)) || put_user(h, (param->ptr3+3))) // Return the total number of frames with the completed bit set 
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (param->ptr3+2));
			return -EFAULT;
		}
		l = desc->app[0];
		h = desc->app[1];
		if (put_user(l, (param->ptr3+4)) || put_user(h, (param->ptr3+5))) // Return the Time frame from the last descriptor processed
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (param->ptr3+4));
			return -EFAULT;
		}
		if (dma_status & XAXIDMA_ERR_ALL_MASK)
		{
			/* Cur desc is now the descriptor causing the error, so look it up to get more info */
			error_code = -ZYNQMP_DMA_ERROR_FIRMWARE_DETECTED;
			l = 0;
			h = 0;
			i = cur_desc;
			desc = dev_stat->dma_priv[stream].desc_virt_base+i;
		}
		else if (error_desc != -1)
		{
			/* Error detector in descriptors */
			desc = dev_stat->dma_priv[stream].desc_virt_base+error_desc;
			l = desc->app[0];
			h = desc->app[1];
			i = error_desc;
		}
		else if (num_completed == 0)
		{
			i = 0;
			desc = dev_stat->dma_priv[stream].desc_virt_base+i;
			l = 0;
			h = 0;
		}
		else
		{
			i = desc_num;
			desc = dev_stat->dma_priv[stream].desc_virt_base+i;
			l = desc->app[0];
			h = desc->app[1];
		}			
		if (put_user(error_code, (int32_t*)(param->ptr3+6)) || put_user(i, (int32_t*)(param->ptr3+7)) || put_user(dma_status, (param->ptr3+8)) ||	 // Return Error Code and Last or Failing Descriptor, Status register from Core, status from descriptor
			put_user(desc->status, (param->ptr3+9)) || put_user(l, (param->ptr3+10)) || put_user(h, (param->ptr3+11)) )								// Return the Descriptor status, and time frame when defined
		{
			printk(KERN_WARNING "%s: Cannot return data to Address %px\n", driver_name, (int32_t *) (param->ptr3+6));
			return -EFAULT;
		}
	}
	return 12;	// The number of 32 bit words to return as this is a read type command
}

int zynqmp_dma_read_tf_status(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i, desc_num;
	AXIDMADesc *desc;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = zynqmp_get_stream_num(param->num);
	u_int32_t first = param->addr_space;
	u_int32_t num = param->and_mask;
	Xsp3TFStatus tf_status, *ptr;

	if (stream < 0)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA read TF Status command requires 1 and only 1 DMA stream at a time, not 0x%X\n", param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if (stream != ZYNQMP_DMA_STREAM_BNUM_SCALERS && stream != XSP3M_DMA_STREAM_BNUM_HIST_FRAMES)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA read TF Status command should be used only on Scalars or Hist Frames streams  not %d\n", stream);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}

	if (first >= dma[stream].defined_desc)	// Allow to wrap round.
	{
		printk(KERN_WARNING "[ERROR] read TF Status: First Descriptor=%u out of range 0..%d\n", first, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}
	desc = dev_stat->dma_priv[stream].desc_virt_base+first;

	ptr = (Xsp3TFStatus *)param->ptr3;
	desc_num= first;
	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area(desc, num*sizeof(AXIDMADesc));
	for (i=0; i<num; i++)
	{
		if (desc->status & XAXIDMA_BD_STS_COMPLETE_MASK)
			tf_status.state = 1;
		else
			tf_status.state = 0;
	
		tf_status.time_frame = (int64_t)(desc->app[0]) | (int64_t)(desc->app[1]) << 32;	//!< Extended time frame in circular buffer mode
		tf_status.markers   = desc->app[2];

		if (copy_to_user((void __user *)ptr, (void *) &tf_status, sizeof(Xsp3TFStatus)))
		{
			return -EFAULT;
		}
		ptr++;
		desc++;
		desc_num++;
		if (desc_num == dma[stream].defined_desc)
		{
			desc = dev_stat->dma_priv[stream].desc_virt_base;
			desc_num = 0;
		}
	}
	return num;
}


int zynqmp_dma_read_markers(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i, desc_num;
	AXIDMADesc *desc;
	DMAStream *dma = dev_stat->status_block.stream_def;
	int stream = zynqmp_get_stream_num(param->num);
	u_int32_t first = param->addr_space;
	u_int32_t num = param->and_mask;
	u_int32_t *ptr;

	if (stream < 0)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA read TF Markers command requires 1 and only 1 DMA stream at a time, not 0x%X\n", param->num);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if (stream != ZYNQMP_DMA_STREAM_BNUM_SCALERS && stream != XSP3M_DMA_STREAM_BNUM_HIST_FRAMES)
	{
		DBGLEVEL(MSG_ERROR, "[ERROR] DMA read TF Markers command should be used only on Scalars or Hist Frames streams  not %d\n", stream);
		return -ZYNQMP_DMA_ERROR_BAD_STREAM;
	}

	if ((dma[stream].state & ZYNQMP_DMA_STATE_DESC_BUILT) == 0)
	{
		printk(KERN_WARNING "[ERROR] Stream %d has no DMA descriptors\n", stream);
		return -ZYNQMP_DMA_ERROR_UNCONF_DESC;
	}

	if (first >= dma[stream].defined_desc)	// Allow to wrap round.
	{
		printk(KERN_WARNING "[ERROR] read TF Markers: First Descriptor=%u out of range 0..%d\n", first, dma[stream].defined_desc-1);
		return -ZYNQMP_DMA_ERROR_DESC_RANGE;
	}
	desc = dev_stat->dma_priv[stream].desc_virt_base+first;

	ptr = (u_int32_t *)param->ptr3;
	desc_num= first;
	if (!access_ok( ptr, sizeof(int32_t)*num))
		return -EFAULT;

	if (dev_stat->access_flags[dma[stream].desc_addr_space] & ACCESS_CACHEFLUSH)
		__inval_dcache_area(desc, num*sizeof(AXIDMADesc));
	for (i=0; i<num; i++)
	{
		__put_user(desc->app[2], ptr);

		ptr++;
		desc++;
		desc_num++;
		if (desc_num == dma[stream].defined_desc)
		{
			desc = dev_stat->dma_priv[stream].desc_virt_base;
			desc_num = 0;
		}
	}
	return num;
}
#endif

u_int32_t * zynqmp_dma_buffer_ptr(DevStatics *dev_stat, int stream, u_int64_t byte_offset, u_int64_t *remaining_bytes, u_int64_t *chunk_bytes, int *addr_space, u_int64_t *dma_base, int *seg_num)
{
	u_int64_t max_bytes;
	int i;

	if (stream <0 || stream >= ZYNQMP_DMA_STREAM_NUM)
		return NULL;
	
	for (i=0; i<ZYNQMP_NUM_DMA_SEGMENTS; i++)
	{
		if (byte_offset < dev_stat->status_block.stream_def[stream].buff_seg[i].size)
		{
			max_bytes = dev_stat->status_block.stream_def[stream].buff_seg[i].size-byte_offset;
			if (max_bytes > *remaining_bytes)
			{
				*chunk_bytes = *remaining_bytes;
				*remaining_bytes = 0;
			}
			else
			{
				*chunk_bytes = max_bytes;
				*remaining_bytes -= max_bytes;
			}
			*addr_space = dev_stat->status_block.stream_def[stream].buff_seg[i].addr_space;
			if (dma_base != NULL)
				*dma_base= dev_stat->status_block.stream_def[stream].buff_seg[i].dma_base+byte_offset;
			if (seg_num != NULL)
				*seg_num = i;
			return (u_int32_t *)((u_int8_t *)(dev_stat->dma_priv[stream].buff_virt_base[i])+byte_offset);
		}
		else
		{
			byte_offset -= dev_stat->status_block.stream_def[stream].buff_seg[i].size;
		}
	}
	return NULL;
}
