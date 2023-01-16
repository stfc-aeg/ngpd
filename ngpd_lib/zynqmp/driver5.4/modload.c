/* 
	Generic Driver for use on ZynqMP.
	10/02/2021	Derived from XSPRESS4 Zynq driver
*/


#include <linux/module.h>
#include <linux/types.h>   /* ulong and friends */ 
#include <linux/sched.h>   /* current, task_struct, other goodies */ 
#include <linux/fcntl.h>   /* O_NONBLOCK etc. */ 
#include <linux/errno.h>   /* return values */ 
#include <linux/ioport.h>  /* request_region() */ 
/* #include <linux/config.h> */ /* system name and global items */ 
/* #include <linux/malloc.h>  kmalloc, kfree */ 
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>    /* char dev */ 
#include <asm/io.h>        /* inb() inw() outb() ... */ 
#include <asm/irq.h>       /* unreadable, but useful */ 
#include <linux/of_address.h>
#include <linux/of_dma.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include <net/sock.h>
#include <net/tcp.h>
#include <net/inet_common.h>
//#include "xlldma.h"
//#include "xparameters.h"

//#include "xspress3.h"
#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "driver_static.h"
#include "dma_control.h"

MODULE_LICENSE("GPL");

DevStatics *dev_statics;

int device_major = 0;    /* default: dynamic */

int debug = 0;
char * driver_name= "ngpd";
#define ADDR_SPACE_USED_ALL     15
#define ADDR_SPACE_USED_NGPD     1

#define ADDR_SPACE_CACHE_NONE 0
#define ADDR_SPACE_CACHE_WC   1
#define ADDR_SPACE_CACHE_ON   2


#define XPAR_FABRIC_HIST_LIST_DMA_S2MM_INTROUT_INTR 64		// Picked out of xparamters.h

typedef struct {
	u_int64_t base, size, used_on, cache, access_flags;
	char name[20];
} AddrSpace;


AddrSpace *addr_space;

AddrSpace addr_space_ngzmp_xu7[ZYNQMP_NUM_ADDR_SPACE] ={
{ 	0x80000000L, 0x00001000, ADDR_SPACE_USED_ALL, ADDR_SPACE_CACHE_NONE, ACCESS_FORCE32BIT, "ngzmp-local" },	
{ 	0xB0000000L, 0x08000000, ADDR_SPACE_USED_ALL, ADDR_SPACE_CACHE_NONE, ACCESS_FORCE32BIT, "ngzmp-regs" },	
{ 	0x80030000L, 0x00010000, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_NONE, ACCESS_FORCE32BIT, "ngzmp-jesd" },	// Access to JESD and other Xilinx Cores
{ 	0x80080000L, 0x00010000, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_NONE, ACCESS_FORCE32BIT, "ngzmp-dma" },	// Access to DMA engines
//{  ZYNQMP_PS_DRAM_PHYS_BASE_LOW, ZYNQMP_PS_DRAM_PHYS_SIZE_LOW, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_NONE, 0, "ngzmp-ps-dram-low" } ,	// View of PS DRAM
//{  ZYNQMP_PS_DRAM_PHYS_BASE_HIGH, ZYNQMP_PS_DRAM_PHYS_SIZE_HIGH, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_NONE, 0, "ngzmp-ps-dram-high" } ,	// View of PS DRAM
{  ZYNQMP_PS_DRAM_PHYS_BASE_LOW, ZYNQMP_PS_DRAM_PHYS_SIZE_LOW, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_ON, ACCESS_CACHEFLUSH, "ngzmp-ps-dram-low" } ,	// View of PS DRAM
{  ZYNQMP_PS_DRAM_PHYS_BASE_HIGH, ZYNQMP_PS_DRAM_PHYS_SIZE_HIGH, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_ON, ACCESS_CACHEFLUSH, "ngzmp-ps-dram-high" } ,	// View of PS DRAM
//{  ZYNQMP_PL_DRAM_PHYS_BASE, ZYNQMP_PL_DRAM_PHYS_SIZE, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_WC, 0, "ngzmp-pl-dram" } ,	// View of PL DRAM
{  ZYNQMP_PL_DRAM_PHYS_BASE, ZYNQMP_PL_DRAM_PHYS_SIZE, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_ON, ACCESS_CACHEFLUSH, "ngzmp-pl-dram" } ,	// View of PL DRAM
{ 	0x800A0000L, 0x00002000, ADDR_SPACE_USED_ALL, ADDR_SPACE_CACHE_NONE, ACCESS_FORCE32BIT, "ngzmp-hist" },	// Access to the Histogrammer control regs and associated test GPIO.
//{	0x80080000, 0x00080000, ADDR_SPACE_USED_NGPD, ADDR_SPACE_CACHE_NONE, "ngzmp-rdma" },	// If we add UDP core

#if HIST_CACHE_OPTION==HIST_CACHE_ALL_COHERENT
//{   0x10000000, 0x18000000, ADDR_SPACE_USED_XSP3MINI, ADDR_SPACE_CACHE_WC, "xsp3mini-pb-scope" },
//{   0x28000000, 0x18000000, ADDR_SPACE_USED_XSP3MINI, ADDR_SPACE_CACHE_ON, "xsp3mini-hist" }
#else
//{   0x10000000, 0x18000000, ADDR_SPACE_USED_XSP3MINI, ADDR_SPACE_CACHE_WC, "xsp3mini-pb-scope" },
//{   0x28000000, 0x18000000, ADDR_SPACE_USED_XSP3MINI, ADDR_SPACE_CACHE_WC, "xsp3mini-hist" },
//{   0x10000000, 0x08000000, ADDR_SPACE_USED_XSP3MINI, ADDR_SPACE_CACHE_NONE, "xsp3mini-pb-scope" },
//{   0x18000000, 0x08000000, ADDR_SPACE_USED_XSP3MINI, ADDR_SPACE_CACHE_NONE, "xsp3mini-hist" }
#endif

}; 

