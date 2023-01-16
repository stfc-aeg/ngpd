/*
 * hist_speed.c
 *
 *  Created on: 12/09/2012
 *      Author: wih
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "datamod.h"
#include "xspress3.h"
#include "xspress3_data_mod.h"
#include "xspress3test.h"
#include "errors.h"

#define HIST_PACKET_SIZE 8000    // UDP datagram size
extern XSP3Path Xsp3Sys[];

int hist_speed_test(int path, int nbits);
u_int32_t * build_hist_test_pattern(int nwords, int nbits);

int hist_speed_tests(int path) {
#if 0
	int i;
//	int nbits[3] = {12, 16, 22};
	mprintf(1, "HIST-SPEED\n");
	for (i=0;i<3;i++)
	{
		hist_speed_test(path, nbits[i]);
	}
#else
	int nbits;

	for (nbits = 12; nbits <= 26; nbits++)
		hist_speed_test(path, nbits);
#endif
	return 0;
}

int hist_speed_test(int path, int nbits) {
	int nbytes;
	int nwords;
	u_int32_t * test_pattern;
	u_int32_t *ptr;
	int i;
	int packet_gap;
	int rc = 0;
	int card = 0;
//	int errors=0;
	int io_errors = 0;
	int nbits_eng = 12;
	u_int32_t timea;
	u_int32_t stream_mask = XSP3_DMA_STREAM_MASK_PLAYBACK | XSP3_DMA_STREAM_MASK_SCOPE0 | XSP3_DMA_STREAM_MASK_SCOPE1
			| XSP3_DMA_STREAM_MASK_SCALERS | XSP3_DMA_STREAM_MASK_HIST_TO_DRAM | XSP3_DMA_STREAM_MASK_DRAM_TO_10G
			| XSP3_DMA_STREAM_MASK_10G_TO_DRAM;

	u_int32_t reg15;
	XSP3_DMA_MsgBuildDesc msgBuildDesc;
	struct timeval start, send_done, all_done;
//	int num_frames;
	int total;
	XSP3_DMA_StatusBlock statusBlock;
	int chan;
	u_int32_t status;
	FILE *filep;
	double send_time, hist_time;
	char fname[100];
	int dropped;
	XSP3_DMA_MsgStart msgStart;

	memset((void *)&msgStart, 0, sizeof(msgStart));

	sprintf(fname, "hist_times_nbits%d.dat", nbits);
	if ((filep = fopen(fname, "w")) == NULL) {
		printf("Cannot open output file '%s'\n", fname);
		return 1;
	}

	rc = xsp3_get_dma_status_block(path, card, &statusBlock);
	if (rc != 0) {
		printf("hist_speed_test:Error reading DMA status block: %s\n", xsp3_get_error_message());
		num_io_errors++;
		return -1;
	}

	nbytes = statusBlock.stream_def[XSP3_DMA_STREAM_BNUM_PLAYBACK].data_size;
	printf("Found scope data size=%d bytes\n", nbytes);
	nwords = nbytes / 8;
	nwords *= 2;
	nbytes = nwords * 4;

	/* Build test pattern */
	test_pattern = build_hist_test_pattern(nwords, nbits);

#if 1
	rc = xsp3_write_data_10g(path, card, test_pattern, XSP3_DMA_STREAM_BNUM_PLAYBACK, 0, nbytes, 0);
	if (rc < 0) {
		mprintf(MP_ERREX, "Writing test pattern failed: %s\n", xsp3_get_error_message());
		return 1;
	}
