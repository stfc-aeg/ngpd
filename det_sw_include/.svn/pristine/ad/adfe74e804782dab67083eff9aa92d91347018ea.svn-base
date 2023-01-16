/** @file  	 ml605fg.h
	@brief   Header file for Interface library to the ml605fg.ko device driver.
	@author  W. Helsby  

 */

#ifndef ML605FG_H
#define ML605FG_H


uint32_t frm_grab_get_mem_size_bytes(int fd);
int frm_grab_get_width_height(int fd, int *width, int *height);
int frm_grab_get_bytes_per_pixel(int fd);
uint32_t frm_grab_read_revision(int fd);
uint32_t frm_grab_read_reg_present(int fd);
uint32_t frm_grab_read_run_mode(int fd);
uint32_t frm_grab_read_test_pat(int fd);

int frm_grab_generate_mem(int fd, uint32_t start, uint32_t num, u_int32_t data_type, uint32_t seed);
int frm_grab_read_mem(int fd, uint32_t start, uint32_t num, void *ptr, u_int32_t data_type);

int frm_grab_read_reg(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int frm_grab_read_bram(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int frm_grab_write_mem(int fd, uint32_t start, uint32_t num, void *ptr, u_int32_t data_type);
int frm_grab_write_reg(int fd, uint32_t start, uint32_t num, uint32_t *ptr); 
int frm_grab_write_bram(int fd, uint32_t start, uint32_t num, uint32_t *ptr);
int frm_grab_write_fmc200_reg(int fd, uint32_t start, uint32_t num, uint8_t *ptr); 
int frm_grab_read_fmc200_reg(int fd, uint32_t start, uint32_t num, uint8_t *ptr);


int frm_grab_write_run_mode(int fd, uint32_t d);
int frm_grab_write_test_pat_reg(int fd, uint32_t d);
int frm_grab_setup_sprite(int fg, int width, int height, int dx, int dy);
int frm_grab_setup_vid_in(int fd, uint32_t cont_reg);
int frm_grab_start_test_pat(int fd, int period_ms, int tp_type);
int frm_grab_start_normal(int fd, int disable_dval, int debouce_fval);
int frm_grab_reset_vdma(int fd, int which);
int frm_grab_bit_slip(int fd, int num);


int frm_grab_fmc200_setup_video(int fd, int options);
int frm_grab_write_fmc_clk_delay(int fd, uint32_t delay);
int frm_grab_roi_read(int fd, int num, FG_ROI *rois);
int frm_grab_setup_processing(int fd, FG_Processing *proc_opt);
u_int32_t frm_grab_get_mem_area_offset(int fd, int mem_area);
int frm_grab_read_frame_status(int fd, int *vid_in, int *current, int *prev1, int *prev2, int *prev3, int *mask, int *monitor, int *bbox); 
int frm_grab_read_frame_status_full(int fd, uint32_t *vid_in_and_prev1, uint32_t *prev2_and_mask, uint32_t *prev3_and_monitor, uint32_t *current_and_bbox );
int frm_grab_set_exposure(int fd, int even_expose, int odd_expose, int even_gap, int odd_gap);

#endif