module_param(device_major, int, S_IRUGO);
module_param(debug, int, S_IRUGO);

//dev_t dev_num;
//struct cdev *cdev = NULL;
/* irqreturn_t local_irq_handler(int irq, void *dev_id, struct pt_regs *unused); */
//static const struct of_device_id of_match[] __devinitdata = {
static const struct of_device_id of_match[] = {
  { .compatible = "stfc,ngzmp-1.00.a", },
  {}
};

int drv_remove (struct platform_device *pdev);
int create_server_sockets(DevStatics *dev_stat);
int tcp_keepalive(struct socket *sock);

struct file_operations fops=
{
	.owner = THIS_MODULE,
	.llseek 	= NULL,
	.read  	= NULL,
	.write 	= NULL,
//    .readdir= NULL,
    .poll   = NULL,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
  	.unlocked_ioctl  = zynqmp_ioctl,
#else
  	.ioctl  = zynqmp_ioctl,
#endif
  	.mmap   = zynqmp_mmap,
  	.open   = zynqmp_open,
  	.release= zynqmp_close
};

int num_devices=1;

struct workqueue_struct * wq=NULL;

#define BACKLOG 10

int drv_probe (struct platform_device *pdev)
{
	const struct of_device_id *match;
    DevStatics *dev_stat;
    struct device_node *node;
//    struct resource *res;
    int i, err;
	u_int32_t revision;
	int family;
	
	match = of_match_device(of_match, &pdev->dev);
  	if (!match)
    	return -EINVAL;

    dev_stat = devm_kzalloc(&pdev->dev, sizeof(DevStatics), GFP_KERNEL);
    if (!dev_stat)
        return -ENOMEM;

//    dev_stat->dev = &(pdev->dev);
    node = pdev->dev.of_node;

	platform_set_drvdata(pdev, dev_stat);

  	printk(KERN_ERR "%s: Loading ZynqMP Driver\n", driver_name);
  /* Look for a major */
	if (device_major)
	{
		dev_stat->dev_num = MKDEV(device_major, 0);
		err = register_chrdev_region(dev_stat->dev_num, num_devices, "ngzmp");
	}
	else
	{
		err = alloc_chrdev_region(&dev_stat->dev_num, 0, num_devices, "ngzmp");
		device_major = MAJOR(dev_stat->dev_num);
	}
	if (err != 0)
	{
		printk (KERN_ERR "NGZMP : Cannot allocate device number\n");
		return err;
	}

	dev_stat->zynqmp_version = NGPDZynqMP_XU7; // Only thing supported yet is NGPD in ZynqMP

	dev_stat->hist_list_irq = -1;
	dev_stat->hist_frame_irq = -1;
	dev_stat->scalar_irq = -1;
	dev_stat->num_scope = 1;

	addr_space = addr_space_ngzmp_xu7;

	for (i=0; i<ZYNQMP_NUM_ADDR_SPACE; i++)
	{
		if (dev_stat->zynqmp_version == NGPDZynqMP_XU7 && !(addr_space[i].used_on & ADDR_SPACE_USED_NGPD))
			continue;		
		if (debug)
			printk(KERN_INFO "%s : Requesting Address space '%s' Base=0x%08llX, Size=0x%08llX\n", driver_name, addr_space[i].name, addr_space[i].base, addr_space[i].size);
		strcpy(dev_stat->addr_space_name[i], addr_space[i].name);
		if (request_mem_region(addr_space[i].base, addr_space[i].size, dev_stat->addr_space_name[i]) == NULL)
		{
			printk (KERN_ERR "%s : Cannot request Address space '%s' Base=0x%08llX, Size=0x%08llX\n", driver_name, addr_space[i].name, addr_space[i].base, addr_space[i].size);
			drv_remove(pdev);
			return -1;
		}

		dev_stat->phys_base[i] = addr_space[i].base;
		dev_stat->size_bytes[i] = addr_space[i].size;
		dev_stat->access_flags[i] = addr_space[i].access_flags;
//		if ((dev_statics.virt_base[i] = ioremap_wc(dev_statics.phys_base[i], dev_statics.size_bytes[i] )) == NULL)
		if (addr_space[i].cache == ADDR_SPACE_CACHE_ON)
		{
			printk(KERN_INFO "%s : Mapping address space %d with cache ON, access flags=%08X\n", driver_name, i, dev_stat->access_flags[i]);
			if ((dev_stat->virt_base[i] = ioremap_cache(dev_stat->phys_base[i], dev_stat->size_bytes[i] )) == NULL)
			{
				printk (KERN_ERR "%s : Cannot ioremap Address space '%s' Base=0x%08llX, Size=0x%08llX\n", driver_name, addr_space[i].name, addr_space[i].base, addr_space[i].size);
				drv_remove(pdev);
				return -ENODEV;
			}
		}
		else if (addr_space[i].cache == ADDR_SPACE_CACHE_WC)
		{
			printk(KERN_INFO "%s : Mapping address space %d with cache WC, access flags=%08X\n", driver_name, i, dev_stat->access_flags[i]);
			if ((dev_stat->virt_base[i] = ioremap_wc(dev_stat->phys_base[i], dev_stat->size_bytes[i] )) == NULL)
			{
				printk (KERN_ERR "%s : Cannot ioremap Address space '%s' Base=0x%08llX, Size=0x%08llX\n", driver_name, addr_space[i].name, addr_space[i].base, addr_space[i].size);
				drv_remove(pdev);
				return -ENODEV;
			}
		}
		else
		{
			printk(KERN_INFO "%s : Mapping address space %d with cache OFF, access flags=%08X\n", driver_name, i, dev_stat->access_flags[i]);
/*			if ((dev_stat->virt_base[i] = ioremap_nocache(dev_stat->phys_base[i], dev_stat->size_bytes[i] )) == NULL) */
			if ((dev_stat->virt_base[i] = ioremap(dev_stat->phys_base[i], dev_stat->size_bytes[i] )) == NULL)  /* in 5.4 ioremap and ioremap_nocahce are both  __ioremap((addr), (size), __pgprot(PROT_DEVICE_nGnRE)) */
			/* In 5.10 only ioremap() exists and is  __ioremap((addr), (size), __pgprot(PROT_DEVICE_nGnRE))*/
			{
				printk (KERN_ERR "%s : Cannot ioremap Address space '%s' Base=0x%08llX, Size=0x%08llX\n", driver_name, addr_space[i].name, addr_space[i].base, addr_space[i].size);
				drv_remove(pdev);
				return -ENODEV;
			}
		}
		if (debug>1)
			printk(KERN_INFO "%s : .... mapped to virtual address %px\n", driver_name, dev_stat->virt_base[i]);

		if (i == 0)
		{
			printk( "%s : About to read revision register from VA %px\n", driver_name, dev_stat->virt_base[i]);
			revision = ZYNQMP_IORead32(dev_stat->virt_base[i]);
			printk( "%s : Found Local revision=%08X\n", driver_name, revision);
			family = (revision >> 24) & 0xFF;
			if (family == 16)
			{
				dev_stat->zynqmp_version = NGPDZynqMP_XU7;
				printk(KERN_INFO "%s: Detected NGPD \n", driver_name);
				dev_stat->num_scope = 3;
			}
			else
			{
				/* Usually 3 for Xspress4, leave up to 15 for other Zynq based systems, 16 onwards for ZynqMP, versal etc. */
				printk(KERN_ERR"%s: Unexpected family number=%d\n", driver_name, family);
			}

/*			major = (revision >> 12) & 0xFFF;
			if (major/16 == 2)
			{
				dev_stat->xspress_version = Xspress3Mini;
				printk(KERN_INFO "%s: Detected Xspress3 mini\n", driver_name);
				dev_stat->num_scope = 2;
			}
*/
		}
	}
//	mutex_init(&dev_stat->dma_mutex);
	init_rwsem(&dev_stat->dma_sem);
#if ZYNQMP_USE_SOCKETS
	if (dev_stat->zynqmp_version == Xspress3Mini)
	{
		wq = alloc_workqueue("%s", WQ_UNBOUND ,0, "xsp4wq");
		if (wq == NULL)
		{
			printk(KERN_ERR "%s : Cannot allocate work queue\n", driver_name);
			drv_remove(pdev);
			return -ENODEV;
		}
		dev_stat->hist_list_irq = irq_of_parse_and_map(node, 0);
		dev_stat->hist_frame_irq = irq_of_parse_and_map(node, 1);
		dev_stat->scalar_irq = irq_of_parse_and_map(node, 2);
		printk(KERN_NOTICE "%s : Found hist_list_irq = %d, hist_frame_irq=%d, scalar_irq=%d\n", driver_name, dev_stat->hist_list_irq, dev_stat->hist_frame_irq, dev_stat->scalar_irq );
	   	err = request_irq(dev_stat->hist_list_irq, dma_irq_handler, IRQF_SHARED, "hist_irq", dev_stat);
	    if (err)
	    {
	        printk(KERN_ERR ": %s unable to request Hist list IRQ %d\n", driver_name, dev_stat->hist_list_irq);
	        return err;
	    }
		if (dev_stat->hist_frame_irq > 0)
		{
			err = request_irq(dev_stat->hist_frame_irq, hist_frame_dma_irq_handler, IRQF_SHARED, "hist_frame_irq", dev_stat);
			if (err)
			{
				printk(KERN_ERR ": %s unable to request Hist Frames IRQ %d\n", driver_name, dev_stat->hist_frame_irq);
				return err;
			}
			err = request_irq(dev_stat->scalar_irq, scalar_dma_irq_handler, IRQF_SHARED, "scalar_irq", dev_stat);
			if (err)
			{
				printk(KERN_ERR ": %s unable to request Scalar IRQ %d\n", driver_name, dev_stat->scalar_irq);
				return err;
			}
			if ((err=create_server_sockets(dev_stat)))
			{
				drv_remove (pdev);
				return err;
			}

		}
	}
#endif
	/*
	init_waitqueue_head(&(midas_dev_statics.wait_q));
	*/
	dev_stat->valid = 1;
	dev_statics = dev_stat;
    cdev_init(&dev_stat->cdev, &fops);
    dev_stat->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev_stat->cdev, dev_stat->dev_num, 1);
    if (err) 
    {
        dev_err(&pdev->dev, "cdev_add() failed\n");
		drv_remove (pdev);
		return err;
    }
	dev_stat->cdev_done = 1;

    dev_stat->class = class_create(THIS_MODULE, driver_name);
    if (IS_ERR(dev_stat->class))
    {
	    dev_err(&pdev->dev, "failed to create class\n");
		drv_remove (pdev);
		return -1;
    }

    dev_stat->dev = device_create(dev_stat->class, &pdev->dev, dev_stat->dev_num, dev_stat, driver_name);
    if (IS_ERR(dev_stat->dev)) 
    {
        dev_err(&pdev->dev, "unable to create device\n");
 		drv_remove (pdev);
		return -1;
    }

  	return err; /* success */
}

