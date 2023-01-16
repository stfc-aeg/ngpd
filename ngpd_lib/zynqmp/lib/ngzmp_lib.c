/* 	Neutron Gamm Pulse Discriminatorin ZynqMP link library
	ngzmp specific parts to site alongside zynqmp_lib.c
	 W. Helsby  Daresbury Lab 24/2/2021

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <linux/fs.h>
#include <time.h>

#include "ngpd.h"
#include "ngzmp_lib_driver.h"
#include "ngzmp_driver.h"
#include "zynqmp_lib_driver.h"
#include "zynqmp_xadc.h"

static AXIHistConfig hist_config;
static int hist_config_read;
static size_t total_mem_words;
static size_t hist_words_per_stream;
static int axi_words_per_stream;
static int hist_words_per_axi_word;

char ngpd_error_message[NGPD_MAX_ERROR_MESSAGE + 2];
const char *axi_hist_cache_names[] = {"None", "Inc Cache single", "Inc Cache multiple", "Sorter", "Sorter with neighbour coalesce", "Unknown5", "Unknown6", "Unknown7",
 "Unknown8", "Unknown9", "Unknown10", "Unknown11", "Unknown12", "Unknown13", "Unknown14", "Unknown15" };
const char *axi_hist_read_write_names[] = { "None", "Reads wait for all writes sent", "Reads wait for all writes finished", "Buffer read data until all reads sent", "Buffer read data until all reads finished",
 "Unknown5", "Unknown6", "Unknown7", "Unknown8", "Unknown9", "Unknown10", "Unknown11", "Unknown12", "Unknown13", "Unknown14", "Unknown15" };

char *ngpd_get_error_message()
{
	ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
	return ngpd_error_message;
}
int ngzmp_config(int ncards, int num_tf, char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int create_module, char* modname, int debug, int card_index)
{
	return ngzmp_config_details(ncards, num_tf, baseIPaddress, basePort, baseMACaddress, num_chan, create_module, modname, debug, card_index, 1);
}

int ngzmp_config_details(int ncards, int num_tf, char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int create_module, char* modname, int debug, int card_index, int do_init)
{	
	int path;
	u_int32_t value;
	int version;
	int num_xadc;

	path =  open("/dev/ngpd", O_RDWR);
#if 0
	if (path >= 0)
	{
		xsp4_read(path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_REVISION, 1, &value);
		version = (value >> 24) & 0xFF;
		printf("Local Revision register=%08X=> hardware version=%d\n", value, version);
		if (version == 3 || version == 4)
		{
			u_int32_t status;
			xsp4_read(path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_C2C_CSR, 1, &status);
			if (!(status & XSP4_ZYNQ_LOCAL_C2C_GOOD))
			{
				if (version == 4)
				{
					xsp4_program_virtex7(path, 0, 0, &status);
				}

				printf("Link is not up, trying to reset\n");
				xsp4_reset_c2c(path, 0, &status);
				printf("After reset link status = %02X\n", status);
			}
			xsp4_generation = XspressGen4;
		}
		else
			xsp4_generation = XspressGen3Mini;

		num_xadc = xsp4_xadc_init(XSP4_XADC_INIT_READ_ONLY);
		if (xsp4_generation == XspressGen4 && num_xadc != 2)
			printf("Expected 2 XADCs in XSPRESS4, but found %d\n", num_xadc);

		if (xsp4_generation == XspressGen3Mini && num_xadc != 1)
			printf("Expected 1 XADCs in XSPRESS3Mini, but found %d\n", num_xadc);

		if (do_init)
		{
			printf("Calling xs3_dma_config_memory\n");
			xsp3_dma_config_memory(path, 0, 0);
			printf("Finished xs3_dma_config_memory\n");
		}
		else
		{
			printf("Omitted config mmemory\n");
		}
	}
#endif
	return path;
}

int ngzmp_hist_read_reg(int path, int card, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start = offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_HIST;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;
	return ioctl(path, ZYNQMP_READ, &pb);
}

int ngzmp_hist_write_reg(int path, int card, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start = offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_HIST;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;
	return ioctl(path, ZYNQMP_WRITE, &pb);
}


int ngzmp_hist_read_config(int path, int card, AXIHistConfig * config)
{
	int rc = 0;
	u_int32_t regs[4];

	rc = ngzmp_hist_read_reg(path, card, AXI_HIST_IDENT_REG, 4, regs);
	if (rc < 0 )
		return rc;

	if (regs[0] != AXI_HIST_IDENTIFIER)
	{
		printf("ngzmp_hist_read_config: Error reading identifier register: read 0x%08X, expedted 0x%08X\n", regs[0], AXI_HIST_IDENTIFIER);
		return -1;
	}
	config->NBitsDataHist	= 1 << AXI_HIST_CG0_NBITS_DATA(regs[1]);
	config->NBitsDataAXI	= 1 << AXI_HIST_CG0_NBITS_DATA_AXI(regs[1]);
	config->NumStreams		= AXI_HIST_CG0_NUM_STREAMS(regs[1]);
	config->Major 			= AXI_HIST_CG0_MAJOR(regs[1]);
	config->Minor 			= AXI_HIST_CG0_MINOR(regs[1]);
	config->NBitsAddrTopFixed = AXI_HIST_CG1_NBITS_ADDR_TOP(regs[2]);
	config->NBitsStream		= AXI_HIST_CG1_NBITS_STREAM(regs[2]);
	config->NBitsBankAndGroup= AXI_HIST_CG1_GET_NBITS_BANKANDGROUP(regs[2]);
	config->NBitsAddrMem	= AXI_HIST_CG1_GET_NBITS_ADDR(regs[2]);
	config->NBitsAddrIn		= AXI_HIST_CG1_GET_NBITS_ADDR_IN(regs[2]);
	config->StreamAtBottom  = AXI_HIST_CG1_GET_STREAM_AT_BOTTOM(regs[2]);
	config->HistWordsPerAXIWord = config->NBitsDataAXI/config->NBitsDataHist;
	config->TotalMemWords 	= (((u_int64_t)1)<<config->NBitsAddrMem)/(config->NBitsDataHist/8);
	config->HistWordsPerStream = config->TotalMemWords/config->NumStreams;
	config->AXIWordsPerStream = config->HistWordsPerStream/config->HistWordsPerAXIWord;
	config->NBitsSubStream		= AXI_HIST_CG2_GET_NBITS_SUBSTREAM(regs[3]);
	config->HistTPGPresent		= AXI_HIST_CG2_GET_TPG_PRESENT(regs[3]);
	config->Cache				= AXI_HIST_CG2_GET_CACHE(regs[3]);
	config->ReadWaitWrite		= AXI_HIST_CG2_GET_READWAITWRITE(regs[3]);

	hist_config = *config;
	hist_config_read = 1;
	total_mem_words = config->TotalMemWords;
	hist_words_per_stream = config->HistWordsPerStream;
	hist_words_per_axi_word = config->HistWordsPerAXIWord ;
	axi_words_per_stream = config->AXIWordsPerStream;

	return 0;
}

/**
	Clear region of histogram memory in a single stream (generally use all stream version)

@param path			Path to ngzmp device including access to histogrammer registers.
@param card			Unused on ZynqMP, allow consistent API with x86_64 version
@param stream		Stream Number 0.. NumStreams-1
@param start_word	First word to clear, measured within streams s aaddress psace, in AXI words.
@param num_words	Number of AXI word to clerar within stream.
@return   			0 on success or -1 on error
*/
int ngzmp_hist_clear_stream_start(int path, int card, int stream, int start_word, int num_words)
{
	u_int32_t cont_start_num[3];
	
	if (!hist_config_read)
	{
		AXIHistConfig dummy_config;
		ngzmp_hist_read_config(path, card, &dummy_config);
	}
	if (hist_config.StreamAtBottom)
	{
		start_word *= hist_config.NumStreams;
		start_word += stream;
	}
	else
	{
		start_word += stream * axi_words_per_stream;
	}
	cont_start_num[0] = 0;

	cont_start_num[1] = start_word;
	cont_start_num[2] = num_words;
	if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_stream_start: Error writing clear control registers path=%d", path);
		return -1;
	}
	return 0;
}
/**
	Start clear region of histogram memory all streams.

@param path			Path to ngzmp device including access to histogrammer registers.
@param card			Unused on ZynqMP, allow consistent API with x86_64 version
@param start_word	First word to clear, measured within streams address space, in AXI words.
@param num_words	Number of AXI word to clear within stream.
@return   			0 on success or -1 on error
*/

