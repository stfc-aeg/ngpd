/*
 * ngzmp_dma_protocol.h
 *
 *  Created on: 15/2/2021
 *      Author: wih73
 */


	  
#ifndef NGPD_DMA_PROTOCOL_H_
#define NGPD_DMA_PROTOCOL_H_

/**
	@defgroup ZYNQMP_DMA   Macros to control the DMA engines in the FPGA.
	@ingroup ZYNQMP_MACROS
*/

//! [ZYNQMP_DMA_COMMANDS]
#define ZYNQMP_DMA_CMD_RESET				1
#define ZYNQMP_DMA_CMD_CONFIG_MEMORY		2
#define ZYNQMP_DMA_CMD_BUILD_DESC			3
#define ZYNQMP_DMA_CMD_START				4
#define ZYNQMP_DMA_CMD_BUILD_AND_START		5
#define ZYNQMP_DMA_CMD_BUILD_TEST_TEST_PAT	6
#define ZYNQMP_DMA_CMD_PRINT_DATA			7
#define ZYNQMP_DMA_CMD_PRINT_SCOPE			8
#define ZYNQMP_DMA_CMD_RESEND				9
#define ZYNQMP_DMA_CMD_READ_STATUS			10
#define ZYNQMP_DMA_CMD_BUILD_DEBUG_DESC		11
#define ZYNQMP_DMA_CMD_PRINT_DESC			12
#define ZYNQMP_DMA_CMD_CHECK_RX_DESC		13
#define ZYNQMP_DMA_CMD_SET_MSG_SERVER		14
#define ZYNQMP_DMA_CMD_SET_MSG_DRIVER		15
#define ZYNQMP_DMA_CMD_GET_DESC_STATUS		16
#define ZYNQMP_DMA_CMD_REUSE				17
#define ZYNQMP_DMA_CMD_CHECK_RX_DESC_CIRCULAR 18
#define ZYNQMP_DMA_CMD_READ_TF_STATUS		19
#define ZYNQMP_DMA_CMD_READ_TF_MARKERS		20
#define ZYNQMP_DMA_CMD_READ_SEND_LIST		21
#define ZYNQMP_DMA_CMD_STOP					22
#define ZYNQMP_DMA_CMD_LOAD_FILTER			23

//! [ZYNQMP_DMA_COMMANDS]

#define ZYNQMP_DMA_ERROR_OK			0
#define ZYNQMP_DMA_ERROR_UNKNOWN		1
#define ZYNQMP_DMA_ERROR_BAD_COMMAND	2
#define ZYNQMP_DMA_ERROR_BAD_STREAM	3
#define ZYNQMP_DMA_ERROR_BAD_PAY_LOAD	4
#define ZYNQMP_DMA_ERROR_UNCONF_DESC	5
#define ZYNQMP_DMA_ERROR_UNCONF_BUFF	6
#define ZYNQMP_DMA_ERROR_NOT_TX		7
#define ZYNQMP_DMA_ERROR_NOT_RX		8
#define ZYNQMP_DMA_ERROR_DESC_RANGE	9
#define ZYNQMP_DMA_ERROR_DATA_RANGE	10
#define ZYNQMP_DMA_ERROR_MSG_LEVEL	11
#define ZYNQMP_DMA_ERROR_OUTSIDE_VA	12
#define ZYNQMP_DMA_INSUFFICIENT_MEM	13

#define ZYNQMP_DMA_ERROR_FIRMWARE_DETECTED 20
#define ZYNQMP_DMA_ERROR_DESC_SOF_EOF 	21
#define ZYNQMP_DMA_ERROR_DESC_LENGTH 		22
#define ZYNQMP_DMA_ERROR_DESC_TIME_FRAME 	23

/* Note these match Xsp3ErrFlag_* where possible */ 
#define ZYNQMP_DMA_ERROR_DESC_OVERRUN		0x0200
#define ZYNQMP_DMA_ERROR_DESC_TF_MASK		0x0400
#define ZYNQMP_DMA_ERROR_DESC_SOFEOF		0x0800
#define ZYNQMP_DMA_ERROR_DESC_COUNT		0x1000
#define ZYNQMP_DMA_ERROR_SOCKET			0x2000