int drv_remove (struct platform_device *pdev)
{
	int i;
	DevStatics *dev_stat = platform_get_drvdata(pdev);

  	printk(KERN_NOTICE "%s: driver unloading\n", driver_name);

	if (wq != NULL)
		destroy_workqueue(wq);

#if ZYNQMP_USE_SOCKETS
	printk(KERN_NOTICE "%s : Shutting down sockets\n", driver_name);
	if (dev_stat->hist_list_server_sock != NULL)
	{
		kernel_sock_shutdown(dev_stat->hist_list_server_sock, 0);
		dev_stat->hist_list_server_sock->ops->release(dev_stat->hist_list_server_sock);
	}

	if (dev_stat->hist_frame_server_sock != NULL)
	{
		kernel_sock_shutdown(dev_stat->hist_frame_server_sock, 0);
		dev_stat->hist_frame_server_sock->ops->release(dev_stat->hist_frame_server_sock);
	}

	if (dev_stat->scalar_server_sock != NULL)
	{
		kernel_sock_shutdown(dev_stat->scalar_server_sock, 0);
		dev_stat->scalar_server_sock->ops->release(dev_stat->scalar_server_sock);
	}

	if (dev_stat->hist_list_transfer_sock != NULL)
	{
		kernel_sock_shutdown(dev_stat->hist_list_transfer_sock, 0);
		dev_stat->hist_list_transfer_sock->ops->release(dev_stat->hist_list_transfer_sock);
	}

	if (dev_stat->hist_frame_transfer_sock != NULL)
	{
		kernel_sock_shutdown(dev_stat->hist_frame_transfer_sock, 0);
		dev_stat->hist_frame_transfer_sock->ops->release(dev_stat->hist_frame_transfer_sock);
	}

	if (dev_stat->scalar_transfer_sock != NULL)
	{
		kernel_sock_shutdown(dev_stat->scalar_transfer_sock, 0);
		dev_stat->scalar_transfer_sock->ops->release(dev_stat->scalar_transfer_sock);
	}

	printk(KERN_NOTICE "%s : Freeing IRQs\n", driver_name);

	if (dev_stat->hist_list_irq >= 0)
		free_irq(dev_stat->hist_list_irq, dev_stat);             // Free Vector
	if (dev_stat->hist_frame_irq >= 0)
		free_irq(dev_stat->hist_frame_irq, dev_stat);             // Free Vector
	if (dev_stat->scalar_irq >= 0)
		free_irq(dev_stat->scalar_irq, dev_stat);             // Free Vector
#endif
#if HIST_CACHE_OPTION!=HIST_CACHE_ALL_COHERENT
	if (dev_stat->hist_virt_base != NULL)
		iounmap(dev_stat->hist_virt_base);
#endif
	for (i=0; i<ZYNQMP_NUM_ADDR_SPACE; i++)
	{
		if (dev_stat->virt_base[i] != NULL)
			iounmap(dev_stat->virt_base[i]);

		if (dev_stat->phys_base[i] != 0)
			release_mem_region(dev_stat->phys_base[i], dev_stat->size_bytes[i]);
	}

	if (!IS_ERR_OR_NULL(dev_stat->dev))
		device_destroy(dev_stat->class, dev_stat->dev_num);	// Note call with class
	if (!IS_ERR_OR_NULL(dev_stat->class))
		class_destroy(dev_stat->class);
	if (dev_stat->cdev_done)
		cdev_del(&dev_stat->cdev);
	unregister_chrdev_region(dev_stat->dev_num, num_devices);

	return 0;
}