int ngzmp_hist_clear_all_start(int path, int card, int start_word, int num_words)
{
	u_int32_t cont_start_num[3];
	
	if (!hist_config_read)
	{
		AXIHistConfig dummy_config;
		ngzmp_hist_read_config(path, card, &dummy_config);
	}
	if (hist_config.StreamAtBottom)
	{
		start_word *= hist_config.NumStreams;
		num_words *= hist_config.NumStreams;
	}
	cont_start_num[0] = AXI_HIST_CC_CLEAR_ALL;

	cont_start_num[1] = start_word;
	cont_start_num[2] = num_words;
	if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Error writing clear control registers path=%d", path);
		return -1;
	}
	return 0;
}

int ngzmp_hist_clear_start(int path, int card, int start_word, int num_words, int all_streams)
{
	u_int32_t cont_start_num[3];
	
	cont_start_num[0] = 0;
	if (all_streams)
		cont_start_num[0] |= AXI_HIST_CC_CLEAR_ALL;

	cont_start_num[1] = start_word;
	cont_start_num[2] = num_words;
	if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start: Error writing clear control registers path=%d", path);
		return -1;
	}
	return 0;
}

int ngzmp_hist_clear_start_tpgen(int path, int card, int start_word, int num_words, int all_streams)
{
	u_int32_t cont_start_num[3];
	
	cont_start_num[0] = AXI_HIST_CC_USE_TPG;
	if (all_streams)
		cont_start_num[0] |= AXI_HIST_CC_CLEAR_ALL;

	cont_start_num[1] = start_word;
	cont_start_num[2] = num_words;
	if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start: Error writing clear control registers path=%d", path);
		return -1;
	}
	return 0;
}