#define ZYNQMP_DMA_ERROR_MOD_OVERRUN		0x010000

#define ZYNQMP_DMA_ERROR_DESC_OVERRUN_SCAL	0x020000
#define ZYNQMP_DMA_ERROR_DESC_TF_MASK_SCAL	0x040000
#define ZYNQMP_DMA_ERROR_DESC_SOFEOF_SCAL		0x080000
#define ZYNQMP_DMA_ERROR_DESC_COUNT_SCAL		0x100000
#define ZYNQMP_DMA_ERROR_SOCKET_SCAL			0x200000
#define ZYNQMP_DMA_ERROR_MOD_OVERRUN_SCAL		0x1000000

#define ZYNQMP_DMA_BD_STS_COMPLETE_MASK	0x80000000 /**< Completed */
#define ZYNQMP_DMA_BD_STS_DEC_ERR_MASK	0x40000000 /**< Decode error */
#define ZYNQMP_DMA_BD_STS_SLV_ERR_MASK	0x20000000 /**< Slave error */
#define ZYNQMP_DMA_BD_STS_INT_ERR_MASK	0x10000000 /**< Internal err */
#define ZYNQMP_DMA_BD_STS_ALL_ERR_MASK	0x70000000 /**< All errors */
#define ZYNQMP_DMA_BD_STS_RXSOF_MASK	0x08000000 /**< First rx pkt */
#define ZYNQMP_DMA_BD_STS_RXEOF_MASK	0x04000000 /**< Last rx pkt */
#define ZYNQMP_DMA_BD_STS_ALL_MASK		0xFC000000 /**< All status bits */

/*! @defgroup NGZMP_DMA_STREAM_NUMBER  Number used to identify DMA streams controlled ZynqMP FPGA.
    @ingroup NGZMP_DMA
  @{
*/
#define ZYNQMP_DMA_STREAM_BNUM_DIRECT		0		//!< No DMA stream specified, use absolute addresses. .. Common to all uses of driver?

#define NGZMP_DMA_STREAM_BNUM_DIRECT		0		//!< No DMA stream specified, use absolute addresses.
#define NGZMP_DMA_STREAM_BNUM_PLAYBACK0		1		//!< Playback DMA and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_BNUM_PLAYBACK1		2		//!< Playback DMA and associated descriptors and memory buffer. Currently unused.
#define NGZMP_DMA_STREAM_BNUM_SCOPE0		3		//!< Scope0 DMA (for scope streams 0:2) and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_BNUM_SCOPE1		4		//!< Scope1 DMA (for scope streams 3:5) and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_BNUM_SCOPE2		5		//!< Scope2 DMA (for scope streams 3:5) and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_BNUM_HIST_TEST		6		//!< DMA for sending list of address to histogramme for testing.
#define NGZMP_DMA_STREAM_BNUM_HIST_READ		7		//!< DMA for reading data from Histogram PL memory to PS DRAM

/* These can be set the for use in any design which uses the UDP core */
#define ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G	8		//!< Use DMA Output PS DRAM over 10 G Ethernet.
#define ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM	9		//!< Use DMA filling PS DRAM from 10 G Ethernet.


//! @}
#define ZYNQMP_DMA_STREAM_NUM				16

/*! @defgroup NGZMP_DMA_STREAM_MASK  Binary Mask used to identify multiple DMA streams in FPGA.
    @ingroup ZGZMP_DMA
    @{
*/
#define NGZMP_DMA_STREAM_MASK_DIRECT		(1<<NGZMP_DMA_STREAM_BNUM_DIRECT)		//!< No DMA stream specified, use absolute addresses.
#define NGZMP_DMA_STREAM_MASK_PLAYBACK0		(1<<NGZMP_DMA_STREAM_BNUM_PLAYBACK0)		//!< Playback DMA and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_MASK_PLAYBACK1		(1<<NGZMP_DMA_STREAM_BNUM_PLAYBACK1)		//!< Playback DMA and associated descriptors and memory buffer. Currently unused.
#define NGZMP_DMA_STREAM_MASK_SCOPE0		(1<<NGZMP_DMA_STREAM_BNUM_SCOPE0)		//!< Scope0 DMA (for scope streams 0:2) and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_MASK_SCOPE1		(1<<NGZMP_DMA_STREAM_BNUM_SCOPE1)		//!< Scope1 DMA (for scope streams 3:5) and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_MASK_SCOPE2		(1<<NGZMP_DMA_STREAM_BNUM_SCOPE2)		//!< Scope2 DMA (for scope streams 3:5) and associated descriptors and memory buffer.
#define NGZMP_DMA_STREAM_MASK_HIST_TEST		(1<<NGZMP_DMA_STREAM_BNUM_HIST_TEST)		//!< DMA for sending list of address to histogramme for testing.
#define NGZMP_DMA_STREAM_MASK_HIST_READ		(1<<NGZMP_DMA_STREAM_BNUM_HIST_READ)		//!< DMA for reading data from Histogram PL memory to PS DRAM

