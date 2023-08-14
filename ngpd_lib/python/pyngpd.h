#define NGPD_MEASURE_TAIL_THRES_SIZE 256 //!< Number of values to write to tail thres BRAM

#define DISP_2D		1
#define DISP_1D_XY	2
#define DISP_1D_Y	3

#define DATA_LONG	1
#define DATA_SHORT	2
#define DATA_FLOAT	3
#define DATA_DOUBLE	4

#define DATA_SHORT_DIG1 32
#define DATA_SHORT_DIG2 33

#define DATA_FIXED8_DIG1 40
#define DATA_FIXED8_DIG2 41
#define DATA_FIXED8_DIG3 42
#define DATA_FIXED8_DIG4 43
#define DATA_FIXED8_DIG5 44
#define DATA_FIXED8_DIG6 45
#define DATA_FIXED8_DIG7 46
#define DATA_FIXED8_DIG8 47


/* The structure contains x y and title strings */
#define MOD_LABLEN 80
#define MOD_TITLEN 80


typedef void mh_com;
typedef void mh_data;

#define IMG_MOD_2D				0
#define IMG_MOD_3D				1

typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

typedef int64_t off_t;

typedef void mh_com;

const uint32_t NGPD_RUN_FLAGS_SCOPEMODE         = (1<<4);
const uint32_t NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS = (1<<5);
const uint32_t NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD = (1<<6);
const uint32_t NGPD_MEASURE_TAIL_THRES_SIZE = 256;

typedef enum
{
	ImgModUnlinkNone=0,
	ImgModUnlinkCreated=1,
	ImgModUnlinkAll=2
} ImgModUnlink;

typedef enum 
{
	NGPDHistEnbHeight=1, 
	NGPDHistEnbTailSum=2,
	NGPDHistEnbFallTime=4,
	NGPDHistEnbTailRatio=8,
	NGPDHistEnbHgtTailSum=0x10,
	NGPDHistEnbHgtFallTime=0x20,
	NGPDHistEnbHgtTailRatio=0x40,

	NGPDHistEnbAll=0x7F,
	NGPDHistSeparateNeutrons=0x10000,
	NGPDHistDiscardPileup=0x20000
} NGPDHistEnables;

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
	struct mod_header
	{
		int32_t data_type; 		/* data type long short etc see above 			*/
		int32_t disp_type;		/* Prefered display technique see above			*/
		uint32_t num_x;/* Number of elements in x direction 			*/
		uint32_t num_y;/* Number of elements in y direction			*/
		int32_t version;		/* incrementing version for derived images? 	*/
							/* Get a block of data so that it can be read 	*/
		double aspect;		/* Aspect ratio of pixels X wid / Y wid (-ve = ignore)*/
		char x_label[MOD_LABLEN+2];
		char y_label[MOD_LABLEN+2];
		char z_label[MOD_LABLEN+2];
		char title[MOD_TITLEN+2];
	} head;
	uint32_t data[1];
}	MOD_IMAGE;

typedef struct
{
	struct mod_header3d
	{
		int32_t data_type; 		/* data type long short etc see above 			*/
		int32_t disp_type;		/* Prefered display technique see above			*/
		uint32_t num_x;			/* Number of elements in x direction 			*/
		uint32_t num_y;			/* Number of elements in y direction			*/
		uint32_t num_t;
		int32_t version;		/* incrementing version for derived images? 	*/
							/* Get a block of data so that it can be read 	*/
		double aspect;		/* Aspect ratio of pixels X wid / Y wid (-ve = ignore)*/
		char x_label[MOD_LABLEN+2];
		char y_label[MOD_LABLEN+2];
		char t_label[MOD_LABLEN+2];
		char z_label[MOD_LABLEN+2];
		char title[MOD_TITLEN+2];
		int labels_num, labels_size, labels_spare;		/* For Y labels, future expansion */
		int data_offset;
	} head;
	int labels_offset[1];		/* First label offset if labels_size > 0 */
}	MOD_IMAGE3D;

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

int ngpd_hist_setup(int path, int max_height, int max_tail_sum, int max_fall_time, double ratio_scale, int nbins_height, int nbins_tail_sum, int nbins_fall_time, int nbins_ratio, NGPDHistEnables enables);

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

//HISTOGRAMMING METHODS
int _os_datmod (const char *name, off_t s, uint16_t *attr_rev, uint16_t *type_lang, uint32_t perm, void **mod, void **mod_head);
int _os_link(const char **namep, void **mod_head, void **mod, uint16_t *type_lang, uint16_t *att_rev);
int munlink(void *mod_head);
int _os_unlink(void *mod_head);
int munlink_linux(void *mod_head, ImgModUnlink what);

MOD_IMAGE *id_mkmod (const char *name, int num_x, int num_y, const char *x_lab, const char *y_lab,  int data_type, void **mod_head);
MOD_IMAGE3D *id_mkmod3d (const char *name, int num_x, int num_y, int num_t, const char *x_lab, const char *y_lab, const char *t_lab, char ** labels, int data_type, mh_com **mod_head);
size_t datamod_size(void *mod);
MOD_IMAGE *id_mkmod_err_msg (const char *name, int num_x, int num_y, const char *x_lab, const char *y_lab, int data_type, mh_com **mod_head, char *err_msg, int max_err_msg);
MOD_IMAGE3D *id_mkmod3d_err_msg (const char *name, int num_x, int num_y, int num_t, const char *x_lab, const char *y_lab, const char *t_lab, char ** labels, int data_type, mh_com **mod_head, char *err_msg, int max_err_msg);

uint32_t *id_get_ptr(void *mod, int x, int y, int t);
int id_clear_mod(void *p);
int id_copy_mod(void *s, void *d);
char *id_get_label(void *p, int row);
int id_mod_get_shape(void *m, int *num_x, int *num_y, int *num_t);
int id_get_num_x(void *p);
int id_get_num_y(void *p);
int id_get_num_t(void *p);
int id_get_data_type(void *p);
// int _os_link2(const char *namep, void **mod_head, void **mod, uint16_t *type_lang, uint16_t *att_rev);

// NGPDPathType NGPDPath[];