int ngzmp_hist_clear_wait(int path, int card)
{
	struct timespec delay, remaining;
	u_int32_t status;
	int polls=0;

	delay.tv_sec = 0;
	delay.tv_nsec= 1E6;
	do
	{
		if (ngzmp_hist_read_reg(path, card, AXI_HIST_CLEAR_BUSY, 1, &status) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_wait: Error reading status from path=%d", path);
			return -1;
		}
		if (status == 0)
			break;
		remaining = delay;
		while (nanosleep(&remaining, &remaining) == EINTR);
		polls++;
		if (polls > 2000)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_wait: Timeout waiting for clear to finish, status=0x%08X", status);
			return -1;
		}
	} while (1);
	return 0;
}

int ngpd_read_chan_regs(int path, int chan, int region, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	if (chan < 0 || chan > 31)
	{
		fprintf(stderr, "ngpd_read_chan_regs: Invalid channel %d, expected 0..%d\n", chan, NGZMP_CHANS_PER_CARD-1);
		return -1;
	}
	if (region < 0 || region > 15)
	{
		fprintf(stderr, "ngpd_read_chan_regs: Invalid channel region %d, expected 0..15\n", chan);
		return -1;
	}
	pb.start = chan << (NGZMP_ADDR_SLICE_CHAN-2) | region << (NGZMP_ADDR_SLICE_REGION-2) | offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_REGS;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;
	return ioctl(path, ZYNQMP_READ, &pb);
}

int ngpd_write_chan_regs(int path, int chan, int region, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;
	int rc;

	if (chan < 0)
	{
		for (chan=0; chan<NGZMP_CHANS_PER_CARD; chan++)
		{
			rc = ngpd_write_chan_regs(path, chan, region, offset, size, value);
		}
		return rc;
	}
	else if (chan >= NGZMP_CHANS_PER_CARD)
	{
		fprintf(stderr, "ngpd_write_chan_regs: Invalid channel %d, expected 0..%d\n", chan, NGZMP_CHANS_PER_CARD-1);
		return -1;
	}
	if (region < 0 || region > 15)
	{
		fprintf(stderr, "ngpd_write_chan_regs: Invalid channel region %d, expected 0..15\n", chan);
		return -1;
	}
	pb.start = chan << (NGZMP_ADDR_SLICE_CHAN-2) | region << (NGZMP_ADDR_SLICE_REGION-2) | offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_REGS;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_WRITE, &pb);
}


int ngpd_write_glob_regs(int path, int card,  int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start = 1 << (NGZMP_ADDR_BIT_GLOB_REGS-2) | offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_REGS;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_WRITE, &pb);
}
int ngpd_read_glob_regs(int path, int card, int offset, int size, u_int32_t *value)
{
	ZynqMPPb pb;

	pb.start = 1 << (NGZMP_ADDR_BIT_GLOB_REGS-2) | offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_REGS;
	pb.ptr = value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_READ, &pb);
}

int ngpd_rmw_glob_regs(int path, int card,  int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value)
{
	ZynqMPPb pb;

	pb.start = 1 << (NGZMP_ADDR_BIT_GLOB_REGS-2) | offset;
	pb.num = 0;
	pb.addr_space = ZYNQMP_ADDR_SPACE_REGS;
	pb.and_mask = and_mask;
	pb.or_mask = or_mask;
	pb.ptr = ret_value;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_RMW, &pb);
}

int ngpd_filter_load_integer(int path, int chan, int num_words, u_int32_t *data, u_int32_t *status)
{
	ZynqMPPb pb;

	if (status != NULL)
		*status=0xFFFFFFFF;
	pb.start = chan;
	pb.num = num_words;
	pb.addr_space = 0;
	pb.ptr = data;
	pb.ptr2 = status;
	pb.ptr3 = NULL;
	
	return ioctl(path, NGZMP_LOAD_FILTER, &pb);
}


