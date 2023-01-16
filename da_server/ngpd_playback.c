/*----------------------------------------------------------------------
 *	Neutron Gamma Pulse Discriminator Device for DA with multi-client server.
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include <ngpd.h>



/*----- Private global variables -----*/
/*----- global variables -----*/
/*----- Private function prototypes -----*/

int ngpd_playback_generate_cmd(int quals, int cn, int path, int card);
int ngpd_playback_generate_zynq(int quals, int cn, int path, int card, int num_pb_streams);
int ngpd_playback_load_cmd(int quals, int cn, int path, int card, char *filename);


PARSE_TREE ngpd_playback_tree[] = {
	PT_COMMAND ("generate-test-pat", "%d %d ramp delta square baseline %d height %d period %d num-t %d = %d",
			    "Generate simple test pattern into playback memory",
			    "Path/Card (-1 to copy to all cards)/Incrementing count/Unit impulse/Square wave/baseline (default 0)/height (default 32768)/Period default 2049/Limit amount of test pattern memory used",
			    ngpd_playback_generate_cmd),
	PT_COMMAND ("generate-tp-zynq", "%d %d %d = %d",
			    "Request the Zynq to generate simple test pattern into playback memory",
			    "Path/Card (-1 to copy to all cards)/Number of streams (1, 2, 4 or 8)",
			    ngpd_playback_generate_zynq),
	PT_COMMAND ("load", "%d %d %s num-t %d = %d",
			    "Load Playback data from 16 bit scope mode file",
			    "Path/Card (future expansion)/File name/Limit amount of file used",
			    ngpd_playback_load_cmd),
	PT_END
};


int ngpd_playback_generate_zynq(int quals, int cn, int path, int card, int num_pb_streams)
{
	int rc;
	ZYNQMP_DMA_MsgTestPat msg;
	int run_flags;

	memset(&msg, 0, sizeof(ZYNQMP_DMA_MsgTestPat));

	switch (num_pb_streams)
	{
	case 1:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK1; break;
	case 2:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK2; break;
	case 4:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK4; break;
	case 8:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK8; break;
	default:
			sv_printf(cn, "! ERROR: unexpected number of playback streams=%d, expecetyd 1, 2, 4 or 8\n", num_pb_streams);
			return -1;
	}

	rc = ngzmp_dma_build_test_pat(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0, &msg);
	if (rc < 0 )
	{
		sv_printf(cn, "! ERROR: building test patterns for playback: rc=%d:%s\n", rc, ngpd_get_error_message());
		return -1;
	}
	run_flags = ngpd_get_run_flags(path);
	run_flags |= NGPD_RUN_FLAGS_PLAYBACK | NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS;
	rc = ngpd_set_run_flags(path, run_flags);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: ngpd_playback_generate_zynq: Cannot set run flags : %s\n", ngpd_get_error_message());
	}
	rc = ngpd_set_playback_streams(path, num_pb_streams, 0);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: ngpd_playback_generate_zynq: Cannot set number of playback streams : %s\n", ngpd_get_error_message());
	}
	return 0;
}

int ngpd_playback_generate_cmd(int quals, int cn, int path, int card)
{
	NGPDTestPattern test_pat;
	int rc;
	int run_flags;
	int num_t;

	test_pat.num_t = NGPD_PLAYBACK_NUM_T;
	test_pat.i_baseline = 0;
	test_pat.i_height = 32768;
	test_pat.i_period = 2049;

	switch (quals & 7)
	{
	case 1:	test_pat.type = NGPDTP_Ramp; break;
	case 2:	test_pat.type = NGPDTP_Delta; break;
	case 4:	test_pat.type = NGPDTP_Square; break;
	default:
		sv_printf(cn, "! ERROR: Please specify one and only one tets pattern type\n");
		return -1;
	}
	
	if (quals & 8) 
		test_pat.i_baseline = Qualifier[3].IntArg;

	if (quals & 0x10) 
		test_pat.i_height = Qualifier[4].IntArg;

	if (quals & 0x20) 
		test_pat.i_period = Qualifier[5].IntArg;

	if (quals & 0x40)
		test_pat.num_t = 32*(Qualifier[6].IntArg/32);

	run_flags = ngpd_get_run_flags(path);
	run_flags |= NGPD_RUN_FLAGS_PLAYBACK | NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS;
	rc = ngpd_set_run_flags(path, run_flags);

	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: ngpd_playback_generate_cmd: Cannot set run flags : %s\n", ngpd_get_error_message());
	}

	rc = ngpd_playback_generate(path, card, &test_pat);

	if (rc < 0)
	{
		sv_printf(cn, "! ERROR Generating playback test pattern : %s\n", ngpd_get_error_message());
	}

	return rc;
}


int ngpd_playback_load_cmd(int quals, int cn, int path, int card, char *filename)
{
	int max_num_t=0;
	int rc;
	int run_flags;

	if (quals & 1)
		max_num_t = Qualifier[0].IntArg;

	rc = ngpd_playback_load(path, card, filename, max_num_t);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR Loading playback file : %s\n", ngpd_get_error_message());
		return rc;
	}

	run_flags = ngpd_get_run_flags(path);
	run_flags |= NGPD_RUN_FLAGS_PLAYBACK | NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS;
	rc = ngpd_set_run_flags(path, run_flags);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: ngpd_playback_load_cmd: Cannot set run flags : %s\n", ngpd_get_error_message());
	}

	return rc;
}