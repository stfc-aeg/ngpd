/*
	10//02/2021	Ported code to run as Linux/ARM64 device driver, 
	Idea is to make the driver mor eeasily reusable.
	This portion is specific to the Neutron Gamma Pulse Discriminator

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


#include "ngzmp.h"
#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"

#define NGZMP_DMA_SCOPE0_BASEADDR 		0x1000
#define NGZMP_DMA_SCOPE1_BASEADDR 		0x2000
#define NGZMP_DMA_SCOPE2_BASEADDR 		0x3000
#define NGZMP_DMA_10G_TX_RX_BASEADDR    0x4000
#define NGZMP_DMA_HIST_READ_BASEADDR   	0x5000

#define NGZMP_DMA_PB0_BASEADDR 			0x6000

#define NGZMP_MAX_NUM_CHAN 8
#include "xaxidma_hw.h"

int zynmp_dma_config_memory(DevStatics *dev_stat, int layout);
int ngzmp_dma_get_desc_status(DevStatics *dev_stat, ZynqMPPb *param);


void zynqmp_dma_init(DevStatics *dev_stat)
{
	int i;
	DMAStream *dma = dev_stat->status_block.stream_def;

	memset((void *)dma, 0, sizeof(DMAStream)*ZYNQMP_DMA_STREAM_NUM);
	dma[0].state = 0;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].base_addr = NGZMP_DMA_PB0_BASEADDR;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].is_tx = 1;

	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].base_addr = NGZMP_DMA_SCOPE0_BASEADDR;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].is_tx = 0;

//	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK1].base_addr = NGZMP_DMA_SCOPE_AND_PB1_BASEADDR;
//	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK1].is_tx = 1;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].base_addr = NGZMP_DMA_SCOPE1_BASEADDR;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].is_tx = 0;

	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].base_addr = NGZMP_DMA_SCOPE2_BASEADDR;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].is_tx = 0;

	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].base_addr = NGZMP_DMA_10G_TX_RX_BASEADDR;
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].is_tx = 1;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].base_addr = NGZMP_DMA_10G_TX_RX_BASEADDR;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].is_tx = 0;

	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].base_addr = NGZMP_DMA_HIST_READ_BASEADDR;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].is_tx = 1;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].base_addr = NGZMP_DMA_HIST_READ_BASEADDR;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].is_tx = 0;

	for (i=0;i< ZYNQMP_DMA_STREAM_NUM; i++)
		dev_stat->dma_priv[i].virt_base = (u_int32_t*)(dev_stat->virt_base[ZYNQMP_ADDR_SPACE_DMA]+dma[i].base_addr/sizeof(u_int32_t));

	for (i=0;i<ZYNQMP_DMA_STREAM_NUM; i++)
	{
		dev_stat->last_checked[i] = -1;
		dev_stat->num_good[i] = 0;
		dev_stat->num_completed[i] = 0;
		dev_stat->next_time_frame[i] = 0;
		dma[i].readout_error_flags = 0;
		dma[i].readout_error_data = 0;
	}

}

/* The ARM64 can see all of it PS DRAM but mapped in two regions.
	2 GBytes at 0, with the first 512 MBytes used by Linux.
	2 GBytes at 0x8_0000_0000.

	2 GByte of PL RAM shared between histogram and Playback.

	Histogram size.
	Biggest:
	4 (32 bit)* 2k*2k * 2 (tail and time) * 4 (ngp +spare) * 8 (numChan) = 256*4Meg = 1 GByte ... Seems too much.
	Suggest limit to 512 MBytes, So can do 2kx2k with just TailSum, or smaller with both.
	
	Playback size 3/2 GBytes. 
	Scope size: depends on other requireemnts. Currently allocate 3 GBytes

	Don't need hist read buffer.
	May want event list buffer.

	UDP	TX .. usually used to send scope mode so just need descriptors, but what about Sending hist read?
	UDP	RX .. usually used to receive playback so just need descriptors. Also not cannot DMA into this buffer directly


In typical 10 g readback of 2/6 of memory aim for 600 frames of size 2/3 total memory / 600 
But allow sufficient descriptors to access all 4 Gbytes = 3/2*600 = 900 + spares.


*/
/* Limit the maximum block size to fit the hardware and be 512 bit = 64 bytes aligned */
#define MAX_BLOCK_BYTES 0x7FFFC0

