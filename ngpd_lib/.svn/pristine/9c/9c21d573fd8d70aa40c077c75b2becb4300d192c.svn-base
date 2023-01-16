#ifndef ZYNQMP_LIB_DRIVER_H
#define ZYNQMP_LIB_DRIVER_H


int zynqmp_read(int path, int card, int addr_space, int offset, int size, u_int32_t *value);
int zynqmp_write(int path, int card, int addr_space, int offset, int size, u_int32_t *value);
int zynqmp_rmw(int path, int card, int addr_space, int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value);
int zynqmp_dma(int path, int card, int cmd, int stream_mask, void *msg, int32_t *ret_value, u_int32_t *details);
int zynqmp_dma_read(int path, int card, int cmd, int stream_mask, int first, int num, void *msg, int32_t *ret_value, u_int32_t *details);
int zynqmp_read_dma_status_block(int path, int card, int offset_lwords, int size_lwords, u_int32_t *value);
int zynqmp_read_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *value);
int zynqmp_write_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *value);
int zynqmp_memzero(int path, int card, int addr_space, int offset, int size);
int zynqmp_zero_dma_buff(int path, int card, int stream, int offset, int size);
int zynqmp_read_row(int path, int card, int addr_space, int row_len, int num_streams, int row, u_int32_t offset, u_int32_t size, u_int32_t *data);

#if 0
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