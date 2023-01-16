/*
 * zynqmp_api.c
 *
 *  Created on: 16/6/2021
 *      Author: wih73
 * Derived from xspress3Api.c by grm84
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#ifdef _MSC_VER
#if _MSV_VER>=1600 
#include <stdint.h>
#else
	typedef __int8              int8_t;
	typedef __int16             int16_t;
	typedef __int32             int32_t;
	typedef __int64             int64_t;
	typedef unsigned __int8     uint8_t;
	typedef unsigned __int16    uint16_t;
	typedef unsigned __int32    uint32_t;
	typedef unsigned __int64    uint64_t;
#endif
typedef uint8_t u_int8;
typedef uint16_t u_int16;
typedef uint32_t u_int32;
typedef int32_t   int32;
typedef int16_t   int16;
typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
#endif

#if defined(__MINGW32__) | defined (__MINGW64__) | defined(_MSC_VER)
#include <winsock2.h>

#define SHUT_RDWR SD_BOTH
static int endprotoent() 
{
}
static int endhostent() 
{
}
#define socket_close closesocket
#undef EINTR
#define EINTR WSAEINTR
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINVAL
#define EINVAL WSA_INVALID_PARAMETER
#undef EBADF
#define EBADF WSAEBADF
#undef ECONNRESET
#define ECONNRESET WSAECONNRESET
#undef ECONNABORTED
#define ECONNABORTED WSAECONNABORTED
#undef ENOTCONN
#define ENOTCONN WSAENOTCONN
#undef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#define sock_errno WSAGetLastError ()
#else
#define sock_errno errno
#define socket_close close
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif

#include <pthread.h>
#include "zynqmp_api.h"
#include "zynqmp_protocol.h"

#define SBIT(val,bit)		val |=  (1 << (bit-1))
#define CMPBIT(val, bit)	val &   (1 << (bit-1))

#define ZYNQMP_OK			0
#define ZYNQMP_ERROR		(-1)
#define ZYNQMP_WOULD_BLOCK	(-20)

#define MAGIC 				0xdeadbeef

#define IP_IDENT_COUNT 		0xDB00
#define IP_PKT_LENGTH_BASE 	0x1c
#define UDP_LENGTH_BASE 	0x0008
#define PACKET_SPLIT_SIZE 	0x3e6
#define INT_PKT_GAP_VAL 	0xA // Seems to work down to 1 or even 0, but 10 is a 1% overheadwith 1000 word (8000 byte) packets
								// But Virtex-5 could not put packets back to back in scope mode due to lack of DMA bandwith, was actually about 1000
								// Virtex-7 can make back to back packets .. and the T310 card needs about a 500 cycle gap, set a layer higher at the momnet.
#define INT_PKT_GAP_EN 	0x1		
#define DEBUG_MODE_EN 	0x2
#define DEBUG_MODE_STEP 0x4
#define FXD_PKT_SZE 	0x8

// Packet header
typedef struct _protocol_header
{
	u_int32_t magic;			// Always 0xDEADBEEF
	u_int8_t  command;
	u_int8_t  bus_target;
	u_int8_t  data_width;
	u_int8_t  state;
	u_int32_t address;
	u_int32_t payload_sz;
} ProtocolHeader;

// Supported commands (v2)
enum protocol_commands
{
	CMD_UNSUPPORTED = 0,
	CMD_ACCESS      = 1,
	CMD_INTERNAL	= 2,
	CMD_ACQUIRE		= 3,
	CMD_PERSONALITY	= 4
};

enum protocol_bus_type
{
	BUS_UNSUPPORTED = 0,
	BUS_EEPROM      = 1,
	BUS_I2C         = 2,
	BUS_RAW_REG     = 3,	//!< V5P memory-mapped peripherals
	BUS_RDMA        = 4,	//!< Downstream configuration
	BUS_SPI			= 5,	//!< SPI bus
	BUS_DIRECT		= 6,	//!< Direct memory write
	BUS_FAN_CONT    = 7,	//!< Access to the fan control internal memory.
	BUS_FEM_CONFIG  = 8		//!< Access to the FEM Config
};

// Size of data
enum protocol_data_width
{
    WIDTH_UNSUPPORTED = 0,
    WIDTH_BYTE        = 1,	// 8bit
    WIDTH_WORD        = 2,	// 16bit
    WIDTH_LONG        = 3	// 32bit
};

// Status bit bank
enum protocol_status
{
	STATE_UNSUPPORTED = 0,
	STATE_READ        = 1,
	STATE_WRITE       = 2,
	STATE_ACK         = 6,
	STATE_NACK        = 7
};
#define ZYNQMP_MAX_ERROR_MESSAGE 200
static char error_message[ZYNQMP_MAX_ERROR_MESSAGE+2];

// forward declaration of internal functions
static int32_t sendTransaction(int fd, u_int8_t* transaction, u_int32_t bytes_in, int non_blocking, u_int32_t * remaining); 
static unsigned char* getMacAddressFromIP(char *ipName);
static void to_bytes(char *ipName, unsigned char* b, int n);


static void encodeWrTransaction2(u_int8_t* transaction, u_int32_t seq, u_int8_t cmd, u_int8_t bus, u_int8_t width, u_int32_t address, u_int32_t num_ops, u_int8_t* payload, u_int32_t payloadSize); 
static int32_t responseTransaction2(int fd, u_int8_t* transaction, u_int32_t bytes) ;
static void encodeRdTransaction2(u_int8_t* transaction, u_int32_t seq, u_int8_t cmd, u_int8_t bus, u_int8_t width, u_int32_t address, u_int32_t num_ops, u_int32_t payloadSize); 
static int32_t responsePayload2(int fd, u_int8_t *payload, u_int32_t bytes);

//static u_int32_t base_address = 

// The FEM app can now resize its buffer up to 1Meg.
// Still need to work in chunks if I am uploading a 256 MByte test pattern.
#if 1
//#define MAX_TX_PAYLOAD_LWORDS 	(16*1024)
//#define MAX_RX_PAYLOAD_LWORDS 	255

#else
//#define MAX_TX_PAYLOAD_LWORDS 	500
//#define MAX_RX_PAYLOAD_LWORDS 	255

#endif
/**
 * @ingroup internal
 * Initialises client connection to a FEM.
 * @param hostname string representation of FEM IP address in dotted quad format
 * @param fem1_port integer port number to connect to a FEM using original FEM1 protocol
 * @param fem2_port integer port number to connect to a FEM using modified FEM2 protocol
 * @return a handle for connected FEM (Or NULL on Error)
 */
ZynqMPHandle* zynqmp_initialise(const char* hostname, int port)
{
	struct hostent *host;
	struct protoent *protocol;
	struct sockaddr_in remote_addr;
	int opt = 1;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) malloc(sizeof(ZynqMPHandle));
	memset(zynqmp, 0, sizeof(ZynqMPHandle));
	if (zynqmp == NULL) 
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_initialise: Can't allocate ZynqMPHandle");
		return NULL;
	}
	if ((host = gethostbyname(hostname)) == 0) 
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_initialise: Can't get gethostbyname");
		endhostent();
		goto exit_error;
	} 
	if ((zynqmp->m_skt = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_initialise: Can't create socket");
		endhostent();
		goto exit_error;
	}
	remote_addr.sin_family = host->h_addrtype;
	size_t len = host->h_length;
	memcpy(&remote_addr.sin_addr.s_addr, host->h_addr, len);
	endhostent();
	remote_addr.sin_port = htons(port);
	if (connect(zynqmp->m_skt, (struct sockaddr *) &remote_addr, sizeof(struct sockaddr_in)) >= 0) 
	{
		zynqmp->m_valid = 1;
		zynqmp->protocol_generation = ZynqMPGen1;
		zynqmp->max_tx_bytes = ZYNQMP_TO_FEM_BUFFER_BYTES;
		zynqmp->max_rx_bytes = ZYNQMP_FROM_FEM_BUFFER_MAX;
	}
	else
	{
		socket_close(zynqmp->m_skt);
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Connection to server refused. Is the server running?");
		goto exit_error;
	}
	protocol = getprotobyname("tcp");
	if (protocol == 0)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Cannot get protocol TCP\n");
		zynqmp->m_valid = 0;
		goto exit_error;
	} 
	if (setsockopt(zynqmp->m_skt, protocol->p_proto, TCP_NODELAY, (char *) &opt, 4) < 0) 
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Cannot Set socket options");
		zynqmp->m_valid = 0;
		endprotoent();
		goto exit_error;
	}

	endprotoent();


	pthread_mutex_init(&(zynqmp->mutex), NULL);
	return zynqmp;

exit_error:
	free(zynqmp);
	return NULL;

}

/** 
 * @ingroup internal
 * Closes the xspress3 client connection to a FEM/
 *
 * @param femHandle an instance of the connected FEM
 */
void  zynqmp_close(ZynqMPHandle* zynqmp)
{
	shutdown (zynqmp->m_skt,  SHUT_RDWR);
	socket_close(zynqmp->m_skt);
	zynqmp->m_skt = -1;
	zynqmp->m_valid = 0;
	free (zynqmp);
}

#if 0


int xspress3FemWriteDMAorClk(void* femHandle, int offset, size_t num_writes, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Direct access to DMA peripherals not supported on FEM 1");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_DMA, ZYNQMP_WIDTH_LONG, offset, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}

int xspress3FemReadDMAorClk(void* femHandle, int offset, size_t num_reads, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Direct access to DMA peripherals not supported on FEM 1");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_DMA, ZYNQMP_WIDTH_LONG, offset, num_reads, 1, (u_int8_t *)value);
	}
	return -1;
}