MODULE_DEVICE_TABLE(of, of_match);


static struct platform_driver platform_driver = {
  .probe = drv_probe,
  .remove = drv_remove,
  .driver = {
    .name = "ngzmp",
    .owner = THIS_MODULE,
    .of_match_table = of_match,
  },
};

module_platform_driver(platform_driver);

MODULE_AUTHOR("STFC");
MODULE_DESCRIPTION("NGPD ZynqMP Driver");

#if ZYNQMP_USE_SOCKETS
void accept_hist_list(struct sock *sk);
void accept_hist_frame(struct sock *sk);
void accept_scalar(struct sock *sk);

void change_state_hist_list(struct sock *sk)
{
	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering change_state_hist_list, state=%d\n", driver_name, sk->sk_state);

	dev_stat->orig_state_change(sk);
	if (sk->sk_state == TCP_CLOSE_WAIT)
		queue_work(wq, &dev_stat->hist_list_close_wait);
}

void change_state_hist_frame(struct sock *sk)
{
	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering change_state_hist_frame, state=%d\n", driver_name, sk->sk_state);
	dev_stat->orig_state_change(sk);
	if (sk->sk_state == TCP_CLOSE_WAIT)
		queue_work(wq, &dev_stat->hist_frame_close_wait);
}
void change_state_scalar(struct sock *sk)
{
	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering change_state_scalar, state=%d\n", driver_name, sk->sk_state);

	dev_stat->orig_state_change(sk);

	if (sk->sk_state == TCP_CLOSE_WAIT)
		queue_work(wq, &dev_stat->scalar_close_wait);
}

