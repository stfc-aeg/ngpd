#ifndef NGPD_LIB_DRIVER_H
#define NGPD_LIB_DRIVER_H

#define NGPD_MAX_ERROR_MESSAGE 200

int zynqmp_dma_wait(int path, int card, int stream, double timeout);

#if 0

int xsp4_write(int path, int addr_space, int offset, int size, u_int32_t *value);
int xsp4_read(int path, int addr_space, int offset, int size, u_int32_t *value);
int xsp4_rmw(int path, int addr_space, int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value);
int xsp4_dma_v7(int path, int cmd, int stream_mask, void *msg, int32_t *ret_value, u_int32_t *details);
int xsp4_dma_read_v7(int path, int cmd, int stream_mask, int first, int num, void *msg, int32_t *ret_value, u_int32_t *details);
int xsp4_read_dma_status_block(int path, int offset, int size, u_int32_t *value);
//int xsp3_write_reg(int path, int chan, int region, int offset, int size, u_int32_t *value)
//int xsp3_read_reg(int path, int chan, int region, int offset, int size, u_int32_t *value)
int xsp3_dma_print_scope_data(int path, int card, XSP3_DMA_MsgPrint *msg);
int xsp3_dma_print_desc(int path, int card, u_int32_t stream, XSP3_DMA_MsgPrintDesc *msg);
//int xsp3m_write_dma_buff(int path, int stream, int offset, int size, u_int32_t *value);
int xsp4_memzero(int path, int addr_space, int offset, int size);
int xsp4_zero_dma_buff(int path, int stream, int offset, int size);
int xsp3m_write_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *value);
int xsp3m_read_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *value);

int xsp4_reset_c2c(int path, int card, u_int32_t *status);
int xsp4_program_virtex7(int path, int card, int fwsel, u_int32_t *status);

int xsp4_spi_open_chan();
int xsp4_spi_close_chan();
int xsp4_spi_open_timing();

int xsp4_spi_chan_wr_rd(int path, int chan, int region, int n, u_int16_t * tx_data, u_int16_t *rx_data);
int xsp4_spi_timing_wr_rd(int path, int chip, int n, u_int32_t *data, u_int32_t * rx_data);

int xsp4_spi_read_psu_revision(int path, u_int16_t *rx_data);
int xsp4_spi_read_psu_control(int path, u_int16_t *rx_data);

extern XspressGeneration xsp4_generation;
int xsp4_i2c_open_adc();

int xsp4_write_spi_par_exp(int path, int card, int chip, int reg, int val);
int xsp4_read_spi_par_exp(int path, int card, int chip, int reg, int *val);
#endif
#endif