int zynqmp_dma_config_mem(DevStatics *dev_stat, ZynqMPPb *param)
{
	return zynqmp_dma_config_memory(dev_stat, param->num);
}

int zynqmp_dma_config_memory(DevStatics *dev_stat, int layout)
{
	/* Only case so far is Playback plus scope plus 0 scalers */
	u_int64_t phys_base;
	u_int64_t total_memory_high;
	u_int64_t total_memory_low;
	u_int64_t memory_hist_read_and_desc;
	u_int64_t total_memory_pl;
	u_int64_t pb_size, scope_size;
	u_int64_t hist_read_size;
	u_int64_t desc_phys;
	u_int64_t dma_base;
	u_int64_t buffer_total;
	int pb_desc, scope_desc, max10g_tx_desc, max10g_rx_desc, hist_test_desc, hist_read_desc;
//	int num_tf = 0;
	DMAStream *dma = dev_stat->status_block.stream_def;
//	int num_tf = 65536;
	int i, j;

	total_memory_high = ZYNQMP_PS_DRAM_PHYS_SIZE_HIGH;
	total_memory_low  = ZYNQMP_PS_DRAM_PHYS_SIZE_LOW;
	total_memory_pl   = ZYNQMP_PL_DRAM_PHYS_SIZE;
	
	pb_size = total_memory_pl-NGZMP_TOTAL_HIST_SIZE_BYTES;
	/* to provide for extra descriptors to allow for segmenation calculate descriptor per segemnt, allow extras, then scale to total */
	pb_desc = pb_size/MAX_BLOCK_BYTES;
	pb_desc = (pb_desc+7)/8;		// Number per segment
	pb_desc += 2;		// Allow for an extra at beginning and end
	pb_desc *= 8;

	pb_size -= sizeof (AXIDMADesc)*pb_desc;

	phys_base = ZYNQMP_PS_DRAM_PHYS_BASE_LOW;

	scope_size = (1024*1024*1024);		// Total 3 GBytes, 1 GByte per stream.
	scope_size &= 0xFFFFFFC0;		/* 3 regions of this size make scope total */

	max10g_tx_desc = 1000;
	max10g_rx_desc = 2*(pb_size/(128*8000)+2);

	scope_desc = scope_size/(MAX_BLOCK_BYTES) +1;	// Allow 1 extra for remainder
	scope_desc += 2;		// Allow 1 padding incase of late alignment issues and spread on LOW/HIGH boundary.
	scope_desc *= 3;		// In 1 stream mode, scope stream 0 needs descriptors for all of memory 
	hist_test_desc = scope_desc;
	memory_hist_read_and_desc = total_memory_high+total_memory_low-3*scope_size-sizeof(AXIDMADesc)*(max10g_tx_desc+max10g_rx_desc+3*scope_desc+hist_test_desc);
	printk(KERN_WARNING "Memory from hist read buffer and descriptors=%lld\n", memory_hist_read_and_desc);

	hist_read_desc = memory_hist_read_and_desc/(MAX_BLOCK_BYTES + sizeof(AXIDMADesc)) + 2;

	hist_read_size = memory_hist_read_and_desc-hist_read_desc*sizeof(AXIDMADesc);

	for (i=0; i<ZYNQMP_DMA_STREAM_NUM; i++)
	{
		dma[i].data_size = 0;
		dma[i].state = 0;
		dev_stat->dma_priv[i].desc_virt_base = NULL;
		dma[i].num_desc  =0;
		dma[i].defined_desc = 0;
		dma[i].desc_addr_space = ZYNQMP_ADDR_SPACE_PS_LOW; // All in Low address space
		for (j=0;j<ZYNQMP_NUM_DMA_SEGMENTS; j++)
		{
			memset(&dma[i].buff_seg[i], 0, sizeof(struct ZYNQMP_DMA_BuffSegment));
			dma[i].buff_seg[j].addr_space = ZYNQMP_ADDR_SPACE_PS_LOW;
		}
	} 

	DBGLEVEL(MSG_SPARSE, "Num descriptors pb (x2) = %d, scope (x3)=%d, histTest=%d, Histread=%d, 10G TX=%d, 10G RX=%d size=%zu\n", pb_desc, scope_desc, hist_test_desc, hist_read_desc, max10g_tx_desc, max10g_rx_desc, sizeof(AXIDMADesc));
	dma[0].buff_seg[0].cpu_base = ZYNQMP_PS_DRAM_PHYS_BASE_LOW;
	dma[0].buff_seg[0].dma_base = ZYNQMP_PS_DRAM_PHYS_BASE_LOW;
	dma[0].buff_seg[0].size = ZYNQMP_PS_DRAM_PHYS_SIZE_LOW;
	dma[0].buff_seg[1].cpu_base = ZYNQMP_PS_DRAM_PHYS_BASE_HIGH;
	dma[0].buff_seg[1].dma_base = ZYNQMP_PS_DRAM_PHYS_BASE_HIGH;
	dma[0].buff_seg[1].size = ZYNQMP_PS_DRAM_PHYS_SIZE_HIGH;

	/* For the playback DMA, the DMA see the memory at address 0, but the CPU sees it at ZYNQMP_PL_DRAM_PHYS_BASE */
//	dma_base = NGZMP_CHAN_HIST_SIZE_BYTES*NGZMP_CHANS_PER_CARD;
	dma_base = NGZMP_CHAN_HIST_SIZE_BYTES*NGZMP_CHANS_PER_CARD;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].desc_dma = dma_base;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].desc_cpu = dma_base+ZYNQMP_PL_DRAM_PHYS_BASE;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].num_desc = pb_desc;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].max_block_bytes = MAX_BLOCK_BYTES;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].frame_rule = AllOneFrame;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].desc_addr_space = ZYNQMP_ADDR_SPACE_PL_RAM; // All in Low address space
	dma_base += dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].num_desc*sizeof(AXIDMADesc);
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[0].dma_base = dma_base;
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[0].cpu_base = dma_base+ZYNQMP_PL_DRAM_PHYS_BASE;
//	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[0].size = total_memory_pl/8-(pb_desc*sizeof(AXIDMADesc)+NGZMP_CHAN_HIST_SIZE_BYTES);
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[0].size = total_memory_pl-(pb_desc*sizeof(AXIDMADesc)+NGZMP_CHAN_HIST_SIZE_BYTES*NGZMP_CHANS_PER_CARD);
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[0].addr_space =ZYNQMP_ADDR_SPACE_PL_RAM;