void accept(struct socket *server, struct socket **transfer)
{
	DevStatics *dev_stat = (DevStatics *)server->sk->sk_user_data;
	int ret;

	printk(KERN_WARNING "%s : Entering accept callback server socket=%px, new_socket=%px\n", driver_name, server, *transfer);

	ret = kernel_accept(server, transfer, 0);

	if (ret || 1)
		printk(KERN_ERR "%s : kernel_accept returned error %d\n", driver_name, ret);

	if (ret)
	{
		printk(KERN_ERR "%s : kernel accept failed\n", driver_name);
		*transfer = NULL;
		return;
	}
/*	if ((*transfer)->sk->sk_data_ready == accept_hist_list || (*transfer)->sk->sk_data_ready == accept_hist_frame || (*transfer)->sk->sk_data_ready == accept_scalar)
		(*transfer)->sk->sk_data_ready = dev_stat->orig_data_ready;
*/

	write_lock_bh(&((*transfer)->sk->sk_callback_lock));	
	(*transfer)->sk->sk_data_ready = dev_stat->orig_data_ready;
	(*transfer)->sk->sk_user_data = dev_stat;
	if (server == dev_stat->hist_list_server_sock)
		(*transfer)->sk->sk_state_change = change_state_hist_list;
	else if (server == dev_stat->hist_frame_server_sock)
		(*transfer)->sk->sk_state_change = change_state_hist_frame;
	else if (server == dev_stat->scalar_server_sock)
		(*transfer)->sk->sk_state_change = change_state_scalar;
	write_unlock_bh(&((*transfer)->sk->sk_callback_lock));	

	printk(KERN_WARNING "%s : Setting keep alive on new socket\n", driver_name);

	tcp_keepalive(*transfer);

/*	socket_send(*transfer, msg, strlen(msg)); */

	printk(KERN_WARNING "%s : Finished accept\n", driver_name);

}

