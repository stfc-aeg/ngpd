/* 
XSPRESS4 Driver for use on Zynq.
	Created : 23/4/2015
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
