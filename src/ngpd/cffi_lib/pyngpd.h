#define NGPD_MEASURE_TAIL_THRES_SIZE        256                    //!< Number of values to write to tail thres BRAM
#define NGPD_MEASURE_TAIL_SUB_SIZE            1024                //!< Number of values to write to each half of tail subtract BRAMs


typedef enum {
    NGPD_RUN_FLAGS_PLAYBACK                = (1<<0),
    NGPD_RUN_FLAGS_PLAYBACK_ALT            = (2<<0),
    NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS    = (1<<2),
    NGPD_RUN_FLAGS_SCOPEMODE            = (1<<4),
    NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS    = (1<<5),
    NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD    = (1<<6),
    NGPD_RUN_FLAGS_SCOPE_OUT_TEST        = (1<<15),
    NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM    = (1<<16),

    NGPD_RUN_FLAGS_DEFAULT                = (NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM),
    NGZMP_RUN_FLAGS_DEFAULT                = (0)
} NGPDRunFlags;

typedef enum
{
        NGPDDataSrcADCFWD=0,
        NGPDDataSrcADCRev=1,
        NGPDDataSrcPlayback=4
} NGPDDataSrc;


typedef struct ngpd_chan_cont_t 
{
    int use_pb_start;
    int data_src;
    int inv_data;
} NGPDChanCont;

typedef enum 
{
    NGPDDummy_None=0,
    NGPDDummy_FPGA=1,
    NGPDDummy_ADC=2,
    NGPDDummy_Preamp=4,
    NGPDDummy_All=0xFF
} NGPDDummy;

