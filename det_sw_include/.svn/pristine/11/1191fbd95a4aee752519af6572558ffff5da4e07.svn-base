#ifndef _TFG2_H
#define _TFG2_H
#define TFG2_MEM_SIZE_BYTES (256*1024*1024)

#define TFG2_MEM_ROWLEN 1024
#define TFG2_CC_NUM_CHAN 32
#define TFG2_CC_NUM_TF  (2*1024*1024)

typedef struct _tfg_config
{
	TFG2Mem * tfdata;
 	int num;
 	uint32_t cycles;
 	uint32_t frame_inc;
	uint32_t port_cont;				// Port output control
	uint32_t tfout_cont;			// Frame outputs control
	uint32_t start_cont;			// Start Control
	uint32_t debouncea, debounceb;	// Debounce for external starts
	uint32_t s1_cont;				// Control register in S1
	uint32_t s2_cont;				// Control register in S2
	uint32_t s1_cont_b;				// Control register B in S1
	uint32_t extra_start_cont;		// Extra start control for I12 onwards
	uint32_t debouncec, debounced;	// Debounce for external starts
	uint32_t tfout_cont_b;			// Additional control for TF out 3
	uint32_t spare_a, spare_b;      // Spares so can add features but hopefull driver stays backwards compatible (ish)
	uint32_t spare_c, spare_d;      // Spares so can add features but hopefull driver stays backwards compatible (ish)
} TFG2Config;

typedef struct _cc_config
{
	uint32_t control;				// Control Register
	uint32_t debouncea, debounceb;	// Debounce for scalers starts
	uint32_t control_b;
	uint32_t control_c;
	uint32_t spare_a, spare_b;      // Spares so can add features but hopefull driver stays backwards compatible (ish)
} TFG2CCConfig;

typedef struct _tfg_op
{
	uint32_t tf; // Internal time frame values
	uint32_t ttl_tf;	// TTL time frame assuming all are output or correctly looped back
	uint32_t s1_comb_tf;
	uint32_t s2_comb_tf;
	uint32_t port;
	uint32_t time_int;
	uint32_t status;
	uint32_t status_mask;
	uint32_t cc_stat;
	uint32_t cc_stat_mask;
	double start_time, duration;
} TFG2Outputs;


extern double tfg2_time_units[8]; 
extern char *tfg2_trig_full_names[17];
extern char *tfg2_trig_short_names[17];
extern char *tfg2_clk_names[4];
extern char *tfg2_lemo_names[7];
extern char *tfg2_alt_trigger_names[17][8];
extern char *tfg2_tf3_section_names[4];
extern char *tfg2_tf3_mem_bit_names[4];
extern char *tfg2_extra_veto_names[32];

#ifndef TFG_DIRECT
uint32_t tfg2_rd_tfg_status(int path);
uint32_t tfg2_rd_tfg_cont(int path);
uint32_t tfg2_rd_tfg_cur_cycle(int path);
uint32_t tfg2_rd_tfg_frame(int path);
uint32_t tfg2_rd_tfg_port(int path);
uint32_t tfg2_rd_tfg_addr(int path);
uint32_t tfg2_rd_tfg_timer(int path);
uint32_t tfg2_rd_tfg_starts(int path);
uint32_t tfg2_rd_tfg_circ_time(int path);
uint32_t tfg2_rd_tfg_edge_pos(int path);
uint32_t tfg2_rd_tfg_ttl_tf(int path);
uint32_t tfg2_rd_tfg_s1_comb_tf(int path);
uint32_t tfg2_rd_tfg_s2_comb_tf(int path);
uint32_t tfg2_rd_tfg_time_int(int path);
uint32_t tfg2_rd_s1_cont(int path);
uint32_t tfg2_rd_s2_cont(int path);
uint32_t tfg2_rd_ddr_cont(int path);
uint32_t tfg2_rd_cc_cont(int path);
uint32_t tfg2_rd_cc_stat(int path);
uint32_t tfg2_rd_cc_debouncea(int path);
uint32_t tfg2_rd_cc_debounceb(int path);
uint32_t tfg2_rd_sp_cont(int path);
uint32_t tfg2_rd_tfg_cycles(int path);
uint32_t tfg2_rd_tfg_port_cont(int path);
uint32_t tfg2_rd_tfg_tfout_cont(int path);
uint32_t tfg2_rd_tfg_start_cont(int path);
uint32_t tfg2_rd_tfg_frame_inc(int path);
uint32_t tfg2_rd_tfg_debouncea(int path);
uint32_t tfg2_rd_tfg_debounceb(int path);
uint32_t tfg2_rd_tfg_tfout_cont_b(int path);

