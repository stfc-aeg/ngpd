/* 	ZYNQ MP  link library
	Firset developed for Neutron Gamm Pulse discriminator and AXI Hist testing, this file generic function, possibly reusable
	 W. Helsby  Daresbury Lab 24/2/2021

 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <linux/fs.h>
#include "ngzmp.h"
#include "ngzmp_dma_protocol.h"
#include "ngzmp_driver.h"
#include "zynqmp_lib_driver.h"
#include "zynqmp_xadc.h"

int zynqmp_read(int path, int card, int addr_space, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start =  offset;
	pb.num = size;
	pb.addr_space = addr_space;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_READ, &pb);
}

int zynqmp_write(int path, int card, int addr_space, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start =  offset;
	pb.num = size;
	pb.addr_space = addr_space;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_WRITE, &pb);
}

int zynqmp_rmw(int path, int card, int addr_space, int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value)
{
	ZynqMPPb pb;
	pb.start =  offset;
	pb.num = 0;
	pb.addr_space = addr_space;
	pb.and_mask = and_mask;
	pb.or_mask = or_mask;
	pb.ptr = ret_value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_RMW, &pb);
}

int zynqmp_read_row(int path, int card, int addr_space, int row_len, int num_streams, int row, u_int32_t offset, u_int32_t size, u_int32_t *data)
{
	ZynqMPPb pb;
	u_int32_t col;

	col = offset % row_len;
	offset -= col;
	offset *= num_streams;
	offset += row*row_len+col;

	pb.start = offset;
	pb.num = size;
	pb.addr_space = addr_space;
	pb.ptr = data;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;
	pb.row_len = row_len;
	pb.row_stride = row_len*num_streams;
//	printf("ngzmp_hist_read_stream: start=0x%08X, row_len=0x%02X, row_stride=0x%02X\n", pb.start, pb.row_len, pb.row_stride);
	return ioctl(path, ZYNQMP_READ_ROW, &pb);
}

int zynqmp_dma(int path, int card, int cmd, int stream_mask, void *msg, int32_t *ret_value, u_int32_t *details)
{
	ZynqMPPb pb;
	pb.start =  cmd;
	
	pb.num = stream_mask;	
	pb.addr_space = 0;
	pb.and_mask = 0;
	pb.or_mask = 0;
	pb.ptr = msg;
	pb.ptr2 = (u_int32_t *)ret_value;
	pb.ptr3 = details;

	return ioctl(path, ZYNQMP_DMA_V7, &pb);
}

int zynqmp_dma_read(int path, int card, int cmd, int stream_mask, int first, int num, void *msg, int32_t *ret_value, u_int32_t *details)
{
	ZynqMPPb pb;
	pb.start =  cmd;
	
	pb.num = stream_mask;	
	pb.addr_space = first;	// Slotted in here for backwards compatibility
	pb.and_mask = num;
	pb.or_mask = 0;
	pb.ptr = msg;
	pb.ptr2 = (u_int32_t *)ret_value;
	pb.ptr3 = details;

	return ioctl(path, ZYNQMP_DMA_V7, &pb);
}


int zynqmp_read_dma_status_block(int path, int card, int offset_lwords, int size_lwords, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start =  offset_lwords;
	pb.num = size_lwords;
	pb.addr_space = 0;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_READ_DMA_STATUS_BLOCK, &pb);
}

int zynqmp_read_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start =  offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_DMA_BUFF + stream;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;
	return ioctl(path, ZYNQMP_READ, &pb);
}

int zynqmp_write_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start =  offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_DMA_BUFF + stream;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_WRITE, &pb);
}

int zynqmp_memzero(int path, int card, int addr_space, int offset, int size)
{
	ZynqMPPb pb;

	pb.start =  offset;
	pb.num = size;
	pb.addr_space = addr_space;
	pb.ptr = NULL;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_MEMZERO, &pb);
}

int zynqmp_zero_dma_buff(int path, int card, int stream, int offset, int size)
{
	ZynqMPPb pb;

	pb.start =  offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_DMA_BUFF + stream;
	pb.ptr = NULL;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_MEMZERO, &pb);
}