int ngpd_read_jesd_regs(int path, int card, int link, int offset, int num_regs, u_int32_t *data)
{
	ZynqMPPb pb;

	pb.start = (link&3) << 10  | offset;
	pb.num = num_regs;
	pb.addr_space = ZYNQMP_ADDR_SPACE_JESD;
	pb.ptr = data;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_READ, &pb);
}
int ngpd_write_jesd_regs(int path, int card, int link, int offset, int num_regs, u_int32_t *data)
{
	ZynqMPPb pb;

	pb.start = (link&3) << 10  | offset;
	pb.num = num_regs;
	pb.addr_space = ZYNQMP_ADDR_SPACE_JESD;
	pb.ptr = data;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;

	return ioctl(path, ZYNQMP_WRITE, &pb);
}

int ngzmp_dma_reset(int path, int card, u_int32_t function_mask)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_RESET, function_mask, NULL, NULL, NULL);
}
int ngzmp_dma_config_memory(int path, int card, int layout, int just_read, int force)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_CONFIG_MEMORY, layout, NULL, NULL, NULL);
}

int ngzmp_dma_build_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgBuildDesc *msg, int *num_desc)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_BUILD_DESC, stream, msg, num_desc, NULL);
}

int ngzmp_dma_build_debug_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgBuildDebugDesc *msg)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_BUILD_DEBUG_DESC, stream, msg, NULL, NULL);
}
int ngzmp_dma_start(int path, int card, u_int32_t stream_mask, ZYNQMP_DMA_MsgStart *msg)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_START, stream_mask, msg, NULL, NULL);
}
int ngzmp_dma_build_test_pat(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgTestPat *msg)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_BUILD_TEST_TEST_PAT, stream, msg, NULL, NULL);
}
int ngzmp_dma_print_data(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgPrint *msg)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_PRINT_DATA, stream, msg, NULL, NULL);
}
int ngzmp_dma_print_scope_data(int path, int card, ZYNQMP_DMA_MsgPrint *msg)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_PRINT_SCOPE, 0, msg, NULL, NULL);
}
int ngzmp_dma_print_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgPrintDesc *msg)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_PRINT_DESC, stream, msg, NULL, NULL);
}
int ngzmp_get_dma_status_block(int path, int card, ZYNQMP_DMA_StatusBlock *statusBlock)
{
	int i;
	ZYNQMP_DMA_StatusBlock sb4;

	i= zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&sb4);
	if (i < 0)
		return i;
	for (i=0; i<ZYNQMP_DMA_STREAM_NUM; i++)
	{
		statusBlock->status[i] = sb4.status[i];
		statusBlock->stream_def[i] = sb4.stream_def[i];
	}
	return 0;
}	

int	ngzmp_dma_check_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgCheckDesc *msg, int32_t *good_desc, u_int32_t *completed_desc, u_int32_t *last_frame, u_int32_t *status)
{
	int rc;
	u_int32_t details[3]= {0, 0, 0};
	rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_CHECK_RX_DESC, stream, msg, good_desc, details);
	if (rc >= 0)
	{
		if (completed_desc != NULL) *completed_desc = details[0];
		if (last_frame != NULL) *last_frame = details[1];
		if (status != NULL) *status = details[2];
	}
	return rc; 
}
/*
int ngzmp_dma_resend(int path, int card, u_int32_t stream, u_int32_t first, u_int32_t num)
{
	return zynqmp_dma(path, ZYNQMP_DMA_CMD_RESEND, 1<<stream, msg, NULL);
}
*/

int ngzmp_dma_read_status(int path, int card, u_int32_t stream_mask, int32_t *dma_status)
{
	return zynqmp_dma(path, card, ZYNQMP_DMA_CMD_READ_STATUS, stream_mask, NULL, dma_status, NULL);
}

char *ngzmp_get_error_message()
{
	return "";
}