int xspress3FemReadDMABuff(void* femHandle, int dma_stream, u_int32_t address, size_t num_reads, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "DMA buffer access is not supported on FEM1, but perhaps you need xspress3FemReadRawReg");
		return -1;
	}

	return fem2AccessRead(zynqmp, ZYNQMP_BUS_DMA_BUFF+dma_stream, ZYNQMP_WIDTH_LONG, address, num_reads, 1, (u_int8_t *)value);
}
int xspress3FemReadDMABuffRoI(void* femHandle, int dma_stream, u_int32_t address, u_int32_t row_length_ops, u_int32_t num_rows, u_int32_t src_stride_ops, u_int32_t dst_stride_ops, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "DMA buffer access is not supported on FEM1, but perhaps you need xspress3FemReadRawReg");
		return -1;
	}

	return fem2AccessReadRoI(zynqmp, ZYNQMP_BUS_DMA_BUFF+dma_stream, ZYNQMP_WIDTH_LONG, address, row_length_ops, num_rows, src_stride_ops, dst_stride_ops, 1, (u_int8_t*) value);
}

int xspress3FemWriteDMABuff(void* femHandle, int dma_stream, u_int32_t address, size_t num_writes, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "DMA buffer access is not supported on FEM1, but perhaps you need xspress3FemReadRawReg");
		return -1;
	}

	return fem2AccessWrite(zynqmp, ZYNQMP_BUS_DMA_BUFF+dma_stream, ZYNQMP_WIDTH_LONG, address, num_writes, 1, (u_int8_t *)value);
}


/** 
 * @ingroup internal
 * Perform a single integer write transaction to the FEM-II MGT DRP regions
 *
 * This function performs a write transaction to the FEM-II into the Multi Gigabit Transceivers used for board to board comms in XSPRESS4. 
 * The address is described as a link number and word offset from the start of this link.
 * The function writes 1 off 32 bit int.
 *
 * @param femHandle an instance of the connected FEM
 * @param link 	the borad to board link number, currently 0..5
 * @param offset the offset from link start in lwords
 * @param value a single 32 bit integer value to write
 * @return 0 if success
 */
int xspress3FemSetIntMGTDRP(void* femHandle, int link, int offset, u_int32_t value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "MGT DRP access is not supported on FEM1.");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		u_int32_t address =  ((link & 0x7) << 10) | (offset);
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_MGT_DRP, ZYNQMP_WIDTH_LONG, address, 1, 1, (u_int8_t *)&value);
	}
	return -1;
}
/** 
 * @ingroup internal
 * Write an array of integers to the FEM-II MGT DRP regions
 *
 * This function performs a write transaction to the FEM-II into the Multi Gigabit Transceivers used for board to board comms in XSPRESS4. 
 * The address is described as a link number amd word offset from the start of this link.
 * The function writes for size off 32 bit ints.
 *
 *
 * @param femHandle an instance of the connected FEM
 * @param link 	the borad to board link number, currently 0..5
 * @param offset the offset from link start in lwords
 * @param num_writes Number of 32 bit ints to write
 * @param value Pointer to an array of  32 bit integer values to write
 * @return 0 if success
 */

int xspress3FemSetIntArrayMGTDRP(void* femHandle, int link, int offset, size_t num_writes, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "MGT DRP access is not supported on FEM1.");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		u_int32_t address =  ((link & 0x7) << 10) | (offset);
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_MGT_DRP, ZYNQMP_WIDTH_LONG, address, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}
/** 
 * @ingroup internal
 * Read a single integers from the FEM-II MGT DRP regions
 *
 * This function performs an read transaction to the FEM-II into the Multi Gigabit Transceivers used for board to board comms in XSPRESS4. 
 * The address is described as a link number amd word offset from the start of this link.
 * This function reads 1 off 32 bit int.
 *
 *
 * @param femHandle 	An instance of the connected FEM
 * @param link 			The board to board link number, currently 0..5
 * @param offset 		The offset from link start in lwords
 * @param value 		Pointer to a 32 bit integer to return the int.
 * @return 0 if success
 */

int xspress3FemGetIntMGTDRP(void* femHandle, int link, int offset, u_int32_t *value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "MGT DRP access is not supported on FEM1.");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		u_int32_t address =  ((link & 0x7) << 10) | (offset);
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_MGT_DRP, ZYNQMP_WIDTH_LONG, address, 1, 1, (u_int8_t *)value);
	}
	return -1;
}

/** 
 * @ingroup internal
 * Read an array of integers from the FEM-II MGT DRP regions
 *
 * This function performs a read transaction to the FEM-II into the Multi Gigabit Transceivers used for board to board comms in XSPRESS4. 
 * The address is described as a link number amd word offset from the start of this link.
 * The function reads for size off 32 bit ints.
 *
 *
 * @param femHandle 	An instance of the connected FEM
 * @param link 			The board to board link number, currently 0..5
 * @param offset 		The offset from link start in lwords
 * @param num_reads 	Number of 32 bit ints to read
 * @param value 		Pointer to an array of  32 bit integer values to read into.
 * @return 0 if success
 */
int xspress3FemGetIntArrayMGTDRP(void* femHandle, int link, int offset, size_t num_reads, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "MGT DRP access is not supported on FEM1.");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		u_int32_t address =  ((link & 0x7) << 10) | (offset);
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_MGT_DRP, ZYNQMP_WIDTH_LONG, address, num_reads, 1, (u_int8_t *)value);
	}
	return -1;
}

int xspress3FemWriteDriverStatics(void* femHandle, int offset, size_t num_writes, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Write to Driver statics is not supported on FEM 1");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_READOUT_MODE, ZYNQMP_WIDTH_LONG, offset, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}

int xspress3FemReadDriverStatics(void* femHandle, int offset, size_t num_reads, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Write to Driver statics is not supported on FEM 1");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_READOUT_MODE, ZYNQMP_WIDTH_LONG, offset, num_reads, 1, (u_int8_t *)value);
	}
	return -1;
}




int32_t xspress3FemConfigUDP(void* femHandle, char* fpgaMACaddress, char* fpgaIPaddress, int fpgaPort, char* hostIPaddress, int hostPort) {
	int rc = ZYNQMP_OK;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	unsigned char* hostMAC = getMacAddressFromIP(hostIPaddress);
	unsigned char fpgaMAC[6];
	unsigned char fpgaIP[4];
	unsigned char hostIP[4];
	u_int32_t address;
	u_int32_t value;

	if (hostMAC == NULL) {
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Unable to get MAC address from Host IPaddress=%s", hostIPaddress);
		return ZYNQMP_ERROR;
	}
	to_bytes(fpgaMACaddress, fpgaMAC, 6);
	to_bytes(fpgaIPaddress, fpgaIP, 4);
	to_bytes(hostIPaddress, hostIP, 4);
	 // UDP Block 0 MAC Source Lower 32
	address = 0x0;
	value = (fpgaMAC[3] << 24) + (fpgaMAC[2] << 16) + (fpgaMAC[1] << 8) + fpgaMAC[0];
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}
	// UDP Block 0 MAC Source Upper 16/Dest Lower 16
	address = 0x1;
	value = (hostMAC[1] << 24) + (hostMAC[0] << 16) + (fpgaMAC[5] << 8) + (fpgaMAC[4]);
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}
	// UDP Block 0 MAC Dest Upper 32
	address = 0x2;
	value = (hostMAC[5] << 24) + (hostMAC[4] << 16) + (hostMAC[3] << 8) + hostMAC[2];
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}
	free(hostMAC);
	// UDP Block 0 IP Ident / Header Length
	address = 0x4;
	value = (IP_IDENT_COUNT << 16) + IP_PKT_LENGTH_BASE;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	// UDP Block 0 IP Dest Addr / Checksum
	address = 0x6;
	value = (hostIP[1] << 24) + (hostIP[0] << 16) + (0xDE << 8) + 0xAD;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	// UDP Block 0 IP Src Addr / Dest Add
	address = 0x7;
	value = (fpgaIP[1] << 24) + (fpgaIP[0] << 16) + (hostIP[3] << 8) + hostIP[2];
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	// UDP Block 0 IP Src Port / Src Addr
	address = 0x8;
	value = ((fpgaPort & 0xff) << 24) + ((fpgaPort & 0xff00) << 8) + (fpgaIP[3] << 8) + fpgaIP[2];
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	// UDP Block 0 UDP Length / Dest Port
	address = 0x9;
	value = (UDP_LENGTH_BASE << 16) + ((hostPort & 0xff) << 8) + (hostPort >> 8);
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	address = 0xB;
//	value = 0xFF<<8;	// Turn off all input filtering
	value = 0xFF<<8;	// Turn off all input filtering
//	value = 0;			// Turn on all input filtering
	value = 0xB3 << 8;		// Turn on filtering on UDP protocol and ip_vhl_ipv4_5 and Destination port.

	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}

	// UDP Block 0 Packet Size
	address = 0xC;
	value = PACKET_SPLIT_SIZE;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	// UDP Block 0 IFG Value
	address = 0xD;
	value = INT_PKT_GAP_VAL;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	// Tx and RX Pol to suit FEM hardware
	address = 0xE;
	value = 0xAA;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}

	// UDP Block 0 IFG Enable
	address = 0xF;
	value = INT_PKT_GAP_EN;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		return rc;
	}
	return rc;

}


int xspress3FemSetHostPort(void* femHandle, int hostPort) {
	int rc = ZYNQMP_OK;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	u_int32_t value;
	u_int32_t address = 0x9;
	u_int32_t safe;
	if ((rc = xspress3FemRDMARead(zynqmp, address, 1, &value)) >= 0) {
		value &= 0xffff0000;
		value |=  ((hostPort & 0xff) << 8) + (hostPort >> 8);
		safe = value;
		if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) >= 0) {
			u_int32_t value2;
			if ((rc = xspress3FemRDMARead(zynqmp, address, 1, &value2)) >= 0) {
				if (safe != value2) {
					snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Failed to set FEM host port to %d\n", hostPort);
					rc = -1;
				}
			}
		}
	}
	return rc;
}

