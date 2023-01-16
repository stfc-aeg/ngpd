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

irqreturn_t xsp4_dma_irq_handler(int irq, void *data)
{
	DevStatics *dev_stat=data;
//	DevStatics *dev_stat=xsp4_dev_statics;
	int stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
	DMAStream *dma = dev_stat->status_block.stream_def;
	u_int32_t status;		
	u_int32_t control;

	if (dma[stream].is_tx)
	{
		status = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_TX_OFFSET+XAXIDMA_SR_OFFSET)/4);
	}
	else
	{
		status = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
		if (status & (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK))
		{
			control = XSP4_IORead32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			control &= ~XAXIDMA_IRQ_IOC_MASK & ~XAXIDMA_IRQ_ERROR_MASK;
			XSP4_IOWrite32(dev_stat->dma_virt_base[stream]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
			if (dev_stat->readout_mode & Xsp3mRd_SendHistList)
			{
				queue_work(xsp4_wq, &dev_stat->hist_list_send);
			}
			else
				tasklet_schedule(&dev_stat->hist_tasklet);
		}
	}

	return IRQ_HANDLED;
}

void hist_tasklet(unsigned long mnum)
{
	DevStatics *dev_stat = xsp4_dev_statics;
	DMAStream *dma = dev_stat->status_block.stream_def;
	AXIDMADesc *desc;
	int dnum;
	u_int32_t status, control;
	u_int32_t *hist, *ptr;
	int num;
	int chan;
	int buff_len;
	int offset;
	u_int32_t desc_phys;
	u_int32_t eng_status;



	eng_status = XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
	XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, eng_status);


	DBGLEVEL(MSG_NORMAL, "Hist tasklet starting from desc num %d, Cur desc=%08X, Phys base=%08X, cur desc num=%d\n", dev_stat->hist_cur_desc, 
			XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4), dev_stat->status_block.stream_def[XSP3M_DMA_STREAM_BNUM_HIST_LIST].desc, 
			(XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4)-dev_stat->status_block.stream_def[XSP3M_DMA_STREAM_BNUM_HIST_LIST].desc)/sizeof(AXIDMADesc)	);
	if (dev_stat->status_block.stream_def[XSP3M_DMA_STREAM_BNUM_SOFT_HIST0].data_size > 0)
	{
		dnum = dev_stat->hist_cur_desc;
		do 
		{
			desc = dev_stat->desc_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST];
			desc += dnum;
			status = desc->status;
			DBGLEVEL(MSG_VERBOSE, "Hist tasklet: Processing descriptor %d, status=0x%08X\n", dnum, status);
			if (status & XAXIDMA_BD_STS_COMPLETE_MASK)
			{
				if ((status & XAXIDMA_BD_STS_ALL_ERR_MASK))
				{
					dev_stat->hist_errors++;
					if (status & XAXIDMA_BD_STS_SLV_ERR_MASK)
					{
						DBGLEVEL(MSG_SPARSE, "Hist Tasklet restarting DMA after Slave error, status=0x%08X\n", status);
						XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, XAXIDMA_CR_RESET_MASK);		// Writing to RX side as all DMAs have an RX side, but some do not have a TX side at the moment.
						while ( XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4) & XAXIDMA_CR_RESET_MASK);
						desc_phys = dma[XSP3M_DMA_STREAM_BNUM_HIST_LIST].desc;
						if (dnum != dev_stat->status_block.stream_def[XSP3M_DMA_STREAM_BNUM_HIST_LIST].defined_desc-1)
							desc_phys += sizeof(AXIDMADesc)*(dnum+1);
						XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CDESC_OFFSET)/4, desc_phys);
						
						control = XAXIDMA_CR_RUNSTOP_MASK | 1 << 16;
						XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);

					}
				}
				else
				{
					if (status & XAXIDMA_BD_STS_RXSOF_MASK)
					{
						ptr = desc->buff_virt_addr;
						num = (status & 0x7FFFFF)>>2;
						num --;
						chan = *ptr++;
						DBGLEVEL(MSG_VERBOSE, "Hist tasklet: Chan %d, First offset = 0x%08X\n", chan, *ptr);
						if (chan >= 0 && chan < XSP3M_MAX_CHAN && (hist=dev_stat->dma_buff_virt_base[XSP3M_DMA_STREAM_BNUM_SOFT_HIST0+chan] ) != NULL && 
							(buff_len=dev_stat->status_block.stream_def[XSP3M_DMA_STREAM_BNUM_SOFT_HIST0+chan].data_size/4) > 0)
						{
							DBGLEVEL(MSG_VERBOSE, "Hist tasklet: Chan %d, Buffer VAddr=%px, buffer length=%d words\n", chan, hist, buff_len);
							while (num-- > 0)
							{
								offset = *ptr & 0x0FFFFFFF;
								if (offset < buff_len)
									hist[offset]++;
								ptr++;
							}
						}
					}
				}
				desc->status = 0;
				desc_phys = dma[XSP3M_DMA_STREAM_BNUM_HIST_LIST].desc;
				wmb();
				XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_TDESC_OFFSET)/4, desc_phys+(dnum)*sizeof(AXIDMADesc));

				dnum = (dnum+1) % dma[XSP3M_DMA_STREAM_BNUM_HIST_LIST].defined_desc;
			}
			else
			{
				break;
			}
		} while (1);
		/* We do not clear the IRQ again here. Often at the end we get an extra IRQ, start the tasklet and find no more frames.
		This is slightly inefficient, but I think safe .. if we cleared the IRQ here after the last look at the completed frames we could miss the last block of addresses
		*/
		dev_stat->hist_cur_desc = dnum;
		control = XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
		control |= XAXIDMA_IRQ_IOC_MASK;
		XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
	}
}

