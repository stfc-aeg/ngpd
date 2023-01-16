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

#include "xspress3_dma_protocol.h"
#include "xspress4_protocol.h"
#include "xsp4_driver.h"
#include "driver_static.h"
#include "dma_control.h"


#include "xaxidma_hw.h"

irqreturn_t xsp4_frame_irq(int irq, void *data, int stream, struct work_struct *work)
{
	DevStatics *dev_stat=data;
	DMAStream *dma = dev_stat->status_block.stream_def;
	u_int32_t status;		
	u_int32_t control;

//	printk(KERN_WARNING "xspress4 device driver, irq=%d, dev_stat=%px\n", irq, dev_stat);
	if (dma[stream].is_tx)
	{
		status = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4);
	}
	else
	{
		status = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
		if (status & XAXIDMA_IRQ_IOC_MASK)
		{
			control = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			control &= ~XAXIDMA_IRQ_IOC_MASK;
			XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
			XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, XAXIDMA_IRQ_IOC_MASK);
			queue_work(xsp4_wq, work);
		}
	}

	return IRQ_HANDLED;
}

irqreturn_t xsp4_scalar_dma_irq_handler(int irq, void *data)
{
	DevStatics *dev_stat = (DevStatics *)data;
	return xsp4_frame_irq(irq, data, XSP3_DMA_STREAM_BNUM_SCALERS, &dev_stat->scalar_send );
}
irqreturn_t xsp4_hist_frame_dma_irq_handler(int irq, void *data)
{
	DevStatics *dev_stat = (DevStatics *)data;
	return xsp4_frame_irq(irq, data, XSP3M_DMA_STREAM_BNUM_HIST_FRAMES, &dev_stat->hist_frame_send);
}

#define MAX_SEGS 30