int32_t xspress3FemSetFarmLUT(void* femHandle, int index, char* hostIPaddress, int hostPort) {
	int rc = ZYNQMP_OK;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	unsigned char* hostMAC = getMacAddressFromIP(hostIPaddress);
	unsigned char hostIP[4];
	u_int32_t address;
	u_int32_t value;

	if (index <0 || index > 255)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "xspress3FemSetFarmLUT: index %d is out of range 0..255", index);
		return ZYNQMP_ERROR;
	}

	if (hostMAC == NULL) {
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Unable to get MAC address from Host IPaddress=%s", hostIPaddress);
		return ZYNQMP_ERROR;
	}
	to_bytes(hostIPaddress, hostIP, 4);

	// Host Port
	// UDP Block 0 UDP Length / Dest Port
	address = 0x10000+index;
	value = hostPort;
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}
	printf("Setting UDP LUT[%d].HostPort=%d\n", index, hostPort);	
	// Host IP
	address = 0x10100+index;
	value = (hostIP[0] << 24) + (hostIP[1] << 16) + (hostIP[2] << 8) + (hostIP[3] << 0);
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}

	// UDP Block 0 MAC Source Upper 16/Dest Lower 16
	address = 0x10200+2*index;
	value = (hostMAC[2] << 24) + (hostMAC[3] << 16) + (hostMAC[4] << 8) + (hostMAC[5]);
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}

	address = 0x10200+2*index+1;
	value = (hostMAC[0] << 8) + (hostMAC[1]);
	if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) < 0) {
		free(hostMAC);
		return rc;
	}
	free(hostMAC);
	return 0;

}

int xspress3FemSetPacketSize(void* femHandle, int sizeInBytes) {
	int rc = ZYNQMP_OK;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	u_int32_t address = 0xC;
	u_int32_t value;
	if ((rc = xspress3FemRDMARead(zynqmp, address, 1, &value)) >= 0) {
		value &= 0xFFFF0000;
		value |= sizeInBytes/8-2;
		rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value);
	}
	return rc;

}

int xspress3FemResetFrameCounter(void* femHandle) {
	int rc = ZYNQMP_OK;
	u_int32_t value;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	u_int32_t address = 0xF;
	
	if (zynqmp == NULL || zynqmp->m_valid == 0)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		if ((rc = xspress3FemRDMARead(zynqmp, address, 1, &value)) >= 0) {
			value |= 1 << 31;
			if ((rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value)) >= 0) {
				value &= ~(1 << 31);
				rc = xspress3FemRDMAWrite(zynqmp, address, 1, &value);
			}
		}
	}
	else
	{
		 xspress3FemRDMARMW(zynqmp,  address, 0xFFFFFFFF, 1<<31, NULL);
		 rc = xspress3FemRDMARMW(zynqmp,  address, 0x7FFFFFFF, 0, NULL);
	}
	return rc;
}

/* This command allows write to the DRMA over the 1 GBits/ link and is not used in production code.
	For XSPRESS3 the address is a physical byte address.
	For XSPRESS4 it is a word offset in to the mapped area of DRAM 
*/

int xspress3FemDRAMWrite(void* femHandle, u_int32_t address, size_t num_writes, u_int32_t* value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL || zynqmp->m_valid == 0)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessWrite(zynqmp, BUS_DIRECT, WIDTH_LONG, address, num_writes, sizeof(u_int32_t), (u_int8_t*) value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_RDRAM, ZYNQMP_WIDTH_LONG, address, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}

/**
 * @ingroup internal 
 * Perform a write transaction to the FEM-1 or FEM-2 personality bus
 *
 * @param femHandle  	Pointer to femHandle returned by {@link xspress3FemInitialise}
 * @param sub_command  	Command to pass through FEM personality route to the FEM-1 PPC1 or Zynq device driver.
 * @param stream_mask 	Mask to tell FEM-1 PPC1 or FYNQ device driver which DMA stream to perform command on.
 * @param size 			Size of the different ZYNQMP_DMA_Mgs type structures being used in lwords.
 * @param value 		Pointer to the message to be passed to FEM-1 PPC1 or ZYNQ.
 * @param num_details	Number of Lwords of additional return data in XSPRESS3Mini adn XSPRESS4 (Zynq) only.
 * @param details		Pointer to return additional return data in XSPRESS3Mini adn XSPRESS4 (Zynq) only.
 * @return  Single word return code from FEM-1 PPC or ZYNQ driver or negative error code.
 * bus_target = Sub Command
 * data_width = WIDTH_LONG
 * state = 0 (ignored)  -- Suggest set Write bit so in future can consider read command.
 * address =  Function mask
 * payload =  Different ZYNQMP_DMA_Mgs type for each command
 */
int xspress3FemPersonalityWrite(void* femHandle, u_int32_t sub_command, u_int32_t stream_mask, size_t size, u_int32_t* value, int num_details, u_int32_t *details) 
{
	int rc = ZYNQMP_OK;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	// Issue the write transaction
	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
		rc = femPersonalityWrite(zynqmp, sub_command, WIDTH_LONG, stream_mask, size , (u_int8_t*) value);
	else
		rc = fem2PersonalityWrite(zynqmp, sub_command, ZYNQMP_WIDTH_LONG, stream_mask, size , (u_int8_t*) value, num_details, details);
	return rc;
}


/**
 * @ingroup internal 
 * Perform a read transaction to the FEM-2 personality bus
 *
 * @param femHandle  	Pointer to femHandle returned by {@link xspress3FemInitialise}
 * @param sub_command  	Command to pass through FEM personality route to the Zynq device driver.
 * @param stream_mask 	Mask to tell  ZYNQ device driver which DMA stream to perform command on.
 * @param width_bytes	Width of each operation in bytes
 * @param first			Offset to first read operation if this is supported
 * @param num_ops 		Number of read operations to perform
 * @param value 		Pointer to the message to be passed to FEM-1 PPC1 or ZYNQ.
 * @return  			0 on success or negative error code.
 */
int xspress3FemPersonalityRead(void* femHandle, u_int32_t sub_command, u_int32_t stream_mask, u_int32_t width_bytes, u_int32_t first, u_int32_t num_ops, void* value) 
{
	int rc = ZYNQMP_OK;
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	// Issue the write transaction
	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "xspress3FemPersonalityRead is not supported on FEM1.");
		return -1;
	}
	else
		rc = fem2PersonalityRead(zynqmp, sub_command, width_bytes, first, num_ops, 1, (u_int8_t*) value, stream_mask, 0, 0);

	return rc;
}


int xspress3FemGetDMAStatusBlock(void* femHandle, size_t size, u_int32_t *value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	u_int32_t address = 0x8A000000;
	u_int32_t num_reads = size / sizeof(u_int32_t);
	if (zynqmp == NULL)
		return -1;
	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessRead(zynqmp, BUS_RAW_REG, WIDTH_LONG, address, num_reads, sizeof(u_int32_t), (u_int8_t *)value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_DMA_STATUS, ZYNQMP_WIDTH_LONG, 0, num_reads, 1, (u_int8_t *)value);
	}
	return -1;
}

int xspress3FemRDMARead(void* femHandle, u_int32_t address, size_t num_reads, u_int32_t *value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessRead(zynqmp, BUS_RDMA, WIDTH_LONG, address, num_reads, 1, (u_int8_t *)value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_RDMA, ZYNQMP_WIDTH_LONG, address, num_reads, 1, (u_int8_t *)value);
	}
	return -1;
}

int xspress3FemRDMAWrite(void* femHandle, u_int32_t address, size_t num_writes, u_int32_t *value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessWrite(zynqmp, BUS_RDMA, WIDTH_LONG, address, num_writes, 1, (u_int8_t*) value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_RDMA, ZYNQMP_WIDTH_LONG, address, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}

/** 
 * @ingroup internal
 * Perform a read modify (and, or) write to a single location in the FEM RDMA space.
 *
 * This function performs an RMW transaction to the FEM into the xspress3 register space, at an address caclulated from the channel number, region number within channel and offset within region
 * specified in the arguments for 1 off 32 bit int.
 *
 * @param femHandle an instance of the connected FEM
 * @param address Address offst in words within the RDMA space.
 * @param and_mask Value to be anded with the data. Set bit to 0 to clear, otherwise all 1s
 * @param or_mask Value to be ored with the data. Set bit to 1 to set, otherwise all 0s
 * @param ret_value Pointer to return the 32 bit value calculated and written back, NULL to omit return.
 * @return 0 if success
 */
int xspress3FemRDMARMW(void* femHandle, int address, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;
	u_int32_t value;
	int rc;
	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		rc = femAccessRead(zynqmp, BUS_RDMA, WIDTH_LONG, address, 1, sizeof(u_int32_t), (u_int8_t *)&value);
		if (rc < 0) return rc;
		value = (value & and_mask) | or_mask;
		if (ret_value != NULL)
			*ret_value = value;
		return femAccessWrite(zynqmp, BUS_RDMA, WIDTH_LONG, address, 1, sizeof(u_int32_t), (u_int8_t *)&value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessReadModifyWrite(zynqmp, ZYNQMP_BUS_RDMA, ZYNQMP_WIDTH_LONG, address, and_mask, or_mask, ret_value);
	}
	return -1;
}


int xspress3FemWriteFanControl(void* femHandle, int offset, size_t num_writes, u_int32_t* value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessWrite(zynqmp, BUS_FAN_CONT, WIDTH_LONG, offset, num_writes, 1, (u_int8_t*) value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_FAN_CONT, ZYNQMP_WIDTH_LONG, offset, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}

int xspress3FemReadFanControl(void* femHandle, u_int32_t offset, size_t num_reads, u_int32_t* value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessRead(zynqmp, BUS_FAN_CONT, WIDTH_LONG, offset, num_reads, 1, (u_int8_t*) value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_FAN_CONT, ZYNQMP_WIDTH_LONG, offset, num_reads, 1, (u_int8_t*) value);
	}
	return -1;
}

int xspress3FemConfigWrite(void* femHandle, u_int32_t address, size_t num_writes, u_int8_t *value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		return femAccessWrite(zynqmp, BUS_FEM_CONFIG, WIDTH_BYTE, address, num_writes, 1, (u_int8_t*) value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessWrite(zynqmp, ZYNQMP_BUS_FEM_CONFIG, ZYNQMP_WIDTH_BYTE, address, num_writes, 1, (u_int8_t*) value);
	}
	return -1;
}

int xspress3FemConfigRead(void* femHandle, u_int32_t address, size_t num_reads, u_int8_t *value)
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		printf("xspress3FemConfigRead : Calling femAccessRead\n");
		return femAccessRead(zynqmp, BUS_FEM_CONFIG, WIDTH_BYTE, address, num_reads, 1, (u_int8_t*) value);
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_FEM_CONFIG, ZYNQMP_WIDTH_BYTE, address, num_reads, 1, (u_int8_t*) value);
	}
	return -1;
}


