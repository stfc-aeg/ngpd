/*
 * commandProcessor.h
 *
 * Manages the decoding of received packets and generation
 * of response packets using LwIP library
 *
 */

#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_


#include "fem.h"
#include "xspress4_protocol.h"
#if 0
#include "raw.h"
#include "rdma.h"
#include "i2c_lm82.h"
#include "i2c_lm75.h"
#include "i2c_24c08.h"
#include "mailbox.h"
#include "xspress3_spi.h"
#include "fan_control.h"
#endif

// WIH replacement, with more control:
#define GLOBAL_DEBUG 1

#ifdef  GLOBAL_DEBUG
	#define DBGLEVEL(level, ...)		if (msg_level >= level) printf(__VA_ARGS__)
#else
	#define DBGLEVEL(...)
#endif


extern int msg_level;
#define MSG_ERROR	0
#define MSG_QUIET	0
#define MSG_SPARSE	1
#define MSG_NORMAL	2
#define MSG_VERBOSE	3


enum packetStatus {
	STATE_START = 0,
	STATE_GOT_HEADER = 1,
	STATE_HDR_VALID = 2,
	STATE_GOT_PYLD = 3,
	STATE_COMPLETE = 4
};

struct clientStatus {
	int state;						//! State machine status, of packetStatus
	int hdr_bytes_read;				//! Number of header bytes received on current packet
	struct zynqmp_protocol_header *pHdr;	//! Pointer to protocol header buffer
	u_int8_t *pPayload;					//! Pointer to payload buffer
	int payloadBufferSz;			//! Currently allocated size for pPayload
	int payload_bytes_required;		//! Number of payload bytes required for the current command.
	int payload_bytes_read;			//! Number of payload bytes received on current packet
	int timeoutCount;				//! Counter for timeouts
	u_int8_t *pBusDirect;					//! Pointer for Bus Direct writes.
	int BusDirectSize;				//! Bytes Received in BusDirect mode
	int sequence;
};

void command_processor();
void disconnectClient(struct clientStatus* pState, int *pIndex, fd_set* pFdSet, int *pNumConnectedClients);
u_int32_t commandHandler(struct zynqmp_protocol_header* pRxHeader, struct zynqmp_protocol_header *pTxHeader, u_int8_t* pRxPayload, u_int8_t* pTxPayload);
int32_t handlePersonalityCommand(struct zynqmp_protocol_header* pRxHeader, struct zynqmp_protocol_header *pTxHeader, u_int8_t* pRxPayload, u_int8_t* pTxPayload);
int validateHeaderContents(struct zynqmp_protocol_header *pHeader);
void generateBadPacketResponse(struct zynqmp_protocol_header *pHeader, int clientSocket);

extern int zynqmp_path;
extern char *progname;

#endif /* COMMANDPROCESSOR_H_ */