void xsp4_frame_worker(DevStatics *dev_stat, int stream, struct socket *sock, struct mutex *sock_mutex)
{
	int cur_desc;
	AXIDMADesc *desc, *desc_start, *dp;
	int num_desc;
	u_int32_t status, control;
//	unsigned char *buff;
	int num_bytes;
//	u_int64_t next_time_frame, desc_time_frame;
	int sent;
	struct kvec vec[2*MAX_SEGS];
	struct kvec vec_copy[2*MAX_SEGS];
	struct msghdr msg;
	int retries=0;
	int n;
	int i;
	size_t total_bytes;
	u_int32_t desc_phys;

	if (sock == NULL)
	{
		up_read(&dev_stat->dma_sem);
		mutex_unlock(sock_mutex);
		return ; // Give up without re-enabling IRQ
	}
	cur_desc = dev_stat->readout_cur_desc[stream];
//	next_time_frame = dev_stat->next_time_frame[stream];
	desc = dev_stat->desc_virt_base[stream] +  cur_desc;
	num_desc = dev_stat->status_block.stream_def[stream].defined_desc;

	if (debug >=2 )
		printk(KERN_WARNING "%s : Entered xsp4_frame_worker, stream=%d, cur_desc=%d, num_desc=%d, desc=%px\n", driver_name, stream, cur_desc, num_desc, desc);
	while ((status = desc->status) & XAXIDMA_BD_STS_COMPLETE_MASK)
	{
		desc_start = desc;
		n = 0;
		while (((status = desc->status) & XAXIDMA_BD_STS_COMPLETE_MASK) && n < MAX_SEGS)
		{
			if (debug >=2)
				printk(KERN_WARNING "%s : Scanning descriptors: stream=%d, cur_desc=%d, num_desc=%d, n=%d, desc=%px\n", driver_name, stream, cur_desc, num_desc, n, desc);
#if 0
/* Whatever happens, send the packet with its status word dand descriptor time frame so we can unpick  errors at the x86_64 end, where we can at  least tell the user */

			if ((status & XAXIDMA_BD_STS_ALL_ERR_MASK))
			{
				dev_stat->readout_error_flags[stream] |= status & XAXIDMA_BD_STS_ALL_ERR_MASK;
				printk(KERN_ERR "%s : DMA on stream %d stopped at desc %d with error code %08X\n", driver_name, stream, cur_desc, status);
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->next_time_frame[stream] = next_time_frame;
				up_read(&dev_stat->dma_sem);
				mutex_unlock(sock_mutex);
				return;
			}
			if ((status & (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK))
			{
				dev_stat->readout_error_flags[stream] |= ~status & (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXSOF_MASK);
				printk(KERN_ERR "%s : DMA on stream %d found invalid SOF/EOF at desc=%d status=%08X\n", driver_name, stream, cur_desc, status);
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->next_time_frame[stream] = next_time_frame;
				up_read(&dev_stat->dma_sem);
				mutex_unlock(sock_mutex);
				return;
			}
			desc_time_frame = *(u_int64_t *)(desc->app);
			if (desc_time_frame != next_time_frame)
			{
				dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_DESC_TF_MASK;
				printk(KERN_ERR "%s : DMA on stream %d found time frame mismatch at desc=%u TF=%llu, expected=%llu, status=%08X\n", driver_name, stream, cur_desc, desc_time_frame, next_time_frame, status);
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->next_time_frame[stream] = next_time_frame;
				up_read(&dev_stat->dma_sem);
				mutex_unlock(sock_mutex);
				return;
			}
#endif
			dev_stat->last_time_frame[stream] = *(u_int64_t *)(desc->app);
			cur_desc++;
			if (cur_desc == dev_stat->readout_cur_desc[stream] || (cur_desc == num_desc && dev_stat->readout_cur_desc[stream] == 0))
			{
				dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_DESC_OVERRUN;
				printk(KERN_ERR "%s : DMA on stream %d Over run at desc=%d\n", driver_name, stream, cur_desc);
				desc->app[1] |= 0x80000000;
				/* mark over run by setting the MSbit */				
/*	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->next_time_frame[stream] = next_time_frame;
				return;
*/
				break; 
			}
//			next_time_frame++;
			desc++;
			n++;


			if (cur_desc == num_desc)
				break;
		}
		dp = desc_start;
		total_bytes = 0;
		if (debug >=3)
			printk(KERN_WARNING "%s : Building IOVec: stream=%d, n=%d, dp=%px\n", driver_name, stream, n, dp);
		for (i=0; i<n; i++)
		{
			vec[2*i].iov_base = (unsigned char *)(((u_int32_t *)dp)+7);
			vec[2*i].iov_len  = 4 * sizeof(u_int32_t);
			total_bytes += 4*sizeof(u_int32_t);
			vec[2*i+1].iov_base = (unsigned char *)dp->buff_virt_addr;
			vec[2*i+1].iov_len = num_bytes = dp->control & ~XAXIDMA_BD_CTRL_ALL_MASK;
			total_bytes += num_bytes;
			dp++;
			vec_copy[2*i]   = vec[2*i];
			vec_copy[2*i+1] = vec[2*i+1];
			if (debug >=3 )
				printk (KERN_WARNING "%s : vec[%d].iov_base=%px, vec[%d].iov_len=%d, vec[%d].iov_base=%px, vec[%d].iov_len=%d. First word=%08X\n", driver_name, 2*i, vec[2*i].iov_base, 2*i, vec[2*i].iov_len, 
									2*i+1, vec[2*i+1].iov_base, 2*i+1, vec[2*i+1].iov_len, *((u_int32_t *)vec[2*i].iov_base) );
		}
		memset(&msg,0,sizeof(msg));

#if 0
		sent = kernel_sendmsg(sock, &msg, vec, 2*n, total_bytes); /*send message */
/*		if (sent == -ENOSPC || sent == -EAGAIN)
		{
			retries++;
			if (retries > 60 )
			{
				printk(KERN_ERR "%s : Cannot send data after 60 retries, 30 seconds\n", driver_name);
			    return sent;
			}
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(HZ/2);
			continue;
		}
		*/
		if (sent < 0)
		{
		    printk(KERN_ERR "%s : kernel_sendmsg error = %d\n", driver_name, sent);
			dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_SOCKET;
			dev_stat->readout_error_data[stream] = sent;
			up_read(&dev_stat->dma_sem); 	
			mutex_unlock(sock_mutex);
		    return ;
		}
		else if (sent != total_bytes)
		{
			printk(KERN_ERR "Sent %d bytes out of %zu. vec=%px msg.msg_iter.iov=%px\n", sent, total_bytes, vec, msg.msg_iter.kvec);
			printk(KERN_ERR "Number of segments=%d, msg.msg_iter.nr_segs=%lu msg.msg_iter.count=%zu\n", 2*n, msg.msg_iter.nr_segs, msg.msg_iter.count);
			for (i=0;i<2*n; i++)
				printk(KERN_ERR "Orginal base=%px, len=%d  now base=%px, len=%d\n", vec_copy[i].iov_base, vec_copy[i].iov_len, vec[i].iov_base, vec[i].iov_len );
		}
#else
		/* Split the kernel_sendmsg int o tis 2 lines, so can retry the sendmsg easily */
		if (debug >= 3)
			printk(KERN_WARNING "%s : Calling iov_iter: stream=%d, n=%d, total_bytes=%zd\n", driver_name, stream, n, total_bytes);
		iov_iter_kvec(&msg.msg_iter, WRITE | ITER_KVEC, vec, 2*n, total_bytes);

		retries = 0;
		while (msg_data_left(&msg))
		{
			if (debug >= 3)
				printk(KERN_WARNING "%s : Calling sock_sendmsg: stream=%d, msg_data_left=%zd\n", driver_name, stream, msg_data_left(&msg));
			sent = sock_sendmsg(sock, &msg);
			if (unlikely(sent < 0))
			{
				if (sent == -ENOSPC || sent == -EAGAIN)
				{
					retries++;
					if (retries > 60 )
					{
						printk(KERN_ERR "%s : Cannot send data after 60 retries, 30 seconds\n", driver_name);
						up_read(&dev_stat->dma_sem); 	
						mutex_unlock(sock_mutex);
					    return ;
					}
					set_current_state(TASK_INTERRUPTIBLE);
					schedule_timeout(HZ/2);
				}
				else
				{
					printk(KERN_ERR "%s : Cannot send data error=%d\n", driver_name, sent);
					up_read(&dev_stat->dma_sem); 	
					mutex_unlock(sock_mutex);
				    return ;
				}
			}
		} 
#endif
		dp = desc_start;
		while (dp < desc)
		{
			dp->status = 0;
			dp->app[3] = 0;
			dp++;
		}
		if (cur_desc > 0)
		{
			desc_phys = dev_stat->status_block.stream_def[stream].desc;
			desc_phys += (cur_desc-1)*sizeof(AXIDMADesc);
			wmb();
			XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys);
		}
		if (cur_desc == num_desc)
		{
			cur_desc = 0;
			desc = dev_stat->desc_virt_base[stream];
		}
	 	dev_stat->readout_cur_desc[stream] = cur_desc;
//	 	dev_stat->next_time_frame[stream] = next_time_frame;
		/* After first pass, if the we have hit the end of the list, we now wrap round back to the beginning, with dev_stat->readout_cur_desc[stream] cur_desc, desc pointing back to the beginning
		   If there are more completed frame we find these and output in a 2nd call to xsp4_socket_send
		   If we have not hit the end of the list, dev_stat->readout_cur_desc[stream] cur_desc, desc point to the first ununsed DMA desc
		   If it has now been filled we have another go  at sending the rest
		   If not we are ready to break out of the outer while loop
		*/
		/* Think it is safe to clear the IRQ here as we will check again for any more completed descriptors */
		XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, XAXIDMA_IRQ_IOC_MASK);
		mb(); // Need this write finish before we check the descriptor
	}
	up_read(&dev_stat->dma_sem);
	mutex_unlock(sock_mutex);
	if (debug >= 2)
		printk(KERN_WARNING "%s : Re-enabling DMA-IRQ on stream %d\n", driver_name, stream);
	control = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
	control |= XAXIDMA_IRQ_IOC_MASK;
	XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
}

