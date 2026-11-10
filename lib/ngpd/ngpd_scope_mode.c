#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#ifdef _MSC_VER
#include <windows.h>
#define S_IRUSR 0
#define S_IWUSR 0
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IROTH 0
#endif
#include "datamod.h"
#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"


/**
 * @ingroup internal
 * Create a shared data module in /dev/shm to store Scope mode data.
 * Usually called by {@link ngpd_config}
 *

 * @param name		 		Name for data module
 * @param num_cards			Number of Cards (XSPRESS3 boxes) in the system
 * @param lwords_per_card	Number of LWords on FEM DRAM used in each card, which must match
 * @param mod_head          Pointer to return Module header pointer to allow unlinking of module.
 * @param layout			Layout of scope mode module see {@link XSP3_SCOPE_MOD_LAYOUT}
 * @return					Pointer to the beginning of the header of the data module.
 */
NGPDScopeModule *ngpd_scope_mod_create(char *name, int num_cards, size_t nbytes_per_card, mh_com ** mod_head, int layout) 
{
	NGPDScopeModule *mod;
	mh_com *lmod_head;
	u_int16_t attr_rev = mkattrevs(MA_REENT,1);
	u_int32_t perm = MP_OWNER_READ | MP_OWNER_WRITE | MP_GROUP_READ | MP_WORLD_READ;
	u_int16_t type_lang = mktypelang(MT_DATA, ML_ANY);
	int err;
	int i;
	size_t mod_size;
	size_t num_valid_bytes = 0;

	printf("ngpd_scope_mod_create(name='%s')\n", name);
	if (mod_head == NULL)
		mod_head = &lmod_head; /* So don't bus error */

	err = _os_link(&name, mod_head, (void **) &mod, &type_lang, &attr_rev);
	if (err == 0)
	{
//		printf("Found existing module '%s'\n", name);
		if (strncmp(mod->head.magic, NGPD_SCOPE_MODULE_MAGIC, 8) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "Found data module '%s' but it does not start with the correct magic number", name);
			return NULL;
		}
		if (mod->head.layout != layout)
		{
			if ((mod->head.layout >= NGPD_SCOPE_LAYOUT_1 && mod->head.layout <= NGPD_SCOPE_LAYOUT_6) && (layout >= NGPD_SCOPE_LAYOUT_1 && layout <= NGPD_SCOPE_LAYOUT_6)) 
			{
				/* This is OK, */
				// mod->head.layout = layout; // Leave it alone so can stop and start da.server and/or imgd and carry on from where we were
			}
			else if ((mod->head.layout >= NGZMP_SCOPE_LAYOUT_4 && mod->head.layout <= NGZMP_SCOPE_LAYOUT_10) && (layout >= NGZMP_SCOPE_LAYOUT_4 && layout <= NGZMP_SCOPE_LAYOUT_10)) 
			{
				/* This is OK, */
			}
			else
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "Found data module '%s' but layout=%d != %d", name, mod->head.layout, layout);
				return NULL;
			}
		}
		if (mod->head.num_cards != num_cards) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "Found data module '%s' but num_cards=%d < requested = %d", name,
					mod->head.num_cards, num_cards);
			return NULL;
		}
		if (mod->head.nbytes_per_card != nbytes_per_card)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "Found data module '%s' but maximum size = %zu != requested= %zu",
					name, mod->head.nbytes_per_card, nbytes_per_card);
			return NULL;
		}
		return mod;
	}

	mod_size = (size_t)num_cards * ((size_t)nbytes_per_card) + sizeof(NGPDScopeModule) - sizeof(u_int16_t);
	if (layout >= NGZMP_SCOPE_LAYOUT_4 && layout <= NGZMP_SCOPE_LAYOUT_10)
	{
		num_valid_bytes = nbytes_per_card/(sizeof(u_int16_t)*NGPD_SCOPE_CHUNK_NUM_T*8);
		mod_size += (size_t)num_cards*num_valid_bytes;
	}
//	printf("Creating scope mode module size %ld bytes (note sizeof(off_t)=%d\n", mod_size, sizeof(off_t));
	err = _os_datmod(name, mod_size , &attr_rev, &type_lang, perm, (void **) &mod, (mh_data **) mod_head);

	if (err) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "Cannot create Module '%s', errno=%d", name, errno);
		return NULL;
	}
	/* Now link again ready to use, so first unlink does not make it fall out */
	err = _os_link(&name, mod_head, (void **) &mod, &type_lang, &attr_rev);
//	printf("after _os_link : mod=%p\n", mod);
	if (err != 0) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "Cannot link to module I just made. Why?");
		return NULL;
	}
	strcpy(mod->head.magic, NGPD_SCOPE_MODULE_MAGIC);
	mod->head.layout = layout;
	mod->head.num_cards = num_cards; // Number of FEM cards contained within
	mod->head.hw_version = 0; // Hardware version in case we need to change interpretation of values.
	mod->head.nbytes_per_card = nbytes_per_card; 
	
	ngpd_scope_calc_num_t(mod) ;

	for (i = 0; i < NGPD_MAX_NUM_CARDS; i++) {
		mod->head.card[i].chan_sel[0] = 0;
		mod->head.card[i].chan_sel[1] = 0;
		mod->head.card[i].src_sel[0] = 0;
		mod->head.card[i].src_sel[1] = 0;
		mod->head.card[i].alternate[0] = 0;
		mod->head.card[i].alternate[1] = 0;
	}
	if (num_valid_bytes > 0)
	{
		for (i=0; i<num_cards; i++)
			mod->head.card[i].valid_offset = num_cards*nbytes_per_card+num_valid_bytes*i;
	}

	return mod;
}
/**
 * @ingroup fullcontrol
 * Return pointer to shared data module in /dev/shm usdd to store Scope mode data.
 *

 * @param path		 		Path number returned by  {@link ngpd_config()}
 * @return					Pointer to the beginning of the header of the data module.
 */
NGPDScopeModule *ngpd_scope_get_mod(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_get_mod: Invalid path=%d", path);
		return NULL;
	}
	return NGPDPath[path].scope_mod;
}

/**
 * @addtogroup scope_module
 
 * @{ 
 */
 
 /**
 * Get pointer to data for specified card and stream with the data module.
 *
 * For XSPRESS3 the original layout, to minimise processing, the data is arranged to suit the hardware limitations of the Virtex-5. The data supplied by 2 DMA engines and is split into 2 buffers.
 * Each streams data is 16 bit data interleaved so an increment of 3 words is needed to get to the next word of the stream. This is XSP3_SCOPE_MOD_LAYOUT_2X3.
 * For XSPRESS3-mini the firmware can use 1 or 2 scope mode DMAs as necessary. The arrangement is complicated, but this function hides the details. 
 * For XSPRESS4 all 4, 8 or 16 streams are interleaved together with an increment of 4, 8 or 16 see {@link ngpd_scope_mod_get_inc}.
 * @param mod   Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @param card 	The number of the card in the xspress3 system,
 *        		0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        		where this value has been passed to {@link ngpd_config()} at xspress3 system configuration.
 * @param stream 0...5 for XSP3_SCOPE_MOD_LAYOUT_2X3, 0..9 for XSP3_SCOPE_MOD_LAYOUT_DIFFS_TEST, generally 0..ngpd_scope_mod_get_nstreams()-1  
 * @return Pointer to the 16 bit data.
 */