int ngzmp_hist_read_dma(int path, int card, u_int32_t offset, u_int32_t size, u_int32_t *data, u_int32_t flags)
{
	int rc;
	u_int32_t first_axi_word, last_axi_word;
	u_int32_t axi_words_to_read;
	int buffer_axi_words;
	ZYNQMP_DMA_StatusBlock dma_status;
	ZYNQMP_DMA_MsgBuildDesc msg_build_desc;
	ZYNQMP_DMA_MsgStart msg_start;
	u_int32_t hist_control[3];
	u_int32_t buff_first_word, buff_num_words, bounding_word;
	int32_t dma_halted;

	if (!hist_config_read)
	{
		AXIHistConfig dummy_config;
		ngzmp_hist_read_config(path, card, &dummy_config);
	}

	if (offset+size > total_mem_words)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Region %u for %u does not fit in buffer %lu words", offset, size, total_mem_words);
		return -1;
	}
	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error reading DMA status block");
		return rc;
	}

	buffer_axi_words = dma_status.stream_def[NGZMP_DMA_STREAM_BNUM_HIST_READ].data_size/(hist_config.NBitsDataAXI/8);

	first_axi_word = offset/hist_words_per_axi_word;
	last_axi_word = (offset+size-1)/hist_words_per_axi_word;

	axi_words_to_read = 1+last_axi_word-first_axi_word;

	while (axi_words_to_read > 0)
	{
		u_int32_t num_axi_words;
		int32_t num_desc;
		if (axi_words_to_read > buffer_axi_words)
			num_axi_words = buffer_axi_words;
		else
			num_axi_words = axi_words_to_read;

		memset(&msg_build_desc, 0, sizeof(ZYNQMP_DMA_MsgBuildDesc));
		memset(&msg_start, 0, sizeof(ZYNQMP_DMA_MsgStart));
	
		msg_build_desc.src_stream = NGZMP_DMA_STREAM_BNUM_HIST_READ;
		msg_build_desc.addr = 0;
		msg_build_desc.size_bytes = ((u_int64_t)num_axi_words) * (hist_config.NBitsDataAXI/8);

		hist_control[0] = flags;
		hist_control[1] = first_axi_word;
		hist_control[2] = num_axi_words;
		
		if ((rc = zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_READ_CONT, 1, hist_control)) < 0)	// Write just the first word to stop the previous read and reset FIFO
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error writing to Hist control registers");
			return rc;
		}

		if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_STOP, 1<<NGZMP_DMA_STREAM_BNUM_HIST_READ, NULL, &dma_halted, NULL)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error stopping DMA");
			return rc;
		}
		if (dma_halted != (1<<NGZMP_DMA_STREAM_BNUM_HIST_READ))
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: DMA not halted DMA, halted mask=0x%08X", dma_halted);
			return -1;
		}

		if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_BUILD_DESC, NGZMP_DMA_STREAM_BNUM_HIST_READ, &msg_build_desc, &num_desc, NULL)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error reading building DMA descriptors start %ld, size %ld", msg_build_desc.addr, msg_build_desc.size_bytes);
			return rc;
		}

		if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_START, 1<<NGZMP_DMA_STREAM_BNUM_HIST_READ, &msg_start, NULL, NULL)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error start DMA");
			return rc;
		}

		if ((rc = zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_READ_CONT, 3, hist_control)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error writing to Hist control registers");
			return rc;
		}
	
		if ((rc = zynqmp_dma_wait(path, card, NGZMP_DMA_STREAM_BNUM_HIST_READ, 1.0)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error waiting for DMA to finish");
			return rc;
		}

		buff_first_word = offset-first_axi_word*hist_words_per_axi_word;
		bounding_word = (last_axi_word+1)*hist_words_per_axi_word;		// 1 word above the last available in buffer.
		buff_num_words = bounding_word-buff_first_word;
		if (buff_num_words > size)
			buff_num_words = size;
//		printf("offset=0x%X, Size=%X => first_axi_word=%X, num_axi_words=%X, buff_first_word=%X, buff_num_words=%X, data=%p\n", offset, size, first_axi_word, num_axi_words, buff_first_word, buff_num_words, data);
		if ((rc = zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_HIST_READ, buff_first_word, buff_num_words, data)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error reading DMA buffer from %u for %u", buff_first_word, buff_num_words);
			return rc;
		}
		data += buff_num_words;
		size -= buff_num_words;
		first_axi_word += num_axi_words;
		axi_words_to_read -= num_axi_words;
	}
	return 0;
}