/*	for (i=1; i<8; i++)
	{
		dma_base = i*(total_memory_pl/8)+NGZMP_CHAN_HIST_SIZE_BYTES;
		dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[i].dma_base = dma_base;
		dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[i].cpu_base = dma_base+ZYNQMP_PL_DRAM_PHYS_BASE;
		dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[i].size = total_memory_pl/8-NGZMP_CHAN_HIST_SIZE_BYTES;
		dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].buff_seg[i].addr_space =ZYNQMP_ADDR_SPACE_PL_RAM;
	}
	*/
	dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;

	/* For all other DMA the CPU and DMA see the memory at the same address. */
	desc_phys = ZYNQMP_PS_DRAM_PHYS_BASE_LOW;

	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].desc_cpu = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].desc_dma = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].num_desc = scope_desc;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].max_block_bytes = MAX_BLOCK_BYTES;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].frame_rule = AllOneFrame;
	desc_phys += dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].num_desc*sizeof(AXIDMADesc);
	
	phys_base = (1024*1024*1024)-64*1024;		// From Vitis top 64k appears not to be available
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[0].dma_base = phys_base;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[0].cpu_base = phys_base;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[0].size = 1024*1024*1024;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[0].addr_space =ZYNQMP_ADDR_SPACE_PS_LOW;
	phys_base = ZYNQMP_PS_DRAM_PHYS_BASE_HIGH;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[1].dma_base = phys_base;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[1].cpu_base = phys_base;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[1].size = ZYNQMP_PS_DRAM_PHYS_SIZE_HIGH;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[1].addr_space = ZYNQMP_ADDR_SPACE_PS_HIGH;

	/* Give all 3 scope mode buffers a view of the same data and use ZYNQMP_DMA_MsgBuildDesc.addr to access the correct addresses */
	for (i=0;i<ZYNQMP_NUM_DMA_SEGMENTS; i++)
	{
		dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].buff_seg[i] = dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[i];
		dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].buff_seg[i] = dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[i];
	}

	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].desc_cpu = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].desc_dma = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].num_desc = scope_desc;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].max_block_bytes = MAX_BLOCK_BYTES;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].frame_rule = AllOneFrame;
	desc_phys += dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].num_desc*sizeof(AXIDMADesc);

	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].desc_cpu = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].desc_dma = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].num_desc = scope_desc;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].max_block_bytes = MAX_BLOCK_BYTES;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].frame_rule = AllOneFrame;
	desc_phys += dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].num_desc*sizeof(AXIDMADesc);

	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].desc_cpu = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].desc_dma = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].num_desc = hist_test_desc;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].max_block_bytes = MAX_BLOCK_BYTES;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].frame_rule = AllOneFrame;
	desc_phys += dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].num_desc*sizeof(AXIDMADesc);

	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].desc_cpu = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].desc_dma = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].num_desc = hist_read_desc;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].max_block_bytes = MAX_BLOCK_BYTES;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].frame_rule = AllOneFrame;
	desc_phys += dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].num_desc*sizeof(AXIDMADesc);

	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].desc_cpu = desc_phys;
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].desc_dma = desc_phys;
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].num_desc = max10g_tx_desc;
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].max_block_bytes = (((3*pb_size/600)| 0x1F) + 1) & MAX_BLOCK_BYTES;
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].frame_rule = FramePerDesc;
	desc_phys += dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].num_desc*sizeof(AXIDMADesc);

	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].desc_cpu = desc_phys;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].desc_dma = desc_phys;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].num_desc = max10g_rx_desc;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].max_block_bytes = 8000*128;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].state = ZYNQMP_DMA_STATE_DESC_CONF;
	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].frame_rule = FramePerDesc;
	desc_phys += dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].num_desc*sizeof(AXIDMADesc);

	DBGLEVEL(MSG_SPARSE, "Built total descriptors %d=%08X\n", (int)((desc_phys-ZYNQMP_PS_DRAM_PHYS_BASE_LOW)/sizeof(AXIDMADesc)), (int)((desc_phys-ZYNQMP_PS_DRAM_PHYS_BASE_LOW)/sizeof(AXIDMADesc)));


	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].buff_seg[0].dma_base = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].buff_seg[0].cpu_base = desc_phys;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].buff_seg[0].size  = hist_read_size;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].buff_seg[0].addr_space  = ZYNQMP_ADDR_SPACE_PS_LOW;
	dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;
	desc_phys += dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].buff_seg[0].size;
	
	DBGLEVEL(MSG_SPARSE,"Setting Hist read buffer to use data area start=%010llX, to=%08llX bytes\n", dma[NGZMP_DMA_STREAM_BNUM_HIST_READ].buff_seg[0].cpu_base, desc_phys);

	dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE1].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;
	dma[NGZMP_DMA_STREAM_BNUM_SCOPE2].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;

	DBGLEVEL(MSG_SPARSE,"Setting scope mode to use data area start=%010llX, End=%08llX bytes\n", dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[0].cpu_base, 
		dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[2].cpu_base+dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[2].size);

