/*
	Generic Driver for use on ZynqMP.
	Application specific IOCTLS for Neutron Gamma Pulse Discriminator in ZynqMP
	17/06/2021	Derived from generic ioctl.c

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

#include "ngzmp.h"
#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"


long zynqmp_extra_ioctl ( unsigned int cmd, DevStatics *dev_stat, ZynqMPPb *param)
{
	u_int32_t __iomem *statusP, *filter, *ccP;
	u_int32_t status, coeff, s, chan_cont;
	int rc=0;
	u_int32_t __user *uptr;
	int attempt, i, j;

    switch (cmd) 
    {
    case NGZMP_LOAD_FILTER:
		if (debug >= MSG_VERBOSE)
			printk("%s: NGZMP_LOAD_FILTER chan=0x%08X, num=0x%08X\n", driver_name, param->start, param->num); 
		if (param->start>NGZMP_CHANS_PER_CARD || param->num != NGZMP_FILT_NUM_COEFF)
		{
			status = 0xFFFFFFFF;
			if (param->ptr2 != NULL)
				put_user(status, param->ptr2);
			return -EINVAL; 
		}
		statusP = dev_stat->virt_base[ZYNQMP_ADDR_SPACE_REGS]+NGZMP_FILTER_STATUS +(1<<(NGZMP_ADDR_BIT_GLOB_REGS-2));
		filter = dev_stat->virt_base[ZYNQMP_ADDR_SPACE_REGS] + (param->start<<(NGZMP_ADDR_SLICE_CHAN-2))+NGZMP_CHAN_FILTER;
		ccP = dev_stat->virt_base[ZYNQMP_ADDR_SPACE_REGS] + (param->start<<(NGZMP_ADDR_SLICE_CHAN-2))+NGZMP_CHAN_CONT;
		chan_cont = ZYNQMP_IORead32(ccP);
		ZYNQMP_IOWrite32(ccP, 0); // Force chan cont 0 so data valid is fixed high, so filter can load the new coefficients.
		for (attempt=0; attempt<10; attempt++)
		{
			status = ZYNQMP_IORead32(statusP);
			if (!(status & NGZMP_FILTER_STATUS_RELOAD_TREADY(param->start)))
			{
				printk(KERN_WARNING "%s: NGZMP_LOAD_FILTER: Filter Coeff port reports not ready for first word, chan=%d, attempt=%d, status=%08X\n", driver_name, param->start, attempt, status);
				status |= NGZMP_FILTER_STATUS_MISSING_TREADY(param->start);
				ZYNQMP_IOWrite32(filter, NGZMP_FILTER_FORCE_CONF);
				continue;
			}
			uptr = param->ptr;
			for (i=0;i<param->num;i++)
			{
				get_user(coeff, uptr);
				uptr++;
				coeff = NGZMP_FILTER_COEF(coeff);
				if (i == param->num-1)
					coeff |= NGZMP_FILTER_TLAST;
				ZYNQMP_IOWrite32(filter, coeff);
				
				status = ZYNQMP_IORead32(statusP);
				if (debug>=3)
					printk(KERN_WARNING "%s: NGZMP_LOAD_FILTER: Word %d of %d, chan=%d, attempt=%d, status=0x%08X\n", driver_name, i, param->num, param->start, attempt, status);

				if (status & NGZMP_FILTER_STATUS_TLAST_UNEXPECTED(param->start))
				{
					printk(KERN_WARNING "%s: NGZMP_LOAD_FILTER: Detected unexpected tlast at word %d of %d, chan=%d, attempt=%d, status=0x%08X\n", driver_name, i, param->num, param->start, attempt, status);
					coeff = 0;
					for (j=0; j<NGZMP_FILT_NUM_COEFF; j++)
					{
						// Out of step, so push in 0 until we see missing tlast
						ZYNQMP_IOWrite32(filter, coeff);
						s = ZYNQMP_IORead32(statusP);
						if (s & NGZMP_FILTER_STATUS_TLAST_MISSING(param->start))
							break;
					}
					break;
				}
				if (status & NGZMP_FILTER_STATUS_TLAST_MISSING(param->start))
				{
					ZYNQMP_IOWrite32(filter, NGZMP_FILTER_FORCE_CONF);
					printk(KERN_WARNING "%s: NGZMP_LOAD_FILTER: Detected missing tlast at word %d of %d, chan=%d, attempt=%d, status=0x%08X\n", driver_name, i, param->num, param->start, attempt,  status);
#if 0
					coeff = 0;
					for (j=0; j<NGZMP_FILT_NUM_COEFF; j++)
					{
						// Out of step, so push in 0 until we see missing tlast .. Not clear that this will work without reseting the core, or forcing a config cycle
						ZYNQMP_IOWrite32(filter, coeff);
						s = ZYNQMP_IORead32(statusP);
						if (s & NGZMP_FILTER_STATUS_TLAST_MISSING(param->start))
							break;
					}
#endif
					break;
				}
				if ( !(status & NGZMP_FILTER_STATUS_RELOAD_TREADY(param->start)))
				{
					int j;
					
					for (j=0; j<10; j++)
					{
						status = ZYNQMP_IORead32(statusP);
						if (status & NGZMP_FILTER_STATUS_RELOAD_TREADY(param->start))
							break;
					}
					if (j == 10)
					{
						/* Since tlast_missing seems to pulse for 1 cycel, this is the clue that we are out of step */
						if (i== param->num-1)
						{
							printk(KERN_WARNING "%s: NGZMP_LOAD_FILTER: Filter does not come back to tready after sending last word and then config, chan=%d, attempt=%d, status=0x%08X\n", driver_name, param->start, attempt,  status);
						}
						else
						{
							ZYNQMP_IOWrite32(filter, NGZMP_FILTER_FORCE_CONF);
							printk(KERN_WARNING "%s: NGZMP_LOAD_FILTER: Filter not ready after word %d of %d, chan=%d, attempt=%d, status=0x%08X\n", driver_name, i, param->num, param->start, attempt,  status);
						}
						status |= NGZMP_FILTER_STATUS_MISSING_TREADY(param->start);
						break;
					}
				}
			}
			if ( !(status & (NGZMP_FILTER_STATUS_TLAST_MISSING(param->start) | NGZMP_FILTER_STATUS_TLAST_UNEXPECTED(param->start) | NGZMP_FILTER_STATUS_MISSING_TREADY(param->start))))
			{
				if (debug >= 2)
					printk(KERN_WARNING "%s: Load Filter: Success after attempt %d, status = 0x%08X\n", driver_name, attempt, status);
				break;
			}
			else
			{
				printk(KERN_WARNING "%s: Load Filter: Failed attempt %d, status = 0x%08X\n", driver_name, attempt, status);
			}
		}
		ZYNQMP_IOWrite32(ccP, chan_cont); // Restore chan_cont.
		if (param->ptr2 != NULL)
			put_user(status, param->ptr2);
		rc = 0;
		break;

    default:
		rc = -EINVAL;
		break;
	}
	return rc;
}