int xspress3FemXADCRead(void* femHandle, u_int32_t address, size_t size, u_int32_t *value) 
{
	ZynqMPHandle* zynqmp = (ZynqMPHandle*) femHandle;

	if (zynqmp == NULL)
		return -1;
	// Build a payload array from the parameters passed

	if (zynqmp->protocol_generation == ProtocolFEM1)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "xspress3FemXADCRead:is not supported on FEM1.");
		return -1;
	}
	else if (zynqmp->protocol_generation == ProtocolFEM2)
	{
		return fem2AccessRead(zynqmp, ZYNQMP_BUS_XADC, ZYNQMP_WIDTH_LONG, address, size, 1, (u_int8_t *)value);
	}
	return -1;
}


#endif

static int32_t sendTransaction(int fd, u_int8_t* transaction, u_int32_t bytes_in, int non_blocking, u_int32_t * remaining) 
{
	int rc;
	u_int32_t bytes = bytes_in;
	
#if defined(__MINGW32__) || defined(_MSC_VER)
	u_long nb = non_blocking;
	ioctlsocket(fd, FIONBIO, &nb);
#endif

	while (bytes > 0)
	{
#if defined(__MINGW32__) || defined(_MSC_VER)
		while ((rc = send(fd, (const void*) transaction, bytes, 0)) < 0 && sock_errno == EINTR);
#else
		while ((rc = send(fd, (const void*) transaction, bytes, non_blocking?MSG_DONTWAIT:0)) < 0 && sock_errno == EINTR);
#endif		

		if (rc < 0 && (sock_errno == EAGAIN || sock_errno == 	EWOULDBLOCK))
		{
			if (remaining != NULL)
				*remaining = bytes;
#if defined(__MINGW32__) || defined(_MSC_VER)
			nb = 0;
			ioctlsocket(fd, FIONBIO, &nb);
#endif
			return ZYNQMP_WOULD_BLOCK;
		}
		if (rc < 0) 
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "sendTransaction: Sending transaction to server failed %d errno: %d", rc, sock_errno);
#if defined(__MINGW32__) || defined(_MSC_VER)
			nb = 0;
			ioctlsocket(fd, FIONBIO, &nb);
#endif
			return ZYNQMP_ERROR;
		}
		bytes -= rc;
		transaction += rc;
	} 
	if (remaining != NULL)
		*remaining = 0;
#if defined(__MINGW32__) || defined(_MSC_VER)
	nb = 0;
	ioctlsocket(fd, FIONBIO, &nb);
#endif
	return rc;
}


static void to_bytes(char *ipName, unsigned char* b, int n) {
	char *end;
	char* iptr = ipName;
	int i;
	for (i=0; i<n; i++) {
		b[i] = (unsigned char) strtol(iptr, &end, 10);
		iptr = end + 1;
	}
}

#if 0
/** 
 * @ingroup internal
 * Get the MAC address corresponding to the given IP address
 *
 * @param ipName byte array containing the IP address
 * @return the mac address of the interface as a byte array or NULL if not found
 */
static unsigned char* getMacAddressFromIP(char *ipName) {

	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];
	struct sockaddr *sdl;
	unsigned char *ptr;
	char *ifa_name = NULL;
	unsigned char* mac_addr = (unsigned char*) calloc(sizeof(unsigned char), 6);

	if (getifaddrs(&ifaddr) == -1) {
		printf("getifaddrs returns error\n");
		return NULL;
	}

	//iterate to find interface name for given server_ip
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr != NULL) {
			family = ifa->ifa_addr->sa_family;
			if (family == AF_INET) {
				s = getnameinfo(ifa->ifa_addr,
						(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host,
						NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
				if (s != 0) {
					return NULL;
				}
				if (strcmp(host, ipName) == 0) {
					ifa_name = ifa->ifa_name;
				}
			}
		}
	}
	if (ifa_name == NULL) {
		return NULL;
	}
	int i;
	//iterate to find corresponding MAC address
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		family = ifa->ifa_addr->sa_family;
		if (family == PF_PACKET && strcmp(ifa_name, ifa->ifa_name) == 0) {
			sdl = (struct sockaddr *) (ifa->ifa_addr);
			ptr = (unsigned char *) sdl->sa_data;
			ptr += 10;
			for (i=0; i<6; i++) {
				mac_addr[i] = *ptr++;
			}
			break;
		}
	}
	freeifaddrs(ifaddr);
	return mac_addr;
}
#endif


/**
 * @ingroup internal
 * Execute a write transaction on the connected ZynqMP
 *
 * @param zynqmp  Pointer to ZynqMPHandle returned by {@link xspress3FemInitialise}
 * @param bus FEM bus to write to
 * @param width_bytes width of each write in bytes, typically 1, 2 or 4.
 * @param address address of first write transaction
 * @param num_writes number of bytes to write
 * @param offset_inc_scale  Scaling to apply to address increment when writing in chunks.
 * @param payload vector of writes of appropriate width
 * @return 0 on success of -1 error code.
 */
int32_t zynqmp_access_write(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t num_writes, int offset_inc_scale, u_int8_t* payload )
{
	u_int32_t chunk_size_bytes = ZYNQMP_TO_FEM_BUFFER_BYTES;
	u_int32_t chunk_size_ops, max_chunk_size_ops, chunk_size_ack;
	int bytes;
	u_int8_t cmd;
	u_int32_t buffer[(sizeof(ZynqMPProtocolHeader)+ZYNQMP_TO_FEM_BUFFER_BYTES)/sizeof(u_int32_t)];	// Buffer that will be 32 bit aligned.
	u_int8_t *transaction = (u_int8_t *)buffer;
	ZynqMPProtocolHeader rx_buff;
	u_int32_t ops_to_write, ops_to_ack, remaining, bytes_sent;
	int rc;
	max_chunk_size_ops = chunk_size_bytes/width_bytes;
	int final_rc = 0;

	if (zynqmp == NULL || !zynqmp->m_valid)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_write: Not connected to the ZynqMP");
		return -2;  //ZYNQMP_ERROR;
	}
//	printf("Starting fem2AccessWrite at Address %08X for %d=%08X\n", address, num_writes, num_writes);
	pthread_mutex_lock (&(zynqmp->mutex));
	ops_to_write = num_writes;
	ops_to_ack = num_writes;
	remaining = 0;
	while (final_rc == 0 && (ops_to_write > 0 || ops_to_ack > 0))
	{
		while (ops_to_write > 0 || remaining > 0)
		{
			if (remaining  == 0)
			{
				if (ops_to_write <= max_chunk_size_ops)
					chunk_size_ops = ops_to_write;
				else
					chunk_size_ops = max_chunk_size_ops;

				if (chunk_size_ops <= 4)
				{
					cmd = ZYNQMP_CMD_WRITE_EMB;
					bytes = sizeof(ZynqMPProtocolHeader);
					encodeWrTransaction2(transaction, ++(zynqmp->seq), cmd, bus, width_bytes, address, chunk_size_ops, payload, chunk_size_ops*width_bytes);
				}
				else
				{
					cmd = ZYNQMP_CMD_WRITE;
					bytes = sizeof(ZynqMPProtocolHeader) + chunk_size_ops*width_bytes;
					encodeWrTransaction2(transaction, ++(zynqmp->seq), cmd, bus, width_bytes, address, chunk_size_ops, payload, chunk_size_ops*width_bytes);
				}
				remaining = bytes;
/*				printf("Send Write transaction start address=%d for %d ops of %d starting =%02X,%02X,%02X,%02X. trans address=%d\n", address, chunk_size_ops, ops_to_write, 
					transaction[sizeof(ZynqMPProtocolHeader)], transaction[sizeof(ZynqMPProtocolHeader)+1], transaction[sizeof(ZynqMPProtocolHeader)+2], transaction[sizeof(ZynqMPProtocolHeader)+3],
					((ZynqMPProtocolHeader *)transaction)->address);
*/
			}
			else
			{
//				printf("Sending %d remaining bytes of write transaction, address=%d\n", remaining, address) ;
			}
			bytes_sent = bytes-remaining;
			if ((rc = sendTransaction(zynqmp->m_skt, transaction+bytes_sent, remaining, ops_to_write != ops_to_ack, &remaining)) <= 0) 
			{
				if (rc == ZYNQMP_WOULD_BLOCK)
				{
//					printf(".... Would block\n");
					if (remaining == 0)
					{
						printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
						exit(1);
					}
					break;
				}
				else
				{
					pthread_mutex_unlock (&(zynqmp->mutex));
		//			printf(" Send failed\n");
					snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_write: sendTransaction failed");
					return -3 ; // ZYNQMP_ERROR;
				}
			}
			if (remaining != 0)
				printf("CODING ERROR AT %s:%d, remaining not dropped to 0, WHY?\n", __FILE__, __LINE__);
			ops_to_write -= chunk_size_ops;
			payload += chunk_size_ops*width_bytes;
			address += chunk_size_ops*offset_inc_scale;
		}
//		printf("Waiting for acknowledge at %d ops\n", ops_to_ack);
		if (responseTransaction2(zynqmp->m_skt, (u_int8_t *)&rx_buff, sizeof(ZynqMPProtocolHeader)) <= 0) 
		{
			char old_msg[ZYNQMP_MAX_ERROR_MESSAGE];
			pthread_mutex_unlock (&(zynqmp->mutex));
//			printf(" Send failed\n");
			strcpy(old_msg, error_message);
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_write: %s", old_msg);
			return -4; //ZYNQMP_ERROR;
		}
	// Check for an ACK and the absence of a NACK on the response
		if (rx_buff.state)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_write: FEM response contained error code=%d", rx_buff.state);
			final_rc =  -5; //ZYNQMP_ERROR;
		}
		// The payload of the response to a write transaction should be the identical header as sent, the number of sucessful write is returned in num_ops

		if (ops_to_ack <= max_chunk_size_ops)
			chunk_size_ack = ops_to_ack;
		else
			chunk_size_ack = max_chunk_size_ops;
		if (rx_buff.num_ops != chunk_size_ack)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE,
				"zynqmp_access_write: Length mismatch during ZYNQMP write transaction: requested = %d  responded = %d", chunk_size_ack, rx_buff.num_ops);
			final_rc = -6; // ZYNQMP_ERROR;
		}
		ops_to_ack -= chunk_size_ack;
	}
	if (final_rc != 0)
	{
		while (ops_to_ack > ops_to_write || remaining > 0)	// Consume Acks for posted writes, also compete current command if part way through
		{
			while (remaining > 0)
			{
				bytes_sent = bytes-remaining;
				if ((rc = sendTransaction(zynqmp->m_skt, transaction+bytes_sent, remaining, ops_to_write != ops_to_ack, &remaining)) <= 0) 
				{
					if (rc == ZYNQMP_WOULD_BLOCK)
					{
	//					printf(".... Would block\n");
						if (remaining == 0)
						{
							printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
							exit(1);
						}
						break;
					}
					else
					{
						pthread_mutex_unlock (&(zynqmp->mutex));
			//			printf(" Send failed\n");
						snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_write: sendTransaction failed");
						return -3 ; // ZYNQMP_ERROR;
					}
				}
				if (remaining != 0)
					printf("CODING ERROR AT %s:%d, remaining not dropped to 0, WHY?\n", __FILE__, __LINE__);
				ops_to_write -= chunk_size_ops;
				payload += chunk_size_ops*width_bytes;
				address += chunk_size_ops*offset_inc_scale;
			}
	//		printf("Waiting for acknowledge at %d ops\n", ops_to_ack);
			if (responseTransaction2(zynqmp->m_skt, (u_int8_t *)&rx_buff, sizeof(ZynqMPProtocolHeader)) <= 0) 
			{

				pthread_mutex_unlock (&(zynqmp->mutex));
	//			printf(" Send failed\n");
				snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_write: responseTransaction2 failed");
				return -4; //ZYNQMP_ERROR;
			}
			// The payload of the response to a write transaction should be the identical header as sent, the number of sucessful write is returned in num_ops

			if (ops_to_ack <= max_chunk_size_ops)
				chunk_size_ack = ops_to_ack;
			else
				chunk_size_ack = max_chunk_size_ops;
			ops_to_ack -= chunk_size_ack;
		}
	}
	pthread_mutex_unlock (&(zynqmp->mutex));
