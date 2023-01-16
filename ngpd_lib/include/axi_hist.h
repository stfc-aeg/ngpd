/********************************************************************************************************************************************************
* 
* axi_hist.h:	Defines for control of the AXI histogram block via its AXI Lite slave interface
*
*	Author:		W. Helsby
*	Date:	15/2/2021
*
********************************************************************************************************************************************************/

#ifndef AXI_HIST_H
#define AXI_HIST_H
typedef struct 
{
	int NBitsDataHist;		//!<	Data width of each histogram element, 8, 16 or 32 
	int NBitsDataAXI;		//!<	Dat width if AXI data bus.  To the firware clears and reads are addressed in term of these words.
	int NumStreams;			//!< 	Number of parallel histogramming streams
	int NBitsAddrTopFixed;	//!< 	Number of fixed bits at top of address space.
	int NBitsStream;		//!<	Number of address bits (at top of used address space) used to identify the histigram stream
	int NBitsSubStream;		//!<	Number of bits of counter used to processes 2**n events per stream in an overall cycle. Addresses must not repeat within a cycle.
	int NBitsBankAndGroup;	//!<  	Total number of bank and bank group select bit at top of use address space. These bits usually come from the stream bits.
	int NBitsAddrMem;		//!<	Total Number of address bits to form the Byte address of the memory, excluding any top fixed bits.
	int	NBitsAddrIn;		//!<	Width of the input addresses used to specify the location to be histogrammed. Adress in terms of NBitsDataHist words and excluding the stream select bits
	int StreamAtBottom;		//!<	The address arrangement puits the stream adress at the bottom (above 1 AXI width) to allow use with ROW_COL_BANK address layout
	int HistWordsPerAXIWord;//!< 	Number of histogram words in each AXI word (calculated from NBitsDataAXI/NBitsDataHist)
	int HistTPGPresent;		//!<	Flags to indicate which (if any) Histogram test pattern generators are present.
	int64_t TotalMemWords;	//!< 	Total number of words or size NBitsDataHist 
	int64_t HistWordsPerStream;	//!< Number of words of size NBitsDataHist per stream
	int64_t AXIWordsPerStream;	//!< Num of wrods of size NBitsDataAXI per stream.
	int Major, Minor;		//!< AXI hist firmware version
	int Cache;				//!< Cache options configured into build
	int ReadWaitWrite;		//!< Read wait for Write and Wwrite wait for Read options configured in firmware
} AXIHistConfig;

#define AXI_HIST_IDENT_REG		0
#define AXI_HIST_CONFIG0		1
#define AXI_HIST_CONFIG1		2
#define AXI_HIST_CONFIG2		3
#define AXI_HIST_HBM_PORTS		4
#define AXI_HIST_CLEAR_BUSY		5
#define AXI_HIST_TEST_STATUS	6		//!< Test feature status

#define AXI_HIST_CLEAR_MASK		16
#define AXI_HIST_CLEAR_CONT		17
#define AXI_HIST_CLEAR_START	18
#define AXI_HIST_CLEAR_NUM		19
#define AXI_HIST_READ_MASK		20
#define AXI_HIST_READ_CONT		21
#define AXI_HIST_READ_START		22
#define AXI_HIST_READ_NUM		23
#define AXI_HIST_TESTPAT_MASK	24
#define AXI_HIST_TESTPAT_CONT	25
#define AXI_HIST_TESTPAT_LENGTH	26
#define AXI_HIST_TESTPAT_SIZE	27

#define AXI_HIST_IDENTIFIER	('H' | 'i'<<8 | 's' <<16 | 't' <<24)
#define AXI_HIST_CG0_NBITS_DATA(x)		((x)&0xF)
#define AXI_HIST_CG0_NBITS_DATA_AXI(x)	(((x)>>4)&0xF)
#define AXI_HIST_CG0_NUM_STREAMS(x)		(((x)>>8)&0xFF)
#define AXI_HIST_CG0_MAJOR(x)			(((x)>>24)&0xFF)
#define AXI_HIST_CG0_MINOR(x)			(((x)>>16)&0xFF)

#define AXI_HIST_CG1_NBITS_ADDR_TOP(x)	(((x)>>0)&0xF)
#define AXI_HIST_CG1_NBITS_STREAM(x)	(((x)>>4)&0xF)
#define AXI_HIST_CG1_GET_NBITS_BANKANDGROUP(x)	(((x)>>8)&0xF)
#define AXI_HIST_CG1_GET_NBITS_ADDR(x)			(((x)>>12)&0xFF)
#define AXI_HIST_CG1_GET_NBITS_ADDR_IN(x)		(((x)>>20)&0xFF)
#define AXI_HIST_CG1_GET_STREAM_AT_BOTTOM(x)	(((x)>>31)&1)

#define AXI_HIST_CG2_GET_NBITS_SUBSTREAM(x)		(((x)>>0)&0xF)
#define AXI_HIST_CG2_GET_TPG_PRESENT(x)			(((x)>>8)&0xFF)
#define AXI_HIST_CG2_GET_CACHE(x)				(((x)>>16)&0xF)
#define AXI_HIST_CG2_GET_READWAITWRITE(x)		(((x)>>20)&0xF)

