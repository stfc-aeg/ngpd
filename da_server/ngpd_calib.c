#include "header.h"
#include "peak_fit.h"
#include "ngpd.h"
#include "ngpd_cal.h"

int ngpd_calib_offsets(int quals, int cn, int path, double target);

PARSE_TREE ngpd_calib_tree[] = {
	PT_COMMAND ("hist-scope", "%d num-pass %d chan %d noise = %d",
				"Histogram, RAW ADC values",
				"Path/Specify number of passes to accumulate/Limit to  specified channel/Measure Noise/",
				ngpd_scope_hist ),
	PT_COMMAND ("measure-noise", "%d num-pass %d chan %d = %d",
				"Measure noise from RAW ADC values",
				"Path/Specify number of passes to accumulate/Limit to  specified channel",
				ngpd_measure_noise ),
	PT_COMMAND ("hist-diffs", "%d length %d persist %s num-pass %d col-sat %d chan %d event-tol %g playback %s errant-diff %d diff-trig-sep %d vary-sep first %d last %d  = %d",
				"Histogram differentials from 'clean' background chunks",
				"Path/Chunk length (default 1024)/Make persistence mode display of good chunks/"
				"Number of passes/Colour Saturation for persistence display/Process single Channel/"
				"Event tolerance filter (unused)/Playback command/"
				"Persistence display diff-trig > +- threshold/Specify Diff Trigger separation, otherwise read current hardware setting/"
				"Test a range of filte separations (2..17)/First Channel (default 0)/Last Channel (num_chan-1).",
				ngpd_histogram_diffs),

	PT_COMMAND ("measure-diffs", "%d diff-trig vary-sep no-mask-0-diff = %d",
				"Measure Noise and percentiles from Histograms from hist-diffs",
				"Path/Measure Differential Trigger B Diff/Vary separation for diff trigger differential/"
				"Don't mask 0 difference",
				ngpd_measure_diff_hist),

	PT_COMMAND("adjust-trigger-thres", "%d chan %d first-chan %d last-chan %d dtrig-std-dev %g dtrig-fraction-b %g = %d",
			 	"Adjust Trigger Thresholds using outputs of hist-diffs",
			 	"Path/Do Single Channel/First Channel (default 0)/Last Channel (default n-1)/"
			 	"Set Diff-trig Threshold by scaling sigma/Set Diff-trig Threshold by fraction of cumulative frequency distribution/",
				ngpd_adjust_trigger_thresholds),

	PT_COMMAND ("measure-trigger", "%d pre %d post %d persist %s average %s num-pass %d num-y %d "
				"col-sat %d chan %d min-event %d max-event %d min-hgt-neutron %d max-hgt-neutron %d "
				"extra-pre %d extra-post %d playback %s two-phase three-phase auto-neutron-hgt with-servo no-threads first %d last %d by-width %d fine-time %d = %d",
				"Measure Events and event signals as seen by trigger-b",
				"Path/Number of pre rest points (default 40)/Number of post reset points (default 160)/"
				"Enable persistance display into module name/"
				"Enable averaging display into module name/"
				"Number of passes (default 1)/Module height for persitance ( > than largest event) /"
				"Colour Saturation control 0 = off (default) 1..20 on/Channel rather than all/"
				"Use maximum value to see if an event > this has occured/Use maximum value to see if an event < this has occured/"
				"Use maximum value to see if an event > this is a neutron/Use maximum value to see if an event < this is a neutron/"
				"Check for triggers in this time before start of window (default=post)/Check for triggers in this time after end of window (default=pre)/Run the supplied playback command (with embedded %d for num_passs)/"
				"Run second phase which checks that event follow average within (defualt 5) sigma/Add a Third phase filter with now better defined sigma/"
				"Determine K-alpha peak position automatically from MCA/Take data from servo out for event shape, OTD timing not available/"
				"Disable multi-threaded code/First Channel (default 0)/Last Channel (default num-1)/Form separate average for each OTD width up to width specified/Form separate averages with fine time current 4 or 8 or 16/",
				ngpd_measure_trigger),
	PT_COMMAND ("adjust-trigger-timing", "%d %s %s chan %d first %d last %d "
				"sig-a-first-thres %g sig-a-last-thres %g wide-a-first-thres %g wide-a-last-thres %g wide-a-first-offset %d wide-a-last-offset %d "
				"sig-b-first-thres %g sig-b-last-thres %g wide-b-first-thres %g wide-b-last-thres %g wide-b-first-offset %d wide-b-last-offset %d "
				"wide-a-pulse fit-ramp %d check %d = %d",
		 		"Adjust timing of the single cycle and stretched triggers from the differential trigger using results from measure-trigger",
		 		"Path/Averaged Event signal module root name ..._phase3/"
		 		"Module name for per channel results/"
		 		"Single Channel/First Channel/Last Channel/"
		 		"Threshold (default 0.005) for start of 'analogue' pulse for adjusting wide A/Threshold (default 0.015) for end of 'analogue' pulse when adjusting wide A/"
		 		"Threshold (default 0.5) for start of wide-A event signal/Threshold (default 0.5) for end of wide-A event signal/"
		 		"Offset (default -2) from signal to wide-A Start/Offset (default 10) from signal to wide-A end/"
		 		"Threshold (default 0.005) for start of 'analogue' pulse for adjusting wide B/Threshold (default 0.015) for end of 'analogue' pulse when adjusting wide B/"
		 		"Threshold (default 0.5) for start of wide-B event signal/Threshold (default 0.5) for end of wide-B event signal/"
		 		"Offset (default -2) from signal to wide-B Start/Offset (default 10) from signal to wide-B end/"
				"Make wide-A signal a single cycle pulse to mark start of next event/Fit ramp from LHS to this point (before edge)/If value != 0, just print what changes would be, but do not change/",
		 		ngpd_adjust_trigger_timing),
	PT_COMMAND ("calc-tail-subtract", "%d %s %s chan %d fit-ramp %d scale-tail %g end-offset %d first %d last %d = %d",
				"Calculate Pulse tail correction tables from output of measure-trigger",
				"Path/Root average module name/root output file name for tail subtraction (_nn.dat appended)"
				"Do just one channel/Lin least squares fit servo ramp from this time onwards (not yet implememnted)/Scale tail by this amount/"
				"Assume end of event has been offset by this amount, adjust trigger to match/First Channel/Last Channel",
				ngpd_calc_tail_subtract),
	PT_COMMAND ("adjust-offsets", "%d %g first %d last %d num-pass %d save %s = %d",
				"Adjust the pre-amp offsets to move teh ADC dat to the desired value",
				"Path/Target value 0 to 65535/First channel (default 0)/Last Channel (default num_chan-1)/Number of passes (default 10)/"
				"Filename to save resulting offsets",
				ngpd_calib_offsets ),
				PT_END
};

int ngpd_calib_offsets(int quals, int cn, int path, double target)
{
	int first_chan=0;
	int last_chan = -1;
	int num_pass = 10;
	int adjust_only;
	int rc;
	char *save_fname = NULL;
	
	if (quals & 1)
		first_chan = Qualifier[0].IntArg;
	
	if (quals & 2)
		last_chan = Qualifier[1].IntArg;
	if (quals & 4)
		num_pass = Qualifier[2].IntArg;
	if (quals & 0x8)
		save_fname = Qualifier[3].StringArg;
	
	adjust_only = (quals & 8);
	rc = ngpd_cal_offsets(path,  NULL, NULL, first_chan, last_chan, target, num_pass, adjust_only, save_fname);
	if (rc < 0)
		sv_printf(cn, "! ERROR: %s\n", ngpd_cal_get_error_message());
	
	return rc;
}