void accept_hist_list(struct sock *sk)
{
	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering accept_hist_list, state=%d\n", driver_name, sk->sk_state);
//    read_lock_bh(&sk->sk_callback_lock);

    /*
     * ->sk_data_ready is also called for a newly established child socket
     * before it has been accepted and the accepter has set up their
     * data_ready.. we only want to queue listen work for our listening
     * socket
     */
    if (sk->sk_state == TCP_LISTEN)
		queue_work(wq, &dev_stat->hist_list_accept);

 //   read_unlock_bh(&sk->sk_callback_lock);
}


void error_hist_list(struct sock *sk)
{
//	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering error_hist_list, state=%d\n", driver_name, sk->sk_state);
}
void accept_hist_frame(struct sock *sk)
{
	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering accept_hist_frame, state=%d\n", driver_name, sk->sk_state);
//    read_lock_bh(&sk->sk_callback_lock);

    /*
     * ->sk_data_ready is also called for a newly established child socket
     * before it has been accepted and the accepter has set up their
     * data_ready.. we only want to queue listen work for our listening
     * socket
     */
    if (sk->sk_state == TCP_LISTEN)
		queue_work(wq, &dev_stat->hist_frame_accept);

//    read_unlock_bh(&sk->sk_callback_lock);
}

void accept_scalar(struct sock *sk)
{
	DevStatics *dev_stat = (DevStatics *)sk->sk_user_data;

	printk (KERN_WARNING "%s : Entering accept_scalar, state=%d\n", driver_name, sk->sk_state);
//    read_lock_bh(&sk->sk_callback_lock);

    /*
     * ->sk_data_ready is also called for a newly established child socket
     * before it has been accepted and the accepter has set up their
     * data_ready.. we only want to queue listen work for our listening
     * socket
     */
    if (sk->sk_state == TCP_LISTEN)
		queue_work(wq, &dev_stat->scalar_accept);

//	read_unlock_bh(&sk->sk_callback_lock);
}

void hist_list_accept_worker(struct work_struct *work)
{
	DevStatics *dev_stat = container_of(work, DevStatics, hist_list_accept);
	accept(dev_stat->hist_list_server_sock, &dev_stat->hist_list_transfer_sock);
}
void hist_frame_accept_worker(struct work_struct *work)
{
	DevStatics *dev_stat = container_of(work, DevStatics, hist_frame_accept);
	accept(dev_stat->hist_frame_server_sock, &dev_stat->hist_frame_transfer_sock);
}
void scalar_accept_worker(struct work_struct *work)
{
	DevStatics *dev_stat = container_of(work, DevStatics, scalar_accept);
	accept(dev_stat->scalar_server_sock, &dev_stat->scalar_transfer_sock);
}

void hist_list_close_wait_worker(struct work_struct *work)
{
	DevStatics *dev_stat = container_of(work, DevStatics, hist_list_close_wait);
	int i=0;
	printk(KERN_WARNING "%s : Entering hist_list_close_wait_worker\n", driver_name);
//	i = mutex_trylock(&dev_stat->sock_mutex);
/* The mutex protecion here that if the PC end disconnects the transfer worker threads may be running or may be finished, waiting for the next IRQ
If they are running they, they will find out that the socket is disconnected at the next write, but they don't want the socket releasing under their feet */

	i =  mutex_lock_interruptible(&dev_stat->hist_list_mutex);
	if (i==0)
	{
		dev_stat->hist_list_transfer_sock->sk->sk_state_change = dev_stat->orig_state_change;	// We should not get any more change state message from this socket as it goes through the close sequence
		if (dev_stat->hist_list_transfer_sock != NULL)
		{
			kernel_sock_shutdown(dev_stat->hist_list_transfer_sock, SHUT_RDWR);
			printk(KERN_WARNING "%s : .... calling release\n", driver_name);
//			dev_stat->hist_list_transfer_sock->ops->release(dev_stat->hist_list_transfer_sock);
			sock_release(dev_stat->hist_list_transfer_sock);
		}
		dev_stat->hist_list_transfer_sock = NULL;
		mutex_unlock(&dev_stat->hist_list_mutex);
	}

}

