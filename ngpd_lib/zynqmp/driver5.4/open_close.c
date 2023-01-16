/*
	22/9/2004	Started Linux Driver port

*/
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/types.h>   /* ulong and friends */ 
#include <linux/sched.h>   /* current, task_struct, other goodies */ 
#include <linux/fcntl.h>   /* O_NONBLOCK etc. */ 
#include <linux/errno.h>   /* return values */ 
#include <linux/ioport.h>  /* request_region() */ 
/* #include <linux/malloc.h>  kmalloc, kfree */ 
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>    /* char dev */ 
#include <linux/mm.h>
 
#include <asm/io.h>        /* inb() inw() outb() ... */ 
#include <asm/irq.h>       /* unreadable, but useful */ 
#include <asm/pgtable.h>       /* unreadable, but useful */ 
#include <net/sock.h>
#include <linux/rwsem.h>

#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"
#include "xaxidma_hw.h"

int zynqmp_open (struct inode *inode, struct file *filp)
{
  	DevStatics *dev_stat;
  	int mnum;

	dev_stat = container_of(inode->i_cdev, DevStatics, cdev);

	mnum = iminor(inode);
	if (mnum > 0  || mnum > num_devices || dev_stat == NULL || dev_stat->valid == 0)
		return -ENODEV;

	filp->private_data = dev_stat;
	if (dev_stat->usecount == 0)
	{ /* first open */
		sema_init(&dev_stat->sem, 1);
		zynqmp_dma_init(dev_stat);
		zynqmp_dma_config_memory(dev_stat, 0);
	  	dev_stat->usecount++;
		if (debug >= MSG_NORMAL)
			printk(KERN_NOTICE "%s: Finished FIRST open OK: use count now =%d, private_data=%px\n", driver_name, dev_stat->usecount, filp->private_data);
  	}
	else
	{
	  	dev_stat->usecount++;
		if (debug >= MSG_NORMAL)
			printk(KERN_NOTICE "%s: Finished additional open OK: use count now =%d, private_data=%px\n", driver_name, dev_stat->usecount, filp->private_data);
	}
  	return 0;
}

int zynqmp_close (struct inode *inode, struct file *filp)
{
  	DevStatics *dev_stat = filp->private_data;
//	u_int32_t control, status;
/*	int i, dev_id;
	u_int32_t *io_base, csr;
*/
  	if (dev_stat->usecount == 1)
  	{ /* last close */
		printk(KERN_NOTICE "%s:  closing\n", driver_name);
		/* Make IRQs go away BEFORE removing handler */
#if 0
		if (dev_stat->xspress_version == Xspress3Mini)
		{
			status = XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4);
			control = XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			control &= ~XAXIDMA_IRQ_IOC_MASK;
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_LIST]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, XAXIDMA_IRQ_IOC_MASK);
			tasklet_kill(&dev_stat->hist_tasklet);

			control = XSP4_IORead32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_FRAMES]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			control &= ~XAXIDMA_IRQ_IOC_MASK;
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_FRAMES]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3M_DMA_STREAM_BNUM_HIST_FRAMES]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, XAXIDMA_IRQ_IOC_MASK);

			control = XSP4_IORead32(dev_stat->dma_virt_base[XSP3_DMA_STREAM_BNUM_SCALERS]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4);
			control &= ~XAXIDMA_IRQ_IOC_MASK;
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3_DMA_STREAM_BNUM_SCALERS]+(XAXIDMA_RX_OFFSET+XAXIDMA_CR_OFFSET)/4, control);
			XSP4_IOWrite32(dev_stat->dma_virt_base[XSP3_DMA_STREAM_BNUM_SCALERS]+(XAXIDMA_RX_OFFSET+XAXIDMA_SR_OFFSET)/4, XAXIDMA_IRQ_IOC_MASK);

		}
#endif		
  	}
  	dev_stat->usecount--;
  	return 0;
}

int zynqmp_mmap (struct file *file, struct vm_area_struct *vma)
{
	DevStatics *dev_stat = file->private_data;
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long vsize = vma->vm_end - vma->vm_start;
	unsigned long psize;
	unsigned long pfn;
	int addr_region;
	int rc = 0;

	addr_region = iminor(file_inode(file));
	if (addr_region <0 || addr_region >= ZYNQMP_NUM_ADDR_SPACE)
	{
		printk(KERN_ERR "%s : Cannot mmap region number %d\n", driver_name, addr_region);
		return -EINVAL;
	}
	psize = dev_stat->size_bytes[addr_region] - off;
	pfn = dev_stat->phys_base[addr_region] >> PAGE_SHIFT;

	if (vsize > psize)
	{
		printk(KERN_ALERT "%s: drv_mmap(): span too high (vsize = %d psize = %d)\n", driver_name,  (int) vsize, (int) psize);
		rc = -EINVAL;
	}
	else
	{
		vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;
		vma->vm_page_prot =  pgprot_noncached(vma->vm_page_prot);

		if (remap_pfn_range(vma, vma->vm_start, pfn, vsize, vma->vm_page_prot))
		{
			printk(KERN_ALERT "%s: drv_mmap(): remap_page_range error\n", driver_name);
			rc = -EAGAIN;
		}
		else if (debug >= MSG_NORMAL)
		{
			printk(KERN_INFO "%s: page frame number 0x%x\n", driver_name, (int) pfn);
			printk(KERN_INFO "%s: virtual address   0x%x\n", driver_name, (int) vma->vm_start);
			printk(KERN_INFO "%s: virtual size      0x%x\n", driver_name, (int) vsize);
		}
	}
	return rc;
}