/* Hist test buffer, reuses scope mode */
	for (i=0;i<ZYNQMP_NUM_DMA_SEGMENTS; i++)
		dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].buff_seg[i] = dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[i];
	dma[NGZMP_DMA_STREAM_BNUM_HIST_TEST].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;

/* Default for 10 G TX is to send scope mode buffer */
	for (i=0;i<ZYNQMP_NUM_DMA_SEGMENTS; i++)
		dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].buff_seg[i] = dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].buff_seg[i];
	dma[ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;

/* Default for 10 G RX is to receive playback  buffer  .. need to come up with an answer for this as cannot see the memory (I think) */
//	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].data_start = dma[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_start;
//	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].data_size  = 2*pb_size;
//	dma[ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM].state |= ZYNQMP_DMA_STATE_BUFFER_CONF;

	for (i=0;i<ZYNQMP_DMA_STREAM_NUM; i++) 
	{
		dev_stat->dma_priv[i].desc_virt_base = (AXIDMADesc *)(((u_int8_t *)(dev_stat->virt_base[dma[i].desc_addr_space]))+(dma[i].desc_cpu-dev_stat->phys_base[dma[i].desc_addr_space]));
		buffer_total = 0;
		for (j=0; j<ZYNQMP_NUM_DMA_SEGMENTS; j++)
		{
			int addr_space;
			addr_space = dma[i].buff_seg[j].addr_space;
			buffer_total += dma[i].buff_seg[j].size;
			dev_stat->dma_priv[i].buff_virt_base[j] = (u_int32_t *)(((u_int8_t *)(dev_stat->virt_base[addr_space]))+(dma[i].buff_seg[j].cpu_base-dev_stat->phys_base[addr_space]));
		}
		dma[i].data_size = buffer_total;


		DBGLEVEL(MSG_NORMAL,"Allocating descriptors for stream %d at CPU Paddr=%010llX, Vaddr=%px, First buffer phys=%010llX, VAddr=%px, total=%010llX\n", i, dma[i].desc_cpu, dev_stat->dma_priv[i].desc_virt_base, 
			dma[i].buff_seg[0].cpu_base, dev_stat->dma_priv[i].buff_virt_base[0], dma[i].data_size);
	}
	dev_stat->status_block.mem_layout = layout;
	return 0;
}