void hist_frame_close_wait_worker(struct work_struct *work)
{
	DevStatics *dev_stat = container_of(work, DevStatics, hist_frame_close_wait);
	if (mutex_lock_interruptible(&dev_stat->hist_frame_mutex) == 0)
	{
		dev_stat->hist_frame_transfer_sock->sk->sk_state_change = dev_stat->orig_state_change;	// We should not get any more change state message from this socket as it goes through the close sequence
		if (dev_stat->hist_frame_transfer_sock != NULL)
		{
			kernel_sock_shutdown(dev_stat->hist_frame_transfer_sock, SHUT_RDWR);
//			dev_stat->hist_frame_transfer_sock->ops->release(dev_stat->hist_frame_transfer_sock);
			sock_release(dev_stat->hist_frame_transfer_sock);
		}
		dev_stat->hist_frame_transfer_sock = NULL;
		mutex_unlock(&dev_stat->hist_frame_mutex);
	}
}

void scalar_close_wait_worker(struct work_struct *work)
{
	DevStatics *dev_stat = container_of(work, DevStatics, scalar_close_wait);
	if (mutex_lock_interruptible(&dev_stat->scalar_mutex) == 0)
	{
		dev_stat->scalar_transfer_sock->sk->sk_state_change = dev_stat->orig_state_change;	// We should not get any more change state message from this socket as it goes through the close sequence
		if (dev_stat->scalar_transfer_sock != NULL)
		{
			kernel_sock_shutdown(dev_stat->scalar_transfer_sock, SHUT_RDWR);
//			dev_stat->scalar_transfer_sock->ops->release(dev_stat->scalar_transfer_sock);
			sock_release(dev_stat->scalar_transfer_sock);
		}
		dev_stat->scalar_transfer_sock = NULL;
		mutex_unlock(&dev_stat->scalar_mutex);
	}
}

int create_server_sockets(DevStatics *dev_stat)
{
	struct sockaddr_in saddr = {};
	int ret;

	INIT_WORK(&dev_stat->hist_list_accept, hist_list_accept_worker);
	INIT_WORK(&dev_stat->hist_frame_accept, hist_frame_accept_worker);
	INIT_WORK(&dev_stat->scalar_accept, scalar_accept_worker);

	INIT_WORK(&dev_stat->hist_list_send, hist_list_worker);
	INIT_WORK(&dev_stat->hist_frame_send, frame_worker_hist);
	INIT_WORK(&dev_stat->scalar_send, frame_worker_scalar);


	INIT_WORK(&dev_stat->hist_list_close_wait, hist_list_close_wait_worker);
	INIT_WORK(&dev_stat->hist_frame_close_wait, hist_frame_close_wait_worker);
	INIT_WORK(&dev_stat->scalar_close_wait, scalar_close_wait_worker);

	mutex_init(&dev_stat->hist_list_mutex);
	mutex_init(&dev_stat->hist_frame_mutex);
	mutex_init(&dev_stat->scalar_mutex);

	ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &dev_stat->hist_list_server_sock);
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "%s: error creating hist-list socket", driver_name);
		return ret;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(XSP4_CMD_PORT+1);
	saddr.sin_addr.s_addr = INADDR_ANY;

	ret = kernel_bind(dev_stat->hist_list_server_sock, (struct sockaddr *)&saddr, sizeof(saddr));
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "%s : error binding hist-list socket to port %d", driver_name, XSP4_CMD_PORT+1);
		return ret;
	}

	ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &dev_stat->hist_frame_server_sock);
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "%s: error creating hist-frames socket", driver_name);
		return ret;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(XSP4_CMD_PORT+2);
	saddr.sin_addr.s_addr = INADDR_ANY;

	ret = kernel_bind(dev_stat->hist_frame_server_sock, (struct sockaddr *)&saddr, sizeof(saddr));
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "%s : error binding hist-frames socket to port %d", driver_name, XSP4_CMD_PORT+2);
		return ret;
	}

	ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &dev_stat->scalar_server_sock);
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "%s: error creating scalar socket", driver_name);
		return ret;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(XSP4_CMD_PORT+3);
	saddr.sin_addr.s_addr = INADDR_ANY;

	ret = kernel_bind(dev_stat->scalar_server_sock, (struct sockaddr *)&saddr, sizeof(saddr));
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "%s : error binding scalar socket to port %d\n", driver_name, XSP4_CMD_PORT+3);
		return ret;
	}
	/*
	if ((ret=tcp_keepalive(dev_stat->hist_list_server_sock)) < 0)
	{
		printk(KERN_ERR "%s : error setting keep alive options on hist list socket\n", driver_name);
		return ret;
	}
	if ((ret=tcp_keepalive(dev_stat->hist_frame_server_sock)) < 0)
	{
		printk(KERN_ERR "%s : error setting keep alive options on hist frames socket\n", driver_name);
		return ret;
	}
	if ((ret=tcp_keepalive(dev_stat->scalar_server_sock)) < 0)
	{
		printk(KERN_ERR "%s : error setting keep alive options on scalar socket\n", driver_name);
		return ret;
	}
	*/
	read_lock_bh(&dev_stat->hist_list_server_sock->sk->sk_callback_lock);	
	dev_stat->orig_data_ready = dev_stat->hist_list_server_sock->sk->sk_data_ready;
	dev_stat->orig_state_change = dev_stat->hist_list_server_sock->sk->sk_state_change;
	dev_stat->hist_list_server_sock->sk->sk_user_data = dev_stat;
	dev_stat->hist_list_server_sock->sk->sk_data_ready = accept_hist_list;