int ngzmp_hist_read_discard(int path, int card, u_int32_t offset, u_int32_t size)
{
	int rc;
	u_int32_t first_axi_word, last_axi_word;
	u_int32_t axi_words_to_read;
	int buffer_axi_words;
	ZYNQMP_DMA_StatusBlock dma_status;
	u_int32_t hist_control[3], hist_status;
	u_int32_t buff_first_word, buff_num_words, bounding_word;
	double timeout=1.0;
	struct timeval start, now;
	struct timespec delay, remaining;
	double t;

	if (!hist_config_read)
	{
		AXIHistConfig dummy_config;
		ngzmp_hist_read_config(path, card, &dummy_config);
	}

	if (offset+size > total_mem_words)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_discard: Region %u for %u does not fit in buffer %lu words", offset, size, total_mem_words);
		return -1;
	}
	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_discard: Error reading DMA status block");
		return rc;
	}

	buffer_axi_words = dma_status.stream_def[NGZMP_DMA_STREAM_BNUM_HIST_READ].data_size/(hist_config.NBitsDataAXI/8);

	first_axi_word = offset/hist_words_per_axi_word;
	last_axi_word = (offset+size-1)/hist_words_per_axi_word;

	axi_words_to_read = 1+last_axi_word-first_axi_word;

	while (axi_words_to_read > 0)
	{
		u_int32_t num_axi_words;
		if (axi_words_to_read > buffer_axi_words)
			num_axi_words = buffer_axi_words;
		else
			num_axi_words = axi_words_to_read;

		hist_control[0] = AXI_HIST_RC_DISCARD_RD;
		hist_control[1] = first_axi_word;
		hist_control[2] = num_axi_words;
		if ((rc = zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_READ_CONT, 3, hist_control)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error writing to Hist control registers");
			return rc;
		}
	
		gettimeofday(&start, NULL);
		delay.tv_sec = 0;
		delay.tv_nsec=1E6;
		do
		{
			if ((rc = zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TEST_STATUS, 1, &hist_status)) < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_discard: Error reading to Hist status registers");
				return rc;
			}
			if ((hist_status & AXI_HIST_TSTATUS_READ_BUSY) == 0)
				break;
			gettimeofday(&now, NULL);
			t = now.tv_sec-start.tv_sec;
			t += 1E-6*(now.tv_usec-start.tv_usec);

			remaining = delay;
			while (nanosleep(&remaining, &remaining) == EINTR);
		} while (t < timeout);


		buff_first_word = offset-first_axi_word*hist_words_per_axi_word;
		bounding_word = (last_axi_word+1)*hist_words_per_axi_word;		// 1 word above the last available in buffer.
		buff_num_words = bounding_word-buff_first_word;
		size -= buff_num_words;
		first_axi_word += num_axi_words;
		axi_words_to_read -= num_axi_words;
	}
	return 0;
}


int zynqmp_dma_wait(int path, int card, int stream, double timeout)
{
	int rc;
	struct timeval start, now;
	struct timespec delay, remaining;
	double t;
	int32_t dma_status;

	gettimeofday(&start, NULL);
	delay.tv_sec = 0;
	delay.tv_nsec=1E6;
	do
	{
		if ((rc=ngzmp_dma_read_status(path, card, 1<<stream, &dma_status)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "zynqmp_dma_wait: Error reading DMA status  from  stream=%d", stream);
			return rc;
		}
		gettimeofday(&now, NULL);
		t = now.tv_sec-start.tv_sec;
		t += 1E-6*(now.tv_usec-start.tv_usec);

		if (dma_status & 2)
			break;
		remaining = delay;
		while (nanosleep(&remaining, &remaining) == EINTR);

	} while (t < timeout);

	if (dma_status & 2)
		return 0;
	
	snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "zynqmp_dma_wait: Timeout waiting for DMA stream %d to finish, status=0x%08X", stream, dma_status);
	return -1;
}

int ngpd_scope_setup_stream(int path, int card, int stream, int chan_sel, int src_sel, int alt)
{
	u_int32_t regs[6];
	int rc;
	int i;

	card = 0;
	if ((rc=ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 6, regs)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Error Reading scope registers");
		return -1;
	}
	i = stream/8;
	regs[0+i] &= ~NGZMP_SCOPE_CHAN_SEL_SET(stream, 255);
	regs[0+i] |=  NGZMP_SCOPE_CHAN_SEL_SET(stream, chan_sel);
	regs[2+i] &= ~NGZMP_SCOPE_SRC_SEL_SET(stream, 255);
	regs[2+i] |=  NGZMP_SCOPE_SRC_SEL_SET(stream, src_sel);
	regs[4+i] &= ~NGZMP_SCOPE_ALTERNATE_SET(stream, 255);
	regs[4+i] |=  NGZMP_SCOPE_ALTERNATE_SET(stream, alt);

/*	for (i=0; i<6; i++)
		printf("ngpd_scope_setup_stream: scope_regs[%d]=0x%08X\n", i, regs[i]);
*/
	if ((rc=ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 6, regs)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Error writing scope registers");
		return -1;
	}
	return 0;
}