//	printf("Finished fem2AccessWrite at Address %08X OK\n", address);
	return 0;
}

static void encodeWrTransaction2(u_int8_t* transaction, u_int32_t seq, u_int8_t cmd, u_int8_t bus, u_int8_t width, u_int32_t address, u_int32_t num_ops, u_int8_t* payload, u_int32_t payloadSize) 
{
	ZynqMPProtocolHeader *header = (ZynqMPProtocolHeader *)transaction;
	u_int8_t *tptr8;

	header->magic = MAGIC;
	header->sequence = seq;
	header->command = cmd;
	header->bus_target = bus;
	header->data_width = width;
	header->state = 0;
	header->address = address;
	header->num_ops = num_ops;
	header->extra1 = 0;
	header->extra2 = 0;
	header->extra3 = 0;
	if (cmd == ZYNQMP_CMD_WRITE || cmd == ZYNQMP_CMD_PERSONALITY_WRITE)
	{
		tptr8 = transaction + sizeof(ZynqMPProtocolHeader);
		header->payload_sz = payloadSize;
	}
	else
	{
		tptr8 = (u_int8_t *) &(header->payload_sz);
	}
	if (payloadSize > 0)
		memcpy((void*) tptr8, (void*) payload, payloadSize);

}

static int32_t responseTransaction2(int fd, u_int8_t* transaction, u_int32_t bytes) 
{
	int rc;
	u_int8_t *p8=transaction;
	while (bytes > 0)
	{
		while ((rc = recv(fd, (void*) p8, bytes, 0)) < 0 && sock_errno == EINTR) ;
		if (rc < 0)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "responseTransaction2: Reading protocol header response from server failed errno=%d ", sock_errno);
			return ZYNQMP_ERROR;
		}
		bytes -= rc;
		p8 += rc;
	}
		
	// Intialise header from byte stream
	ZynqMPProtocolHeader * hptr = (ZynqMPProtocolHeader*) transaction;
	if (hptr->magic != MAGIC)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "responseTransaction2: Corrupt header magic detected: got 0x%x expected 0x%x",
				hptr->magic, MAGIC);
		return ZYNQMP_ERROR;
	}
	return rc;
}

/** 
 * @ingroup internal
 * Execute a read transaction on the connected FEM-2.
 *
 * @param zynqmp  Pointer to ZynqMPHandle returned by {@link xspress3FemInitialise}
 * @param bus FEM bus to write to
 * @param width_bytes width of each write in bytes, typically 1, 2 or 4.
 * @param address address of first write transaction
 * @param num number of successive addresses to read
 * @param offset_inc_scale scaling for address to increment with each chunk read.
 * @param payload a buffer to accept the return data
 * @return 0 on success of -1 error code.
 */
int32_t zynqmp_access_read(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t num, int offset_inc_scale, u_int8_t* payload)
{
	ZynqMPProtocolHeader tx_header, rx_header;
	u_int32_t chunk_size_bytes = ZYNQMP_FROM_FEM_BUFFER_MAX;
	u_int32_t chunk_size_ops, max_chunk_size_ops, chunk_size_recv;
	u_int32_t read_ops_to_send, read_ops_to_recv;
	u_int32_t remaining, bytes_sent;
	int rc;
	int final_rc = 0;
	max_chunk_size_ops = chunk_size_bytes/width_bytes;

	if (zynqmp == NULL || !zynqmp->m_valid) {
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_read: Not connected to the ZynqMP");
		return ZYNQMP_ERROR;
	}
	pthread_mutex_lock(&(zynqmp->mutex));
	read_ops_to_send = num;
	read_ops_to_recv = num;

	remaining = 0;
	while (final_rc == 0 && (read_ops_to_send > 0 || read_ops_to_recv > 0))
	{
		while (read_ops_to_send > 0 || remaining > 0)
		{
			if (remaining == 0)
			{
				if (read_ops_to_send < max_chunk_size_ops)
					chunk_size_ops = read_ops_to_send;
				else
					chunk_size_ops = max_chunk_size_ops;

		//		printf("femAccessRead: Reading chunck size %d\n", chunk_size_ops);
				encodeRdTransaction2((u_int8_t *)(&tx_header), ++(zynqmp->seq), ZYNQMP_CMD_READ, bus, width_bytes, address, chunk_size_ops, chunk_size_ops*width_bytes);
				remaining = sizeof(ZynqMPProtocolHeader);
//				printf("Sending read from address %d for %d\n", address, chunk_size_ops);
			}
			bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

			if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_ops_to_send != read_ops_to_recv, &remaining)) <= 0)
			{
				if (rc == ZYNQMP_WOULD_BLOCK)
				{
//					printf(".... Would block\n");
					if (remaining == 0)
					{
						printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
						exit(1);
					}
					break;
				}
				else
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_ops_to_send -= chunk_size_ops;
			address += chunk_size_ops*offset_inc_scale;
		}
		if (read_ops_to_recv < max_chunk_size_ops)
			chunk_size_recv = read_ops_to_recv;
		else
			chunk_size_recv = max_chunk_size_ops;

		// Receive a transaction header first to determine payload length
		if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
		{
			pthread_mutex_unlock(&(zynqmp->mutex));
			return ZYNQMP_ERROR;
		}
		if (rx_header.state)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_read: FEM response contained error code=%d", rx_header.state);
			final_rc = -5; //ZYNQMP_ERROR;
		}
		else
		{
			if (rx_header.num_ops != chunk_size_recv) {
				snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", chunk_size_recv, rx_header.num_ops);
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			// already read the response length, now this is the number of bytes of data
			int bytes = chunk_size_recv*width_bytes;
			if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) {
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
		}
		read_ops_to_recv -= chunk_size_recv;
		payload += chunk_size_recv*width_bytes;
	}
	if (final_rc != 0) /* Flush any more read ahead acknowledgement (Good or error) and consume data from any good */
	{
		while (read_ops_to_recv > read_ops_to_send || remaining > 0)
		{
			while (remaining > 0)
			{
				bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

				if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_ops_to_send != read_ops_to_recv, &remaining)) <= 0)
				{
					if (rc == ZYNQMP_WOULD_BLOCK)
					{
	//					printf(".... Would block\n");
						if (remaining == 0)
						{
							printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
							exit(1);
						}
						break;
					}
					else
					{
						pthread_mutex_unlock(&(zynqmp->mutex));
						return ZYNQMP_ERROR;
					}
				}
				read_ops_to_send -= chunk_size_ops;
				address += chunk_size_ops*offset_inc_scale;
			}
			if (read_ops_to_recv < max_chunk_size_ops)
				chunk_size_recv = read_ops_to_recv;
			else
				chunk_size_recv = max_chunk_size_ops;

			// Receive a transaction header first to determine payload length
			if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
			{
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			if (rx_header.state)
			{
					// Probably more errors, but dont't keep printing the same message
			}
			else
			{
				if (rx_header.num_ops != chunk_size_recv)
				{
					snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", chunk_size_recv, rx_header.num_ops);
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
				// already read the response length, now this is the number of bytes of data
				int bytes = chunk_size_recv*width_bytes;
				if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) 
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_ops_to_recv -= chunk_size_recv;
			payload += chunk_size_recv*width_bytes;
		}
	}
	pthread_mutex_unlock(&(zynqmp->mutex));
	return final_rc;
}