uint32_t tfg2_rd_tfg_extra_start_cont(int path);
uint32_t tfg2_rd_tfg_debouncec(int path);
uint32_t tfg2_rd_tfg_debounced(int path);
uint32_t tfg2_rd_s1_cont_b(int path);
uint32_t tfg2_rd_cc_cont_b(int path);
uint32_t tfg2_rd_cc_cont_c(int path);
uint32_t tfg2_rd_xspress2_cont_a(int path);
uint32_t tfg2_rd_xspress2_cont_b(int path);

int tfg2_start(int path);
int tfg2_arm(int path);
int tfg2_clear_tfg_starts(int path);
int	tfg2_wr_cc_cont(int path, uint32_t d);
int	tfg2_wr_cc_debouncea(int path, uint32_t d);
int	tfg2_wr_cc_debounceb(int path, uint32_t d);
int	tfg2_wr_cc_cont_b(int path, uint32_t d);
int	tfg2_wr_cc_cont_c(int path, uint32_t d);
int	tfg2_wr_tfg_extra_start_cont(int path, uint32_t d);
int	tfg2_wr_s1_cont_b(int path, uint32_t d);
int	tfg2_wr_tfg_debouncea(int path, uint32_t d);
int	tfg2_wr_tfg_debounceb(int path, uint32_t d);
int	tfg2_wr_tfg_debouncec(int path, uint32_t d);
int	tfg2_wr_tfg_debounced(int path, uint32_t d);
int	tfg2_wr_tfg_cont(int path, uint32_t d);
int	tfg2_wr_tfg_frame_inc(int path, uint32_t d);
int	tfg2_wr_tfg_port_cont(int path, uint32_t d);
int	tfg2_wr_tfg_tfout_cont(int path, uint32_t d);
int	tfg2_wr_tfg_tfout_cont_b(int path, uint32_t d);
int	tfg2_wr_xspress2_cont_a(int path, uint32_t d);
int	tfg2_wr_xspress2_cont_b(int path, uint32_t d);
#endif

int tfg2_write_mem(int path, int bank, uint32_t start, uint32_t size, uint32_t *ptr);
int tfg2_read_mem(int path, int bank, uint32_t start, uint32_t size, uint32_t *ptr);
int	tfg2_wr_tfg_cycles(int path, uint32_t d);
int	tfg2_wr_tfg_start_cont(int path, uint32_t d);
int	tfg2_continue(int path);
int tfg2_pause(int path);
int tfg2_stop_eoc(int path);
int	tfg2_wr_s1_cont(int path, uint32_t d);
int	tfg2_wr_s2_cont(int path, uint32_t d);
int	tfg2_wr_ddr_cont(int path, uint32_t d);
int	tfg2_wr_sp_cont(int path, uint32_t d);
int tfg2_rd_tfg_outputs(int path, TFG2Outputs *op);
int tfg2_config_tfg(int path, TFG2Config *config);
int tfg2_config_tfg_regs(int path, TFG2Config *config);
int tfg2_irqenb (int path, uint32_t enb_mask, int procid, int signum);
int tfg2_config_cc(int path, TFG2CCConfig *config);


#define SI_TFG2 __SI_CODE(__SI_RT, 1)

int tfg2_clear_wait(int path, int bank,  uint32_t start, uint32_t size);
int tfg2_clear_start(int path, int bank,  uint32_t start, uint32_t size);
int tfg2_wait_for_clear(int path, int bank);


#endif
