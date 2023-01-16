/*
 * protocol.h
 *
 *  Basic protocol for FEM control and configuration over Ethernet
 *
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_


// TODO: Fix this for large payload/multi-packet data reception....
#define ZYNQMP_CMD_PORT				30124
#if 1
#define ZYNQMP_TO_FEM_BUFFER_BYTES	(64*1024)			// Normal payload receive buffer size
#define ZYNQMP_TO_FEM_BUFFER_MAX		1048576				// Maximum payload size permitted
#define ZYNQMP_FROM_FEM_BUFFER_MAX	(64*1024)			// Maximum Read payload size permitted
#else
// Small values to force chunking code to work at PC end
#define ZYNQMP_TO_FEM_BUFFER_BYTES	1000			// Normal payload receive buffer size
#define ZYNQMP_TO_FEM_BUFFER_MAX		1048576				// Maximum payload size permitted
#define ZYNQMP_FROM_FEM_BUFFER_MAX	1020			// Maximum Read payload size permitted

#endif
#define ZYNQMP_PROTOCOL_MAGIC_WORD       0xDEADBEEF


// Macro to decode and display header
#ifdef GLOBAL_DEBUG
#define DUMPHDR(hdr)		printf("Magic: 0x%x\n",hdr->magic); \
							printf("Cmd:   0x%x\n",hdr->command); \
							printf("Bus:   0x%x\n",hdr->bus_target); \
							printf("Width: 0x%x\n",hdr->data_width); \
							printf("Stat:  0x%x\n",hdr->state); \
							printf("Addr:  0x%x\n",hdr->address); \
							printf("Payld: %d\n", hdr->payload_sz)
#else
#define DUMPHDR(hdr)
#endif

// New header format
// Size     Description
// ----------------------------------------------------------------------------
//  32      Magic word   (must be 0xDEADBEEF)
//  32      Sequence number 
//   8      Command type 
//   8      Bus target   (see protocol_bus_type enum)
//   8      Data width   (see protocol_data_width enum)
//   8      Status byte  (see protocol_status enum)
//  32      Address      Target address (for selected bus)
//  32      Payload sz   Size of Tx payload in bytes for write commands, size of rx payload for receive commands, reused for immediate data for dataless commands
//  32*3    Payload[3]  padding to allow datlesscommands to take up to 4 words of immediate data within header.

//  ??		[payload]
// ----------------------------------------------------------------------------
// Total 32 bytes + payload

// Packet header
typedef struct zynqmp_protocol_header
{
	u_int32_t magic;			// Always 0xDEADBEEF
	u_int32_t sequence;			// Sequence number to help check in step if allowing "posting" of writes.
	u_int8_t  command;
	u_int8_t  bus_target;
	u_int8_t  data_width;
	u_int8_t  state;
	u_int32_t address;
	u_int32_t num_ops;
	u_int32_t payload_sz;
	u_int32_t extra1;
	u_int32_t extra2;
	u_int32_t extra3;
} ZynqMPProtocolHeader;

/*
Note that data width is usually strongly linked to bus, so don't need both.
Want CMD to tell protocol layer what to do with possibly some sub options.
For register access (XSPRESS4, RDMA, possible AXI peripherals space). Also config, control and internal status spaces.
	Payload to FEM
		Write payload					Return number of successful ops
	No Payload
		Write immediate (1..4 words)	Return number of successful ops
		Read Modify write				Return value written
	Payload From FEM
		Read 							Return Number of bytes to follow.
For I2C and SPI
	May be more than 1 bus.
	Payload to FEM
		Write payload					Return number of successful ops
	No Payload
		Possible reset / poll activites?
	Payload From FEM
		Read 

Personality commands
	bus  = sub_command
	address = dma mask or number.
	
*/
// Supported commands (XSPRESS4)
enum zynqmp_protocol_commands
{
	ZYNQMP_CMD_UNSUPPORTED = 0,
	ZYNQMP_CMD_READ      	= 1,
	ZYNQMP_CMD_WRITE		= 2,
	ZYNQMP_CMD_WRITE_EMB	= 3,
	ZYNQMP_CMD_RMW		= 4,
	ZYNQMP_CMD_PERSONALITY_READ = 5,
	ZYNQMP_CMD_PERSONALITY_WRITE = 6,
	ZYNQMP_CMD_PERSONALITY_WRITE_EMB = 7,
	ZYNQMP_CMD_MEM_ZERO              = 8,
	ZYNQMP_CMD_READ_INTL      	= 9
};



// Size of data
enum zynqmp_protocol_data_width
{
    ZYNQMP_WIDTH_UNSUPPORTED = 0,
    ZYNQMP_WIDTH_BYTE        = 1,	// 8bit
    ZYNQMP_WIDTH_WORD        = 2,	// 16bit
    ZYNQMP_WIDTH_LONG        = 4	// 32bit	-- so can use directly as data width
};

// Status bit bank
enum zynqmp_protocol_status
{
	ZYNQMP_STATUS_OK 		= 0,	
	ZYNQMP_STATUS_ERROR	= 1,	// Can define more error codes if required.
	ZYNQMP_STATUS_OVERFLOW	= 2,	// Read would overflow buffer
};


#endif /* PROTOCOL_H_ */