int zynqmp_dma_print_scope(DevStatics *dev_stat, ZynqMPPb *param)
{
	int i;
	u_int16_t *sp[3];
	u_int64_t remaining[3], chunk_bytes[3], byte_offset;
	int rc = 0;
	u_int64_t num_bytes;
	DMAStream *dma = dev_stat->status_block.stream_def;
	ZYNQMP_DMA_MsgPrint msg;
	int stream, addr_space;
	int t;

	if (copy_from_user((void *) &msg, (void __user *)param->ptr, sizeof(ZYNQMP_DMA_MsgPrint)))
		return -EFAULT;

	if (!(dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].state & ZYNQMP_DMA_STATE_BUFFER_CONF) )
	{
		printk(KERN_WARNING "%s: [ERROR] Display Scope command on Scope 0, Buffer address and size not defined\n", driver_name);
		return -ZYNQMP_DMA_ERROR_UNCONF_BUFF;
	}

	for (stream=0; stream<3; stream++)
	{
		byte_offset = msg.offset&0xFFFFFFF8;// Force 8 byte aligment so start at stream 0, 4, 8.

		num_bytes= 1000;
		if (msg.num_bytes != 0)
		{
			if (msg.num_bytes < dma[NGZMP_DMA_STREAM_BNUM_SCOPE0+stream].data_size)
				num_bytes = msg.num_bytes;
			else
				num_bytes = dma[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size;
		}
		
		if ( byte_offset + num_bytes > dma[NGZMP_DMA_STREAM_BNUM_SCOPE0+stream].data_size)
		{
			printk(KERN_WARNING "%s [ERROR] Requested region (%010llX, %08llX) does not lie with scope mode mode\n", driver_name, byte_offset, num_bytes);
			return -ZYNQMP_DMA_ERROR_OUTSIDE_VA;
		}
		remaining[stream] = num_bytes;
	}

	rmb();
	chunk_bytes[0] = chunk_bytes[1] = chunk_bytes[2] = 0;
	t = 0;
	while (remaining[0] > 0 && remaining[1] >0 &&  remaining[2] > 0)
	{
		for (stream=0; stream<3; stream++)
		{
			if (chunk_bytes[stream] == 0 && 
				(sp[stream] = (u_int16_t *)zynqmp_dma_buffer_ptr(dev_stat, NGZMP_DMA_STREAM_BNUM_SCOPE0+stream, byte_offset, remaining+stream, chunk_bytes+stream, &addr_space, NULL, NULL)) == NULL)
				return -ZYNQMP_DMA_ERROR_OUTSIDE_VA;
		}
		printk(KERN_WARNING "t=%d:", t);
		if (msg.options & ZYNQMP_DMA_MSG_PRINT_DEC)
		{
			for (stream=0; stream<3; stream++)
			{
				for (i=0; i<4; i++)
					printk(KERN_WARNING " %4d", sp[stream][i] );
				sp[stream] += 4;
			}
		}
		else
		{
			for (stream=0; stream<3; stream++)
			{
				for (i=0; i<4; i++)
					printk(KERN_WARNING " %04X", sp[stream][i] );
				sp[stream] += 4;
			}
		}
		printk(KERN_WARNING "\n");
		t ++;
		byte_offset += 4*sizeof(u_int16_t);
		for (stream=0; stream<3; stream++)
		{
			chunk_bytes[stream] -= 4*sizeof(u_int16_t);
		}
	}

	rmb();
	return rc;

}