/* These can be set the for use in any design which uses the UDP core */
#define ZYNQMP_DMA_STREAM_MASK_DRAM_TO_10G	(1<<ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G)		//!< Use DMA Output PS DRAM over 10 G Ethernet.
#define ZYNQMP_DMA_STREAM_MASK_10G_TO_DRAM	(1<<ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM)		//!< Use DMA filling PS DRAM from 10 G Ethernet.
//! @}

#define ZYNQMP_DMA_STATE_DESC_CONF	(1<<0)
#define ZYNQMP_DMA_STATE_BUFFER_CONF	(1<<1)
#define ZYNQMP_DMA_STATE_DESC_BUILT	(1<<2)
#define ZYNQMP_DMA_STATE_DESC_DEBUG	(1<<3)

#define ZYNQMP_DMA_DEBUG_DESC_SMALL			1
#define ZYNQMP_DMA_DEBUG_DESC_NEAR_PACKET		2
#define ZYNQMP_DMA_DEBUG_DESC_INC_FRAME		4

#define ZYNQMP_DMA_DEBUG_DESC_ALL_SMALL		0x10000
#define ZYNQMP_DMA_DEBUG_DESC_SHORT_BURSTS	0x20000

#define ZYNQMP_DMA_START_CIRCULAR				1		// Set Tail descriptor so DMA will try to cycle the buffers.
#define ZYNQMP_DMA_START_FRAME_NUM				2		//!< Specify frame number rather than descriptor number

/*! @defgroup XSP3_DMA_LAYOUT MACROS to interpret the layout option when configuring memory, currently on XSPRESS3-mini only.
    @ingroup XSP3_DMA
    @{
*/
//! [XSP3_DMA_LAYOUT_CODE]
#define XSP3_CONF_MEM_OVERALL(x)			((x)&0xFF)
#define XSP3_CONF_MEM_GET_OVERALL(x)		((x)&0xFF)

#define XSP3_CONF_MEM_PLAYBACK_SCOPE(x)		(((x)&0x3F)<<8)
#define XSP3_CONF_MEM_GET_PLAYBACK_SCOPE(x)	(((x)>>8)&0x3F)

#define XSP3_CONF_MEM_HIST_OPTIONS(x)		(((x)&0x3F)<<14)
#define XSP3_CONF_MEM_GET_HIST_OPTIONS(x)	(((x)>>14)&0x3F)
#define XSP3_CONF_MEM_HIST_FRAME_SCALARS	0
#define XSP3_CONF_MEM_HIST_ALL_SCALARS		1
#define XSP3_CONF_MEM_HIST_10FRAME_DIAG		4
#define XSP3_CONF_MEM_HIST_ALL_DIAG			5
#define XSP3_CONF_MEM_HIST_10FRAME_LIST		6		/* For test purposes only */
#define XSP3_CONF_MEM_HIST_ALL_LIST			7


#define XSP3_CONF_MEM_NBITS_ENG(x)			(((x)&0xF)<<20)
#define XSP3_CONF_MEM_GET_NBITS_ENG(x)		(((x)>>20)&0xF)

#define XSP3_CONF_MEM_NUM_CHAN(x)			(((x)&0x3F)<<24)
#define XSP3_CONF_MEM_GET_NUM_CHAN(x)		(((x)>>24)&0x3F)
//! [XSP3_DMA_LAYOUT_CODE]