u_int16_t * ngpd_scope_mod_get_ptr(NGPDScopeModule *mod, int card, int stream)
{
	u_int8_t *p8;
	u_int16_t *p16;
	int sl, sh;
	u_int32_t num_scope_words;

	if (card < 0 || card >= mod->head.num_cards) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: card=%d out of range 0 .. %d", card,
				mod->head.num_cards);
		return NULL;
	}
	switch (mod->head.layout)
	{
	case NGPD_SCOPE_LAYOUT_1:
		if (stream < 0 || stream >= 1) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 0);
			return NULL;
		}
		p16 = mod->_data;
		p16 += ((size_t)(mod->head.num_t)) * card+stream;
		return p16;

	case NGPD_SCOPE_LAYOUT_2:
		if (stream < 0 || stream >= 2) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 1);
			return NULL;
		}
		p16 = mod->_data;
		p16 += ((size_t)(mod->head.num_t)) * 2*card+stream;
		return p16;

	case NGPD_SCOPE_LAYOUT_4:
		if (stream < 0 || stream >= 4) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 3);
			return NULL;
		}
		p16 = mod->_data;
		p16 += ((size_t)(mod->head.num_t)) * 4*card+stream;
		return p16;

	case NGPD_SCOPE_LAYOUT_6:
		if (stream < 0 || stream >= 6) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 5);
			return NULL;
		}
		p16 = mod->_data;
		p16 += ((size_t)(mod->head.num_t)) * 6*card+stream;
		return p16;

	case NGZMP_SCOPE_LAYOUT_4:
		if (stream < 0 || stream >= 4) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 3);
			return NULL;
		}
		p8 = (u_int8_t *)mod->_data;
		p8 += card * mod->head.nbytes_per_card;
		p16 = (u_int16_t *)p8;
		p16 += stream;
		return p16;

	case NGZMP_SCOPE_LAYOUT_8:
		if (stream < 0 || stream >= 8) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 7);
			return NULL;
		}
		num_scope_words = mod->head.nbytes_per_card/32;
		num_scope_words	&= 0xFFFFFFFC;
		p8 = (u_int8_t *)mod->_data;
		p8 += card * mod->head.nbytes_per_card;
		sl = stream % 4;
		sh = stream/4;
		p8 += sh*num_scope_words*16;
		p16 = (u_int16_t *)p8;
		p16 += sl;
		return p16;

	case NGZMP_SCOPE_LAYOUT_10:
		if (stream < 0 || stream >= 10) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: stream=%d out of range 0 .. %d", stream, 9);
			return NULL;
		}
		num_scope_words = mod->head.nbytes_per_card/40;
		num_scope_words	&= 0xFFFFFFFC;
		p8 = (u_int8_t *)mod->_data;
		p8 += card * mod->head.nbytes_per_card;
		sl = stream % 4;
		sh = stream/4;
		p8 += sh*num_scope_words*16;
		p16 = (u_int16_t *)p8;
		p16 += sl;
		return p16;

	default:
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_ptr: Module layout=%d is unknown", mod->head.layout);
		return NULL;
	}
}

/**
 * @ingroup scope_module
 * Get increment to use when stepping to next time point when accessing data in the scope mode data module.
 *
 * @param mod   Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @return Integer increment, -1 for error..
 */
int ngpd_scope_mod_get_inc(NGPDScopeModule *mod, int stream) 
{
	switch (mod->head.layout)
	{
	case NGPD_SCOPE_LAYOUT_1: return 1;
	case NGPD_SCOPE_LAYOUT_2: return 2;
	case NGPD_SCOPE_LAYOUT_4: return 4;
	case NGPD_SCOPE_LAYOUT_6: return 6;
	case NGZMP_SCOPE_LAYOUT_4: return 4;
	case NGZMP_SCOPE_LAYOUT_8: return 4;
	case NGZMP_SCOPE_LAYOUT_10: 
		if (stream < 8)
			return 4;
		else
			return 2;
	default:
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_inc: Module layout=%d is unknown", mod->head.layout);
		return -1;
	}
}

/**
 * Get number of streams per card for current scope mode data module layout.
 *
 * @param mod   Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @return Integer number of streams per card, -1 for error..
 */
int ngpd_scope_mod_get_nstreams(NGPDScopeModule *mod) 
{
	switch (mod->head.layout)
	{
	case NGPD_SCOPE_LAYOUT_1: return 1;
	case NGPD_SCOPE_LAYOUT_2: return 2;
	case NGPD_SCOPE_LAYOUT_4: return 4;
	case NGPD_SCOPE_LAYOUT_6: return 6;
	case NGZMP_SCOPE_LAYOUT_4: return 4;
	case NGZMP_SCOPE_LAYOUT_8: return 8;
	case NGZMP_SCOPE_LAYOUT_10: return 10;
	default:
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_nstreams: Module layout=%d is unknown", mod->head.layout);
		return -1;
	}
}

/**
 * @ingroup scope_module
 * Get maximum number of streams per card that can be configured for the scope mode data module.
 *
 * @param mod   Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @return Integer number of streams per card, -1 for error..
 */
int ngpd_scope_mod_max_nstreams(NGPDScopeModule *mod) 
{
	switch (mod->head.layout)
	{
	case NGPD_SCOPE_LAYOUT_1: return 6;
	case NGPD_SCOPE_LAYOUT_2: return 6;
	case NGPD_SCOPE_LAYOUT_4: return 6;
	case NGPD_SCOPE_LAYOUT_6: return 6;
	case NGZMP_SCOPE_LAYOUT_4: return 10;
	case NGZMP_SCOPE_LAYOUT_8: return 10;
	case NGZMP_SCOPE_LAYOUT_10: return 10;
	default:
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mod_get_nstreams: Module layout=%d is unknown", mod->head.layout);
		return -1;
	}
}


/**
 * @name XSPRESS3 Source Descriptions
 * String constants to describe the scope mode data stream sources in XSPRESS3.
 * The XSPRESS3 firmware provides different multiplexers for streams 0, 1 to 3 and 4 to 5.
 * These strings describe the source. They can be used to label the graphs in scope mode displays.
 * The are also used to build pop-up forms in imgd and are the recommend way for any other similar GUI.
 */
 /** @{ */
 /** Source names for XSPRESS3 stream 0.
 * For XSPRESS3 stream 0 is often used for the digital data associated with the other streams, though it can also be used for ADC data (to take 6 ADC inputs) 
 * or for digital data taken across all channels.
 */
/** 
* @name XSPRESS3-Mini Scope source select values
* @{ 
*/
/** Scope source names for streams 0..2
*/
const char *ngpd_scope_name_s0[NGPD_SCOPE_NUM_SRC0] = {"Test Pat Inc 1",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger",
	"Measure", "DataOutput", "Tail Subtract", "EvOut (Versal)" };
const char *ngpd_scope_name_s1[NGPD_SCOPE_NUM_SRC1] = {"Test Pat Inc 2", "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", 
	"Measure",  "DataOutput", "Tail Subtract", "EvOut (Versal)", "Not Impl","Not Impl","Not Impl", "Digital 5 Bit"};
const char *ngpd_scope_name_s2[NGPD_SCOPE_NUM_SRC2] = {"Test Pat Inc 3",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger",
	"Measure", "DataOutput", "Tail Subtract", "EvOut (Versal)"  };
const char *ngpd_scope_name_s3[NGPD_SCOPE_NUM_SRC3] = {"Test Pat Inc 4",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", 
	"Measure",  "DataOutput", "Tail Subtract", "Not Impl", "Not Impl","Not Impl","Digital 4 Bit", "Digital 5 Bit"};
