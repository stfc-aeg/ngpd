/*
 * FPM_xspress3.c
 *
 *  Created on: Mar 16, 2012
 *      Author: wih73
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <errno.h>

#include "fem.h"
#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "commandProcessor.h"
#include "ngzmp_dma_protocol.h"
#include "zynqmp_lib_driver.h"

/**
 * Application specific hardware initialisation function, called by main FEM at end of hardwareInit()
 * @return xstatus, XST_SUCCESS if no errors
 */
int fpmInitHardware()
{
	DBGLEVEL(MSG_QUIET, "\n************************************\n");
	DBGLEVEL(MSG_QUIET, "Personality Module: XSPRESS4\n");
	DBGLEVEL(MSG_QUIET, "************************************\n\n");

	int status=0;

	return status;
}

/*
 * Validates the fields of a header for consistency
 * It is assumed that the header magic word has been verified and the fields are trusted to contain valid data.
 * @param pHeader pointer to header
 *
 * @return 0 if header logically correct, -1 if not
 */
int validatePersonalityHeaderContents(struct zynqmp_protocol_header *pHeader)
{
	// TODO: Implement
	return -1;
}

/*
 * Processes personality-specific received commands over LWIP.  It is assumed that
 * packets are well formed and have been checked before passing to this function.
 * @param pRxHeader pointer to zynqmp_protocol_header for received packet
 * @param pTxHeader pointer to zynqmp_protocol_header for outbound packet
 * @param pRxPayload pointer to payload buffer of received packet
 * @param pTxPayload pointer to payload buffer for outbound packet
 */
int32_t handlePersonalityCommand(	struct zynqmp_protocol_header* pRxHeader,
								struct zynqmp_protocol_header* pTxHeader,
								u_int8_t* pRxPayload,
								u_int8_t* pTxPayload
							)
{
	int i;					// General use variable
//	u_int32_t u;					// General Purpose u_int32_t
	int32_t retCode = 0;			// Return code, often number of requested operations performed, but not always. Use responseSize for read payload
	u_int8_t status = 0;			// Status byte
	u_int32_t* pTxPayload_32  = (u_int32_t*)pTxPayload;
	u_int32_t* pRxPayload_32  = (u_int32_t*)pRxPayload;
	int numRequestedOps=0;

	DBGLEVEL(MSG_NORMAL, "Personality Command: cmd=0x%02X, Sub command=%02X, Address=0x%08X\n", pRxHeader->command, pRxHeader->bus_target, pRxHeader->address);

	switch (pRxHeader->command)
	{
	case ZYNQMP_CMD_PERSONALITY_WRITE:
		numRequestedOps = pRxHeader->payload_sz/pRxHeader->data_width;
		break;

	case ZYNQMP_CMD_PERSONALITY_READ:
		numRequestedOps = pRxHeader->payload_sz/pRxHeader->data_width ;
		if (pRxHeader->payload_sz > ZYNQMP_FROM_FEM_BUFFER_MAX)
		{
			pTxHeader->payload_sz = 0;
			return ZYNQMP_STATUS_OVERFLOW;
		}
		break;

	case ZYNQMP_CMD_PERSONALITY_WRITE_EMB:
		numRequestedOps = pRxHeader->num_ops;
		pRxPayload = (u_int8_t *) (&(pRxHeader->payload_sz));
//		pRxPayload_32 = (u_int32_t *) (&(pRxHeader->payload_sz));
		// Currently not supportewd so
		pTxHeader->state = ZYNQMP_STATUS_ERROR;
		pTxHeader->payload_sz = 0;
		break;

	default:
		pTxHeader->state = ZYNQMP_STATUS_ERROR;
		pTxHeader->payload_sz = 0;
		return -1;
	}

	if (pRxHeader->bus_target == ZYNQMP_DMA_CMD_SET_MSG_SERVER)
	{
		if (pRxHeader->address > MSG_VERBOSE)
		{
			DBGLEVEL(MSG_ERROR, "[ERROR] message level %d is out of range of 0..%d\n", pRxHeader->address, MSG_VERBOSE);
			status = ZYNQMP_STATUS_ERROR;
			retCode = -1;
		}
		else
		{
			msg_level = pRxHeader->address;
			retCode = numRequestedOps;
		}
	}
	else if (pRxHeader->bus_target == ZYNQMP_DMA_CMD_LOAD_FILTER)
	{
		if (pRxHeader->command == ZYNQMP_CMD_PERSONALITY_WRITE)
		{
			i = ngpd_filter_load_integer(zynqmp_path, pRxHeader->address, pRxHeader->num_ops, (u_int32_t *)pRxPayload, &(pTxHeader->extra1));
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				DBGLEVEL(MSG_ERROR, "[ERROR] LOAD FILTER command to channel=%d returned error %d, status=0x%08X\n", pRxHeader->address, i, pTxHeader->extra1);
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
				retCode = 0;
		}
		else
		{
			DBGLEVEL(MSG_ERROR, "[ERROR] LOAD FILTER must be used with command ZYNQMP_CMD_PERSONALITY_WRITE\n");
			status = ZYNQMP_STATUS_ERROR;
			retCode = -1;
		}
	}
	else
	{
		/*
		 * Packet structure:
		 *
		 * magic = 0xDEADBEEF
		 * command = CMD_PERSONALITY
		 * bus_target = Sub Command
		 * data_width = WIDTH_LONG
		 * state = 0 (ignored)
		 * address =  Function mask
		 * payload =  Different XSP3_DMA_Mgs type for each command
		 *
		 */
		if (pRxHeader->command == ZYNQMP_CMD_PERSONALITY_READ)		// Note that For read use extra1 for stream, so that address can be incremented for read ahead applications
		{
			i = zynqmp_dma_read(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->extra1, pRxHeader->address, pRxHeader->num_ops, &(pTxHeader->extra2), &retCode, pTxPayload_32);
		}
		else
		{
			DBGLEVEL(MSG_NORMAL, "[INFO] zynqmp_dma (sub_command=%d, stream=%d, msg Size=%d (32 bit words)\n", pRxHeader->bus_target, pRxHeader->address, numRequestedOps);
			if (msg_level >= MSG_VERBOSE)
			{
				for (i=0; i<numRequestedOps; i++)
					printf("%d : 0x%08X=%d\n", i, pRxPayload_32[i], pRxPayload_32[i]);
			}
			i = zynqmp_dma(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->address, (void *)pRxPayload, &retCode, &(pTxHeader->extra1));
		}
		if (i < 0)
		{
			DBGLEVEL(MSG_ERROR, "[ERROR] DMA command %d to stream_mask=0x%04X returned error %d\n", pRxHeader->bus_target, pRxHeader->address, i);
			status = ZYNQMP_STATUS_ERROR;
			retCode = i;
		}
	}	
	pTxHeader->state = status;
	if (pRxHeader->command == ZYNQMP_CMD_PERSONALITY_READ)	// For read command specify the read paylaod size, fow write leave this field unchanged.
		pTxHeader->payload_sz = pRxHeader->payload_sz;

	DBGLEVEL(MSG_VERBOSE, "XSPRESS3: DMA command return %d=0x%08X, status=%d\n", retCode, retCode, pTxHeader->state);
	return retCode;
}