#define XSP3M_HIST_LIST_BURST_EVENTS		4096

/** @} */


//! [ZYNQMP_DMA_DESCRIPTOR]
/* This has  to be padded to at least 64 bytes boundaries and the padding space is used to store the virtual addresses of the descriptor and optionally data for ARM/Linux code */
typedef struct _axidma_desc
{
	u_int64_t phys_next;
	u_int64_t phys_addr;
	u_int32_t reserved1[2];
	u_int32_t control;
	u_int32_t status;
	u_int32_t app[5];
	u_int32_t frame_num;
//	void *virt_next;
//	u_int32_t *virt_addr;
	u_int32_t *buff_virt_addr;
} AXIDMADesc;
//! [ZYNQMP_DMA_DESCRIPTOR]

//! [ZYNQMP_DMA_STREAM]
#define ZYNQMP_NUM_DMA_SEGMENTS 8
typedef struct
{
	int32_t state;
	int32_t is_tx;
	u_int64_t base_addr;
	u_int64_t desc_cpu;		// Physical address in CPU address space
	u_int64_t desc_dma;		// Physical address in DMA address space
//	u_int64_t data_start;
	u_int64_t data_size;	// Total data size, sum of all segements.
	u_int64_t transfer_start;
	u_int64_t transfer_size;
	u_int32_t num_desc;
	u_int32_t max_block_bytes;
	int32_t defined_desc;
	int32_t num_frames;
	struct ZYNQMP_DMA_BuffSegment {
		u_int64_t size;
		u_int64_t cpu_base;
		u_int64_t dma_base;
		u_int32_t addr_space;		// ZYNQMP_ADDR_SPACE_PS_LOW/ZYNQMP_ADDR_SPACE_PS_HIGH etc
	} buff_seg[ZYNQMP_NUM_DMA_SEGMENTS];
	u_int32_t desc_addr_space;		// ZYNQMP_ADDR_SPACE_PS_LOW/ZYNQMP_ADDR_SPACE_PS_HIGH etc
	u_int32_t readout_cur_desc;
	u_int32_t readout_error_flags;
	u_int32_t readout_error_data;
	enum {AllOneFrame, FramePerDesc, FrameByBytes } frame_rule;
} DMAStream;
//! [ZYNQMP_DMA_STREAM]

typedef struct 
{
	u_int32_t *virt_base;
	AXIDMADesc *desc_virt_base;
	u_int32_t *buff_virt_base[ZYNQMP_NUM_DMA_SEGMENTS];
} DMAPrivate;

//! [ZYNQMP_DMA_MSG_BUILD_DESC]
typedef struct
{
	int src_stream;
	int flags;
	u_int64_t addr;
	u_int64_t size_bytes;
	u_int32_t block_size;
} ZYNQMP_DMA_MsgBuildDesc;
//! [ZYNQMP_DMA_MSG_BUILD_DESC]

//! [ZYNQMP_DMA_MSG_BUILD_DEBUG_DESC]
typedef struct
{
	int src_stream;
	u_int32_t flags;
	u_int64_t addr;
	u_int64_t size_bytes;
	u_int32_t packet_size;
	u_int32_t frame_size;
} ZYNQMP_DMA_MsgBuildDebugDesc;
//! [ZYNQMP_DMA_MSG_BUILD_DEBUG_DESC]


//! [ZYNQMP_DMA_MSG_TEST_PAT]
typedef struct
{
	u_int64_t address;
	u_int64_t num_bytes;
	u_int32_t start_val;
	u_int32_t options;
} ZYNQMP_DMA_MsgTestPat;
//! [ZYNQMP_DMA_MSG_TEST_PAT]

//! [ZYNQMP_DMA_MSG_PRINT]
typedef struct
{
	u_int64_t offset;
	u_int64_t num_bytes;
	u_int32_t options;
} ZYNQMP_DMA_MsgPrint;
//! [ZYNQMP_DMA_MSG_PRINT]

//! [ZYNQMP_DMA_MSG_START]
typedef struct
{
	u_int32_t first_desc;
	u_int32_t num_desc;
	u_int32_t options;
} ZYNQMP_DMA_MsgStart;
//! [ZYNQMP_DMA_MSG_START]