/**
 * @ingroup debug
 * Setup scope mode options.
 *
 * For debug purposes, the system provides the capability to stream multiple 16 bits streams to memory per card, recording raw ADC data or data from various points through the processing.
 * Usually the scope mode recording starts as soon as the run bit is asserted. However, to synchronise across cards, 
 * it is possible to make scope mode wait until the Veto (Count enb) signal is asserted.
 * This can be routed from board to board using to be determined

 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        where this value has been passed to {@link ngpd_config()} at ngpd system configuration.
 *        If card is less than 0 then all cards are selected.
 * @param options Scope mode options defined by Xsp3ScopeOptions
 * @return 0 or a negative error code.
 */
int ngpd_set_scope_options(int path, int card, NGPDScopeOptions *options)
{
	int first_card, last_card;
	u_int32_t scope_cont;
	int layout;
	int rc;

	if ((rc=ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Error Reading scope control register");
		return NGPD_ERROR;
	}
	if (options->nstreams > 0)
	{
		switch (options->nstreams)
		{
		case 4:
			scope_cont &= ~NGZMP_SCOPE_CONT_NUM_STREAMS(0xFF);
			scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(0);
			layout = NGZMP_SCOPE_LAYOUT_4;
			break;

		case 8:
			scope_cont &= ~NGZMP_SCOPE_CONT_NUM_STREAMS(0xFF);
			scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(1);
			layout = NGZMP_SCOPE_LAYOUT_8;
			break;

		case 10:
			scope_cont &= ~NGZMP_SCOPE_CONT_NUM_STREAMS(0xFF);
			scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(2);
			layout = NGZMP_SCOPE_LAYOUT_10;
			break;

		default:
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Unsupported number of streams %d, currently 4, 8 and 10 for ZynqMP", options->nstreams);
			return NGPD_ERROR;
		}
	}
	if ((rc=ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont)) != 0)
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Error Writing scope control register rc=%d: %s", rc, old_msg);
		ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
		return rc;
	}
	return 0;
}
/* Note that if use_dma_reset is set, the reading memory will also reset the hist_test DMA, so stop any tets pattern being sent.
Do not use for concurrent read with test DMA */
int ngzmp_hist_read_stream_dma(int path, int card, int stream, u_int32_t offset, u_int32_t size, u_int32_t *data, int use_dma_reset)
{
	int rc;
	u_int32_t first_axi_word, last_axi_word;
	u_int32_t axi_words_to_read;
	int buffer_axi_words;
	ZYNQMP_DMA_StatusBlock dma_status;
	ZYNQMP_DMA_MsgBuildDesc msg_build_desc;
	ZYNQMP_DMA_MsgStart msg_start;
	u_int32_t hist_control[3];
	u_int32_t buff_first_word, buff_num_words, bounding_word;
	int32_t dma_halted;

	if (!hist_config_read)
	{
		AXIHistConfig dummy_config;
		ngzmp_hist_read_config(path, card, &dummy_config);
	}

	if (offset+size > hist_words_per_stream)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Region %u for %u does not fit in buffer %lu words", offset, size, hist_words_per_stream);
		return -1;
	}
	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error reading DMA status block");
		return rc;
	}

	buffer_axi_words = dma_status.stream_def[NGZMP_DMA_STREAM_BNUM_HIST_READ].data_size/(hist_config.NBitsDataAXI/8);

	first_axi_word = offset/hist_words_per_axi_word;
	last_axi_word = (offset+size-1)/hist_words_per_axi_word;

	axi_words_to_read = 1+last_axi_word-first_axi_word;

	while (axi_words_to_read > 0)
	{
		u_int32_t num_axi_words;
		int32_t num_desc;
		if (axi_words_to_read > buffer_axi_words)
			num_axi_words = buffer_axi_words;
		else
			num_axi_words = axi_words_to_read;

		memset(&msg_build_desc, 0, sizeof(ZYNQMP_DMA_MsgBuildDesc));
		memset(&msg_start, 0, sizeof(ZYNQMP_DMA_MsgStart));
	
		msg_build_desc.src_stream = NGZMP_DMA_STREAM_BNUM_HIST_READ;
		msg_build_desc.addr = 0;
		msg_build_desc.size_bytes = ((u_int64_t)num_axi_words) * (hist_config.NBitsDataAXI/8);

		hist_control[0] = 0;
		if (hist_config.StreamAtBottom)
			hist_control[1] = first_axi_word*hist_config.NumStreams+stream;
		else
			hist_control[1] = first_axi_word+axi_words_per_stream*stream;

		hist_control[2] = num_axi_words;
//		printf("ngzmp_hist_read_stream_dma: stream=%d, Start AXI word=%08X, Num AXI Words=%08X, DMA desc addr=%08lX, DMA desc size=%08lX\n", stream, hist_control[1], hist_control[2], msg_build_desc.addr, msg_build_desc.size_bytes);
//		printf("Press return to stop Read State machine\n") ; getchar();
		if ((rc = zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_READ_CONT, 1, hist_control)) < 0)	// Write just the first word to stop the previous read and reset FIFO
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error writing to Hist control registers");
			return rc;
		}

