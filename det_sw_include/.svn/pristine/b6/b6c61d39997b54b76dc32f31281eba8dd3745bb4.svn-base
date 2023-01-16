/* BSL format */

/* File number */
#define BSL_FILE_HEADER 0
#define BSL_FILE_SAXS 	1
#define BSL_FILE_CAL  	2
#define BSL_FILE_WAXS 	3
#define BSL_FILE_TIME 	4

#define BSL_FILE_SAXS_DTX 11
#define BSL_FILE_SAXS_DTY 21
#define BSL_FILE_WAXS_DTX 13
#define BSL_FILE_WAXS_DTY 23

/*
The 002 file format depends on the hardware and cabling of calibration channels. It is a data set of size (<num_time_frames>, <num_cal_chan>).
With TFG2 it will grow another two rows, one of which is a measure of total live time for each frame and the other is the number of times the TFG calibration channels have accumulated data into the shadow memory. This second row was intended when an approx 1 MHz Calibration channel ADC was proposed. We now propose a 50 MHz ADC with interpolation to 100 MHz which makes a gated integration which exactly matches the X-ray accumulation, so this row is less useful.

I suggest that possibly Q axis files should be stored in another BSL file and preserved with the data so that if data from two different blocks of beamtime are to be compared, they can have their own Q axis.

I have also started to consider storing the dead time corrections with each data set. With the X/Y nature of RAPID2, if a data set (x, y, t) is corrected, two smaller files one (x, t) the other (y, t) are required. This would allow the dead time correction to be reversed if it were discovered that it had failed. It also allows the maximum X/Y read time corrections to be viewed to make a judgement as to whether the global (or local) count rate should be reduced.

Currently I have coded that for the 001 file (SAXS) the corrections are:
*/

/*
011 Correction wrt. x.
021 Correction wrt. y (if 2D)
For 003 file (WAXS)
013 Correction wrt. x.
023 Correction wrt. y (if 2D)
I have considered that flat field corrections could be added in 031 and 033, but this would involve many copies of the same data being stored.
*/

/*
BSL Header File format
The 10 indices of the header file are:
[1] nos. of pixels/channels
[2] nos of raster or time frame if 1D
[3] nos of 2D time frames else 1
[4] endian
[5] data type (see below in your message)
[6] - [9] unused
[10] 1 if another file specified else 0
*/

#define BSL_ENDIAN_MOTORLA 	0
#define BSL_ENDIAN_INTEL 	1
#define BSL_BIG_ENDIAN     0
#define BSL_LITTLE_ENDIAN  1
 
#define BSL_DATA_FLOAT     0
#define BSL_DATA_CHAR      1
#define BSL_DATA_UCHAR     2
#define BSL_DATA_SHORT16   3
#define BSL_DATA_USHORT16  4
#define BSL_DATA_LONG32    5
#define BSL_DATA_ULONG32   6

/*
Proposals
Index 6 : detector type and version 
*/
/* bits 31 to 28 summarise detector type and can be used for display modes */
#define BSL_DET_UNKNOWN      		0
#define BSL_DET_1D_PSD      		0x10000000
#define BSL_DET_2D_PSD      		0x20000000
#define BSL_DET_CAL_CHAN      		0x30000000
#define BSL_DET_DISPLAY      		0xF0000000
/* Bits 27 to 20 identify the organisation allocating the detector */
#define BSL_DET_ORG_STFC			0x00100000
#define BSL_DET_UNKNOWN      		0
/* Bits 19..12 allow 256 detector families */
/* For each familiy bits 11..8 are a sub version of the detector */
/* For each detecotro type bits 7..0 are a serial number, usually ignored for down strem processing */
#define BSL_DET_MASK_SERIAL			0x000000FF
#define BSL_DET_DELAYLINE_OTHER		(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x00000)
#define BSL_DET_DELAYLINE_QUAD		(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x00100)
#define BSL_DET_DELAYLINE_AREA		(BSL_DET_2D_PSD   | BSL_DET_ORG_STFC | 0x00300)