//	dev_stat->hist_list_server_sock->sk->sk_state_change = change_state_hist_list;
// 	dev_stat->hist_list_server_sock->sk->sk_error_report = error_hist_list; 

	read_unlock_bh(&dev_stat->hist_list_server_sock->sk->sk_callback_lock);	

	read_lock_bh(&dev_stat->hist_frame_server_sock->sk->sk_callback_lock);	
	dev_stat->hist_frame_server_sock->sk->sk_user_data = dev_stat;
	dev_stat->hist_frame_server_sock->sk->sk_data_ready = accept_hist_frame;
//	dev_stat->hist_frame_server_sock->sk->sk_state_change = change_state_hist_frame;
	read_unlock_bh(&dev_stat->hist_frame_server_sock->sk->sk_callback_lock);	

	read_lock_bh(&dev_stat->scalar_server_sock->sk->sk_callback_lock);	
	dev_stat->scalar_server_sock->sk->sk_user_data = dev_stat;
	dev_stat->scalar_server_sock->sk->sk_data_ready = accept_scalar;
//	dev_stat->scaler_server_sock->sk->sk_state_change = change_state_hist_list;
	read_unlock_bh(&dev_stat->scalar_server_sock->sk->sk_callback_lock);	

	if (kernel_listen(dev_stat->hist_list_server_sock, BACKLOG) < 0)
	{
		printk(KERN_ERR "%s : error on listen on hist list socket\n", driver_name);
		return ret;
	}
	if (kernel_listen(dev_stat->hist_frame_server_sock, BACKLOG) < 0)
	{
		printk(KERN_ERR "%s : error on listen on hist frame socket\n", driver_name);
		return ret;
	}
	if (kernel_listen(dev_stat->scalar_server_sock, BACKLOG) < 0)
	{
		printk(KERN_ERR "%s : error on listen on scalar socket\n", driver_name);
		return ret;
	}

	printk(KERN_WARNING "%s : Listening for TCP connections\n", driver_name);
/*	ret = sock_setsockopt(&dev_stat->hist_list_sock, SOL_SOCKET, SO_SNDBUF,
			(void *)&buffersize, sizeof(buffersize));	

*/
	return ret;
}

int tcp_keepalive(struct socket *sock)
{
	/* values below based on xs_udp_default_timeout */
	int keepidle = 5; /* send a probe 'keepidle' secs after last data */
	int keepcnt = 5; /* number of unack'ed probes before declaring dead */
	int keepalive = 1;
	int ret = 0;

	ret = kernel_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));
	if (ret < 0)
		goto bail;

	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (char *)&keepcnt, sizeof(keepcnt));
	if (ret < 0)
		goto bail;

	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (char *)&keepidle, sizeof(keepidle));
	if (ret < 0)
		goto bail;

	/* KEEPINTVL is the interval between successive probes. We follow
	 * the model in xs_tcp_finish_connecting() and re-use keepidle.
	 */
	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (char *)&keepidle, sizeof(keepidle));
bail:
	return ret;
}

#endif