//		printf("Press return to stop DMA\n") ; getchar();
		if (use_dma_reset)
		{
			if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_RESET, 1<<NGZMP_DMA_STREAM_BNUM_HIST_READ, NULL, NULL, NULL)) < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error resetting DMA");
				return rc;
			}
		}
		else
		{
			if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_STOP, 1<<NGZMP_DMA_STREAM_BNUM_HIST_READ, NULL, &dma_halted, NULL)) < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error stopping DMA");
				return rc;
			}
			if (dma_halted != (1<<NGZMP_DMA_STREAM_BNUM_HIST_READ))
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: DMA not halted DMA, halted mask=0x%08X", dma_halted);
				return -1;
			}
		}

		if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_BUILD_DESC, NGZMP_DMA_STREAM_BNUM_HIST_READ, &msg_build_desc, &num_desc, NULL)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error reading building DMA descriptors start %ld, size %ld", msg_build_desc.addr, msg_build_desc.size_bytes);
			return rc;
		}

//		printf("Press return to start DMA on stream %d\n", stream) ; getchar();
		if ((rc = zynqmp_dma(path, card, ZYNQMP_DMA_CMD_START, 1<<NGZMP_DMA_STREAM_BNUM_HIST_READ, &msg_start, NULL, NULL)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error start DMA");
			return rc;
		}

//		printf("Press return to start Read state machine\n" ); getchar();
		if ((rc = zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_READ_CONT, 3, hist_control)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error writing to Hist control registers");
			return rc;
		}
	
		if ((rc = zynqmp_dma_wait(path, card, NGZMP_DMA_STREAM_BNUM_HIST_READ, 1.0)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error waiting for DMA to finish");
			return rc;
		}

		buff_first_word = offset-first_axi_word*hist_words_per_axi_word;
		bounding_word = (last_axi_word+1)*hist_words_per_axi_word;		// 1 word above the last available in buffer.
		buff_num_words = bounding_word-buff_first_word;
		if (buff_num_words > size)
			buff_num_words = size;
//		printf("offset=0x%X, Size=%X => first_axi_word=%X, num_axi_words=%X, buff_first_word=%X, buff_num_words=%X, data=%p\n", offset, size, first_axi_word, num_axi_words, buff_first_word, buff_num_words, data);
		if ((rc = zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_HIST_READ, buff_first_word, buff_num_words, data)) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_dma: Error reading DMA buffer from %u for %u", buff_first_word, buff_num_words);
			return rc;
		}
		data += buff_num_words;
		size -= buff_num_words;
		first_axi_word += num_axi_words;
		axi_words_to_read -= num_axi_words;
	}
	return 0;
}

int ngzmp_hist_read_stream(int path, int card, int stream, u_int32_t offset, u_int32_t size, u_int32_t *data)
{
	ZynqMPPb pb;
	u_int32_t word_offset;

	if (!hist_config_read)
	{
		AXIHistConfig dummy_config;
		ngzmp_hist_read_config(path, card, &dummy_config);
	}

	if (offset+size > hist_words_per_stream)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: Region %u for %u does not fit in buffer %lu words", offset, size, hist_words_per_stream);
		return -1;
	}


	if (hist_config.StreamAtBottom)
	{
		word_offset = offset % hist_words_per_axi_word;
		offset -= word_offset;
		offset *= hist_config.NumStreams;
		offset += stream*hist_words_per_axi_word+word_offset;
	}
	else
	{
		offset += stream * axi_words_per_stream*hist_words_per_axi_word;
	}
	pb.start = offset;
	pb.num = size;
	pb.addr_space = ZYNQMP_ADDR_SPACE_PL_RAM;
	pb.ptr = data;
	pb.ptr2 = NULL;
	pb.ptr3 = NULL;
	pb.row_len = hist_words_per_axi_word;
	pb.row_stride = hist_words_per_axi_word*hist_config.NumStreams;
//	printf("ngzmp_hist_read_stream: start=0x%08X, row_ren=0x%02X, row_stride=0x%02X\n", pb.start, pb.row_len, pb.row_stride);
	if (hist_config.StreamAtBottom)
		return ioctl(path, ZYNQMP_READ_ROW, &pb);
	else
		return ioctl(path, ZYNQMP_READ, &pb);
}

int ngzmp_hist_read_row(int path, int card, int row_len, int num_streams, int row, u_int32_t offset, u_int32_t size, u_int32_t *data)
{
	return zynqmp_read_row(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, row_len, num_streams, row, offset, size, data);
}