typedef struct  ngpd_diff_trig_t
{
    int thres, sep, data_delay, trig_delay;
    int delay_a, width_a;        // Delay and stretch for the slow/wide trigger A
    int delay_b, width_b;        // Delay and stretch for the slow/wide trigger B, used to mask accumulation in the baseline subtractor.

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

typedef struct
{
    double col_time;
    enum {NGPDTrigSW, NGPDTrigExtTrigCycles} trig_mode;
    int cycles;
} NGPDITFGSetup;


typedef enum 
{
    NGPDFilter_Unknown=0,
    NGPDFilter_Rectangle,
    NGPDFilter_Gaussian,
    NGPDFilter_Exponential,
    NGPDFilter_Trapezoidal
} NGPDFilterType;

typedef struct 
{
    NGPDFilterType type;
    int iarg1, iarg2;
    double darg;
} NGPDFilter;

typedef struct ngpg_test_pattern_t
{
    enum {NGPDTP_Ramp, NGPDTP_Delta, NGPDTP_Square} type;
    int i_baseline, i_height, i_period;
    int num_t;
} NGPDTestPattern;

typedef enum
{    NGPDScopeOpt_TBD         = 1,        //!< To Be determined
} NGPDScopeOptionFlags;

typedef struct
{    NGPDScopeOptionFlags flags;                    //!< Scope options flags {@link NGPDScopeOptionFlags};
    int nstreams;                                //!< Number of scope streams to use where programmable (xspress3V7 onwards)
} NGPDScopeOptions;

typedef struct
{
    int separate_ngp;
    int enb_hgt_sum;
    int enb_hgt_fall;
    int discard_pu;
    int nbits_height;
    int shift_height;
    int nbits_fall_time;
    int shift_fall_time;
    int nbits_tail_sum;
    int shift_tail_sum;
} NGZMPHistConf;

typedef struct {
    enum {NGZMPClkSrcFPGA, NGZMPClkSrcLMK61E2, NGZMPClkSrcTestClk, NGZMPClkSrcBkPlane } src;
    enum {NGZMPSysRefContinuous, NGZMPSysRefBurstSPI}  sysref;
    enum {NGZMPClockMonDefault=0, NGZMPClockMonClk250=1 } monitor_op;
} NGPDClockSetup;

typedef enum {
    NGZMPHistEnbDisable        = 0,
    NGZMPHistEnbRun            = 1,
    NGZMPHistEnbTestFull    = 5,
    NGZMPHistEnbTestLower    = 7
} NGZMPHistEnable;

typedef enum {
NGPD_SCOPE_STATUS_RUNNING            = (1<<0),
NGPD_SCOPE_STATUS_SCOPEOUT_RUNNING    = (1<<1),
NGPD_SCOPE_STATUS_SCOPEOUT_PAUSED    = (1<<2),
NGPD_SCOPE_STATUS_ITFG_RUNNING        = (1<<3),
NGPD_SCOPE_STATUS_ITFG_WAITING        = (1<<4),
NGPD_SCOPE_STATUS_ITFG_COUNTING        = (1<<5)
} NGPDScopeStatus;

typedef enum {NGZMPADC_Default=0} NGZMPADCFlags;

typedef enum {
    NGPDScopeStream_Error=-1, //!< Error interpreting arguments
    NGPDScopeStream_NotPresent=0,  //!< The stream is not present in the current configuration of the data module.
    NGPDScopeStream_Unsigned=1,    //!< The data is to be interpreted as 16 bit unsigned "analogue" data.
    NGPDScopeStream_Signed=2,      //!< The data is to be interpreted as 16 bit signed "analogue" data.
    NGPDScopeStream_AllDigital=3,  //!< The data is digital data extracted from across all channels as should be displayed as up to 16 digital traces.
    NGPDScopeStream_AuxDigital=4   //!< The data is digital data which is associated with other streams and is often displayed with those streams rather than in its own right.
    } NGPDScopeStream;

#define NGZMP_I2C_NUM_ADT7410_ADC        4            //!< Number of ADT7410 temperature monitor chips on PCB.
#define NGZMP_I2C_NUM_ADT7410_PREAMP    2            //!< Number of ADT7410 temperature monitor chips on PCB.

int ngpd_config_ngzmp(int ncards,  char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int debug, int card_index, int do_init,  int dummy_system);
int ngpd_set_run_flags(int path, int run_flags);
int ngpd_get_run_flags(int path);
int ngpd_i2c_write_preamp_offset(int path, int chan, int num, uint16_t *data);
int ngpd_i2c_read_preamp_offset(int path, int chan, int num, uint16_t *data);
int ngzmp_spi_write_dga(int path, int chan, int num,  uint16_t* value);
int ngzmp_spi_read_dga(int path, int chan, int num,  uint16_t* value);
char *ngpd_get_error_message();

int ngpd_get_num_cards(int path);
int ngpd_get_num_chan(int path);
int ngpd_get_chans_per_card(int path);
int ngpd_dma_system_start(int path, int card, uint32_t pb_start, uint32_t pb_num512, NGPDITFGSetup *itfg_setup);
int ngpd_dma_wait_scope(int path, int card, uint32_t *statusP);

int ngpd_dma_read_playback(int path, int offset, int num_words, uint32_t *data, unsigned int *bytes_receivedP);
int ngpd_dma_read_scope(int path, int card, uint32_t scope_start, uint32_t scope_num512, int options);
int ngpd_scope_setup_stream(int path, int card, int stream, int chan_sel, int src_sel, int alt);
int ngpd_playback_generate(int path, int card, NGPDTestPattern *test_pat);
int ngpd_playback_load(int path, int card, char *filename, int max_num_t);

int ngpd_filter_load_integer(int path, int chan, int num_words, uint32_t *data, uint32_t * status);
int ngpd_filter_load_rect(int path, int chan, int num_ave);
int ngpd_filter_load_exp(int path, int chan, double Tsamples);
int ngpd_filter_load_gaus(int path, int chan, double sigma);
int ngpd_filter_load_trapezoid(int path, int chan, int wid_top, int wid_bot);
int ngpd_write_diff_trigger(int path, int chan, NGPDDiffTrigger * trig);
int ngpd_read_diff_trigger(int path, int chan, NGPDDiffTrigger * trig);
int ngpd_write_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub);
int ngpd_read_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub);
int ngpd_write_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_read_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_write_tail_subtract(int path, int chan, int32_t *data);
int ngpd_read_tail_subtract(int path, int chan, int32_t *data);
int ngpd_write_dae_pulse(int path, int chan, int stretch);
int ngpd_resolve_path_chan_card(int path, int chan, int *thisPath, int *chanIdx, int *cardP);
int ngpd_set_num_chan(int path, int num_chan);
int ngpd_has_bram(int path, int chan, int region_num);
int ngpd_bram_size(int path, int chan, int region_num);
int ngpd_get_playback_nstreams(int path, int card);
void ngpd_close(int path);
int ngpd_scope_mark_all(int path, int card, int value);
int ngpd_get_max_scope_streams(int path);
int ngpd_set_playback_streams(int path, int num_streams, int num_t);
int ngpd_get_playback_streams(int path);
int ngpd_write_chan_cont(int path, int chan, NGPDChanCont * chan_cont);
int ngpd_read_chan_cont(int path, int chan, NGPDChanCont * chan_cont);
int ngpd_read_scope_status(int path, int card, uint32_t *scope_status);
int ngpd_get_chans_on_card(int path, int card, int *first, int *num);
int ngpd_set_scope_options(int path, int card, NGPDScopeOptions *options);
int ngpd_scope_update_mod(int path, int card, int t, int dt);
int ngpd_get_filter_type(int path, int chan, NGPDFilter *filter);
int ngzmp_hist_write_chan_config(int path, int chan, NGZMPHistConf *hist_conf);
int ngzmp_hist_read_chan_config(int path, int chan, NGZMPHistConf *hist_conf);

int ngzmp_hist_clear_wait(int path, int card);
int ngzmp_hist_clear_all_start(int path, int card, int start_word, int num_words);

int ngpd_save_settings(int path, char *file_name);
int ngpd_restore_settings(int path, char *file_name);
int ngpd_clock_setup(int path, int card, NGPDClockSetup *cset);

int ngzmp_hist_enable(int path, int card, NGZMPHistEnable enable);
int ngzmp_hist_read_chan(int path, int chan, uint32_t offset, uint32_t size, uint32_t *data);
int ngzmp_adc_setup_adc(int path, int card, NGZMPADCFlags flags);
int ngzmp_adc_read_status(int path, int card);
int ngpd_scope_read_to_buff(int path, int card, int stream, int t, int dt, uint16_t *ptr);
NGPDScopeStream ngpd_scope_stream_details_path(int path, int card, int stream, int *num_digP, int *bit_posnP, int *dig_strP, int *num_extraP, int *extra_posnP, int *extra_strP, int *scope_incP, int *dig_incP);

int ngpd_clock_check_locked(int path, int card);
int ngpd_i2c_read_adc_temp(int path, int card, float *temp, int *status); 
int ngpd_i2c_read_preamp_temp(int path, int card, float *temp, int *status);
