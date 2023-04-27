#define NGPD_MEASURE_TAIL_THRES_SIZE 256 //!< Number of values to write to tail thres BRAM

typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

typedef void mh_com;

const uint32_t NGPD_RUN_FLAGS_SCOPEMODE         = (1<<4);
const uint32_t NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS = (1<<5);
const uint32_t NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD = (1<<6);
const uint32_t NGPD_MEASURE_TAIL_THRES_SIZE = 256;

typedef struct  ngpd_diff_trig_t
{
	int thres, sep, data_delay, trig_delay;
	int delay_a, width_a;		// Dealy and stretch for the slow/wide trigger A
	int delay_b, width_b;		// Dealy and stretch for the slow/wide trigger B, used to mask accumulation in the baseline subtractor.

} NGPDDiffTrigger;

typedef struct  ngpd_base_sub_t
{
	int use_fixed;
	int fixed;
	int error_limit;
	int div_cont;
} NGPDBaseSubtract;

typedef struct ngpd_measure_t
{
	int use_abs_trig;
	int adaptive_tail_sum;
	int ignore_fall_time;
	int ignore_tail_sum;
	int enable_tail_subtract;
	int tail_subtract_test;
	int tail_subtract_test_neutron;
	int tail_sum_delay, tail_sum_num;
	double fall_time_frac;
	int min_height, max_height;
	int min_fall_time, max_fall_time;
	int min_tail_count;
	int tail_thres_m[NGPD_MEASURE_TAIL_THRES_SIZE];
	int tail_thres_c[NGPD_MEASURE_TAIL_THRES_SIZE];
} NGPDMeasure ;

typedef enum
{	NGPDScopeOpt_TBD 		= 1,		//!< To Be determined
} NGPDScopeOptionFlags;

typedef enum {NGPDGenError= -1, NGPDGenADQ14=0, NGPDGenZynqMP1} NGPDGeneration; 

typedef struct
{	NGPDScopeOptionFlags flags;					//!< Scope options flags {@link NGPDScopeOptionFlags};
	int nstreams;								//!< Number of scope streams to use where programmable (xspress3V7 onwards)
} NGPDScopeOptions;

typedef struct ngpd_scope_data_module
{
	struct ngpd_scope_module_head
	{
		char magic[8];
		int32_t layout;				//!< Module layout to differentiate between any future versions
		int32_t num_cards;			//!< Number of ADQ cards contained within the module. 	
		int32_t hw_version;			//!< hardware version in case we need to change interpretation of values.
		size_t nbytes_per_card;			//!< Number of bytes per card
		int num_t;					//!< Number of time points to use.
		struct
		{
			u_int32_t chan_sel[2], src_sel[2], alternate[2];	//!< Scope mode registers to be interpreted from ngpd.h
			size_t valid_offset;
		} card[8];
	} head;
	u_int16_t _data[1];			//!< Beginning of data, but use {@link ngpd_scope_mod_get_ptr()} for access
}	NGPDScopeModule;

typedef struct
{
	double col_time;
	enum {NGPDTrigSW, NGPDTrigExtTrigCycles} trig_mode;
	int cycles;
} NGPDITFGSetup;

// typedef struct 
// {
// 	int valid;
// 	enum {NGPDSingle, NGPDComposite} type;
// 	NGPDGeneration generation;
// 	void * adq_cu;
// 	void *zynqmp; // ZynqMPHandle *
// 	int starting_card;
// 	int sub_path[NGPD_MAX_NUM_CARDS+1];
// 	int adq_num;
// 	int debug;
// 	int num_cards;
// 	int num_chan;
// 	int max_num_chan;
// 	int chans_per_card;
// 	int chan_of_system;
// 	u_int32_t revision;
// 	u_int32_t playback_first_addr;
// 	u_int32_t playback_last_addr;
// 	u_int32_t scope_first_addr;
// 	u_int32_t scope_last_addr;
// 	u_int32_t scope_out_skip;
// 	u_int32_t playback_skip;
// 	int playback_num_streams;
// 	size_t playback_num_bytes;
// 	int playback_num_t;
// 	int run_flags;
// 	NGPDScopeModule *scope_mod;
// 	mh_com * scope_head;
// 	void histogram;
// 	void dummy_system;
// 	int max_scope_streams;
// 	size_t nbytes_scope_per_card;
// 	int mem_layout;
// 	void hist_config;
// 	int hist_config_read;
// 	void preamp_mirror_offset;
// 	void clock_setup;
// 	void filter[NGZMP_CHANS_PER_CARD];
// 	u_int32_t *dummy_chan_reg[NGZMP_CHANS_PER_CARD];		//!< Used to hold copy of channel registers for dummy systems, 1 array per channel.
// 	u_int32_t *dummy_glob_reg;	
// 	u_int16_t *dummy_chan_offset;
// 	u_int8_t  *dummy_chan_dga;
	
// 	//!< Used to hold copy of global registers for dummy systems, 1 array per dummy card.
// } NGPDPathType;

int ngpd_config_ngzmp(int ncards,  char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int debug, int card_index, int do_init,  int dummy_system);
int ngpd_dummy(int val);
int ngpd_config_adq14(unsigned int adq_num, int debug);
int ngpd_get_num_chan(int path);
int ngpd_write_diff_trigger(int path, int chan, NGPDDiffTrigger * trig);

int ngpd_filter_load_rect(int path, int chan, int num_ave);
int ngpd_filter_load_exp(int path, int chan, double Tsamples);
int ngpd_filter_load_gaus(int path, int chan, double sigma);
// int ngpd_filter_load_trapeziod(int path, int chan, int wid_top, int wid_bot);

int ngpd_write_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub);
int ngpd_write_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_read_measure(int path, int chan, NGPDMeasure * measure);

double ngpd_adc_set_range(int path, int chan, double vrange);
int ngpd_adc_set_offset(int path, int chan, int codes);

int ngpd_set_scope_options(int path, int card, NGPDScopeOptions *options);
int ngpd_scope_mod_get_nstreams(NGPDScopeModule *mod) ;
NGPDScopeModule *ngpd_scope_get_mod(int path);
int ngpd_scope_setup_stream(int path, int card, int stream, int chan_sel, int src_sel, int alt);

int ngpd_write_dae_pulse(int path, int chan, int stretch);
char *ngpd_get_error_message();

int ngpd_set_run_flags(int path, int run_flags);
int ngpd_get_run_flags(int path);

int ngpd_dma_system_start(int path, int card, u_int32_t pb_start, u_int32_t pb_num512, NGPDITFGSetup *itfg_setup);
int ngpd_dma_wait_scope(int path, int card, u_int32_t *statusP);
int ngpd_dma_read_scope(int path, int card, u_int32_t scope_start, u_int32_t scope_num512, int options);

u_int16_t * ngpd_scope_mod_get_ptr(NGPDScopeModule *mod, int card, int stream);
int ngpd_scope_mod_get_inc(NGPDScopeModule *mod, int stream) ;


// int ADQ_GetStreamOverflow(void* adq_cu_ptr, int adq_num);
// int py_GetStreamOverflow();
int* ADQ_GetRevision(void* adq_cu_ptr, int adq_num);
int ADQAPI_GetRevision();



// NGPDPathType NGPDPath[];
