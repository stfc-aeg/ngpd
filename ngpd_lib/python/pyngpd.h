#define NGPD_MEASURE_TAIL_THRES_SIZE		256					//!< Number of values to write to tail thres BRAM

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

int ngpd_config_ngzmp(int ncards,  char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int debug, int card_index, int do_init,  int dummy_system);
int ngpd_dummy(int val);
int ngpd_config_adq14(unsigned int adq_num, int debug);
int ngpd_get_num_chan(int path);
int ngpd_write_diff_trigger(int path, int chan, NGPDDiffTrigger * trig);
int ngpd_filter_load_exp(int path, int chan, double Tsamples);
int ngpd_write_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub);
int ngpd_write_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_read_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_write_dae_pulse(int path, int chan, int stretch);
char *ngpd_get_error_message();