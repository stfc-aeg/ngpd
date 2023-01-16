/*
 * poll_send_list.c
 *
 *  Created on: 09/11/2020
 *      Author: wih73
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "xspress3.h"
#include "xspress3test.h"
#include "xspress4_lib_driver.h"
#define RING_BUFF 256

typedef struct  send_list_status
{
	u_int32_t  desc_phys, cur_desc, tail_desc, status, control;
	u_int32_t readout_cur;
	u_int32_t dma_lock;
	u_int32_t hist_list_lock;
	u_int32_t readout_flags;
	u_int32_t last_tf;
} Xsp4SendListStatus;

int poll_send_list(int path)
{
	int i, j, l, rc;
	Xsp4SendListStatus status[RING_BUFF+1];	/* +1 allow last to be used for scalars */
	struct timespec delay;
	int n=0;
	int successive_locked=10;
	u_int32_t readout_mode, stream;
	i = 0;
	
	xsp3m_get_readout_mode(path, 0, &readout_mode);
	if (readout_mode & Xsp3mRd_SendHistList)
	{
		printf("Polling send activity for Hist List mode  mode printing first v3\n");
		stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
	}
	else
	{
		printf("Polling send activity for Hist Frames mode printing first v3\n");
		stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
	}
	if (test_poll_send > 0)
	{
		for (l=0; l<test_poll_send; l++)
		{
			rc = xsp4_dma_v7(path, XSP3_DMA_CMD_READ_SEND_LIST, stream, status+i, NULL, NULL);
			printf("HistCurDesc = 0x%08X = %d, Tail Dec=0x%08X=%d, Status=0x%08X, Control=%08X, readout_cur=%d, dma_lock=%d, sock_lock=%d, flags=%x, tf=0x%0X\n", status[i].cur_desc, (status[i].cur_desc-status[i].desc_phys)/sizeof(AXIDMADesc), 
							status[i].tail_desc, (status[i].tail_desc-status[i].desc_phys)/sizeof(AXIDMADesc), status[i].status, status[i].control, status[i].readout_cur, status[i].dma_lock, status[i].hist_list_lock, status[i].readout_flags, status[i].last_tf);

			rc = xsp4_dma_v7(path, XSP3_DMA_CMD_READ_SEND_LIST, XSP3_DMA_STREAM_BNUM_SCALERS, status+i, NULL, NULL);
			printf("ScalarsCurDesc = 0x%08X = %d, Tail Dec=0x%08X=%d, Status=0x%08X, Control=%08X, readout_cur=%d, dma_lock=%d, sock_lock=%d, flags=%x, tf=0x%0X\n", status[i].cur_desc, (status[i].cur_desc-status[i].desc_phys)/sizeof(AXIDMADesc), 
					status[i].tail_desc, (status[i].tail_desc-status[i].desc_phys)/sizeof(AXIDMADesc), status[i].status, status[i].control, status[i].readout_cur, status[i].dma_lock, status[i].hist_list_lock, status[i].readout_flags, status[i].last_tf);
		}
		delay.tv_sec = 0;
		delay.tv_nsec = 0.1 * 1E9;
		nanosleep(&delay, NULL);
		i = (i+1) % RING_BUFF;
	}
	else
	{
		while (1)
		{
			rc = xsp4_dma_v7(path, XSP3_DMA_CMD_READ_SEND_LIST, stream, status+i, NULL, NULL);
			if (status[i].hist_list_lock == 0)
			{
				n++;
				if (n == successive_locked)
				{
					for (j=-20; j <=0; j++)
					{
						int k;
						k = (i+RING_BUFF+j) % RING_BUFF;
						printf("********\n");
						printf("Prev HistCurDesc = 0x%08X = %d, Tail Desc=0x%08X=%d, Status=0x%08X, Control=%08X, readout_cur=%d, dma_lock=%d, sock_lock=%d, flags=%x, tf=0x%0X\n", status[k].cur_desc, (status[k].cur_desc-status[k].desc_phys)/sizeof(AXIDMADesc), 
									status[k].tail_desc, (status[k].tail_desc-status[k].desc_phys)/sizeof(AXIDMADesc), status[k].status, status[k].control, status[k].readout_cur, status[k].dma_lock, status[k].hist_list_lock, status[k].readout_flags, status[k].last_tf);
					}
				}
				else if (n > successive_locked)
				{
					printf("HistCurDesc = 0x%08X = %d, Tail Dec=0x%08X=%d, Status=0x%08X, Control=%08X, readout_cur=%d, dma_lock=%d, sock_lock=%d, flags=%x, tf=0x%0X\n", status[i].cur_desc, (status[i].cur_desc-status[i].desc_phys)/sizeof(AXIDMADesc), 
							status[i].tail_desc, (status[i].tail_desc-status[i].desc_phys)/sizeof(AXIDMADesc), status[i].status, status[i].control, status[i].readout_cur, status[i].dma_lock, status[i].hist_list_lock, status[i].readout_flags, status[i].last_tf);
					rc = xsp4_dma_v7(path, XSP3_DMA_CMD_READ_SEND_LIST, XSP3_DMA_STREAM_BNUM_SCALERS, status+RING_BUFF, NULL, NULL);
					printf("ScalarsCurDesc = 0x%08X = %d, Tail Dec=0x%08X=%d, Status=0x%08X, Control=%08X, readout_cur=%d, dma_lock=%d, sock_lock=%d, flags=%x, tf=0x%0X\n", 
						status[RING_BUFF].cur_desc, (status[RING_BUFF].cur_desc-status[RING_BUFF].desc_phys)/sizeof(AXIDMADesc), 
							status[RING_BUFF].tail_desc, (status[RING_BUFF].tail_desc-status[RING_BUFF].desc_phys)/sizeof(AXIDMADesc), status[RING_BUFF].status, status[RING_BUFF].control, status[RING_BUFF].readout_cur, status[RING_BUFF].dma_lock, 
							status[RING_BUFF].hist_list_lock, status[RING_BUFF].readout_flags, status[RING_BUFF].last_tf);
				}
			}
			else
				n = 0;
		}
		delay.tv_sec = 0;
		delay.tv_nsec = 0.1 * 1E9;
		nanosleep(&delay, NULL);
		i = (i+1) % RING_BUFF;
	}
	return 0;
}