static void encodeRdTransaction2(u_int8_t* transaction, u_int32_t seq, u_int8_t cmd, u_int8_t bus, u_int8_t width, u_int32_t address, u_int32_t num_ops, u_int32_t payloadSize) 
{
	ZynqMPProtocolHeader *header = (ZynqMPProtocolHeader *)transaction;
	header->magic = MAGIC;
	header->sequence = seq;
	header->command = cmd;
	header->bus_target = bus;
	header->data_width = width;
	header->state = 0;
	header->address = address;
	header->num_ops = num_ops;
	header->payload_sz = payloadSize;
	header->extra1 = 0;
	header->extra2 = 0;
	header->extra3 = 0;

}

static int32_t responsePayload2(int fd, u_int8_t *payload, u_int32_t bytes) {
	int rc;
	int num;
	u_int8_t *p8 = payload;

	num = bytes;
	while (bytes > 0)
	{
		while ((rc = recv(fd, (void*) p8, bytes, 0)) < 0 && sock_errno == EINTR) ;
		if (rc < 0)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "responsePayload2: Reading payload from server failed fd=%d, errno=%d, payload=%p, bytes=%u, p8=%p", fd, sock_errno, payload, bytes, p8);
			return ZYNQMP_ERROR;
		}
		p8 += rc;
		bytes -= rc;
	}
	return num;
}

int32_t zynqmp_access_rmw(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *result)
{
	ZynqMPProtocolHeader tx_buffer, rx_header;

	if (zynqmp == NULL || !zynqmp->m_valid)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_rmw: Not connected to the ZynqMP");
		return -2;  //ZYNQMP_ERROR;
	}
	pthread_mutex_lock (&(zynqmp->mutex));
	
	encodeRdTransaction2((u_int8_t *)(&tx_buffer), ++(zynqmp->seq), ZYNQMP_CMD_RMW, bus, width_bytes, address, 1, 0);
	tx_buffer.payload_sz = and_mask;
	tx_buffer.extra1 = or_mask;

	if (sendTransaction(zynqmp->m_skt, (u_int8_t *)(&tx_buffer), sizeof(ZynqMPProtocolHeader), 0, NULL) <= 0) 
	{
		pthread_mutex_unlock (&(zynqmp->mutex));
//			printf(" Send failed\n");
		return -3 ; // ZYNQMP_ERROR;
	}
	if (responseTransaction2(zynqmp->m_skt, (u_int8_t *)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0) 
	{
		pthread_mutex_unlock (&(zynqmp->mutex));
//			printf(" Send failed\n");
		return -4; //ZYNQMP_ERROR;
	}
// Check for an ACK and the absence of a NACK on the response
	if (rx_header.state)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "femAccessWrite2: FEM response contained error code=%d", rx_header.state);
		pthread_mutex_unlock (&(zynqmp->mutex));
		return -5; //ZYNQMP_ERROR;
	}
	// The payload of the response to a write transaction should be a single
	// 32bit word indicating the modified word written to memory
	if (result != NULL)
		*result = rx_header.num_ops;
	pthread_mutex_unlock (&(zynqmp->mutex));
//	printf("Finished fem2AccessWrite at Address %08X OK\n", address);
	return ZYNQMP_OK;
}

int32_t zynqmp_personality_write(ZynqMPHandle* zynqmp, u_int32_t subCommand, u_int32_t width_bytes, u_int32_t streamMask, size_t num_lwords, u_int8_t* payload, int num_details, u_int32_t *details, int32_t * remote_ret_code)
{
	int bytes;
	u_int8_t cmd;
	u_int32_t buffer[(sizeof(ZynqMPProtocolHeader)+ZYNQMP_TO_FEM_BUFFER_BYTES)/sizeof(u_int32_t)];	// Buffer that will be 32 bit aligned.
	u_int8_t *transaction = (u_int8_t *)buffer;
	
	if (zynqmp == NULL || !zynqmp->m_valid)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_write: Not connected to the FEM");
		return -2;  //ZYNQMP_ERROR;
	}
	if (num_lwords*width_bytes > ZYNQMP_TO_FEM_BUFFER_BYTES)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_write: Cannot send transaction length %zd bytes", num_lwords*width_bytes);
		return -3;
	}
//	printf("Starting fem2AccessWrite at Address %08X for %d\n", address, num_writes);
	pthread_mutex_lock (&(zynqmp->mutex));
	
	if (num_lwords <= 4)
	{
		cmd = ZYNQMP_CMD_PERSONALITY_WRITE_EMB;
		bytes = sizeof(ZynqMPProtocolHeader);
		encodeWrTransaction2(transaction, ++(zynqmp->seq), cmd, subCommand, width_bytes, streamMask, num_lwords, payload, num_lwords*width_bytes);
	}
	else
	{
		cmd = ZYNQMP_CMD_PERSONALITY_WRITE;
		bytes = sizeof(ZynqMPProtocolHeader) + num_lwords*width_bytes;
		encodeWrTransaction2(transaction, ++(zynqmp->seq), cmd, subCommand, width_bytes, streamMask, num_lwords, payload, num_lwords*width_bytes);
	}

	if (sendTransaction(zynqmp->m_skt, transaction, bytes, 0, NULL) <= 0) 
	{
		pthread_mutex_unlock (&(zynqmp->mutex));
//			printf(" Send failed\n");
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_write: sendTransaction failed");
		return -3 ; // ZYNQMP_ERROR;
	}
	if (responseTransaction2(zynqmp->m_skt, transaction, sizeof(ZynqMPProtocolHeader)) <= 0) 
	{
		pthread_mutex_unlock (&(zynqmp->mutex));
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_write: responseTransaction2 failed");
		return -4; //ZYNQMP_ERROR;
	}
// Check for an ACK and the absence of a NACK on the response
	ZynqMPProtocolHeader* hptr = (ZynqMPProtocolHeader*) transaction;
	if (hptr->state)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_write: FEM response contained error code=%d", hptr->state);
		pthread_mutex_unlock (&(zynqmp->mutex));
		return -5; //ZYNQMP_ERROR;
	}
	// For a personailty write the num_ops field is overwritten with the return code data.
	pthread_mutex_unlock (&(zynqmp->mutex));
//	printf("Finished fem2AccessWrite at Address %08X OK\n", address);
	if (num_details != 0 && details != NULL)
	{
		int i;
		u_int32_t *p=&(hptr->extra1);
		for (i=0; i<num_details; i++)
		{
			*details ++ = *p++;
		}
	}
	if (remote_ret_code != NULL)
		*remote_ret_code = (int32_t)hptr->num_ops;
	return 0;
}

/**
 * @ingroup internal
 * Execute a mem zero transaction on the connected FEM-2.
 *
 * Although the clear does not requrie daat, the transaction is broken as for write operations to force the device driver to exit to allwo muti-task to caryy on normally.
 * @param zynqmp  Pointer to ZynqMPHandle returned by {@link xspress3FemInitialise}
 * @param bus FEM bus to write to
 * @param width_bytes width of each write in bytes, typically 1, 2 or 4.
 * @param address address of first write transaction
 * @param num_writes number of bytes to write
 * @param offset_inc_scale  Scaling to apply to address increment when writing in chunks.
 * @return 0 on success of -1 error code.
 */
int32_t zynqmp_access_memzero(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t num_writes, int offset_inc_scale)
{
	u_int32_t chunk_size_bytes = ZYNQMP_TO_FEM_BUFFER_BYTES;
	u_int32_t chunk_size_ops, max_chunk_size_ops, chunk_size_ack;
	int bytes;
	u_int8_t cmd;
	ZynqMPProtocolHeader buffer;	
	u_int8_t *transaction = (u_int8_t *)&buffer;
	ZynqMPProtocolHeader rx_buff;
	u_int32_t ops_to_write, ops_to_ack, remaining, bytes_sent;
	int rc;
	max_chunk_size_ops = chunk_size_bytes/width_bytes;

	if (zynqmp == NULL || !zynqmp->m_valid)
	{
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_memzero: Not connected to the ZynqMP");
		return -2;  //ZYNQMP_ERROR;
	}
//	printf("Starting fem2AccessWrite at Address %08X for %d=%08X\n", address, num_writes, num_writes);
	pthread_mutex_lock (&(zynqmp->mutex));
	ops_to_write = num_writes;
	ops_to_ack = num_writes;
	remaining = 0;
	while (ops_to_write > 0 || ops_to_ack > 0)
	{
		while (ops_to_write > 0 || remaining > 0)
		{
			if (remaining  == 0)
			{
				if (ops_to_write <= max_chunk_size_ops)
					chunk_size_ops = ops_to_write;
				else
					chunk_size_ops = max_chunk_size_ops;

				cmd = ZYNQMP_CMD_MEM_ZERO;
				bytes = sizeof(ZynqMPProtocolHeader);
				encodeWrTransaction2(transaction, ++(zynqmp->seq), cmd, bus, width_bytes, address, chunk_size_ops, NULL, 0);
				remaining = bytes;
			}
			else
			{
//				printf("Sending %d remaining bytes of write transaction, address=%d\n", remaining, address) ;
			}
			bytes_sent = bytes-remaining;
			if ((rc = sendTransaction(zynqmp->m_skt, transaction+bytes_sent, remaining, ops_to_write != ops_to_ack, &remaining)) <= 0) 
			{
				if (rc == ZYNQMP_WOULD_BLOCK)
				{
//					printf(".... Would block\n");
					if (remaining == 0)
					{
						printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
						exit(1);
					}
					break;
				}
				else
				{
					pthread_mutex_unlock (&(zynqmp->mutex));
		//			printf(" Send failed\n");
					return -3 ; // ZYNQMP_ERROR;
				}
			}
			if (remaining != 0)
				printf("CODING ERROR AT %s:%d, remaining not dropped to 0, WHY?\n", __FILE__, __LINE__);
			ops_to_write -= chunk_size_ops;
			address += chunk_size_ops*offset_inc_scale;
		}
//		printf("Waiting for acknowledge at %d ops\n", ops_to_ack);
		if (responseTransaction2(zynqmp->m_skt, (u_int8_t *)&rx_buff, sizeof(ZynqMPProtocolHeader)) <= 0) 
		{

			pthread_mutex_unlock (&(zynqmp->mutex));
//			printf(" Send failed\n");
			return -4; //ZYNQMP_ERROR;
		}
	// Check for an ACK and the absence of a NACK on the response
		if (rx_buff.state)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynq_access_memzero: FEM response contained error code=%d", rx_buff.state);
			pthread_mutex_unlock (&(zynqmp->mutex));
			return -5; //ZYNQMP_ERROR;
		}
		// The payload of the response to a write transaction should be the identical header as sent, the number of sucessful write is returned in num_ops

		if (ops_to_ack <= max_chunk_size_ops)
			chunk_size_ack = ops_to_ack;
		else
			chunk_size_ack = max_chunk_size_ops;
		if (rx_buff.num_ops != chunk_size_ack)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE,
				"femAccessWrite2: Length mismatch during FEM write transaction: requested = %d  responded = %d", chunk_size_ack, rx_buff.num_ops);
