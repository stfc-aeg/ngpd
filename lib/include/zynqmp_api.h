/*
 * zynqmp_api.h
 *
 *  Created on: 5 Nov 2013
 *      Author: grm84
 */

/* This file defines the API that is used to communicate with the ZynqMP hardware first used on the NGPD */

#ifndef ZYNQMP_API_H_
#define ZYNQMP_API_H_

// #include <stdlib.h>

#if defined(__MINGW32__) & !defined(U_INTX_T)
#define U_INTX_T 1
typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
#endif

typedef struct _ZynqMPHandle {
	int m_skt;
	int m_valid;
	enum {ZynqMPUnknown, ZynqMPGen1} protocol_generation;
	int max_tx_bytes;
	int max_rx_bytes;
	u_int32_t base_address;
	u_int32_t seq;
	pthread_mutex_t mutex;
	int max_posted_writes;
	int num_posted_writes;
} ZynqMPHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* The functions provided by the library.
 */
ZynqMPHandle * zynqmp_initialise(const char* hostIPAdress, int port);
void  zynqmp_close(ZynqMPHandle* zynqmp);
int32_t zynqmp_access_read(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width, u_int32_t address, u_int32_t num_reads, int offset_inc_scale, u_int8_t* payload);
int32_t zynqmp_access_write(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width, u_int32_t address, u_int32_t num_writes, int offset_inc_scale, u_int8_t* payload );
int32_t zynqmp_personality_write(ZynqMPHandle* zynqmp, u_int32_t subCommand, u_int32_t width, u_int32_t streamMask, size_t size, u_int8_t* payload, int num_details, u_int32_t *details, int32_t * remote_ret_code);
int32_t zynqmp_access_memzero(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t num_writes, int offset_inc_scale);
int32_t zynqmp_access_rmw(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *result);
int32_t zynqmp_access_read_roi(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t row_length_ops, u_int32_t num_rows, u_int32_t src_stride_ops, u_int32_t dst_stride_ops, int offset_inc_scale, u_int8_t* payload);
int32_t zynqmp_personality_read(ZynqMPHandle* zynqmp, u_int32_t subCommand, u_int32_t width_bytes, u_int32_t address, u_int32_t num, int offset_inc_scale, u_int8_t* payload, u_int32_t extra1, u_int32_t extra2, u_int32_t extra3);
int32_t zynqmp_access_read_intl(ZynqMPHandle* zynqmp, u_int32_t bus, u_int32_t width_bytes, u_int32_t address, u_int32_t num, int row_len, int num_streams, int stream, u_int8_t* payload);

char * zynqmp_get_error_message();


int zynqmp_SetInt(void* zynqmp, int chan, int region, int offset, u_int32_t value);
int zynqmp_GetInt(void* zynqmp, int chan, int region, int offset, u_int32_t *value);
int zynqmp_SetIntArray(void* zynqmp, int chan, int region, int offset, size_t size, u_int32_t* value);
int zynqmp_GetIntArray(void* zynqmp, int chan, int region, int offset, size_t size, u_int32_t* value);
int zynqmp_DRAMWrite(void* zynqmp, u_int32_t address, size_t size, u_int32_t* value);
int zynqmp_PersonalityWrite(void* zynqmp, u_int32_t sub_command, u_int32_t function_mask, size_t size, u_int32_t* value, int num_details, u_int32_t *details);
int zynqmp_ConfigUDP(void* zynqmp, char* fpgaMACaddress, char* fpgaIPaddress, int fpgaPort, char* hostIPaddress, int hostPort);
int zynqmp_SetHostPort(void* zynqmp, int hostPort);
int zynqmp_SetPacketSize(void* zynqmp, int sizeInBytes);
int zynqmp_ResetFrameCounter(void* zynqmp);
int zynqmp_GetDMAStatusBlock(void* zynqmp, size_t size, u_int32_t *value);
int zynqmp_RDMARead(void* zynqmp, u_int32_t address, size_t size, u_int32_t *value);
int zynqmp_RDMAWrite(void* zynqmp, u_int32_t address, size_t size, u_int32_t *value);
int zynqmp_SPIRead(void* zynqmp, u_int32_t address, size_t size, u_int32_t *value);
int zynqmp_SPIWrite(void* zynqmp, u_int32_t address, size_t size, u_int32_t *value);
int zynqmp_ChanSPIWrite(void* zynqmp, u_int32_t address, size_t num_writes, u_int16_t *value);
int zynqmp_ChanSPIRead(void* zynqmp, u_int32_t address, size_t num_reads, u_int16_t *value);