#define BSL_DET_RAPID1_AREA128X128	(BSL_DET_2D_PSD   | BSL_DET_ORG_STFC | 0x01300)
#define BSL_DET_RAPID2_QUADRANT128	(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x02100)
#define BSL_DET_RAPID2_CURVED128	(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x02200)
#define BSL_DET_RAPID2_AREA128X128	(BSL_DET_2D_PSD   | BSL_DET_ORG_STFC | 0x02300)

#define BSL_DET_EC738_24BIT			(BSL_DET_CAL_CHAN | BSL_DET_ORG_STFC | 0x08100)
#define BSL_DET_EC738_32BIT			(BSL_DET_CAL_CHAN | BSL_DET_ORG_STFC | 0x08200)
#define BSL_DET_TFG2CC_SCALER64		(BSL_DET_CAL_CHAN | BSL_DET_ORG_STFC | 0x09100)

#define BSL_DET_HOTOTHER			(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x10000)
#define BSL_DET_HOTSAXS512			(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x10100)
#define BSL_DET_HOTWAXS512			(BSL_DET_1D_PSD   | BSL_DET_ORG_STFC | 0x10200)

/*Index 7: Processing flags and display flags */
/* Top 4 bits code how data should be understood for display */
#define BSL_DISPLAY_XT		0x10000000	/* First dim X or only position, second Time */
#define BSL_DISPLAY_YT		0x20000000	/* First dim Y from intergarl of 2D image, second Time */
#define BSL_DISPLAY_XYT		0x30000000	/* 3D X/Y/T data set */
#define BSL_DISPLAY_TX		0x40000000	/* Cal chan data were first dimension is time fame */

/* Bottom 8 bits encode what image processing steps have been done */

#define BSL_PROC_DTC		0x001		/* Image has had dead time correction applied */
#define BSL_PROC_REMAP		0x002		/* Image has had Spatial remapping			  */
#define BSL_PROC_FF_DIV		0x004		/* Image has had dead time correction applied */
#define BSL_PROC_BG_SUB		0x008		/* Image has had dead time correction applied */

/* Bits 15..8 define what 2D to 1D data reduction has been done */

#define BSL_PROC_HOZ_INT		0x001		/* Image has been integrated with respect to Y to form Horizontal integeral */
#define BSL_PROC_VERT_INT		0x002		/* Image has been integrated with respect to X to form Vertical integeral */
#define BSL_PROC_SECT_INT		0x003		/* Image has had dead time correction applied */

/* Index 8 and 9 unused, but optional info on processing */

/*

BSL Time file
This is the 004 file and contains information about dynamic experiments. It can be created by da.server by reading the tfg_times shared data module
Version 0 time files are 4 rows
<num_tf> 4 1 <intel> 0 0 0 0 0 0 

Version 1 time files
It appear to be a 1D data set of floats with index line

<num_tf> 8 1 <intel> 0 1 0 0 0 0 
A00004.111

The rows are:
Row 0: Un-scaled length of dead frames (may be 0 with TFG2)
Row 1: Un-scaled length of live frames (in seconds)
Row 2: Length of dead frames Scaled by <num_cycles> and <num_starts>
Row 3: Length of live frames Scaled by <num_cycles> and <num_starts>
Row 4: Start Times of dead frames
Row 5: Start Times of live frames (To make Time axis for dynamic exp.)
Row 6: User port values (0..255) for dead frames
Row 7: User port values (0..255) for live frames

The scaling by num starts requires da.server to be told how many starts have been sent since the last clear. With the original EC740 TFG this can be difficult to know if external starts are used without auto disarm mode enabled on the TFG. With TFG2 the situation is slightly better as a hardware start counter is provided and an explicit measure of live time is provided in one of the Calibration channels.
*/

/* Time file version 2 adds pause infor to the port data
 Note we are storing 32 bit int as floats, so have only 24 bits of precession */

#define BSL_TIME_GET_PAUSE(x) (((x)>>16)&1)
#define BSL_TIME_GET_PSARM(x) (((x)>>17)&15)
#define BSL_TIME_GET_PSPOL(x) (((x)>>21)&1)
 
