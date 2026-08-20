#define NGPD_MEASURE_TAIL_THRES_SIZE 256  //!< Number of values to write to tail thres BRAM
#define NGPD_MEASURE_TAIL_SUB_SIZE   1024 //!< Number of values to write to each half of tail subtract BRAMs

#define NGPD_DIFF_TRIG_MIN_SEP          2
#define NGPD_DIFF_TRIG_MAX_SEP          65
#define NGPD_DIFF_TRIG_MIN_DATA_DELAY   0
#define NGPD_DIFF_TRIG_MAX_DATA_DELAY   63
#define NGPD_DIFF_TRIG_MIN_TRIG_DELAY   0
#define NGPD_DIFF_TRIG_MAX_TRIG_DELAY   63
#define NGPD_DIFF_TRIG_MIN_DELAY_AB     0
#define NGPD_DIFF_TRIG_MAX_DELAY_AB     31
#define NGPD_DIFF_TRIG_MIN_TRIG_STRETCH 1
#define NGPD_DIFF_TRIG_MAX_TRIG_STRETCH 0xFF
#define NGPD_DIFF_TRIG_MIN_THRES        0
#define NGPD_DIFF_TRIG_MAX_THRES        0xFFFF

#define NGPD_MEASURE_SUM_DELAY_MAX  0xFF
#define NGPD_MEASURE_SUM_NUM_MAX    0xFF
#define NGPD_MEASURE_HEIGHT_MAX     0xFFFF
#define NGPD_MEASURE_FALL_TIME_MAX  0xFF
#define NGPD_MEASURE_TAIL_COUNT_MAX 0xFF

#define NGZMP_UCD90160_NUM_RAILS 16 //!< maximum number of rails supported by UCD90160
#define NGZMP_UCD90160_NUM_CHIPS 2  //!< maximum number of UCD90160 chips on board

#define NGZMP_ParamMax 27 //!< Number of parameters when reading FPGA params