//			printf("femAccessWrite2: Length mismatch during FEM write transaction: requested = %d  responded = %d", chunk_size_ops, responseWriteLen);
			pthread_mutex_unlock (&(zynqmp->mutex));
			return -6; // ZYNQMP_ERROR;
		}
		ops_to_ack -= chunk_size_ack;
	}
	pthread_mutex_unlock (&(zynqmp->mutex));
//	printf("Finished fem2AccessWrite at Address %08X OK\n", address);
	return ZYNQMP_OK;
}



/** 
 * @ingroup internal
 * Execute a read region of interest transaction on the connected FEM-2.
 *
 * @param zynqmp  Pointer to ZynqMPHandle returned by {@link xspress3FemInitialise}
 * @param bus FEM bus to write to
 * @param width_bytes width of each write in bytes, typically 1, 2 or 4.
 * @param address address of first write transaction
 * @param row_length_ops number of successive addresses to read in a row
 * @param num_rows   Number of row to read.
 * @param src_stride_ops  Step in operations for address in the data src (FEM-2)
 * @param dst_stride_ops  Step from line to line in destination (payload buffer) in operations 
 * @param offset_inc_scale scaling for address to increment with each chunk read.
 * @param payload a buffer to accept the return data
 * @return 0 on success of -1 error code.
 */
int32_t zynqmp_access_read_roi(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t row_length_ops, u_int32_t num_rows, u_int32_t src_stride_ops, u_int32_t dst_stride_ops, int offset_inc_scale, u_int8_t* payload)
{
	ZynqMPProtocolHeader tx_header, rx_header;
	u_int32_t chunk_size_bytes = ZYNQMP_FROM_FEM_BUFFER_MAX;
	u_int32_t max_chunk_size_ops;
	u_int32_t read_rows_to_send, read_rows_to_recv;
	u_int32_t remaining, bytes_sent;
	int rc;
	max_chunk_size_ops = chunk_size_bytes/width_bytes;
	int row;
	int final_rc = 0;

	if (zynqmp == NULL || !zynqmp->m_valid) {
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_read_roi: Not connected to the ZynqMP");
		return ZYNQMP_ERROR;
	}
	if (row_length_ops > max_chunk_size_ops)
	{
		//* Call existing code line at a time.
		for (row=0; row<num_rows; row++)
		{
			rc = zynqmp_access_read(zynqmp, bus, width_bytes, address, row_length_ops, offset_inc_scale, payload);
			if (rc < 0) return rc;
			address += src_stride_ops*offset_inc_scale;
			payload += dst_stride_ops*width_bytes;
		}
	}
	else
	{
		pthread_mutex_lock(&(zynqmp->mutex));
		

		read_rows_to_send = num_rows;
		read_rows_to_recv = num_rows;

		remaining = 0;
		while (final_rc == 0 && (read_rows_to_send > 0 || read_rows_to_recv > 0))
		{
			while (read_rows_to_send > 0 || remaining > 0)
			{
				if (remaining == 0)
				{
					encodeRdTransaction2((u_int8_t *)(&tx_header), ++(zynqmp->seq), ZYNQMP_CMD_READ, bus, width_bytes, address, row_length_ops, row_length_ops*width_bytes);
					remaining = sizeof(ZynqMPProtocolHeader);
	//				printf("Sending read from address %d for %d\n", address, chunk_size_ops);
				}
				bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

				if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_rows_to_send != read_rows_to_recv, &remaining)) <= 0)
				{
					if (rc == ZYNQMP_WOULD_BLOCK)
					{
	//					printf(".... Would block\n");
						if (remaining == 0)
						{
							printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
							exit(1);
						}
						break;
					}
					else
					{
						pthread_mutex_unlock(&(zynqmp->mutex));
						return ZYNQMP_ERROR;
					}
				}
				read_rows_to_send --;
				address += src_stride_ops*offset_inc_scale;
			}

			// Receive a transaction header first to determine payload length
			if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
			{
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			if (rx_header.state)
			{
				snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_read_roi: FEM response contained error code=%d", rx_header.state);
				final_rc = -5; //ZYNQMP_ERROR;
			}
			else
			{
				if (rx_header.num_ops != row_length_ops) {
					snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", row_length_ops, rx_header.num_ops);
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
				// already read the response length, now this is the number of bytes of data
				int bytes = row_length_ops*width_bytes;
				if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) {
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_rows_to_recv --;
			payload += dst_stride_ops*width_bytes;
		}
		if (final_rc != 0)
		{
			/* Flush all read ahead reads */
			while (read_rows_to_recv > read_rows_to_send || remaining > 0)
			{
				while (remaining > 0) // Post last part of any partial read command
				{
					bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

					if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_rows_to_send != read_rows_to_recv, &remaining)) <= 0)
					{
						if (rc == ZYNQMP_WOULD_BLOCK)
						{
		//					printf(".... Would block\n");
							if (remaining == 0)
							{
								printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
								exit(1);
							}
							break;
						}
						else
						{
							pthread_mutex_unlock(&(zynqmp->mutex));
							return ZYNQMP_ERROR;
						}
					}
					read_rows_to_send --; 
				}

				// Receive a transaction header first to determine payload length
				if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
				if (rx_header.state)
				{
					// Probably more errors, but dont't keep printing the same message
				}
				else
				{
					if (rx_header.num_ops != row_length_ops) {
						snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", row_length_ops, rx_header.num_ops);
						pthread_mutex_unlock(&(zynqmp->mutex));
						return ZYNQMP_ERROR;
					}
					// already read the response length, now this is the number of bytes of data
					int bytes = row_length_ops*width_bytes;
					if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) {
						pthread_mutex_unlock(&(zynqmp->mutex));
						return ZYNQMP_ERROR;
					}
				}
				read_rows_to_recv --;
				payload += dst_stride_ops*width_bytes;
			}
		}
		pthread_mutex_unlock(&(zynqmp->mutex));
	}
	return final_rc;
}

/** 
 * @ingroup internal
 * Execute a read transaction on the connected FEM-2.
 *
 * @param zynqmp  Pointer to ZynqMPHandle returned by {@link xspress3FemInitialise}
 * @param subCommand Reuses FEM bus fro subCommand
 * @param width_bytes width of each write in bytes, typically 1, 2 or 4.
 * @param address address of first read transaction
 * @param num number of successive addresses to read
 * @param offset_inc_scale scaling for remote address to increment with each chunk read.
 * @param payload a buffer to accept the return data
 * @param extra1    Application specific data passed in extra1 field
 * @param extra2    Application specific data passed in extra2 field
 * @param extra3    Application specific data passed in extra3 field
 * @return 0 on success of -1 error code.
 */
int32_t zynqmp_personality_read(ZynqMPHandle* zynqmp, u_int32_t subCommand, u_int32_t width_bytes, u_int32_t address, u_int32_t num, int offset_inc_scale, u_int8_t* payload, u_int32_t extra1, u_int32_t extra2, u_int32_t extra3)
{
	ZynqMPProtocolHeader tx_header, rx_header;
	u_int32_t chunk_size_bytes = ZYNQMP_FROM_FEM_BUFFER_MAX;
	u_int32_t chunk_size_ops, max_chunk_size_ops, chunk_size_recv;
	u_int32_t read_ops_to_send, read_ops_to_recv;
	u_int32_t remaining, bytes_sent;
	int rc;
	int final_rc = 0;
	max_chunk_size_ops = chunk_size_bytes/width_bytes;

	if (zynqmp == NULL || !zynqmp->m_valid) {
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_read: Not connected to the ZynqMP");
		return ZYNQMP_ERROR;
	}
	pthread_mutex_lock(&(zynqmp->mutex));
	read_ops_to_send = num;
	read_ops_to_recv = num;

	remaining = 0;
//	printf("fem2PersonalityRead: subCommand=%d, width=%d, address=%u, num=%u, extra1=%d\n", subCommand, width_bytes, address, num, extra1);
	while (final_rc == 0 && (read_ops_to_send > 0 || read_ops_to_recv > 0))
	{
		while (read_ops_to_send > 0 || remaining > 0)
		{
			if (remaining == 0)
			{
				if (read_ops_to_send < max_chunk_size_ops)
					chunk_size_ops = read_ops_to_send;
				else
					chunk_size_ops = max_chunk_size_ops;

		//		printf("femAccessRead: Reading chunck size %d\n", chunk_size_ops);
				encodeRdTransaction2((u_int8_t *)(&tx_header), ++(zynqmp->seq), ZYNQMP_CMD_PERSONALITY_READ, subCommand, width_bytes, address, chunk_size_ops, chunk_size_ops*width_bytes);
				tx_header.extra1 = extra1;
				tx_header.extra2 = extra2;
				tx_header.extra3 = extra3;
				remaining = sizeof(ZynqMPProtocolHeader);
//				printf("Sending read from address %d for %d\n", address, chunk_size_ops);
			}
			bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

			if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_ops_to_send != read_ops_to_recv, &remaining)) <= 0)
			{
				if (rc == ZYNQMP_WOULD_BLOCK)
				{
//					printf(".... Would block\n");
					if (remaining == 0)
					{
						printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
						exit(1);
					}
					break;
				}
				else
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_ops_to_send -= chunk_size_ops;
			address += chunk_size_ops*offset_inc_scale;
		}
		if (read_ops_to_recv < max_chunk_size_ops)
			chunk_size_recv = read_ops_to_recv;
		else
			chunk_size_recv = max_chunk_size_ops;

		// Receive a transaction header first to determine payload length