void xsp4_frame_worker_scalar(struct work_struct *work)

{
	DevStatics *dev_stat = (DevStatics *)container_of(work, DevStatics, scalar_send);
	down_read(&dev_stat->dma_sem); 	
	mutex_lock(&dev_stat->scalar_mutex);	// Hold this mutex so that socket cannot be released until we have tried to use it and discovered an error 
	xsp4_frame_worker(dev_stat, XSP3_DMA_STREAM_BNUM_SCALERS, dev_stat->scalar_transfer_sock, &dev_stat->scalar_mutex);
}

void xsp4_frame_worker_hist(struct work_struct *work)
{
	DevStatics *dev_stat = (DevStatics *)container_of(work, DevStatics, hist_frame_send);
	down_read(&dev_stat->dma_sem); 	
	mutex_lock(&dev_stat->hist_frame_mutex);	// Hold this mutex so that socekt cannot be released until we have tried to use it and discovered an error 
	xsp4_frame_worker(dev_stat, XSP3M_DMA_STREAM_BNUM_HIST_FRAMES, dev_stat->hist_frame_transfer_sock, &dev_stat->hist_frame_mutex);
}

void xsp4_hist_list_worker(struct work_struct *work)
{
	DevStatics *dev_stat=  (DevStatics *)container_of(work, DevStatics, hist_list_send); 
	int stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
	int cur_desc;
	AXIDMADesc *desc, *desc_start, *dp;
	int num_desc;
	u_int32_t status, control;
	int n;
	size_t total_bytes;
	u_int32_t desc_phys;

	down_read(&dev_stat->dma_sem); 	
	mutex_lock(&dev_stat->hist_list_mutex);
	if (dev_stat->hist_list_transfer_sock == NULL)
	{
		up_read(&dev_stat->dma_sem); 	
		mutex_unlock(&dev_stat->hist_list_mutex);
		return ; // Give up without re-enabling IRQ
	}
	cur_desc = dev_stat->readout_cur_desc[stream];
	desc = dev_stat->desc_virt_base[stream] +  cur_desc;
	num_desc = dev_stat->status_block.stream_def[stream].defined_desc;
	if (debug >= 2)
		printk(KERN_WARNING "%s : Entered xsp4_hist_list_worker, stream=%d, cur_desc=%d, num_desc=%d, desc=%px\n", driver_name, stream, cur_desc, num_desc, desc);

	while ((status = desc->status) & XAXIDMA_BD_STS_COMPLETE_MASK)
	{
		desc_start = desc;
		n = 0;
		while (((status = desc->status) & XAXIDMA_BD_STS_COMPLETE_MASK) && n < MAX_SEGS)
		{
			if (debug >= 2)
				printk(KERN_WARNING "%s : Scanning descriptors: stream=%d, cur_desc=%d, num_desc=%d, n=%d, desc=%px\n", driver_name, stream, cur_desc, num_desc, n, desc);

/* For hist list, the data has  work markers in the top 4bits, so even if we get lost, can probably still histogram OK
	Detect errors and record them here.
*/
			if ((status & XAXIDMA_BD_STS_ALL_ERR_MASK))
			{
				dev_stat->readout_error_flags[stream] |= status & XAXIDMA_BD_STS_ALL_ERR_MASK;
				printk(KERN_ERR "%s : DMA on stream %d stopped at desc %d with error code %08X\n", driver_name, stream, cur_desc, status);
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->readout_error_flags[stream] |= (status & XAXIDMA_BD_STS_ALL_ERR_MASK);
				up_read(&dev_stat->dma_sem); 	
				mutex_unlock(&dev_stat->hist_list_mutex);
				return;
			}
			if ((status & (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK)) != (XAXIDMA_BD_STS_RXSOF_MASK |XAXIDMA_BD_STS_RXEOF_MASK))
			{
				dev_stat->readout_error_flags[stream] |= ~status & (XAXIDMA_BD_STS_RXSOF_MASK | XAXIDMA_BD_STS_RXSOF_MASK);
				if (debug || !(dev_stat->readout_error_flags[stream] & XSP3_DMA_ERROR_DESC_SOFEOF))
					printk(KERN_ERR "%s : DMA on stream %d found invalid SOF/EOF at desc=%d status=%08X\n", driver_name, stream, cur_desc, status);
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_DESC_SOFEOF;
				dev_stat->readout_error_data[stream]  = cur_desc;
				/* Try to stagger on? */
			}
			if ((status & 0xFFFFFF) != desc->control)
			{
				if (debug || !(dev_stat->readout_error_flags[stream] & XSP3_DMA_ERROR_DESC_COUNT))
					printk(KERN_ERR "%s : DMA on stream %d found short packet at desc=%d control=0x%08X, status=%08X\n", driver_name, stream, cur_desc, desc->control, status);
				dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_DESC_COUNT;
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
				dev_stat->readout_error_data[stream]  = cur_desc;
				/* Try to stagger on? */
			}
			if (debug)
			{
				u_int16_t *p16=(u_int16_t *)desc->buff_virt_addr;
				int j;
				int nw = (desc->status & 0xFFFFFF)>>1;
				for (j=0; j<nw; j++)
				{
					if ((*p16 & 0xF010) == 0x9010)
						printk(KERN_NOTICE "xsp4 : EOF marker @ desc %d, word %d, count=%d = 0x%04X\n", cur_desc, j, dev_stat->eof_count++, *p16);
					p16++;
				}
			} 
			cur_desc++;
			if (cur_desc == dev_stat->readout_cur_desc[stream] || (cur_desc == num_desc && dev_stat->readout_cur_desc[stream] == 0))
			{
				if (debug || !(dev_stat->readout_error_flags[stream] & XSP3_DMA_ERROR_DESC_OVERRUN))
					printk(KERN_ERR "%s : DMA on stream %d Over run at desc=%d\n", driver_name, stream, cur_desc);
				dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_DESC_OVERRUN;
				dev_stat->readout_error_data[stream]  = cur_desc;
	 			dev_stat->readout_cur_desc[stream] = cur_desc;
			}
			desc++;
			n++;

			if (cur_desc == num_desc)
				break;
		}
		dp = desc_start;
		total_bytes = dp->control*n;
		dp = desc_start;
		if (xsp4_socket_send(dev_stat->hist_list_transfer_sock, (unsigned char *)dp->buff_virt_addr, total_bytes) < 0)
		{
			dev_stat->readout_error_flags[stream] |= XSP3_DMA_ERROR_SOCKET;
			up_read(&dev_stat->dma_sem); 	
			mutex_unlock(&dev_stat->hist_list_mutex);
			return;
		}

		while (dp < desc)
		{
			dp->status = 0;
			dp->app[3] = 0;
			dp++;
		}
		if (cur_desc > 0)
		{
			desc_phys = dev_stat->status_block.stream_def[XSP3M_DMA_STREAM_BNUM_HIST_LIST].desc;
			desc_phys += (cur_desc-1)*sizeof(AXIDMADesc);
			wmb();
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys);
		}

		if (cur_desc == num_desc)
		{
			cur_desc = 0;
			desc = dev_stat->desc_virt_base[stream];
		}
	 	dev_stat->readout_cur_desc[stream] = cur_desc;
//	 	dev_stat->next_time_frame[stream] = next_time_frame;
		/* After first pass, if the we have hit the end of the list, we now wrap round back to the beginning, with dev_stat->readout_cur_desc[stream] cur_desc, desc pointing back to the beginning
		   If there are more completed frame we find these and output in a 2nd call to xsp4_socket_send
		   If we have not hit the end of the list, dev_stat->readout_cur_desc[stream] cur_desc, desc point to the first ununsed DMA desc
		   If it has now been filled we have another go  at sending the rest
		   If not we are ready to break out of the outer while loop
		*/
		/* Think it is safe to clear the IRQ here as we will check again for any more completed descriptors */
		XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, XAXIDMA_IRQ_IOC_MASK);
		mb(); // Need this write finish before we check the descriptor

	}
	up_read(&dev_stat->dma_sem); 	
	mutex_unlock(&dev_stat->hist_list_mutex);
	if (debug >= 2)
		printk(KERN_WARNING "%s : Re-enabling DMA-IRQ on stream %d\n", driver_name, stream);
	control = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
	control |= XAXIDMA_IRQ_IOC_MASK;
	XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
}

int xsp4_socket_send(struct socket *sock, unsigned char * data, int len)
{
	int sent;
	struct kvec vec;
	struct msghdr msg;
	int retries=0;
	memset(&msg,0,sizeof(msg));

	if (sock == NULL)
		return -3;

	if (sock->state != SS_CONNECTED)
	{
	    printk(KERN_ERR "%s : socket not connected, state=%d\n", driver_name,sock->state);
	    return -1;
	}

	while (len > 0)
	{
		vec.iov_base=data;
		vec.iov_len=len;
		sent = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len); /*send message */
		if (sent == -ENOSPC || sent == -EAGAIN)
		{
			retries++;
			if (retries > 60 )
			{
				printk(KERN_ERR "%s : Cannot send data after 60 retries, 30 seconds\n", driver_name);
			    return sent;
			}
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(HZ/2);
			continue;
		}
		else if (sent < 0)
		{
		    printk(KERN_ERR "%s : kernel_sendmsg error = %d\n", driver_name, sent);
		    return sent;
		}
		data += sent;
		len -= sent;
	}

	return 0;
}