#define AXI_HIST_CACHE_NONE					0	//!< No Cache or data sorter
#define AXI_HIST_CACHE_INC_CACHE_SINGLE		1	//!< Perform increments into same and neighbouring locations in BRAM then accumulate into DRAM. Suits small spectra
#define AXI_HIST_CACHE_INC_CACHE_MULTIPLE	2	//!< Perform increments into same and neighbouring locations in BRAM then accumulate into DRAM. Multiple table for marge spectra, not yet implemented
#define AXI_HIST_CACHE_SORTER				3	//!< Sort events to group events in same HBM page, to use with sub-streams master. Supports larger spectra
#define AXI_HIST_CACHE_SORTER_NEB			4	//!< Sort events to group events in same HBM page and coalesce neighouring hists, to use with sub-streams master. Supports small and larger spectra

#define AXI_HIST_READ_WAIT_WRITES_NONE		0	//!< No additional delays, start writes as soon as data available from read, start next read as soon as write tot the bank is done.
#define AXI_HIST_READ_WAIT_WRITES_SENT		1	//!< Start writes as soon as data available from read, delay next reads until all write have been sent. (wvalid=1, wready=1)
#define AXI_HIST_READ_WAIT_WRITES_DONE		2	//!< Start writes as soon as data available from read, delay next reads until all write have been finished. (bvalid=1, bready=1)
#define AXI_HIST_BUFFER_READ_WAIT_SENT		3	//!< Buffer read data to delay writes until all reads sent (arvalid=1, arready=1), delay next reads until all write have been finished. (bvalid=1, bready=1)
#define AXI_HIST_BUFFER_READ_WAIT_DONE		4	//!< Buffer read data to delay writes until all reads done (rvalid=1, rready=1), delay next reads until all write have been finished. (bvalid=1, bready=1)

#define AXI_HIST_CC_CLEAR_ALL	(1<<0)			//!< Clear all streams
#define AXI_HIST_CC_USE_TPG		(1<<1)			//!< Use write/clear test pattern data instead of clearing to 0.
#define AXI_HIST_RC_DISCARD_RD	(1<<31)			//!< Discard Read data when reading.
#define AXI_HIST_RC_ACROSS_STREAMS	(1<<0)		//!< When configured for stream bits near bottom of address, allow burst access to read across streams. If StreamAtBottom==0 this is ignored.

#define AXI_HIST_TC_HIST_TPG(x) (((x)&3)<<0)	//!< Select Histogram event data test pattern generator source

#define AXI_HIST_TPG_NONE		0
#define AXI_HIST_TPG_RAMP		1
#define AXI_HIST_TPG_RAND		2
#define AXI_HIST_TPG_ROLLING	3

#define AXI_HIST_TPG_ROLL_SET_SIZE(x)	(((x)&3)<<4)		//!< Size 0..3 codes Energy part of Test pattern to be 64, 256, 1024 or 4096
#define AXI_HIST_TPG_ROLL_CALC_SIZE(x)	(1<<(((x)&3)*2+6))	//!< Size 0..3 codes Energy part of Test pattern to be 64, 256, 1024 or 4096
#define AXI_HIST_TPG_ROLL_SET_OCC(x)	((x)&7)				//!< Occupancy 0..6 1/255 ... 1/3 emulated Hixtec occupancy
#define AXI_HIST_TPG_ROLL_NPIXELS		256					//!< Number of pixels per test patter nstream.


#define AXI_HIST_TPG_HAS_RAMP		(1<<0)
#define AXI_HIST_TPG_HAS_RAND		(1<<1)
#define AXI_HIST_TPG_HAS_ROLLING	(1<<2)

#define AXI_HIST_TSTATUS_READ_BUSY 	(1<<0)
#define AXI_HIST_TSTATUS_TPG_BUSY 	(1<<1)

#define AXI_HIST_MAX_STREAMS 256
#define AXI_HIST_TEST_NUM_STREAMS 8

#define AXI_HIST_TEST_VALID (1<<31)
#define AXI_HIST_TEST_DELAY (1<<30)
#define AXI_HIST_TEST_FIRST (1<<29)
#define AXI_HIST_TEST_GET_OFFSET(x) ((x)&0x1FFFFFFF)

#define AXI_HIST_TP_MAX_PEAKS	4
typedef struct
{
	int num_bins;				//!< Number of energy bins, typcially 4096 for Xspress3/4
	int num_peaks;				//!< Number of K-alpha peaks (k-beta infered at 1.1 x position 0.2 * height)
	struct 
	{
		int posn;				//!<	k-alpha peak position (bins)
		double height;
		int fwhm;				//!<	k-alpha peak FWHM (bins)
	} peak[AXI_HIST_TP_MAX_PEAKS];
	double pileup2;				//!<	number of counts in 2x pileup peak compared to k alpha (typically 0.2)
	double background;			//!<	Level of backgound compared to k alpha (typically 0.01)
	int average_events_per_tf;	//!< Average number of events in each time frame. Loops round when reaches end.
} AXIHistTPSpectra;
extern const char *axi_hist_cache_names[];
extern const char *axi_hist_read_write_names[];

#endif