//		printf("fem2PersonalityRead calling responseTransaction2 for header\n");
		if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
		{
			pthread_mutex_unlock(&(zynqmp->mutex));
			return ZYNQMP_ERROR;
		}
		if (rx_header.state)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_personality_read: ZYNQMP response contained error code=%d", rx_header.state);
			final_rc = -5; //ZYNQMP_ERROR;
		}
		else
		{
			if (rx_header.num_ops != chunk_size_recv) {
				snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", chunk_size_recv, rx_header.num_ops);
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			// already read the response length, now this is the number of bytes of data
			int bytes = chunk_size_recv*width_bytes;
			if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) {
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
		}
		read_ops_to_recv -= chunk_size_recv;
		payload += chunk_size_recv*width_bytes;
	}
	if (final_rc != 0) /* Flush any more read ahead acknowledgement (Good or error) and consume data from any good */
	{
		while (read_ops_to_recv > read_ops_to_send || remaining > 0)
		{
			while (remaining > 0)
			{
				bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

				if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_ops_to_send != read_ops_to_recv, &remaining)) <= 0)
				{
					if (rc == ZYNQMP_WOULD_BLOCK)
					{
	//					printf(".... Would block\n");
						if (remaining == 0)
						{
							printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
							exit(1);
						}
						break;
					}
					else
					{
						pthread_mutex_unlock(&(zynqmp->mutex));
						return ZYNQMP_ERROR;
					}
				}
				read_ops_to_send -= chunk_size_ops;
				address += chunk_size_ops*offset_inc_scale;
			}
			if (read_ops_to_recv < max_chunk_size_ops)
				chunk_size_recv = read_ops_to_recv;
			else
				chunk_size_recv = max_chunk_size_ops;

			// Receive a transaction header first to determine payload length
			if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
			{
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			if (rx_header.state)
			{
					// Probably more errors, but dont't keep printing the same message
			}
			else
			{
				if (rx_header.num_ops != chunk_size_recv)
				{
					snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", chunk_size_recv, rx_header.num_ops);
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
				// already read the response length, now this is the number of bytes of data
				int bytes = chunk_size_recv*width_bytes;
				if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) 
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_ops_to_recv -= chunk_size_recv;
			payload += chunk_size_recv*width_bytes;
		}
	}
	pthread_mutex_unlock(&(zynqmp->mutex));
	return final_rc;
}

char * zynqmp_get_error_message()
{
	return error_message;
}


/** 
 * @ingroup internal
 * Execute a read interleaved transaction on the connected ZynqMP, particularly to suit interleaved memory streams of AXIHist
 *
 * @param zynqmp  Pointer to ZynqMPHandle returned by {@link xspress3FemInitialise}
 * @param bus FEM bus to write to
 * @param width_bytes width of each write in bytes, typically 1, 2 or 4.
 * @param address Word offset of first read transaction within the chosen histogram memory stream
 * @param num number of successive addresses to read
 * @param row_len	Number of contiguous locations before jumping over interleaving
 * @param row_stride	Number of wors to jump from start of currentro to next row in the same stream.
 * @param stream 	Stream number of interleaved data
 * @param payload a buffer to accept the return data
 * @return 0 on success of -1 error code.
 */
int32_t zynqmp_access_read_intl(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t num, int row_len, int num_streams, int stream, u_int8_t* payload)
{
	ZynqMPProtocolHeader tx_header, rx_header;
	u_int32_t chunk_size_bytes = ZYNQMP_FROM_FEM_BUFFER_MAX;
	u_int32_t chunk_size_ops, max_chunk_size_ops, chunk_size_recv;
	u_int32_t read_ops_to_send, read_ops_to_recv;
	u_int32_t remaining, bytes_sent;
	int rc;
	int final_rc = 0;
	max_chunk_size_ops = chunk_size_bytes/width_bytes;

	if (zynqmp == NULL || !zynqmp->m_valid) {
		snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_read_intl: Not connected to the ZynqMP");
		return ZYNQMP_ERROR;
	}
	pthread_mutex_lock(&(zynqmp->mutex));
	read_ops_to_send = num;
	read_ops_to_recv = num;

	remaining = 0;
	while (final_rc == 0 && (read_ops_to_send > 0 || read_ops_to_recv > 0))
	{
		while (read_ops_to_send > 0 || remaining > 0)
		{
			if (remaining == 0)
			{
				if (read_ops_to_send < max_chunk_size_ops)
					chunk_size_ops = read_ops_to_send;
				else
					chunk_size_ops = max_chunk_size_ops;

		//		printf("femAccessRead: Reading chunck size %d\n", chunk_size_ops);
				encodeRdTransaction2((u_int8_t *)(&tx_header), ++(zynqmp->seq), ZYNQMP_CMD_READ_INTL, bus, width_bytes, address, chunk_size_ops, chunk_size_ops*width_bytes);
				tx_header.extra1 = row_len;
				tx_header.extra2 = num_streams; 
				tx_header.extra3 = stream; 
				remaining = sizeof(ZynqMPProtocolHeader);
//				printf("Sending read from address %d for %d\n", address, chunk_size_ops);
			}
			bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

			if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_ops_to_send != read_ops_to_recv, &remaining)) <= 0)
			{
				if (rc == ZYNQMP_WOULD_BLOCK)
				{
//					printf(".... Would block\n");
					if (remaining == 0)
					{
						printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
						exit(1);
					}
					break;
				}
				else
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_ops_to_send -= chunk_size_ops;
			address += chunk_size_ops;
		}
		if (read_ops_to_recv < max_chunk_size_ops)
			chunk_size_recv = read_ops_to_recv;
		else
			chunk_size_recv = max_chunk_size_ops;

		// Receive a transaction header first to determine payload length
		if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
		{
			pthread_mutex_unlock(&(zynqmp->mutex));
			return ZYNQMP_ERROR;
		}
		if (rx_header.state)
		{
			snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "zynqmp_access_read_intl: FEM response contained error code=%d", rx_header.state);
			final_rc = -5; //ZYNQMP_ERROR;
		}
		else
		{
			if (rx_header.num_ops != chunk_size_recv) {
				snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", chunk_size_recv, rx_header.num_ops);
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			// already read the response length, now this is the number of bytes of data
			int bytes = chunk_size_recv*width_bytes;
			if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) {
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
		}
		read_ops_to_recv -= chunk_size_recv;
		payload += chunk_size_recv*width_bytes;
	}
	if (final_rc != 0) /* Flush any more read ahead acknowledgement (Good or error) and consume data from any good */
	{
		while (read_ops_to_recv > read_ops_to_send || remaining > 0)
		{
			while (remaining > 0)
			{
				bytes_sent = sizeof(ZynqMPProtocolHeader)-remaining;

				if ((rc=sendTransaction(zynqmp->m_skt, ((u_int8_t *)(&tx_header))+bytes_sent, remaining, read_ops_to_send != read_ops_to_recv, &remaining)) <= 0)
				{
					if (rc == ZYNQMP_WOULD_BLOCK)
					{
	//					printf(".... Would block\n");
						if (remaining == 0)
						{
							printf("Coding error at %s:%d Would block with remaining = 0, WHY\n", __FILE__, __LINE__);
							exit(1);
						}
						break;
					}
					else
					{
						pthread_mutex_unlock(&(zynqmp->mutex));
						return ZYNQMP_ERROR;
					}
				}
				read_ops_to_send -= chunk_size_ops;
				address += chunk_size_ops;
			}
			if (read_ops_to_recv < max_chunk_size_ops)
				chunk_size_recv = read_ops_to_recv;
			else
				chunk_size_recv = max_chunk_size_ops;

			// Receive a transaction header first to determine payload length
			if (responseTransaction2(zynqmp->m_skt, (u_int8_t*)&rx_header, sizeof(ZynqMPProtocolHeader)) <= 0)
			{
				pthread_mutex_unlock(&(zynqmp->mutex));
				return ZYNQMP_ERROR;
			}
			if (rx_header.state)
			{
					// Probably more errors, but dont't keep printing the same message
			}
			else
			{
				if (rx_header.num_ops != chunk_size_recv)
				{
					snprintf(error_message, ZYNQMP_MAX_ERROR_MESSAGE, "Length mismatch when reading: requested %d got %d", chunk_size_recv, rx_header.num_ops);
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
				// already read the response length, now this is the number of bytes of data
				int bytes = chunk_size_recv*width_bytes;
				if (responsePayload2(zynqmp->m_skt, payload, bytes) < 0) 
				{
					pthread_mutex_unlock(&(zynqmp->mutex));
					return ZYNQMP_ERROR;
				}
			}
			read_ops_to_recv -= chunk_size_recv;
			payload += chunk_size_recv*width_bytes;
		}
	}
	pthread_mutex_unlock(&(zynqmp->mutex));
	return final_rc;
}
