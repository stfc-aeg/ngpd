/*
 * dma_control.h
 *
 *  Created on: 27 Mar 2015
 *      Author: wih73
 */

#ifndef DMA_CONTROL_H_
#define DMA_CONTROL_H_

void zynqmp_dma_init(DevStatics *dev_stat);
int zynqmp_dma_config_mem(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_config_memory(DevStatics *dev_stat, int layout);
int zynqmp_dma_reset(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_build_desc(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *num_desc);
int zynqmp_dma_stop(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *haltedP);
int zynqmp_dma_start(DevStatics *dev_stat, ZynqMPPb *param );
int zynqmp_dma_resend(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_read_status(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *rc2);
int zynqmp_dma_print_desc(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_check_rx_desc(DevStatics *dev_stat, ZynqMPPb *param, u_int32_t *num_complete);

int zynqmp_dma_build_test_pat(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_print_scope(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_print_data(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_build_debug_desc(DevStatics *dev_stat, ZynqMPPb *param);

int zynqmp_get_stream_num(int stream_mask);
int zynqmp_dma_reuse(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_check_rx_desc_circular(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_read_tf_status(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_read_markers(DevStatics *dev_stat, ZynqMPPb *param);

#define MSG_ERROR	0
#define MSG_QUIET	0
#define MSG_SPARSE	1
#define MSG_NORMAL	2
#define MSG_VERBOSE	3
#define DBGLEVEL(level, ...)		if (debug >= level) printk( KERN_WARNING __VA_ARGS__)

#endif /* DMA_CONTROL_H_ */