int zynqmp_ReadRawReg(void* zynqmp, u_int32_t address, size_t size, u_int32_t* value);
int zynqmp_I2CWrite(void* zynqmp, u_int32_t fem_i2c_bus, u_int32_t address, size_t size, u_int8_t *value);
int zynqmp_I2CRead(void* zynqmp, u_int32_t fem_i2c_bus, u_int32_t address, size_t size, u_int8_t *value);
int zynqmp_WriteFanControl(void* zynqmp, int offset, size_t size, u_int32_t* value);
int zynqmp_ReadFanControl(void* zynqmp, u_int32_t offset, size_t size, u_int32_t* value);
int zynqmp_ConfigWrite(void* zynqmp, u_int32_t address, size_t size, u_int8_t *value);
int zynqmp_ConfigRead(void* zynqmp, u_int32_t address, size_t size, u_int8_t *value);

int32_t zynqmp_SetFarmLUT(void* zynqmp, int index, char* hostIPaddress, int hostPort);
int zynqmp_RMW(void* zynqmp, int chan, int region, int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value);
int zynqmp_RDMARMW(void* zynqmp, int address, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value);

int zynqmp_WriteDMABuff(void* zynqmp, int dma_stream, u_int32_t address, size_t num_writes, u_int32_t* value); 
int zynqmp_ReadDMABuff(void* zynqmp, int dma_stream, u_int32_t address, size_t num_reads, u_int32_t* value); 
int zynqmp_ZeroDMABuff(void* zynqmp, int dma_stream, u_int32_t address, size_t num_writes); 
int zynqmp_ReadDMABuffRoI(void* zynqmp, int dma_stream, u_int32_t address, u_int32_t row_length_ops, u_int32_t num_rows, u_int32_t src_stride_ops, u_int32_t dst_stride_ops, u_int32_t* value);


int zynqmp_SetIntMGTDRP(void* zynqmp, int link, int offset, u_int32_t value);
int zynqmp_SetIntArrayMGTDRP(void* zynqmp, int link, int offset, size_t num_writes, u_int32_t* value); 
int zynqmp_GetIntMGTDRP(void* zynqmp, int link, int offset, u_int32_t *value);
int zynqmp_GetIntArrayMGTDRP(void* zynqmp, int link, int offset, size_t num_reads, u_int32_t* value);

int zynqmp_I2CWriteRegAddr(void* zynqmp, u_int32_t fem_i2c_bus, u_int32_t address, u_int32_t reg_addr, size_t size, u_int8_t *value);
int zynqmp_I2CReadRegAddr(void* zynqmp, u_int32_t fem_i2c_bus, u_int32_t address, u_int32_t reg_addr, size_t size, u_int8_t *value);
int zynqmp_XADCRead(void* zynqmp, u_int32_t address, size_t size, u_int32_t *value);

int zynqmp_WriteDMAorClk(void* zynqmp, int offset, size_t num_writes, u_int32_t* value);
int zynqmp_ReadDMAorClk(void* zynqmp, int offset, size_t num_reads, u_int32_t* value);

int zynqmp_MPSPIWrite(void* zynqmp, int chip, int reg, u_int32_t *value);
int zynqmp_MPSPIRead(void* zynqmp, int chip, int reg, u_int32_t *value);
int zynqmp_PersonalityRead(void* zynqmp, u_int32_t sub_command, u_int32_t stream_mask, u_int32_t width_bytes, u_int32_t first, u_int32_t num_ops, void* value); 
int zynqmp_WriteDriverStatics(void* zynqmp, int offset, size_t num_writes, u_int32_t* value);
int zynqmp_ReadDriverStatics(void* zynqmp, int offset, size_t num_reads, u_int32_t* value); 

#ifdef __cplusplus
}  /* Closing brace for extern "C" */
#endif

#endif /* ZYNQMP_API_H_ */