//! [ZYNQMP_DMA_MSG_RESEND]
typedef struct
{
	u_int32_t first_desc;
	u_int32_t num_desc;
	u_int32_t options;
} ZYNQMP_DMA_MsgResend;
#define ZYNQMP_DMA_RESEND_FRAME_NUM	1		//!< Specify frame number rather than descriptor number
//! [ZYNQMP_DMA_MSG_RESEND]


//! [ZYNQMP_DMA_MSG_REUSE]
typedef struct
{
	u_int32_t first_desc;
	u_int32_t num_desc;
	u_int32_t options;
} ZYNQMP_DMA_MsgReuse;
//! [ZYNQMP_DMA_MSG_REUSE]

//! [ZYNQMP_DMA_MSG_PRINT_DESC]
typedef struct
{
	u_int32_t first_desc;
	u_int32_t num_desc;
	u_int32_t options;
} ZYNQMP_DMA_MsgPrintDesc;
//! [ZYNQMP_DMA_MSG_PRINT_DESC]

//! [ZYNQMP_DMA_MSG_CHECK_DESC]
typedef struct
{
	u_int32_t first_desc;
	u_int32_t num_desc;
	u_int32_t options;
} ZYNQMP_DMA_MsgCheckDesc;
//! [ZYNQMP_DMA_MSG_CHECK_DESC]



//! [ZYNQMP_DMA_MSG_CHECK]
#define ZYNQMP_DMA_MSG_CHECK_NONE		1			// Do not do any checks, disables default checks
#define ZYNQMP_DMA_MSG_CHECK_LENGTH	2			// Check length from trailer in 10G Rx and Scalers
#define ZYNQMP_DMA_MSG_CHECK_10GRX	4			// Check UDP Header flags and frame
#define ZYNQMP_DMA_MSG_CHECK_FRAME_PER_DESC	8	// Check SOF and EOF present, 1 local link frame per descriptor
#define ZYNQMP_DMA_MSG_CHECK_1_FRAME	0x10		// Check SOF in first desc and EOF in last, 1 large local link frame (scope)
#define ZYNQMP_DMA_MSG_CHECK_TIMEFRAME 0x20			// Check time frame is in the app0 field for Xspress3 mini hist and scalars

//! [ZYNQMP_DMA_MSG_CHECK]

//! [ZYNQMP_DMA_MSG_GET_DESC_STATUS]
typedef struct
{
	u_int32_t first_desc;
} ZYNQMP_DMA_MsgGetDescStatus;
//! [ZYNQMP_DMA_MSG_GET_DESC_STATUS]


#define ZYNQMP_DMA_MSG_TP_INC32		0
#define ZYNQMP_DMA_MSG_TP_INC8		1
#define ZYNQMP_DMA_MSG_TP_SLIDE		2
#define ZYNQMP_DMA_MSG_TP_DEADBEEF	4

#define ZYNQMP_DMA_MSG_TP_PLAYBACK1	10
#define ZYNQMP_DMA_MSG_TP_PLAYBACK2	11
#define ZYNQMP_DMA_MSG_TP_PLAYBACK4	12
#define ZYNQMP_DMA_MSG_TP_PLAYBACK8	13

#define ZYNQMP_DMA_MSG_PRINT_1COL		(1<<0)
#define ZYNQMP_DMA_MSG_PRINT_DEC		(1<<1)

typedef struct
{
	u_int32_t status[ZYNQMP_DMA_STREAM_NUM];			// From RX or TX Register
	DMAStream stream_def[ZYNQMP_DMA_STREAM_NUM];
	int mem_layout;
	int padding[4];		// For future expansion
} ZYNQMP_DMA_StatusBlock;



typedef struct {
	u_int64_t num_good;
	u_int64_t num_completed;
	u_int64_t last_tf;
	u_int32_t error_code;
	u_int32_t desc_num;
	u_int32_t dma_status;
	u_int32_t desc_status;
	u_int64_t desc_tf;
} ZYNQMP_DMA_CheckCircular;

#endif /* NGPD_DMA_PROTOCOL_H_ */