typedef enum {
    NGPD_RUN_FLAGS_PLAYBACK            = (1<<0),
    NGPD_RUN_FLAGS_PLAYBACK_ALT        = (2<<0),
    NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS = (1<<2),
    NGPD_RUN_FLAGS_SCOPEMODE           = (1<<4),
    NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS   = (1<<5),
    NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD   = (1<<6),
    NGPD_RUN_FLAGS_SCOPE_OUT_TEST      = (1<<15),
    NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM  = (1<<16),

    NGPD_RUN_FLAGS_DEFAULT  = (NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM),
    NGZMP_RUN_FLAGS_DEFAULT = (0)
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

typedef struct
{
    uint32_t flags;
    uint32_t cycles;
    uint32_t timer_us;
} NGPDITFGStatus;
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
int ngzmp_system_stop(int path, int card);
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
int ngpd_read_itfg_status(int path, int card, NGPDITFGStatus *itfg_status);
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
int ngzmp_i2c_read_ucd90160_vout(int path, int card, int chip, int page, int num,  int32_t* vout);
int ngzmp_read_xadc(int path, int card, int first, int num, int32_t *data);


/* For calibration library */
/* COMMENTED OUT FOR NOW FOR CFFI BUILD DEBUGGING*/

// #define NGPD_CAL_MAX_PHASES 3
// #define NGPD_CAL_MAX_DIFFS 16

// #define NGPD_CAL_DIFF_TRIG     0
// #define NGPD_CAL_DIFF_VARY_SEP 100

// #define NGPD_CAL_NUM_THRES     1
// #define NGPD_CAL_NUM_SIGMA    1

// typedef void MOD_IMAGE3D;
// typedef void MOD_IMAGE;
// typedef void mh_com;
// typedef void NGPDScopeModule;
// #define NGPD_MODNAME_SPACE 82
// #define NGPD_MAX_CHANS        64
// #define NGPD_CAL_MAX_PHASES 3

// /** Data structure of settings to control ngpd_cal_measure_trigger and internal data */
// // typedef struct ngpd_measure_trigger_t
// // {
// //     int path;                //!< Path number of the system, set by ngpd_cal_measure_trigger_default. Should not be changed.
// //     int cn;                    //!< For use with playback testing with da.server.
// //     int num_chan;            //!< Number of channels, set by ngpd_cal_measure_trigger_default. Available for information but should not be changed.
// //     int first_chan;            //!< First channel to process set to 0 by default, but can be changed.
// //     int last_chan;            //!< Last channel to process set to num_chan-1 by default, but can be changed.
// //     char persist_name[NGPD_MODNAME_SPACE];    //!< Root Module name to store persistence views of data. Must be set if persistence mode is required. Also set do_persist.
// //                                             //!< The suffix _phase<phase>_<chan_num> will be appended to the root name to name the module for each phase and channel.
// //     char ave_name[NGPD_MODNAME_SPACE];        //!< Root module name to store averaged data. This is required for the post processing options.
// //                                             //!< The suffix _phase<phase>_<chan_num> will be appended to the root name to name the module for each phase and channel.
// //     char xtk_name[NGPD_MODNAME_SPACE];        //!< Root module name to store averaged data. This is required for the post processing options.
// //                                             //!< The suffix _phase<phase>_<chan_num> will be appended to the root name to name the module for each phase and channel.
// //     int do_persist;            //!< Set to enable production of persistence mode data.
// //     int do_ave;                //!< Set to enable production of averaged data
// //     int do_xtk;                //!< Set to enable production of averaged Crosstalk data
// //     int use_measure;        //!< Set collect data from the data supplied to the measure block of the VHDL after the baseline and optionally tail subtraction. {@link NGPD_CAL_USE_MEASURE_MACROS}
// //     int pre;                //!< Number if samples to store before the event trigger. Default 25 but can be changed.
// //     int post;                //!< Numbre of samples to stire after the event trigger. Default 170, but can be changed.
// //     int no_clear;            //!< Set to disable clear of the data modules, so new data is accumulated on top of the existing data.
// //     int num_pass;            //!< Number of passes running the system and accumulating data. Default 1, but can be increased to improve statistics, particularly if working at low count rates.
// //     int col_sat;            //!< Adjust the appearance of the histogram displays.
// //     int persist_num_y;        //!< Height in bins of the 2-d data modules used for persistence display, default 1200
// //     int auto_neutron_height;    //!< Process data in 2 phases. For the first phase locate the neutron peak height from the MCA ans use this to set the min_hgt_neutron and max_hgt_neutron automatically
// //                                 //!< for a second phase which separates neutrons and gammas.
// //     double *min_event;        //!< Pointer to array of minimum event height to be included in average, set on a per channel basis. 
// //                             //!< The array a malloced by ngpd_cal_measure_trigger_default and initialised to -1 to disable check. The contents of the array can be set by user code.
// //     double *max_event;        //!< Pointer to array of maximum event height to include in average, set on a per channel basis.
// //     double *min_hgt_neutron;//!< Pointer  Pointer to array of minimum event height to be classified as a neutron, set on a per channel basis.
// //                             //!< The array a malloced by ngpd_cal_measure_trigger_default and initialised to -1 to disable separating neutrons and gammas. 
// //                             //!< The contents of the array can be set by user code.
// //                             //!< Alternately the auto_neutron_height feature can be used to set this automatically.
// //     double *max_hgt_neutron;//!< Pointer  Pointer to array of minimum event height to be classified as a neutron, set on a per channel basis.
// //     int num_phase;            //!< Number of phaes of processing, Default 1, but increased to 2 if auto_neutron_height is used.
// //     int disable_multi_thread;    //!< For multi-card systems the library will launch a thread per card. Set this to disable multi-threading.
// //     char *playback;            //!< Pointer to a string storing a da.server ngpd playback load command to allow automatic testing with multiple passes of playback data.
// //     int    (*user_service)(int, char*, int, FILE *, FILE *); //!< Pointer to da.server user_service function (or equivalent) to allow plackback option to be used.    
// //     int num_cards;
// //     double mca_scale;
// //     int extra_pre, extra_post;
// //     struct ngpd_measure_trigger_private_t *private;
// // } NGPDMeasureTrigger;
// /** Data structure of settings to control ngpd_cal_tail_subtract */
// typedef struct {
//     int path;                //!< Path number of the system, set by ngpd_cal_measure_trigger_default. Should not be changed.
//     int num_chan;            //!< Number of channels, set by ngpd_cal_measure_trigger_default. Available for information but should not be changed.
//     int first_chan;            //!< First channel to process set to 0 by default, but can be changed.
//     int last_chan;            //!< Last channel to process set to num_chan-1 by default, but can be changed.
//     char *root_ave_name;    //!< root module name for the averaged data from ngpd_cal_measure_trigger with _phase[1|2] appened as appropriate.
//     char *root_fname;        //!< Root file name to save tail shapes to file if required.
//     int fit_ramp;            //!< For future expansion
//     double scale_tail;        //!< Default 1, for firmware debugging only
//     int end_offset;            //! Default 0, for debugging only
// } NgpdCalTailSubtract;

// /** Data structure of settings to control ngpd_cal_adjust_trigger_timing */
// typedef struct ngpd_cal_adjust_trigger_timing_t
// {
//     int path;                //!< Path number of the system, set by ngpd_cal_measure_trigger_default. Should not be changed.
//     int num_chan;            //!< Number of channels, set by ngpd_cal_measure_trigger_default. Available for information but should not be changed.
//     int first_chan;            //!< First channel to process set to 0 by default, but can be changed.
//     int last_chan;            //!< Last channel to process set to num_chan-1 by default, but can be changed.
//     char * ave_name;        //!< The root name of teh averages data module from ngpd_cal_measure_trigger() e.g. "ave_phase1". "_<chan_num>" will be appended
//     char *res_name;            //!< Module name to store the results in.
//     double sig_first_thres[2];    //!< Threshold as fraction 0 ... 1.0 of the maximum of the analogue pulse shape to define the start of the pulse.
//                                                         //!< Note that the thres for the two wide signal a nd b can be different if required. Default 0.005
//     double sig_last_thres[2];        //!< Threshold as fraction 0 ... 1.0 of the maximum of the analogue pulse shape to define the end of the pulse.
//     double wide_first_thres[2];    //!< Threshold 0 ... 1.0 the determine the start of the 2 wide trigger signals
//     double wide_last_thres[2];    //!< Threshold 0 ... 1.0 the determine the end of the 2 wide trigger signals
//     int wide_first_offset[2];        //!< Required offset in samples from the start of the analogue signal to the digital signal
//     int wide_last_offset[2];        //!< Required offset in samples from the end of the analogue signal to the digital signal
//     int no_clear;                                        //!< Do not clear results data module to allow results from multiple calls on different channel range to splice together.
//     int wide_a_pulse;                                    //!< Force wide_a signal to be a single clock cycle wide marker pulse
//     int fit_ramp;                                        //!< For future expansion
//     int just_check;                                        //!< Process data and print changes, but do to change trigger settings.
// } NgpdCalAdjustTiming;

// /** Data structure of settings to control ngpd_cal_hist_diffs */
// typedef struct ngpd_cal_hist_diff_t
// {
//     int path;                //!< Path number of the system, set by ngpd_cal_measure_trigger_default. Should not be changed.
//     int cn;                    //!< For use with playback testing with da.server.
//     int num_chan;            //!< Number of channels, set by ngpd_cal_measure_trigger_default. Available for information but should not be changed.
//     int first_chan;            //!< First channel to process set to 0 by default, but can be changed.
//     int last_chan;            //!< Last channel to process set to num_chan-1 by default, but can be changed.
//     enum {Normal, VarySep } mode;    //!< Normal mode tests the setting for the trigger, VarySep tests 16 different separations in one run.
//     int chunk_size;            //!< length of chunk of data to process in one go, default 1024 samples.
//     char persist_name[NGPD_MODNAME_SPACE];    //!< Module name to enable persistence mode.
//     int diff_sep[16];        //!< Separation in samples between the values differenced usually read from firmware.
//     int persist_errant;        //!< Use with persistence mode to record events with larger differences
//     int errant_thres[16];    //<! Threshold for an difference to be considered errant and record in errant persistence mode
//     int no_clear;            //!< Usually the data modules are cleared before starting, but setting no_clear prevent this, allowing data from multiple runs can be accumulated, 
//                             //!< or runs over different channels can be spliced together.
//     int num_pass;            //!< Number of passes, default 1
//     int persist_num_y;        //!< Height of persistence data modules.
//     int col_sat;            //!< Adjust the colouring on persistence mode
//     char *playback;            //!< Point to string of a playback command to pass to (user_service) to load different playback files for each pass. Used within da.server
//     int hist_num_x;            //!< Width of the differences histogram, default 1024.
//     int diff_trig_from_hw;    //!< Default 1 takes difference separation from the current firmware settings, 0 => caller will supply diff_sep.
//     int    (*user_service)(int, char*, int, FILE *, FILE *);    //!< Function to process the playback command, particularly with da.server
//     void *private;
// } NgpdCalHistDiffs;

// /** Data structure of settings to control ngpd_cal_adjust_trigger_thresholds */
// typedef struct ngpd_cal_adjust_trigger_thresholds_t 
// {
//     int path;                //!< Path number of the system, set by ngpd_cal_measure_trigger_default. Should not be changed.
//     int num_chan;            //!< Number of channels, set by ngpd_cal_measure_trigger_default. Available for information but should not be changed.
//     int first_chan;            //!< First channel to process set to 0 by default, but can be changed.
//     int last_chan;            //!< Last channel to process set to num_chan-1 by default, but can be changed.
//     enum {Unchanged, ScaleSigma, Fraction } 
//         thres_type[1]; //!< How to set threshold. Note this is an array to allow for future expansion if the trigger engine 
//                             //!< needs more than one threshold. Currently only element thres_type[NGPD_CAL_DIFF_TRIG] is used.
//                             //!< Threshold can Unchanged by this call (for future expansion), set by Scaling sigma or to trigger on a specified (small) fraction of samples.
//     double thres_val[1];     //!< Value to use when calculating the threshold. For ScaleSigma it is a scaling faction, typically 5
//                                             //!< For Faction mode it would be a small number e.g. 0.0001
//     int first_thres;        //!< For future expansion, would allow only specified threshold to be changed if there were more than one. Currently must be NGPD_CAL_DIFF_TRIG
//     int last_thres;            //!< For future expansion, would allow only specified threshold to be changed if there were more than one. Currently must be NGPD_CAL_DIFF_TRIG
//     int no_clear;            //!< Do not clear output data module. Allows results from several calls to this function to be spliced together.
// } NgpdCalAdjustTrigThres;

// /** Data structure of settings to control ngpd_cal_measure_trigger and internal data */
// typedef struct 
// {
//     int path;                //!< Path number of the system, set by ngpd_cal_measure_trigger_default. Should not be changed.
//     int cn;                    //!< For use with playback testing with da.server.
//     int num_chan;            //!< Number of channels, set by ngpd_cal_measure_trigger_default. Available for information but should not be changed.
//     int first_chan;            //!< First channel to process set to 0 by default, but can be changed.
//     int last_chan;            //!< Last channel to process set to num_chan-1 by default, but can be changed.
//     char persist_name[NGPD_MODNAME_SPACE];    //!< Root Module name to store persistence views of data. Must be set if persistence mode is required. Also set do_persist.
//                                             //!< The suffix _phase<phase>_<chan_num> will be appended to the root name to name the module for each phase and channel.
//     char ave_name[NGPD_MODNAME_SPACE];        //!< Root module name to store averaged data. This is required for the post processing options.
//                                             //!< The suffix _phase<phase>_<chan_num> will be appended to the root name to name the module for each phase and channel.
//     char xtk_name[NGPD_MODNAME_SPACE];        //!< Root module name to store averaged data. This is required for the post processing options.
//                                             //!< The suffix _phase<phase>_<chan_num> will be appended to the root name to name the module for each phase and channel.
//     int do_persist;            //!< Set to enable production of persistence mode data.
//     int do_ave;                //!< Set to enable production of averaged data
//     int do_xtk;                //!< Set to enable production of averaged Crosstalk data
//     int use_measure;        //!< Set collect data from the data supplied to the measure block of the VHDL after the baseline and optionally tail subtraction. {@link NGPD_CAL_USE_MEASURE_MACROS}
//     int pre;                //!< Number if samples to store before the event trigger. Default 25 but can be changed.
//     int post;                //!< Numbre of samples to stire after the event trigger. Default 170, but can be changed.
//     int no_clear;            //!< Set to disable clear of the data modules, so new data is accumulated on top of the existing data.
//     int num_pass;            //!< Number of passes running the system and accumulating data. Default 1, but can be increased to improve statistics, particularly if working at low count rates.
//     int col_sat;            //!< Adjust the appearance of the histogram displays.
//     int persist_num_y;        //!< Height in bins of the 2-d data modules used for persistence display, default 1200
//     int auto_neutron_height;    //!< Process data in 2 phases. For the first phase locate the neutron peak height from the MCA ans use this to set the min_hgt_neutron and max_hgt_neutron automatically
//                                 //!< for a second phase which separates neutrons and gammas.
//     double *min_event;        //!< Pointer to array of minimum event height to be included in average, set on a per channel basis. 
//                             //!< The array a malloced by ngpd_cal_measure_trigger_default and initialised to -1 to disable check. The contents of the array can be set by user code.
//     double *max_event;        //!< Pointer to array of maximum event height to include in average, set on a per channel basis.
//     double *min_hgt_neutron;//!< Pointer  Pointer to array of minimum event height to be classified as a neutron, set on a per channel basis.
//                             //!< The array a malloced by ngpd_cal_measure_trigger_default and initialised to -1 to disable separating neutrons and gammas. 
//                             //!< The contents of the array can be set by user code.
//                             //!< Alternately the auto_neutron_height feature can be used to set this automatically.
//     double *max_hgt_neutron;//!< Pointer  Pointer to array of minimum event height to be classified as a neutron, set on a per channel basis.
//     int num_phase;            //!< Number of phaes of processing, Default 1, but increased to 2 if auto_neutron_height is used.
//     int disable_multi_thread;    //!< For multi-card systems the library will launch a thread per card. Set this to disable multi-threading.
//     char *playback;            //!< Pointer to a string storing a da.server ngpd playback load command to allow automatic testing with multiple passes of playback data.
//     int    (*user_service)(int, char*, int, FILE *, FILE *); //!< Pointer to da.server user_service function (or equivalent) to allow plackback option to be used.    
    
//     int thread_index;        //!< Internal use.
//     int num_cards;
//     MOD_IMAGE3D *ave_mod[3][NGPD_MAX_CHANS];
//     mh_com *ave_head[3][NGPD_MAX_CHANS];
//     MOD_IMAGE3D *persist_mod[3][NGPD_MAX_CHANS];
//     mh_com *persist_head[3][NGPD_MAX_CHANS];
//     MOD_IMAGE *num_ave_mod[3];
//     mh_com *num_ave_head[3];
//     MOD_IMAGE3D *mca_mod[3];
//     mh_com *mca_head[3];
//     MOD_IMAGE *ave_by_width_mod[3][NGPD_MAX_CHANS];
//     mh_com *ave_by_width_head[3][NGPD_MAX_CHANS];
//     MOD_IMAGE *num_ave_by_width_mod[3];
//     mh_com *num_ave_by_width_head[3];
//     NGPDScopeModule * scope_mod;
//     MOD_IMAGE3D *xtk_all_mod[3];
//     mh_com *xtk_all_head[3];
//     MOD_IMAGE3D *xtk_neutron_mod[3];
//     mh_com *xtk_neutron_head[3];
//     MOD_IMAGE3D *xtk_gamma_mod[3];
//     mh_com *xtk_gamma_head[3];
//     MOD_IMAGE3D *num_ave_xtk_mod[3];
//     mh_com *num_ave_xtk_head[3];
//     int extra_pre, extra_post;
//     int trig_min_otd, trig_max_otd;
//     int phase; 
//     double scale_sigma;
//     int first_card, last_card;
//     int chans_per_card;
//     double mca_scale;
//     int max_width, max_fine_time;
//     int first_stream, last_stream,  trig_offset, wide_offset_a, wide_offset_b, neutron_offset;
//     char variable_width_mode[NGPD_MAX_CHANS];
//     int need_upper;
// } NGPDMeasureCrosstalk;


// char * ngpd_cal_get_error_message();
// int ngpd_cal_setup_all_ana(int path,  int card, int sync_boxes, int src);
// int ngpd_cal_setup_trig_or_measure(int path,  int card, int use_measure, int for_xtk, int do_upper, int *first_stream, int *last_stream,  int *trig_offset, int *wide_a_offset, int *wide_b_offset, int *neutron_offset, int sync_boxes, int *need_upper);
// int ngpd_cal_setup_trig_or_measure_get_chan(int path, int use_measure, int do_upper, int stream);
// int ngpd_cal_setup_all_chan(int path,  int card, int scope_src, int *first_stream, int *last_stream,  int *trig_offset, int *wide_a_offset, int *wide_b_offset, int *neutron_offset, int sync_boxes);
// int ngpd_cal_setup_all_chan_get_chan(int path, int scope_src, int stream);

// int ngpd_cal_offsets(int path,  void (*nprintf)(void *msg_data, char *, ...), void *msg_data, int first_chan, int last_chan, double target, int num_pass, int adjust_only, char *save_fname);
// int ngpd_cal_measure_trigger_validate(NGPDMeasureTrigger *set);

// void ngpd_cal_measure_trigger_delete(NGPDMeasureTrigger *set);
// NGPDMeasureTrigger *ngpd_cal_measure_trigger_default(int path);
// int ngpd_cal_measure_trigger(NGPDMeasureTrigger *set);
// int ngpd_cal_measure_trigger_start(NGPDMeasureTrigger *set);
// int ngpd_cal_measure_trigger_join(NGPDMeasureTrigger *set);
// double ngpd_cal_measure_trigger_progress(NGPDMeasureTrigger *set);

// NgpdCalTailSubtract *ngpd_cal_tail_subtract_default(int path, char *root_ave_name, char *root_fname);
// void ngpd_cal_tail_subtract_delete(NgpdCalTailSubtract *set);
// int ngpd_cal_tail_subtract (NgpdCalTailSubtract *set);
// NgpdCalAdjustTiming * ngpd_cal_adjust_trigger_timing_default (int path, char *ave_name, char *res_name);
// void ngpd_cal_adjust_trigger_timing_delete(NgpdCalAdjustTiming *set);
// int ngpd_cal_adjust_trigger_timing(NgpdCalAdjustTiming *set);
// void ngpd_cal_accumulate_ave(int16_t * ana_ptr, uint16_t * dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, double baseline, int ana_inc, int dig_inc, uint16_t trig_mask, uint16_t wide_mask_a, uint16_t wide_mask_b);
// void ngpd_cal_accumulate_persist(int16_t * ana_ptr, uint16_t *dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, int base_line, int divide, int cap_len, uint16_t bmask0, uint16_t bmask1, uint16_t bmask2, int ana_inc, int dig_inc);

// NgpdCalHistDiffs * ngpd_cal_hist_diffs_default(int path);
// void ngpd_cal_hist_diffs_delete(NgpdCalHistDiffs *set);
// int ngpd_cal_hist_diffs (NgpdCalHistDiffs *set);
// int ngpd_cal_measure_diff_hist (int path, int diff_num, double fraction, int decades, int num_settings, int mask_0_diff);
// double ngpd_cal_hist_diffs_progress(NgpdCalHistDiffs *set);
// int ngpd_cal_hist_diffs_start(NgpdCalHistDiffs *set);
// int ngpd_cal_hist_diffs_join(NgpdCalHistDiffs *set);


// NgpdCalAdjustTrigThres * ngpd_cal_adjust_trigger_thresholds_default(int path);
// void ngpd_cal_adjust_trigger_thresholds_delete(NgpdCalAdjustTrigThres *set);
// int ngpd_cal_adjust_trigger_thresholds_validate(NgpdCalAdjustTrigThres *set);
// int ngpd_cal_adjust_trigger_thresholds(NgpdCalAdjustTrigThres *set);

// extern int (*ngpd_cal_test_quit)(void);

// #define NGPD_CAL_MOD_ROW_SIG         0        //!< Module row=y=0 : Averaged signal shape
// #define NGPD_CAL_MOD_ROW_TRIG         1        //!< Module row=y=1 : Averaged position of trigger signal
// #define NGPD_CAL_MOD_ROW_WIDE_A     2        //!< Module row=y=2 : Averaged position and width of wide_a signal
// #define NGPD_CAL_MOD_ROW_WIDE_B     3        //!< Module row=y=3 : Averaged position and width of wide_a signal
// #define NGPD_CAL_MOD_ROW_FIT        4        //!< Module row=y=3 : For future expansion
// #define NGPD_CAL_MOD_ROW_CORRECTED    5        //!< Module row=y=3 : For future expansion
// #define NGPD_CAL_MOD_ROW_DIFF1        6        //!< Module row=y=3 : For future expansion. Proposed to calculate first differential to assess under/overshoot.
     
// #define NGPD_CAL_MOD_NUM_ROW         7        //!< Total number of rows in data module

// #define NGPD_CAL_MOD_T_ALL             0        //!< Module t=0 stores all events (or sum or neytron + gamma)
// #define NGPD_CAL_MOD_T_NEUTRON         1        //!< Module t=1 stores    neutrons, when using min_hgt_neutron/max_hgt_neutron to separate neutrons and gammas 
// #define NGPD_CAL_MOD_T_GAMMA         2        //!< Module t=2 stores  gammas (strictly others) when using min_hgt_neutron/max_hgt_neutron to separate neutrons and gammas 

// #define NGPD_CAL_MOD_NUM_T             3        //!< Total number of planes in data module

// #define NGPD_CAL_MAX_DECADES 3            //!< ngpd_cal_measure_diff_hist forms the cumulative sum and finds the threshold to from the fraction, fraction/10, fraction/100 of samples.
// #define NGPD_CAL_HIST_RES_MEAN    0     //!< Module row=y=0: Stores the mean of the histogram. Usually near 0 is the noise is symmetric.
// #define NGPD_CAL_HIST_RES_STDEV   1        //!< Module row=y=1: Stores the standard deviation of the noise.
// #define NGPD_CAL_HIST_RES_FRAC1   2        //!< Module row=y=2: Stores the threshold to trigger on the specified fraction of samples
// #define NGPD_CAL_HIST_RES_FRAC10  3        //!< Module row=y=3: Stores the threshold to trigger on the specified fraction/10 of samples
// #define NGPD_CAL_HIST_RES_FRAC100 4        //!< Module row=y=4: Stores the threshold to trigger on the specified fraction/100 of samples

// #define NGPD_CAL_THRES_DIFF_TRIG           0
// #define NGPD_CAL_THRES_DIFF_TRIG_SIGMA    1
// #define NGPD_CAL_THRES_NUM_ROWS            2


// #define NGPD_CAL_USE_MEASURE_NONE         0    //!< {@link ngpd_cal_setup_trig_or_measure} takes data from Diff Trigger output
// #define NGPD_CAL_USE_MEASURE_RAW         1    //!< {@link ngpd_cal_setup_trig_or_measure} takes raw data from pulse measure block

// #define NGPD_CAL_USE_MEASURE_CORRECTED     2    //!< {@link ngpd_cal_setup_trig_or_measure} takes tail subtracted data from pulse measure block