#endif
	rc = xsp3_format_run(path, -1, XSP3_FORMAT_AUX1_MODE_NONE, 0, XSP3_FORMAT_NBITS_AUX4, 0, XSP3_FORMAT_AUX2_ADC,
			nbits_eng);

	for (packet_gap = 0x8000; packet_gap > 0x0; packet_gap >>= 1) {
		mprintf(1, "Hist speed with packet gap = %d\n", packet_gap);
		if ((rc = xsp3_histogram_clear(path, -1, -1, -1, -1)) < 0) {
			mprintf(MP_ERR, "Error clearing memory: %s\n", xsp3_get_error_message());
			io_errors++;
		}

		rc = xsp3_read_glob_reg(path, card, XSP3_GLOB_TIMING_A, 1, &timea);
		if (rc != 0) {
			printf("error: xsp3_read_glob_reg(): returned %d\n", rc);
			printf("%s\n", xsp3_get_error_message());
			num_io_errors++;
			return -1;
		}
		// Clear the run bit and write global timing register
		timea &= ~XSP3_GLOB_TIMA_RUN;
		rc = xsp3_write_glob_reg(path, card, XSP3_GLOB_TIMING_A, 1, &timea);
		if (rc != 0) {
			printf("hist_speed_test: xsp3_write_glob_reg(): returned %d\n", rc);
			printf("%s\n", xsp3_get_error_message());
			num_io_errors++;
			return -1;
		}
		// setup data path, but hold in reset
		u_int32_t dataPath = XSP3_DPC_SEL_10G_TX_DMA | XSP3_DPC_RX_RESET | XSP3_DPC_TX_RESET;
		rc = xsp3_write_glob_reg(path, card, XSP3_GLOB_DATA_PATH_CONT, 1, &dataPath);
		if (rc != 0) {
			printf("hist_speed_test: xsp3_write_glob_reg(): returned %d\n", rc);
			printf("%s\n", xsp3_get_error_message());
			num_io_errors++;
		}
		// reset all dma channels
		rc = xsp3_dma_reset(path, card, stream_mask);
		if (rc != 0) {
			printf("hist_speed_test: xsp3_data_transfer(): returned %d\n", rc);
			printf("%s\n", xsp3_get_error_message());
			num_io_errors++;
			return -1;
		}

		u_int32_t reg13 = (u_int32_t) packet_gap;
		if (xsp3_write_rdma_reg(path, 0, 13, 1, &reg13) != 0) {
			printf("hist_speed_test: Setting inter packet gap Error writing RDMA reg 13: %s\n",
					xsp3_get_error_message());
			num_io_errors++;
			return -1;
		}
#if 1
		if (xsp3_read_rdma_reg(path, 0, 15, 1, &reg15) != 0) {
			printf("hist_speed_test: Error reading RDMA reg 15\n");
			num_io_errors++;
			return -1;
		}
		reg15 &= ~(1 << 4); // Turn off Force zero UDP Checksum

		if (xsp3_write_rdma_reg(path, 0, 15, 1, &reg15) != 0) {
			printf("hist_speed_test:Error writing RDMA reg 15\n");
			num_io_errors++;
			return -1;
		}
#endif
		// setup data path, releasing reset
		dataPath = XSP3_DPC_SEL_10G_TX_DMA;
		rc = xsp3_write_glob_reg(path, card, XSP3_GLOB_DATA_PATH_CONT, 1, &dataPath);
		if (rc != 0) {
			printf("hist_speed_test:Error writing Global data path control reg\n");
			num_io_errors++;
			return -1;
		}
		rc = xspress3FemSetHostPort(Xsp3Sys[path].femHandle, Xsp3Sys[path].histogram[0].udpsock.hostPort);
		if (rc != 0) {
			printf("hist_speed_test:Error Setting Host Port\n");
			num_io_errors++;
			return -1;
		}
		memset((void *) &msgBuildDesc, 0, sizeof(XSP3_DMA_MsgBuildDesc));
		msgBuildDesc.src_stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		msgBuildDesc.addr = 0; // 0 will not offset data
		msgBuildDesc.size_bytes = 0; // 0 will use default size
		rc = xsp3_dma_build_desc(path, card, XSP3_DMA_STREAM_BNUM_DRAM_TO_10G, &msgBuildDesc);
		if (rc != 0) {
			printf("hist_speed_test:Error building descriptors\n");
			num_io_errors++;
			return -1;
		}
		rc = xsp3_get_dma_status_block(path, card, &statusBlock);
		if (rc != 0) {
			printf("hist_speed_test:Error reading DMA status block: %s\n", xsp3_get_error_message());
			num_io_errors++;
			return -1;
		}

//		num_frames =  statusBlock.stream_def[XSP3_DMA_STREAM_BNUM_DRAM_TO_10G].num_frames;
		Xsp3Sys[path].histogram[0].reset_frame = 1;

		rc = xsp3_dma_start(path, card, XSP3_DMA_STREAM_BNUM_DRAM_TO_10G, &msgStart);
		if (rc != 0) {
			printf("hist_speed_test:Error Starting DMA: %s\n", xsp3_get_error_message());
			num_io_errors++;
		}

		gettimeofday(&start, NULL);
		do {
			status = xsp3_dma_read_status(path, card, 1 << XSP3_DMA_STREAM_BNUM_DRAM_TO_10G);
			rc = xsp3_get_dma_status_block(path, card, &statusBlock);
			printf("Status from xsp3_dma_read_status =%08X, from status block = %08X\n", status,
					statusBlock.status[XSP3_DMA_STREAM_BNUM_DRAM_TO_10G]);
		} while (statusBlock.status[XSP3_DMA_STREAM_BNUM_DRAM_TO_10G] & 2);
		gettimeofday(&send_done, NULL);

		while (xsp3_histogram_is_busy(path, card * 8))
			printf("Hist busy\n");

		gettimeofday(&all_done, NULL);
		total = 0;
		for (chan = 0; chan < 4; chan++) {
			ptr = Xsp3Sys[path].histogram[chan].buffer;
			for (i = 0; i < Xsp3Sys[path].histogram[chan].bufsiz; i++)
				total += *ptr++;
		}
		dropped = xsp3_histogram_get_dropped(path, 0);
		send_time = send_done.tv_sec - start.tv_sec + 1E-6 * (send_done.tv_usec - start.tv_usec);
		hist_time = all_done.tv_sec - start.tv_sec + 1E-6 * (all_done.tv_usec - start.tv_usec);
		printf("Sent %d count, found %d, total time = %g, rate=%g\n", nwords, total, hist_time, total / hist_time);
		fprintf(filep, "%d\t%d\t%d\t%g\t%g\t%d\n", packet_gap, nwords, total, send_time, hist_time, dropped);
	}
	fclose(filep);
	return 0;
}

u_int32_t * build_hist_test_pattern(int nwords, int nbits) {
	u_int32_t * test_pattern, mask;
	u_int32_t *ptr, x;
	int i, j, chan;

	if ((test_pattern = (u_int32_t *) malloc(nwords * sizeof(u_int32_t))) == NULL) {
		mprintf(MP_ERREX, "No memory\n");
		exit(1);
	}
	if (nbits == 32)
		mask = 0xFFFFFFFF;
	else
		mask = (1 << nbits) - 1;

	ptr = test_pattern;

	for (i = 0; i < nwords;) {
		chan = rand() & 3;
		chan <<= 28;
		for (j = 0; j < HIST_PACKET_SIZE / 4 && i < nwords; j++, i++) {
			if (nbits > 15)
				x = (rand() << 15) | (rand() & 0x7FFF);
			else
				x = rand() & 0x7FFF;
			*ptr++ = (x & mask) | chan;
		}
	}
	return test_pattern;
}