const char *ngpd_scope_name_s4[NGPD_SCOPE_NUM_SRC4] = {"Test Pat Inc 5",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", "Measure", "DataOutput", "Tail Subtract"  };
const char *ngpd_scope_name_s5[NGPD_SCOPE_NUM_SRC5] = { "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Not Impl",  
									   "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Not Impl", "Digital 3 Bit", "Digital 5 Bit" };

const char *ngzmp_scope_name_ana[NGZMP_SCOPE_NUM_SRC_ANA] = {"Test Pat Inc",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", "Measure", "DataOutput", "Tail Subtract" };
const char *ngzmp_scope_name_s3[NGZMP_SCOPE_NUM_SRC3] = {"Test Pat Inc",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", "Measure", "DataOutput", "Tail Subtract",
	"Not used", "Not used", "Not used", "Not used", "Digital 5 bit"};
const char *ngzmp_scope_name_s7[NGZMP_SCOPE_NUM_SRC7] = {"Test Pat Inc",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", "Measure", "DataOutput", "Tail Subtract",
	"Not used", "Not used", "Not used", "Not used", "Digital 5 bit"};
const char *ngzmp_scope_name_s8[NGZMP_SCOPE_NUM_SRC8] = {"Test Pat Inc",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", "Measure", "DataOutput", "Tail Subtract",
	"Not used", "Not used", "Not used", "Digital 4 bit"};
const char *ngzmp_scope_name_s9[NGZMP_SCOPE_NUM_SRC9] = {"Test Pat Inc",  "ADC Inp", "Filter Out", "Diff Trig Diff", "Diff Trig Out", "BaseSub Out", "Base Sub Internal", "ABS Trigger", "Measure", "DataOutput", "Tail Subtract",
	"Not used", "Not used", "Digital 4x3 bit+4*1bit", "Digital 4 bit"};


/** @} */

static int ngpd_max_digital_s0to4[16] = {5, 5, 5, 5,   5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 0, 0};
static int ngpd_max_digital_s5[16] = {5, 5, 5, 5,   5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 0, 0};

static int ngzmp_max_digital_ana[16] = {5, 5, 5, 5,   5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 0, 0};
static int ngzmp_max_digital_s3[16] = {5, 5, 5, 5,   5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 0, 0};
static int ngzmp_max_digital_s7[16] = {5, 5, 5, 5,   5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 0, 0};
static int ngzmp_max_digital_s8[16] = {5, 5, 5, 5,   5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 0, 0};
static int ngzmp_max_digital_s9[16] = {0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0};


/** @name alternates Source Signal Descriptions
 * String constants to describe the alternate field options available for each source in xspress4.
 * Each array contains 16 char pointers for the 16 possible alternate values, with unused alternate values set to NULL.
 * 
 * Currently they are used to build pop-up forms in imgd and are the recommend way for any other similar GUI.
 * Potentially the signal names could be extracted to label individual digital traces.
 @{
 */

const char *ngpd_scope_alt_names_s0to4[16][16] =
{ 
/* 0 => Test Pat */
{"00000 : TestPat",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 1 => Data Input */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : DataInput[15:0]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : 0 & DataInput[15:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 2 => Filter out */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : FiltData[16:1]",   "0 & RunSyncADC & 0 & PBStart & TrigInQ : 0 RawData[15:1] (Versal)", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 3=> Diff trigger internals */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Diff[16:1]", "0 & TrigStretched & TrigOut & RawTrig & Armed : Diff[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 4=> Diff trigger Output */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Data[16:1]", "TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Data[16:1]", 
 "0 & TrigStretchedB & TrigStretchedA & TrigOut & RawTrig : Data[16:1]", "0 & TrigStretchedB & TrigStretchedA & TrigOut & RawTrig : Data[16:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 5 => BaseSub output */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[16:1]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 6 Base Sub Internals */
{"0 & TrigStrB & TrigStrA & Trig & AccErr : Baseline",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 7 ABS Trigger */
{"00000 : Not Impl",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 8 => Measure Pulse */
{"SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[16:1]",  "SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[17:2]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : Height[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : FallTime[7:0]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", "Unused", 
 "AckData & SumAborted & Pileup & Busy & Trig : Data[16:1]",  "AckData & SumAborted & Pileup & Busy & Trig : Data[17:2]", 
 "AckData & SumAborted & Pileup & Busy & Trig : Height[16:1]", "AckData & SumAborted & Pileup & Busy & Trig : FallTime[7:0]", "AckData & SumAborted & Pileup & Busy & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", NULL},
/* 9 Data Output */
{"SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : Height[16:1]", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : FallTime", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailCount",  "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailSum[16:1]",  
	"Unused",  "Unused",  "Unused",  "Unused",  
 "AckData & Pileup & SumAborted & SumValid & HgtValid : Height[16:1]", "AckData & Pileup & SumAborted & SumValid & HgtValid : FallTime", "AckData & Pileup & SumAborted & SumValid & HgtValid : TailCount",  "AckData & Pileup & SumAborted & SumValid & HgtValid : TailSum[16:1]",	
	NULL,  NULL,  NULL,  NULL },
/* 10 Tail Subtract */
{"Enable0 & PollRun0 & Running0 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable1 & PollRun1 & Running1 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable2 & PollRun2 & Running2 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable3 & PollRun3 & Running3 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable4 & PollRun4 & Running4 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable5 & PollRun5 & Running5 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable6 & PollRun6 & Running6 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable7 & PollRun7 & Running7 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable8 & PollRun8 & Running8 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable9 & PollRun9 & Running9 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableA & PollRunA & RunningA & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableB & PollRunB & RunningB & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableC & PollRunC & RunningC & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableD & PollRunD & RunningD & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableE & PollRunD & RunningE & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableF & PollRunF & RunningF & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract"  },
/* Event outptus, Versal only */
{"Ev0: Dummy & Last & Samples & Header : Data", "Ev1: Dummy & Last & Samples & Header : Data", NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
/*12..13 unused so far */
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


const char *ngzmp_scope_alt_names_ana[16][16] =
{ 
/* 0 => Test Pat */
{"00000 : TestPat",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 1 => Data Input */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : DataInput[15:0]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : 0 & DataInput[15:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 2 => Filter out */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : FiltData[16:1]",   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 3=> Diff trigger internals */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Diff[16:1]", "0 & TrigStretched & TrigOut & RawTrig & Armed : Diff[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 4=> Diff trigger Output */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Data[16:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 5 => BaseSub output */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[16:1]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 6 Base Sub Internals */
{"0 AccErr & TrigStrB & TrigStrA & Trig : Baseline",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 7 ABS Trigger */
{"00000 : Not Impl",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 8 => Measure Pulse */
{"SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[16:1]",  "SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[17:2]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : Height[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : FallTime[7:0]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", "Unused", 
 "AckData & SumAborted & Pileup & Busy & Trig : Data[16:1]",  "AckData & SumAborted & Pileup & Busy & Trig : Data[17:2]", 
 "AckData & SumAborted & Pileup & Busy & Trig : Height[16:1]", "AckData & SumAborted & Pileup & Busy & Trig : FallTime[7:0]", "AckData & SumAborted & Pileup & Busy & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", NULL},
/* 9 Data Output */
{"SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : Height[16:1]", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : FallTime", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailCount",  "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailSum[16:1]",  
	"Unused",  "Unused",  "Unused",  "Unused",  
 "AckData & Pileup & SumAborted & SumValid & HgtValid : Height[16:1]", "AckData & Pileup & SumAborted & SumValid & HgtValid : FallTime", "AckData & Pileup & SumAborted & SumValid & HgtValid : TailCount",  "AckData & Pileup & SumAborted & SumValid & HgtValid : TailSum[16:1]",	
	NULL,  NULL,  NULL,  NULL },
/* 10 Tail Subtract */
{"Enable0 & PollRun0 & Running0 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable1 & PollRun1 & Running1 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable2 & PollRun2 & Running2 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable3 & PollRun3 & Running3 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable4 & PollRun4 & Running4 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable5 & PollRun5 & Running5 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable6 & PollRun6 & Running6 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable7 & PollRun7 & Running7 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable8 & PollRun8 & Running8 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable9 & PollRun9 & Running9 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableA & PollRunA & RunningA & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableB & PollRunB & RunningB & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableC & PollRunC & RunningC & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableD & PollRunD & RunningD & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableE & PollRunD & RunningE & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableF & PollRunF & RunningF & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract"  },
/*11..13 unused so far */
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

const char *ngzmp_scope_alt_names_s3and7[16][16] =
{ 
/* 0 => Test Pat */
{"00000 : TestPat",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 1 => Data Input */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : DataInput[15:0]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : 0 & DataInput[15:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 2 => Filter out */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : FiltData[16:1]",   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 3=> Diff trigger internals */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Diff[16:1]", "0 & TrigStretched & TrigOut & RawTrig & Armed : Diff[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 4=> Diff trigger Output */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Data[16:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 5 => BaseSub output */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[16:1]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 6 Base Sub Internals */
{"0 AccErr & TrigStrB & TrigStrA & Trig : Baseline",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 7 ABS Trigger */
{"00000 : Not Impl",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 8 => Measure Pulse */
{"SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[16:1]",  "SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[17:2]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : Height[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : FallTime[7:0]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", "Unused", 
 "AckData & SumAborted & Pileup & Busy & Trig : Data[16:1]",  "AckData & SumAborted & Pileup & Busy & Trig : Data[17:2]", 
 "AckData & SumAborted & Pileup & Busy & Trig : Height[16:1]", "AckData & SumAborted & Pileup & Busy & Trig : FallTime[7:0]", "AckData & SumAborted & Pileup & Busy & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", NULL},
/* 9 Data Output */
{"SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : Height[16:1]", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : FallTime", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailCount",  "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailSum[16:1]",  
	"Unused",  "Unused",  "Unused",  "Unused",  
 "AckData & Pileup & SumAborted & SumValid & HgtValid : Height[16:1]", "AckData & Pileup & SumAborted & SumValid & HgtValid : FallTime", "AckData & Pileup & SumAborted & SumValid & HgtValid : TailCount",  "AckData & Pileup & SumAborted & SumValid & HgtValid : TailSum[16:1]",	
	NULL,  NULL,  NULL,  NULL },
/* 10 Tail Subtract */
{"Enable0 & PollRun0 & Running0 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable1 & PollRun1 & Running1 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable2 & PollRun2 & Running2 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable3 & PollRun3 & Running3 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable4 & PollRun4 & Running4 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable5 & PollRun5 & Running5 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable6 & PollRun6 & Running6 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable7 & PollRun7 & Running7 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable8 & PollRun8 & Running8 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable9 & PollRun9 & Running9 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableA & PollRunA & RunningA & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableB & PollRunB & RunningB & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableC & PollRunC & RunningC & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableD & PollRunD & RunningD & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableE & PollRunD & RunningE & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableF & PollRunF & RunningF & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract"  },
/*11..13 unused so far */
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};
const char *ngzmp_scope_alt_names_s8[16][16] =
{ 
/* 0 => Test Pat */
{"00000 : TestPat",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 1 => Data Input */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : DataInput[15:0]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : 0 & DataInput[15:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 2 => Filter out */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : FiltData[16:1]",   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 3=> Diff trigger internals */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Diff[16:1]", "0 & TrigStretched & TrigOut & RawTrig & Armed : Diff[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 4=> Diff trigger Output */
{"TrigStretchedB & TrigStretchedA & TrigOut & RawTrig & Armed : Data[16:1]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 5 => BaseSub output */
{"0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[16:1]",  "0 & RunSyncADC & 0 & PBStart & TrigInQ : BasSubData[17:2]", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 6 Base Sub Internals */
{"0 AccErr & TrigStrB & TrigStrA & Trig : Baseline",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 7 ABS Trigger */
{"00000 : Not Impl",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/* 8 => Measure Pulse */
{"SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[16:1]",  "SumValid & FallValid & HgtValid & LEScaled & Trig : DataRaw[17:2]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : Height[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : FallTime[7:0]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", "Unused", 
 "AckData & SumAborted & Pileup & Busy & Trig : Data[16:1]",  "AckData & SumAborted & Pileup & Busy & Trig : Data[17:2]", 
 "AckData & SumAborted & Pileup & Busy & Trig : Height[16:1]", "AckData & SumAborted & Pileup & Busy & Trig : FallTime[7:0]", "AckData & SumAborted & Pileup & Busy & Trig : TailSum[16:1]", 
 "SumValid & FallValid & HgtValid & LEScaled & Trig : DataCorr[16:1]", "SumValid & FallValid & HgtValid & LEScaled & Trig : TailCorr[16:1]", NULL},
/* 9 Data Output */
{"SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : Height[16:1]", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : FallTime", "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailCount",  "SumValid & FallInWin & SumGEThres & HgtInWin & Neutron : TailSum[16:1]",  
	"Unused",  "Unused",  "Unused",  "Unused",  
 "AckData & Pileup & SumAborted & SumValid & HgtValid : Height[16:1]", "AckData & Pileup & SumAborted & SumValid & HgtValid : FallTime", "AckData & Pileup & SumAborted & SumValid & HgtValid : TailCount",  "AckData & Pileup & SumAborted & SumValid & HgtValid : TailSum[16:1]",	
	NULL,  NULL,  NULL,  NULL },
/* 10 Tail Subtract */
{"Enable0 & PollRun0 & Running0 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable1 & PollRun1 & Running1 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable2 & PollRun2 & Running2 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable3 & PollRun3 & Running3 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable4 & PollRun4 & Running4 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable5 & PollRun5 & Running5 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable6 & PollRun6 & Running6 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable7 & PollRun7 & Running7 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "Enable8 & PollRun8 & Running8 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "Enable9 & PollRun9 & Running9 & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableA & PollRunA & RunningA & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableB & PollRunB & RunningB & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableC & PollRunC & RunningC & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableD & PollRunD & RunningD & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", 
 "EnableE & PollRunD & RunningE & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract", "EnableF & PollRunF & RunningF & StartSubtract	& Neutron : 000 & DestCounter & 000 & NextSubtract"  },
/*11..13 unused so far */
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};
const char *ngzmp_scope_alt_names_s9[16][16] =
{ 
/* 0 => Test Pat */
{"00000 : TestPat",  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
/*1..13 unused so far */
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Unused", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Digital", NULL, NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL },
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


const int ngpd_scale_to_16bit_s0to4[16][16] =
{
/* Test pat is shown unscaled */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 1 => Data Input , unshift 1:1, divided by 2, need to multiply by 2 */
{ 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 2 => Filter out, mult by  2*/
{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
/* 3=> Diff trigger internals */
{ 2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 4=> Diff trigger Output */
{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
/* 5 => BaseSub output */
{ 2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 6 Base Sub Internals */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 7 ABS Trigger */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 8 => Measure Pulse */
{ 2, 4, 2, 1, 2, 2, 2, 1, 2, 4, 2, 1, 2, 2, 2, 1 },
/* 9 Data output */
{ 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1 },
/*10 Tail subtract */
{ 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
/*11..13 unused so far */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

const int ngzmp_scale_to_16bit_ana[16][16] =
{
/* Test pat is shown unscaled */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 1 => Data Input , unshift 1:1, divided by 2, need to multiply by 2 */
{ 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 2 => Filter out, mult by  2*/
{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
/* 3=> Diff trigger internals */
{ 2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 4=> Diff trigger Output */
{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
/* 5 => BaseSub output */
{ 2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 6 Base Sub Internals */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 7 ABS Trigger */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
/* 8 => Measure Pulse */
{ 2, 4, 2, 1, 2, 2, 2, 1, 2, 4, 2, 1, 2, 2, 2, 1 },
/* 9 Data output */
{ 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1 },
/*10 Tail subtract */
{ 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
/*11..13 unused so far */
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

const char *ngpd_scope_alt_names_s5[16][16] =
{ 
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Unused", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
{"Digital", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
};


/** @} */


static NGPDScopeStream ngpd_scope_type_s0to4[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								  			NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed,  NGPDScopeStream_Signed,  NGPDScopeStream_Signed, 
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_AuxDigital, NGPDScopeStream_AuxDigital };

static NGPDScopeStream ngpd_scope_type_s5[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned,  
											   NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned,  
											   NGPDScopeStream_AuxDigital, NGPDScopeStream_AuxDigital, NGPDScopeStream_AllDigital, NGPDScopeStream_AllDigital,
								 			   NGPDScopeStream_AllDigital, NGPDScopeStream_AllDigital, NGPDScopeStream_AuxDigital, NGPDScopeStream_AuxDigital };


static NGPDScopeStream ngzmp_scope_type_ana[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								  			NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed,  NGPDScopeStream_Signed,  NGPDScopeStream_Signed, 
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned };

static NGPDScopeStream ngzmp_scope_type_s3[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								  			NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed,  NGPDScopeStream_Signed,  NGPDScopeStream_Signed, 
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Unsigned, NGPDScopeStream_AuxDigital };

static NGPDScopeStream ngzmp_scope_type_s7[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								  			NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed,  NGPDScopeStream_Signed,  NGPDScopeStream_Signed, 
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Unsigned, NGPDScopeStream_AuxDigital };

static NGPDScopeStream ngzmp_scope_type_s8[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								  			NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_Signed,  
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed,  NGPDScopeStream_Signed,  NGPDScopeStream_Signed, 
								 			 NGPDScopeStream_Signed, NGPDScopeStream_Signed, NGPDScopeStream_AuxDigital, NGPDScopeStream_Unsigned };

static NGPDScopeStream ngzmp_scope_type_s9[16] = { NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned,  
											   NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned, NGPDScopeStream_Unsigned,  
											   NGPDScopeStream_AllDigital, NGPDScopeStream_AllDigital, NGPDScopeStream_AllDigital, NGPDScopeStream_AllDigital,
								 			   NGPDScopeStream_AllDigital, NGPDScopeStream_AuxDigital, NGPDScopeStream_AuxDigital, NGPDScopeStream_AuxDigital };


/**
 * Return a label to describe the describe the data on a given scope stream.
 *
 * @param mod	Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @param card 	The number of the card in the xspress3 system,
 *        		0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        		where this value has been passed to {@link ngpd_config()} at xspress3 system configuration.
 * @param stream	Stream number
 * @return
 */
const char *ngpd_scope_stream_name(NGPDScopeModule *mod, int card, int stream) {
	int src_sel;
	int nstreams = ngpd_scope_mod_get_nstreams(mod);
	if (card < 0 || card >= mod->head.num_cards) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_stream_name: card=%d out of range 0 .. %d", card,	mod->head.num_cards-1);
		return NULL;
	}
	if (stream < 0 || stream >= nstreams)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_stream_name: stream=%d out of range 0 .. %d", stream, nstreams-1);
		return NULL;
	}
	if (mod->head.layout >= NGPD_SCOPE_LAYOUT_1 && mod->head.layout <= NGPD_SCOPE_LAYOUT_6)
	{
		src_sel = NGPD_SCOPE_SRC_SEL_GET(stream, mod->head.card[card].src_sel[0]);
		switch (stream)
		{
		case 0: 
			if (src_sel < NGPD_SCOPE_NUM_SRC0)
				return ngpd_scope_name_s0[src_sel];
			else
				return "Unknown";

		case 1: 
			if (src_sel < NGPD_SCOPE_NUM_SRC1)
				return ngpd_scope_name_s1[src_sel];
			else
				return "Unknown";

		case 2: 
			if (src_sel < NGPD_SCOPE_NUM_SRC2)
				return ngpd_scope_name_s2[src_sel];
			else
				return "Unknown";

		case 3: 
			if (src_sel < NGPD_SCOPE_NUM_SRC3)
				return ngpd_scope_name_s3[src_sel];
			else
				return "Unknown";

		case 4: 
			if (src_sel < NGPD_SCOPE_NUM_SRC4)
				return ngpd_scope_name_s4[src_sel];
			else
				return "Unknown";

		case 5: 
			if (src_sel < NGPD_SCOPE_NUM_SRC5)
				return ngpd_scope_name_s5[src_sel];
			else
				return "Unknown";
		default:
			return "Unknown stream > 5";
		}
	}
	else
	{
		src_sel = NGZMP_SCOPE_SRC_SEL_GET(stream, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]);
		if (stream == 3)
		{
			if (src_sel < NGZMP_SCOPE_NUM_SRC3)
				return 	ngzmp_scope_name_s3[src_sel];
			else
				return  "Unknown";
		}
		else if (stream == 7)
		{
			if (src_sel < NGZMP_SCOPE_NUM_SRC7)
				return 	ngzmp_scope_name_s7[src_sel];
			else
				return  "Unknown";
		}
		else if (stream == 8)
		{
			if (src_sel < NGZMP_SCOPE_NUM_SRC8)
				return 	ngzmp_scope_name_s8[src_sel];
			else
				return  "Unknown";
		}
		else if (stream == 9)
		{
			if (src_sel < NGZMP_SCOPE_NUM_SRC9)
				return 	ngzmp_scope_name_s9[src_sel];
			else
				return  "Unknown";
		}
		else
		{
			if (src_sel < NGZMP_SCOPE_NUM_SRC_ANA)
				return 	ngzmp_scope_name_ana[src_sel];
			else
				return "Unknown";
		}
	}
	return NULL;
}

/**
 * Inquire about a scope mode stream, to discover what type of data it contains. 
 * Also determine if there is any associated digital data and if so how to access it. 
 * In special cases (fine time only so far) also determine how to access the extra data
 * This is the preferred way to determine how to access the digital data associated with a scope mode stream and abstracts away the difference between NGPD generations.
 *
 * @param mod   	Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @param card 		The number of the card in the xspress3 system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        			Where this value has been passed to {@link ngpd_config()} at xspress3 system configuration.
 * @param stream 	0..ngpd_scope_mod_get_nstreams()-1
 * @param *num_digP Pointer to return the number of digital bits associated with this data stream. Pass NULL to disable this return. 
 * @param *bit_posnP Pointer to return the bit position within the 16 bit word of the digital bits associated with this data stream.  Pass NULL to disable this return. 
 * @param **dig_ptrP Pointer to return the pointer to the digital bits associated with this data stream. Pass NULL to disable this return. 
 * @param *num_extraP Pointer to return the number of extra digital bits associated with this data stream. For future expansion. Pass NULL to disable this return. 
 * @param *extra_posnP Pointer to return the bit position of any extra digital bits associated with this data stream. For future expansion. Pass NULL to disable this return. 
 * @param *extra_ptrP Pointer to return the pointer to any extra digital bits associated with this data stream. For future expansion Pass NULL to disable this return. 
 * @param *scope_incP Pointer to return the increment use to jump to teh next sample in this strteam, different for stream 8 and 9. Pass NULL to diabel, but should be used to fully support.
 * @return Codes whether the data is Unsigned 16 bit "analogue" data, Signed 16 bit "analogue" data or digital data {@link NGPDScopeStream}
 */

NGPDScopeStream ngpd_scope_stream_details(NGPDScopeModule *mod, int card, int stream, int *num_digP, int *bit_posnP, u_int16_t **dig_ptrP, int *num_extraP, int *extra_posnP, u_int16_t **extra_ptrP, int *scope_incP, int *dig_incP)
{
	int dig_stream, extra_stream;
	NGPDScopeStream rc;
	
	rc = ngpd_scope_stream_details_num(mod, card, stream, num_digP, bit_posnP, &dig_stream, num_extraP, extra_posnP, &extra_stream, scope_incP, dig_incP);

	if (dig_ptrP != NULL)
		*dig_ptrP = ngpd_scope_mod_get_ptr(mod, card, dig_stream);

	if (extra_ptrP != NULL)
		*extra_ptrP = ngpd_scope_mod_get_ptr(mod, card, extra_stream);

	return rc;
}

/**
 * Inquire about a scope mode stream from the system path, to discover what type of data it contains. 
 * Also determine if there is any associated digital data and if so how to access it, and returns the stream number used to access it.
 * In special cases (fine time only so far) also determine how to access the extra data
 * This is the preferred way to determine how to access the digital data associated with a scope mode stream and abstracts away the difference between NGPD generations.
 *
 * @param mod   	Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @param card 		The number of the card in the xspress3 system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        			Where this value has been passed to {@link ngpd_config()} at xspress3 system configuration.
 * @param stream 	0..ngpd_scope_mod_get_nstreams()-1
 * @param *num_digP Pointer to return the number of digital bits associated with this data stream. Pass NULL to disable this return. 
 * @param *bit_posnP Pointer to return the bit position within the 16 bit word of the digital bits associated with this data stream.  Pass NULL to disable this return. 
 * @param *dig_strP  Pointer to return the Stream number of the digital bits associated with this data stream. Pass NULL to disable this return. 
 * @param *num_extraP Pointer to return the number of extra digital bits associated with this data stream. For future expansion. Pass NULL to disable this return. 
 * @param *extra_posnP Pointer to return the bit position of any extra digital bits associated with this data stream. For future expansion. Pass NULL to disable this return. 
 * @param *extra_strP Pointer to return the stream number of any extra digital bits associated with this data stream. For future expansion Pass NULL to disable this return. 
 * @param *scope_incP Pointer to return the increment use to jump to teh next sample in this stream, different for stream 8 and 9. Pass NULL to disable, but should be used to fully support.
 * @return Codes whether the data is Unsigned 16 bit "analogue" data, Signed 16 bit "analogue" data or digital data {@link NGPDScopeStream}
 */
NGPDScopeStream ngpd_scope_stream_details_path(int path, int card, int stream, int *num_digP, int *bit_posnP, int *dig_strP, int *num_extraP, int *extra_posnP, int *extra_strP, int *scope_incP, int *dig_incP)
{
	NGPDScopeModule *mod;
	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_stream_details_path: invalid path %d", path);
		return NGPD_ERROR;
	}
	mod = NGPDPath[path].scope_mod;
	if (mod == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_stream_details_path: scope module pointer is NULL");
		return NGPD_ERROR;
	}
	return ngpd_scope_stream_details_num(mod, card, stream, num_digP, bit_posnP, dig_strP, num_extraP, extra_posnP, extra_strP, scope_incP, dig_incP);
}
		
/**
 * Inquire about a scope mode stream, to discover what type of data it contains. 
 * Also determine if there is any associated digital data and if so how to access it, and returns the stream number used to access it.
 * In special cases (fine time only so far) also determine how to access the extra data
 * This is the preferred way to determine how to access the digital data associated with a scope mode stream and abstracts away the difference between NGPD generations.
 *
 * @param mod   	Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @param card 		The number of the card in the xspress3 system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        			Where this value has been passed to {@link ngpd_config()} at xspress3 system configuration.
 * @param stream 	0..ngpd_scope_mod_get_nstreams()-1
 * @param *num_digP Pointer to return the number of digital bits associated with this data stream. Pass NULL to disable this return. 
 * @param *bit_posnP Pointer to return the bit position within the 16 bit word of the digital bits associated with this data stream.  Pass NULL to disable this return. 
 * @param *dig_strP  Pointer to return the Stream number of the digital bits associated with this data stream. Pass NULL to disable this return. 
 * @param *num_extraP Pointer to return the number of extra digital bits associated with this data stream. For future expansion. Pass NULL to disable this return. 
 * @param *extra_posnP Pointer to return the bit position of any extra digital bits associated with this data stream. For future expansion. Pass NULL to disable this return. 
 * @param *extra_strP Pointer to return the stream number of any extra digital bits associated with this data stream. For future expansion Pass NULL to disable this return. 
 * @param *scope_incP Pointer to return the increment use to jump to teh next sample in this stream, different for stream 8 and 9. Pass NULL to disable, but should be used to fully support.
 * @return Codes whether the data is Unsigned 16 bit "analogue" data, Signed 16 bit "analogue" data or digital data {@link NGPDScopeStream}
 */
NGPDScopeStream ngpd_scope_stream_details_num(NGPDScopeModule *mod, int card, int stream, int *num_digP, int *bit_posnP, int *dig_strP, int *num_extraP, int *extra_posnP, int *extra_strP, int *scope_incP, int *dig_incP)
{
	int src_sel;
	int num_dig=0, bit_posn=0, dig_stream=0;
	NGPDScopeStream type = NGPDScopeStream_Error;
	int max_dig;
	int num_extra=0, extra_posn=0, extra_stream=0;
	int bp5, ds5, bp4, ds4, bp3, ds3, bp_mixed;
	int nstreams = ngpd_scope_mod_get_nstreams(mod);
	int scope_inc, dig_inc;

	if (card < 0 || card >= mod->head.num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_stream_details: card=%d out of range 0 .. %d", card, mod->head.num_cards-1);
		return NGPDScopeStream_Error;
	}
	if (stream < 0 || stream >= nstreams) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_stream_details: stream=%d out of range 0 .. %d", stream, nstreams-1);
		return NGPDScopeStream_Error;
	}
	//	printf("card=%d, stream=%d, layout=%d\n", card, stream, mod->head.layout);
	if (mod->head.layout >= NGPD_SCOPE_LAYOUT_1 && mod->head.layout <= NGPD_SCOPE_LAYOUT_6)
	{
		src_sel = NGPD_SCOPE_SRC_SEL_GET(stream, mod->head.card[card].src_sel[0]);
		scope_inc = ngpd_scope_mod_get_inc(mod, stream); 
		dig_inc = ngpd_scope_mod_get_inc(mod, stream); 
		if (stream == 5)
		{
			/* The last stream can be (is usually digital) */
			type = ngpd_scope_type_s5[src_sel];
			max_dig = 0;
		}
		else
		{
			type = ngpd_scope_type_s0to4[src_sel];
			max_dig = ngpd_max_digital_s0to4[src_sel];
			bp5 = (stream%3)*5;
			ds5 = stream/3;
			bp3 = (stream%5)*3;
			ds3 = stream/5;

			src_sel = NGPD_SCOPE_SRC_SEL_GET(nstreams-1, mod->head.card[card].src_sel[0]);
			if (ds5 == 0 && src_sel == NGPD_SCOPE_SEL5_DIG_5BIT)
			{
				num_dig = 5;
				bit_posn = bp5;
				dig_stream = nstreams-1;
			}
			else if (ds3 == 0 && src_sel == NGPD_SCOPE_SEL5_DIG_3BIT)
			{
				num_dig = 3;
				bit_posn = bp3;
				dig_stream = nstreams-1;
			}
		}
		if (num_dig > max_dig)
			num_dig = max_dig;
		if (num_digP != NULL)
			*num_digP = num_dig;
		if (bit_posnP != NULL)
			*bit_posnP = bit_posn;
		if (dig_strP != NULL)
			*dig_strP = dig_stream;
		if (num_extraP != NULL)
			*num_extraP = num_extra;
		if (extra_posnP != NULL)
			*extra_posnP = extra_posn;
		if (extra_strP != NULL)
			*extra_strP = extra_stream;
		if (scope_incP != NULL)
			*scope_incP= scope_inc;
		if (dig_incP != NULL)
			*dig_incP= dig_inc;
		return type;
	}
	else if (mod->head.layout >= NGZMP_SCOPE_LAYOUT_4 && mod->head.layout <= NGZMP_SCOPE_LAYOUT_10)
	{
		src_sel = NGZMP_SCOPE_SRC_SEL_GET(stream, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]);
		scope_inc = ngpd_scope_mod_get_inc(mod, stream); 
		if (mod->head.layout == NGZMP_SCOPE_LAYOUT_4)
		{
			if (stream == 3)
			{
				/* The last stream can be (is usually digital) */
				type = ngzmp_scope_type_s3[src_sel];
				max_dig = 0;
			}
			else
			{
				type = ngzmp_scope_type_ana[src_sel];
				max_dig = ngzmp_max_digital_ana[src_sel];
				bp5 = (stream%3)*5;
				src_sel = NGZMP_SCOPE_SRC_SEL_GET(3, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]);
				if (src_sel == NGZMP_SCOPE_SEL37_DIG_5BIT)
				{
					num_dig = 5;
					bit_posn = bp5;
					dig_stream = 3;
				}
			}
		}
		else if (mod->head.layout == NGZMP_SCOPE_LAYOUT_8)
		{
			if (stream == 3)
			{
				/* In 8 stream mode stream 3 and 7 are often used for digital, but can be used for analogue to record all 8 stream */
				type = ngzmp_scope_type_s3[src_sel];
				max_dig = 0;
			}
			else if (stream == 7)
			{
				/* In 8 stream mode stream 3 and 7 are often used for digital, but can be used for analogue to record all 8 stream */
				type = ngzmp_scope_type_s7[src_sel];
				max_dig = 0;
			}
			else
			{
				type = ngzmp_scope_type_ana[src_sel];
				max_dig = ngzmp_max_digital_ana[src_sel];
				if (stream < 3)
				{
					bp5 = (stream%3)*5;
					dig_stream = 3;
				}
				else
				{
					bp5 = ((stream-1)%3)*5;
					dig_stream = 7;
				}
				src_sel = NGZMP_SCOPE_SRC_SEL_GET(dig_stream, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]);
				if (src_sel == NGZMP_SCOPE_SEL37_DIG_5BIT)
				{
					num_dig = 5;
					bit_posn = bp5;
				}
			}
		}
		else
		{
			type = ngzmp_scope_type_ana[src_sel];
			max_dig = ngzmp_max_digital_ana[src_sel];
			if (stream == 3  && NGZMP_SCOPE_SRC_SEL_GET(3, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL37_DIG_5BIT)
			{
				type = ngzmp_scope_type_s3[src_sel];
				max_dig = 0; /* And default num_dig = 0 */
			}
			else if (stream == 7 && NGZMP_SCOPE_SRC_SEL_GET(7, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL37_DIG_5BIT)
			{
				type = ngzmp_scope_type_s7[src_sel];
				max_dig = 0; /* And default num_dig = 0 */
			}
			else if (stream == 8 && NGZMP_SCOPE_SRC_SEL_GET(8, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL8_DIG_4BIT)
			{
				type = ngzmp_scope_type_s8[src_sel];
				max_dig = 0; /* And default num_dig = 0 */
			}
			else if (stream == 9)
			{
				type = ngzmp_scope_type_s9[src_sel];
				max_dig = 0; /* And default num_dig = 0 */
			}
			else
			{
				ds5 = -1;
				ds4 = -1;
				max_dig = ngzmp_max_digital_ana[src_sel];
				if (stream <= 2)
				{
					bp5 = (stream%3)*5;
					ds5 = 0;
				}
				else if (stream >=4 && stream <=6)
				{
					bp5 = stream-4;
					ds5 = 1;
				}
				bp4 = (stream%4)*4;
				ds4 = stream/4;
				if (stream < 4)
					bp_mixed = stream;
				else
					bp_mixed = (stream-4)*2+4;
				/* In 10 stream mode, usualy expect 4 digital bits per stream from stream0..7 stored in stream 8 and 9.
				But check for 5 bit digital in 3 and 7 first */
				if (ds5 == 0 && NGZMP_SCOPE_SRC_SEL_GET(3, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL37_DIG_5BIT)
				{
					num_dig = 5;
					bit_posn = bp5;
					dig_stream = 3;
				}
				else if (ds5 == 1 && NGZMP_SCOPE_SRC_SEL_GET(7, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL37_DIG_5BIT)
				{
					num_dig = 5;
					bit_posn = bp5;
					dig_stream = 7;
				}
				else if (ds4 == 0 && NGZMP_SCOPE_SRC_SEL_GET(8, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL8_DIG_4BIT)
				{
					num_dig = 4;
					bit_posn = bp4;
					dig_stream = 8;
				}
				else if (ds4 == 1 && NGZMP_SCOPE_SRC_SEL_GET(9, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL9_DIG_4BIT)
				{
					num_dig = 4;
					bit_posn = bp4;
					dig_stream = 9;
				}
				else if (NGZMP_SCOPE_SRC_SEL_GET(9, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]) == NGZMP_SCOPE_SEL9_DIG_4X14X2BIT)
				{
					num_dig = (stream<4)?1:2;
					bit_posn = bp_mixed;
					dig_stream = 9;
				}
			}
		}
		if (num_dig > max_dig)
			num_dig = max_dig;
		if (num_digP != NULL)
			*num_digP = num_dig;
		if (bit_posnP != NULL)
			*bit_posnP = bit_posn;
		if (dig_strP != NULL)
			*dig_strP = dig_stream;

		if (num_extraP != NULL)
			*num_extraP = num_extra;
		if (extra_posnP != NULL)
			*extra_posnP = extra_posn;
		if (extra_strP != NULL)
			*extra_strP = extra_stream;
		if (scope_incP != NULL)
			*scope_incP= scope_inc;
		if (dig_incP != NULL)
			*dig_incP= ngpd_scope_mod_get_inc(mod, dig_stream);
//		printf ("ngpd_scope_stream_details (stream=%d) => dig_stream = %d, bit_posn=%d\n", stream, dig_stream, bit_posn);
		return type;
	}
	return NGPDScopeStream_Error;
}

/**
 * Return Channel number (currently within card) for specified stream. This is used to create meaningful row labels when displaying the data.
 *
 * @param mod   Pointer to scope mode module, usually returned by {@link ngpd_scope_get_module} or by linking to module directly.
 * @param card 	The number of the card in the xspress3 system,
 *        		0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        		where this value has been passed to {@link ngpd_config()} at xspress3 system configuration.
 * @param stream 0..ngpd_scope_mod_get_nstreams()-1
 * @return Integer channel number or -1 on error
 */
int ngpd_scope_chan(NGPDScopeModule *mod, int card, int stream) 
{
	int nstreams = ngpd_scope_mod_get_nstreams(mod);
	if (card < 0 || card >= mod->head.num_cards) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_chan: card=%d out of range 0 .. %d", card,
				mod->head.num_cards);
		return NGPD_ERROR;
	}
	if ((mod->head.layout >= NGPD_SCOPE_LAYOUT_1 && mod->head.layout <= NGPD_SCOPE_LAYOUT_6) || (mod->head.layout >= NGZMP_SCOPE_LAYOUT_4 && mod->head.layout <= NGZMP_SCOPE_LAYOUT_10))
	{
		if (stream < 0 || stream >= nstreams) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_chan: stream=%d out of range 0 .. %d", stream, nstreams-1);
			return NGPD_ERROR;
		}
		return NGZMP_SCOPE_CHAN_SEL_GET(stream, mod->head.card[card].chan_sel[0], mod->head.card[card].chan_sel[1]);
	}
	return NGPD_ERROR;

}
/** 
* Set the number of scope streams to use as the layout of the module.
* This only changes the number of streams and num_t in the data module. 
* The new number of streams is written to the firmware using {@link ngpd_scope_settings_to_mod()}.
* @param mod	Pointer to the scope mode data module.
* @param num_streams	New number of streams, which must be consistent with what the firmware can support.
* @return 0 => success, -1 => error
*/
int ngpd_scope_set_streams(NGPDScopeModule *mod, int num_streams) 
{
	if (mod->head.layout >= NGPD_SCOPE_LAYOUT_1 && mod->head.layout <= NGPD_SCOPE_LAYOUT_6)
	{
		switch (num_streams)
		{
		case 1:
			mod->head.layout = NGPD_SCOPE_LAYOUT_1;
			break;
		case 2:
			mod->head.layout = NGPD_SCOPE_LAYOUT_2;
			break;
		case 4:
			mod->head.layout = NGPD_SCOPE_LAYOUT_4;
			break;
		case 6:
			mod->head.layout = NGPD_SCOPE_LAYOUT_6;
			break;

		default:
			printf("Invalid number of streams %d for ADQ14 based scope mode module\n", num_streams);
			return -1;
		}
		ngpd_scope_calc_num_t(mod); 
		return 0;
	}
	else if (mod->head.layout >= NGZMP_SCOPE_LAYOUT_4 && mod->head.layout <= NGZMP_SCOPE_LAYOUT_10)
	{
		size_t num_valid_bytes = mod->head.nbytes_per_card/(sizeof(u_int16_t)*NGPD_SCOPE_CHUNK_NUM_T*8);
		int i;
		switch (num_streams)
		{
		case 4:
			mod->head.layout = NGZMP_SCOPE_LAYOUT_4;
			break;

		case 8:
			mod->head.layout = NGZMP_SCOPE_LAYOUT_8;
			break;

		case 10:
			mod->head.layout = NGZMP_SCOPE_LAYOUT_10;
			break;

		default:
			printf("Invalid number of streams %d for ZynqMP based scope mode module\n", num_streams);
			return -1;
		}
		ngpd_scope_calc_num_t(mod); 
		for (i=0; i<mod->head.num_cards; i++)
			mod->head.card[i].valid_offset = mod->head.num_cards*mod->head.nbytes_per_card+num_valid_bytes*i;
		return 0;		
	}
//	printf("ngpd_scope_set_streams: num_streams=%d => num_t=%d\n", num_streams, mod->head.num_t);
	printf("Unknown scope mode module layout %d\n", mod->head.layout); 
	return -1;
}
/**
* Recalculate the num_t points available after changing the layout of the scope mode module 
* @param mod	Pointer to the scope mode data module.
* @return 0 => success, currently cannot fail, -1 => error
*/
int ngpd_scope_calc_num_t(NGPDScopeModule *mod) 
{
	u_int32_t num_scope_words; // Number of 128 bit words for DMA to transfer 
	switch (mod->head.layout)
	{
	case NGPD_SCOPE_LAYOUT_1:
		mod->head.num_t = mod->head.nbytes_per_card/(1*sizeof(u_int16_t));
		break;
	case NGPD_SCOPE_LAYOUT_2:
		mod->head.num_t = mod->head.nbytes_per_card/(2*sizeof(u_int16_t));
		break;
	case NGPD_SCOPE_LAYOUT_4:
		mod->head.num_t = mod->head.nbytes_per_card/(4*sizeof(u_int16_t));
		break;
	case NGPD_SCOPE_LAYOUT_6:
		mod->head.num_t = mod->head.nbytes_per_card/(6*sizeof(u_int16_t));
		break;

	case NGZMP_SCOPE_LAYOUT_4:
		num_scope_words = mod->head.nbytes_per_card/16;
		num_scope_words	&= 0xFFFFFFFC;
		mod->head.num_t = num_scope_words*2;
		break;
	case NGZMP_SCOPE_LAYOUT_8:
		num_scope_words = mod->head.nbytes_per_card/32;
		num_scope_words	&= 0xFFFFFFFC;
		mod->head.num_t = num_scope_words*2;
		break;
	case NGZMP_SCOPE_LAYOUT_10:
		num_scope_words = mod->head.nbytes_per_card/40;
		num_scope_words	&= 0xFFFFFFFC;
		mod->head.num_t = num_scope_words*2;
		break;
	default:
		return -1;
	}
	printf("Caclulated scope mode num-t=%d\n", mod->head.num_t);
	return 0;
}

int ngpd_scope_mark_all(int path, int card, int value)
{
	NGPDScopeModule *mod;
	int first_card, last_card;
	char *cp;
	size_t num_valid_bytes = 0;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mark_invalid: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		return 0;
	}

	if (card <0 )
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mark_invalid:card %d is outside range 0..%d", card, NGPDPath[path].num_cards-1);
		return NGPD_ERROR;
	}
	else
	{
		first_card = card;
		last_card = card;
	}
	if (value)
		value=255;
	mod = NGPDPath[path].scope_mod;
	num_valid_bytes = mod->head.nbytes_per_card/(sizeof(u_int16_t)*NGPD_SCOPE_CHUNK_NUM_T*8);
	for (card=first_card; card<=last_card; card++)
	{
		cp = (char *) mod->_data;
		cp += mod->head.nbytes_per_card*mod->head.num_cards;
		cp += num_valid_bytes*card;
		memset(cp, value, num_valid_bytes);
	}
	return 0;
}
typedef struct
{
	NGPDScopeModule *mod;
	int path;
	int card;
	int first_chunk, last_chunk;
} NGZMPScopeReader;

void *ngpd_scope_update_mod_one_card(NGZMPScopeReader *reader)
{
	char *cp;
	size_t num_valid_bytes = 0;
	int chunk, byte, bit, i;
	int read_start=-1, read_stop=-1;
	int rc;
	int t, dt, num_t;

	num_valid_bytes = reader->mod->head.nbytes_per_card/(sizeof(u_int16_t)*NGPD_SCOPE_CHUNK_NUM_T*8);
	
	cp = (char *) reader->mod->_data;
	cp += reader->mod->head.nbytes_per_card*reader->mod->head.num_cards;
	cp += num_valid_bytes*reader->card;
	num_t = reader->mod->head.num_t;

	for (chunk=reader->first_chunk; chunk<=reader->last_chunk; chunk++)
	{
		byte = chunk / 8;
		bit = chunk % 8;
		if (cp[byte] & 1 << bit)
		{
			if (read_start >= 0)
			{
				read_stop = chunk-1;
				if (read_stop >= read_start)
				{
					t = read_start*NGPD_SCOPE_CHUNK_NUM_T;
					dt = (read_stop+1-read_start)*NGPD_SCOPE_CHUNK_NUM_T;
					if (t+dt > num_t)
						dt = num_t - t; // Limit data read if chunk is bigger than the remaining data
					rc = ngzmp_scope_read(reader->path, reader->card, t, dt);
					if (rc <0)
						return NULL;
					for (i=read_start; i<=read_stop; i++)
					{
						byte = i / 8;
						bit = i % 8;
						cp[byte] |= 1 << bit;
					}
				}
				read_start = -1;
			}
		}
		else
		{
			if (read_start < 0)
				read_start = chunk;
		}
	}
	if (read_start >= 0)
	{
		read_stop = reader->last_chunk;
		if (read_stop >= read_start)
		{
			t = read_start*NGPD_SCOPE_CHUNK_NUM_T;
			dt = (read_stop+1-read_start)*NGPD_SCOPE_CHUNK_NUM_T;
			if (t+dt > num_t)
				dt = num_t - t; // Limit data read if chunk is bigger than the remaining data
			rc = ngzmp_scope_read(reader->path, reader->card, t, dt);
			if (rc <0)
				return NULL;
			for (i=read_start; i<=read_stop; i++)
			{
				byte = i / 8;
				bit = i % 8;
				cp[byte] |= 1 << bit;
			}
		}
		read_start = -1;
	}

	return reader;
}


int ngpd_scope_update_mod(int path, int card, int t, int dt)
{
	NGPDScopeModule *mod;
	int first_card, last_card;
	int first_chunk, last_chunk;
	NGZMPScopeReader reader[NGZMP_MAX_CARDS];

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mark_invalid: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		return 0;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mark_invalid:card %d is outside range 0..%d", card, NGPDPath[path].num_cards-1);
		return NGPD_ERROR;
	}
	else
	{
		first_card = card;
		last_card = card;
	}

	mod = NGPDPath[path].scope_mod;
	if (dt < 0)
	{
		/* Do all */
		t  =0;
		dt = mod->head.num_t;
	}
	first_chunk = t/NGPD_SCOPE_CHUNK_NUM_T;
	last_chunk = (t+dt-1)/NGPD_SCOPE_CHUNK_NUM_T;

	for (card=first_card; card<=last_card; card++)
	{
		reader[card].mod = mod; 
		reader[card].path = path; 
		reader[card].card = card;
		reader[card].first_chunk = first_chunk;
		reader[card].last_chunk  = last_chunk;
		// Provision here to multi thread, hence use of reader struct for args
		if (ngpd_scope_update_mod_one_card(reader+card) == NULL)
			return -1;
					
	}
	return 0;
}

int ngzmp_scope_read(int path, int card, int t, int dt)
{
	NGPDScopeModule *mod;
	int nstreams;
	int num_dma;
	u_int32_t num_scope_words; // Number of 128 bit words for DMA to transfer 
	int dma;
	size_t offset, num;
	int rc;
	u_int16_t *data;
	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_mark_invalid: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_scope_read: Called on ADQ14 system");
		return NGPD_ERROR;
	}
	mod = NGPDPath[path].scope_mod;
	if (mod == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_scope_read: Scope mode module not defined for this system");
		return NGPD_ERROR;
	}
	switch (mod->head.layout)
	{
	case NGZMP_SCOPE_LAYOUT_4:
		num_scope_words = mod->head.nbytes_per_card/16;
		num_scope_words	&= 0xFFFFFFFC;
		mod->head.num_t = num_scope_words*2;
		num_dma = 1;
		break;
	case NGZMP_SCOPE_LAYOUT_8:
		num_scope_words = mod->head.nbytes_per_card/32;
		num_scope_words	&= 0xFFFFFFFC;
		mod->head.num_t = num_scope_words*2;
		num_dma = 2;
		break;
	case NGZMP_SCOPE_LAYOUT_10:
		num_scope_words = mod->head.nbytes_per_card/40;
		num_scope_words	&= 0xFFFFFFFC;
		mod->head.num_t = num_scope_words*2;
		num_dma = 3;
		break;
	default:
		return -1;
	}

	for (dma=0; dma<num_dma; dma++)
	{
		/* First calculate byte offset amd byte size of transfer */
		data = ngpd_scope_mod_get_ptr(mod, card, dma*4);
		if (dma == 2)
		{
			offset = num_scope_words*16*dma+sizeof(u_int16_t)*2*t;
			num = sizeof(u_int16_t)*2*dt;
			data += 2*t;
		}
		else
		{
			offset = num_scope_words*16*dma+sizeof(u_int16_t)*4*t;
			num = sizeof(u_int16_t)*4*dt;
			data += 4*t;
		}
		offset /= sizeof(u_int32_t);
		num /= sizeof(u_int32_t);
//		printf("Reading scope data: t=%d, dt=%d, dma=%d, offset=0x%08X, num=%08X\n", t, dt, dma, offset, num);
		rc =  ngzmp_read_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_SCOPE0+dma, (int)offset, (int)num, (u_int32_t *)data);
		if (rc < 0)
			return rc;
	}
	return 0;
}
	 
int ngzmp_read_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *data)
{
	int this_path;
	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_dma_buff: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_dma_buff: Called on ADQ14 system");
		return NGPD_ERROR;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		return 0;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_dma_buff: invalid card %d", card);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path <0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_dma_buff: Card=%d resolved illegal sub_path=%d", card, this_path);
			return -1;
		}
	}
	else
	{
		this_path = path;
	}
	return zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_DMA_BUFF+stream, ZYNQMP_WIDTH_LONG, offset, size, 1, (u_int8_t *)data);
}

int ngzmp_write_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *data)
{
	int this_path;
	int rc;
	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_write_dma_buff: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_write_dma_buff: Called on ADQ14 system");
		return NGPD_ERROR;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_write_dma_buff: invalid card %d", card);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path <0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_write_dma_buff: Card=%d resolved illegal sub_path=%d", card, this_path);
			return -1;
		}
	}
	else
	{
		this_path = path;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		return 0;
	}
	rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_DMA_BUFF+stream, ZYNQMP_WIDTH_LONG, offset, size, 1, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_write_dma_buff: Cannot write data: %s : %d", zynqmp_get_error_message(), rc);
	}
	return rc;
}

int ngpd_scope_read_to_buff(int path, int card, int stream, int t, int dt, uint16_t *ptr)
{
	uint16_t *u16p;
	int i, ana_inc;
	int rc;
	int this_path;
	NGPDScopeModule *mod;
	
	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_read_to_buff: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		return 0;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_read_to_buff: invalid card %d", card);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path <0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_read_to_buff: Card=%d resolved illegal sub_path=%d", card, this_path);
			return -1;
		}
	}
	else
	{
		this_path = path;
	}
	mod = NGPDPath[path].scope_mod;
	if (mod == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_read_to_buff: scope_mod is NULL");
		return -1;
	}
	if (t <0 || t >= mod->head.num_t || dt < 0 || dt+t > mod->head.num_t)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_read_to_buff: t=%d and dt=%d does not fir in module num_t=%d", t, dt, mod->head.num_t);
		return -1;
	}		

	rc = ngpd_scope_update_mod(path, card, t, dt);
	if (rc < 0)
		return rc;

	u16p =  ngpd_scope_mod_get_ptr(mod, card, stream);
	ana_inc = ngpd_scope_mod_get_inc(mod, stream);
	if (u16p == NULL || ana_inc < 0)
		return -1;
	
	u16p += ana_inc * t;
	for (i=0; i< dt; i++)
	{
		*ptr++ = *u16p;
		u16p += ana_inc;
	}
	return 0;
}

/** @} */
