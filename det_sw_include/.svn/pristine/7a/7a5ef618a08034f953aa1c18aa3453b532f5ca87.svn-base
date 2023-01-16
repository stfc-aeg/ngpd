
#ifndef XH_H
#define XH_H

int xh_generate_mem(int fd, uint32_t start, uint32_t num, uint32_t seed);
int xh_read_mem(int fd, uint32_t start, uint32_t num, uint32_t *ptr, int un_interleave);
int xh_read_scope(int fd, int t, int dt, int first_adc, int num_adc, uint32_t *ptr);
uint32_t xh_rd_revision(int fd);
uint32_t xh_get_mem_size_words(int fd);
int xh_dma_start(int fd, uint32_t start, uint32_t num);
int xh_dma_start_scope(int fd, uint32_t start, uint32_t num, uint32_t *old_status);
int xh_dma_start_normal(int fd, uint32_t start, uint32_t num_frames, uint32_t *old_status, uint32_t dma_control);

uint32_t xh_rd_dma_status(int fd);
uint32_t xh_rd_dma_burstno_cnt(int fd);
uint32_t xh_rd_dma_burstno_max(int fd);
uint32_t xh_rd_dma_control(int fd);
uint32_t xh_rd_dma_base_addr(int fd);
uint32_t xh_rd_debug_status(int fd);
uint32_t xh_rd_debug_burstno_cnt(int fd);

int xh_wr_dma_control(int fd, uint32_t d);
int xh_wr_dma_base_addr(int fd, uint32_t d);
int xh_wr_dma_burstno_max(int fd, uint32_t d);

int xh_wr_adc_control(int fd, uint32_t d);
uint32_t xh_rd_adc_control(int fd);
uint32_t xh_rd_adc_status(int fd);
int xh_rd_adc_stages(int fd, uint32_t *d);
int xh_rd_adc_tap_delay(int fd, int first, int num, uint32_t *ptr);
int xh_wr_adc_timers(int fd, uint32_t d);
uint32_t xh_rd_adc_timers(int fd);
int xh_rd_adc_eye_width(int fd, int start, int num, uint32_t *ptr);

int xh_rd_adc_bit_slip(int fd, int start, int num, uint32_t *ptr);

int xh_wr_adc_pwrdwn(int fd, uint32_t d);
uint32_t xh_rd_adc_pwrdwn(int fd);

int xh_wr_dma_adc_chip_sel(int fd, uint32_t d);
uint32_t xh_rd_dma_adc_chip_sel(int fd);
int xh_rd_timing_bram(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int xh_wr_timing_bram(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int xh_wr_trig_control(int fd, uint32_t d);
int xh_wr_trig_orbit_delay(int fd, uint32_t d);
int xh_wr_trig_num_groups(int fd, uint32_t d);
uint32_t xh_rd_trig_orbit_delay(int fd);
uint32_t xh_rd_trig_num_groups(int fd);

int xh_rd_trig_status(int fd, XHTrigStatus *p);
int xh_wr_trig_out_cont(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int xh_rd_trig_out_cont(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int xh_wr_trig_led_cont(int fd, uint32_t d);
uint32_t xh_rd_trig_led_cont(int fd);
uint32_t xh_trig_start(int fd);
uint32_t xh_trig_stop(int fd);
uint32_t xh_trig_continue(int fd);
int xh_wr_xdelay(int fd, uint32_t d);
uint32_t xh_rd_xdelay(int fd);
int xh_wr_hv_supply(int fd, uint32_t d);
uint32_t xh_rd_hv_supply(int fd);

int xh_wr_timing_fixed(int fd, uint32_t d);
uint32_t xh_rd_timing_fixed(int fd);
uint32_t xh_rd_orbit_status(int fd);
int xh_rd_inl_bram(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int xh_wr_inl_bram(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int xh_wr_invert_data(int fd, uint32_t d);
uint32_t xh_rd_invert_data(int fd);
int xh_rd_adc_fifo_rd_count(int fd, uint32_t start, uint32_t num, uint32_t *ptr);

#endif
