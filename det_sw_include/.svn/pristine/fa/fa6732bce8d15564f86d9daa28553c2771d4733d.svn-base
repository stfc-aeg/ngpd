/** @file  	 ml605fg_regs.h
	@brief   Header file describing hardware for ML605 FPGA base frame grabber
	@author  W. Helsby  

 */
#ifndef _ML605FG_REGS_H 
#define  _ML605FG_REGS_H 

#define FG_MAX_NUM_FRAMES 32

#define FG_REG_SIZE 0x100000
#define FG_BRAM_SIZE 0x8000
//! [FG_REG_SPACE_BASE_OFFSETS]
#define FG_FMC200_BASE_OFFSET 		0
#define FG_CONT_REG_BASE_OFFSET     0x20000
#define FG_VID_IN_BASE_OFFSET 		0x30000
#define FG_PREV1_BASE_OFFSET 		0x30000
#define FG_ROI_BASE_OFFSET 			0x40000
#define FG_PREV2_BASE_OFFSET 		0x50000
#define FG_MASK_BASE_OFFSET 		0x50000
#define FG_PREV3_BASE_OFFSET 		0x60000
#define FG_MONITOR_BASE_OFFSET 		0x60000
#define FG_CURRENT_BASE_OFFSET 		0x70000
#define FG_BBOX_BASE_OFFSET 		0x70000
#define FG_PCIE_BASE_OFFSET 		0xF0000
//! [FG_REG_SPACE_BASE_OFFSETS]

//! [FG_CONT_REGS_OFFSETS]
#define FG_CONT_REG_REVISION     	0x00
#define FG_CONT_REG_REG_PRESENT  	0x04
#define FG_CONT_REG_RUN_MODE     	0x08
#define FG_CONT_REG_TEST_PAT     	0x0C
#define FG_CONT_REG_SPRITE_SPEED 	0x10
#define FG_CONT_REG_SPRITE_SIZE  	0x14
#define FG_CONT_REG_FMC_CLOCK_DELAY  0x18
#define FG_CONT_REG_THRES_CONT1		0x1C
#define FG_CONT_REG_THRES_CONT2		0x20
#define FG_CONT_REG_THRES_CONT3		0x24
#define FG_CONT_REG_NOISE_REJECT	0x28		// Reg 10
#define FG_CONT_REG_EXPOSE_EVEN		0x2C		// Reg 11
#define FG_CONT_REG_EXPOSE_ODD		0x30		// Reg 12
//! [FG_CONT_REGS_OFFSETS]


#define FG_MB_DRAM_BASE 0x80000000
#define FG_MB_PCIE_BASE 0xE0000000
#define FG_MB_PCIE_MASK 0x3FFFFFFF
#define FG_ROI_NUM_FRM_STORES 4

#define AXIBAR2PCIEBAR_0H	0x208
#define AXIBAR2PCIEBAR_0L	0x20C

//! [FG_CONT_REGISTER]
#define FG_RUN_MODE_RUN			1
#define FG_RUN_MODE_ENB_TEST	2
#define FG_RUN_MODE_TEST_COMB(x)	(((x)&3)<<1)

#define FG_RUN_MODE_DISABLE_DVAL	(1<<8)
#define FG_RUN_MODE_BIT_SLIP		(1<<9)
#define FG_RUN_MODE_DB_FVAL			(1<<10)

#define FG_RUN_MODE_PROC_MODE(x)	(((x)&3)<<11)
#define FG_RUN_MODE_NUM_PREV(x)		(((x)&3)<<13)
#define FG_RUN_MODE_ENB_MASK		(1<<15)
#define FG_RUN_MODE_MONITOR(x)		(((x)&7)<<16)
#define FG_RUN_MODE_ENB_BBOX		(1<<19)

#define FG_RUN_MODE_ALLOW_LAG		(1<<20)
#define FG_RUN_MODE_MAX_LAG(x)		(((x)&0x1F)<<21)
#define FG_RUN_MODE_SKIP_FIRST		(1<<26)
#define FG_RUN_MODE_SKIP_FRAMES		(1<<27)
#define FG_RUN_MODE_SKIP_NUM(x)		(((x)&0xF)<<28)

#define FG_COMB_TEST_TP_OFF 		0
#define FG_COMB_TEST_TP_ONLY 		1
#define FG_COMB_TEST_TP_ADD 		2
#define FG_COMB_TEST_TP_REPLACE 	3
//! [FG_CONT_REGISTER]

//! [FG_TEST_PAT_REGISTER]
#define FG_TEST_PAT_TYPE(x)  ((x)&0xFF)
#define FG_TEST_PAT_PERIOD(x) (((x)&0xFF)<<8)

#define FG_TEST_PAT_TYPE_INC8BIT	0		//!< Incrementing 8 bit test pattern
#define FG_TEST_PAT_TYPE_INC16BIT	1		//!< Incrementing 16 bit test pattern
#define FG_TEST_PAT_TYPE_CHECKER	2		//!< Dual Checker board pattern on a 64x64 scale and 1x1 scale.
#define FG_TEST_PAT_TYPE_SPRITE_A	8		//!< Test pattern with moving "sprite" object
//! [FG_TEST_PAT_REGISTER]

#define FG_SPRITE_CONT_DX(dx)			(((dx)&0xFFFF)<<0)
#define FG_SPRITE_CONT_DY(dy)			(((dy)&0xFFFF)<<16)
#define FG_SPRITE_CONT_WIDTH(w)			(((w)&0xFFFF)<<0)
#define FG_SPRITE_CONT_HEIGHT(h)		(((h)&0xFFFF)<<16)


//! [FG_FMC200_OFFSETS]
#define FG_FMC200_MODE_CONFIG 			0x00
#define FG_FMC200_BAUD_RATE_DIV_UART_A 	0x01
#define FG_FMC200_BAUD_RATE_DIV_UART_B 	0x02
#define FG_FMC200_FMC_GPIO_DIR 			0x03
#define FG_FMC200_BEZEL_GPIO_DIR 		0x04
#define FG_FMC200_X_BIT_SLIP 			0x06
#define FG_FMC200_Y_BIT_SLIP 			0x07
#define FG_FMC200_Z_BIT_SLIP 			0x08
#define FG_FMC200_CAMERA_CTRL_A 		0x09
#define FG_FMC200_CAMERA_CTRL_B 		0x0A
#define FG_FMC200_RX_TX_BUFF_UART_A  	0x0B
#define FG_FMC200_RX_TX_BUFF_UART_B 	0x0C
#define FG_FMC200_UART_STATUS 			0x0D
#define FG_FMC200_POCL_CTRL_A 			0x0E
#define FG_FMC200_POCL_CTRL_B 			0x0F
#define FG_FMC200_FMC_GPIO_DATA 		0x10
#define FG_FMC200_BEZEL_DATA 			0x11
#define FG_FMC200_HEADER_DATA 			0x12
#define FG_FMC200_POCL_STATUS_A 		0x13
#define FG_FMC200_POCL_STATUS_B 		0x14
#define FG_FMC200_CAM_SETUP_A 			0x15
#define FG_FMC200_CAM_SETUP_B 			0x16
#define FG_FMC200_RECONFIG_CTRL 		0x1D
#define FG_FMC200_RECONFIG_DATA 		0x1E
#define FG_FMC200_REV_CONT 				0x1F
//! [FG_FMC200_OFFSETS]

#define FG_FMC200_NUM_REG 32

#define FG_FMC200_MC_RST_CAM_A			1
#define FG_FMC200_MC_RST_CAM_B			2
#define FG_FMC200_MC_MODE(x)			(((x)&7)<<2)
#define FG_FMC200_MC_MODE_BASE			0
#define FG_FMC200_MC_MODE_MEDIUM		1
#define FG_FMC200_MC_MODE_FULL			2
#define FG_FMC200_MC_MODE_EXTENED		3
#define FG_FMC200_MC_MODE_DUAL_BASE		4

#define FG_FMC200_MC_X_NOT_LOCKED		0x20
#define FG_FMC200_MC_Y_NOT_LOCKED		0x40
#define FG_FMC200_MC_Z_NOT_LOCKED		0x80

#define FG_FMC200_FMC_GPIO_DIR_SET_OUT	((x)&0xF)
#define FG_FMC200_BEZEL_GPIO_DIR_SET_OUT ((x)&0xF)
#define FG_FMC200_CAMERA_CTRL_A_REG 	0x10		// CAM A CC[4:1] from this reg
#define FG_FMC200_CAMERA_CTRL_A_VAL(x) 	((x)&0xF)	// CAM A CC[4:1] from this reg
#define FG_FMC200_CAMERA_CTRL_B_REG 	0x10		// CAM B CC[4:1] from this reg
#define FG_FMC200_CAMERA_CTRL_B_VAL(x) 	((x)&0xF)	// CAM B CC[4:1] from this reg

#define FG_FMC200_UART_RX_OVER_RUN_B 	(1<<7)	// indicates over run of UART B Rx buffer. Causes assertion of interrupt output. Note: Enabled only in dual base mode.
#define FG_FMC200_UART_TX_ALMOST_FULL_B (1<<6)	// indicates UART B transmit FIFO almost full. Rising edge causes assertion of interrupt output. Note: Enabled only in dual base mode.
#define FG_FMC200_UART_TX_ALMOST_EMPTY_B (1<<5)	// indicates UART B transmit FIFO almost empty. Rising edge causes assertion of interrupt output. Note: Enabled only in dual base mode.
#define FG_FMC200_UART_RX_VALID_B 		(1<<4)	//indicates UART B receive FIFO contains valid data. Causes assertion of interrupt output. Note: Enabled only in dual base mode.
#define FG_FMC200_UART_RX_OVER_RUN_A 	(1<<3)	// indicates over run of UART A Rx buffer. Causes assertion of interrupt output.
#define FG_FMC200_UART_TX_ALMOST_FULL_A (1<<2)	// indicates UART A transmit FIFO almost full. Rising edge causes assertion of interrupt output.
#define FG_FMC200_UART_TX_ALMOST_EMPTY_A (1<<1)	// indicates UART A transmit FIFO almost empty. Rising edge causes assertion of interrupt output.
#define FG_FMC200_UART_RX_VALID_A 		(1<<0)	//indicates UART A receive FIFO contains valid data. Causes assertion of interrupt output.

/* FG_FMC200_CAM_SETUP_ use for A and B */
#define FG_FMC200_CAM_SETUP_RGB			(1<<7)		// Specifies RGB Mode
#define FG_FMC200_CAM_SETUP_NUM_CHAN(x) ((((x)-1)&7)<<4)	// Specifed the number of Channel of Taps.		
#define FG_FMC200_CAM_SETUP_BIT_DEPTH8	0
#define FG_FMC200_CAM_SETUP_BIT_DEPTH10	2
#define FG_FMC200_CAM_SETUP_BIT_DEPTH12	3
#define FG_FMC200_CAM_SETUP_BIT_DEPTH14	4
#define FG_FMC200_CAM_SETUP_BIT_DEPTH16	5
//! [FG_MEMORY_AREAS]
#define FG_MEM_AREA_RAW 	0		//!< @breif Used for Input video data. This is the normal area to access the video data
#define FG_MEM_AREA_MASK 	1		//!< @breif The Mask image as determined by the change detection firmware.
#define FG_MEM_AREA_MONITOR 2		//!< @breif A Debug monitor image, various uses.
#define FG_MEM_AREA_BBOX 	3		//!< @breif A 1D image which is the list of bounding boxes found
#define FG_MEM_AREA_MAX 3
//! [FG_MEMORY_AREAS]

#define FG_NUM_VECTORS 4

#define FG_BBOX_NUM				4096	//!< @brief Maximum Number of bounding boxes per Stripe.
#define FG_BBOX_NBITS  	  		64		//!< @brief Bounding boxword size
#define FG_BBOX_NUM_STRIPES		8		//!< @brief Number of bounding box stripes.

#define FG_EXPOSE_EXPOSE(x)		((x)&0xFFFF)		//!< @brief Set exposure time in 10 us units.
#define FG_EXPOSE_GAP(x)		(((x)&0xFFFF)<<16)	//!< @brief Set gap between exposures in 10 us units.

/**
 * @brief Typedef for structure to describe a Region of Interest

 * This structure describes the Region of Interest to be read from the DRAM using Video DMAs. 
 * For memory areas storing grey scale data (that is the input image and the monitor image), x, y, width and height are specified in pixels. 
 * If the system is extended to > 8 bits/pixel, the driver should scale the offsets appropriately.
 *
 * However, if the memory area is of type FG_MEM_AREA_MASK, (currently 1 bit per pixel, possibly scaling to 2 bits per pixel) then the region of interest must be byte aligned.
 * The x, y, width and height are in bytes and the user code must discard teh unwanted extar bits as necessary.
*/
typedef struct 
{
	int x;				//!< @brief Pixel Co-ordinates of the top left corner of the Region of interest
	int y;				//!< @brief Pixel Co-ordinates of the top left corner of the Region of interest
	int width;			//!< @brief Width of RoI in pixels
	int height;			//!< @brief  Height of RoI in lines
	int frame;			//!< @brief The frame number. Data is stored in a ring buffer of FG_MAX_NUM_FRAMES frames (32)
	int mem_area;		//!< @brief Specifies which memory area.
						//!<  See @snippet ml605fg_regs.h FG_MEMORY_AREAS
	u_int8_t *ptr;		//!< @brief Pointer to buffer which caller must allocate.
} FG_ROI;

/**
 * @brief Typedef for structure to describe a change detection by comparison with previous frame.
 *
 * There are currently 3 change detection comparision blocks which work by forming the absolute difference between the current frame and a previous frame, with a specified frame delay.
 * These can then be combined by ORing or ANDing to increase or decrease the number of change detected pixels.
 * This structure is used as part of the complete change detetcion processing setup @ref FG_Processing 
*/
typedef struct
{
//	int comb_flags;		//!< @brief Flags to describe how this mask is combined with others.
	int frame_delay;	//!< @brief The delay in frames from the current frame to the subtracted frames
	int abs_thres;
	int rel_thres_scale;
} FG_PrevCompare;

/**
 * @brief Typedef for structure to describe the complete change detection processing configuration.
 *
 * This configures the compete processign chain including enabling the various data paths. 
 * The run mode can be set to record only or for 3 processing options.
 * @snippet ml605fg_regs.h FG_PROC_RUN_MODE
 * There are currently 3 change detection comparision blocks of which 1 to 3 can be used. Each one is setup using @ref FG_PrevCompare
 * The system can run continuosly or in burst mode. 
 * The system can be set to monitor a 8 bits per pixel image from the processing. Currently this can be off or the absolute difference from the first Previous frame comparison block.
*/
typedef struct 
{
	int run_mode;					//!< @brief Sets the Run mode to 
	int tp_mode;					//!< @brief Test pattern generator mode, on, off or combined with camera input.
	int tp_type;					//!< @brief Test pattern type 
									//!< see @snippet ml605fg_regs.h FG_TEST_PAT_REGISTER
	int tp_period;					//!< @brief Test patterm frame cycle period in ms
	int disable_dval;				//!< @brief Disable the dval signal from the camera.
	int debounce_fval;				//!< @brief Enable debounce of the Fval signal. Should not be necessary with reliable setup/hold times.
	int num_prev;					//!< @brief Number of previous frame comparison blocks to use.
	FG_PrevCompare prev[3];			//!< @brief Settings for each to the previous frame comparison blocks.
	int comb_mode;					//!< @brief Combinatorial Logic mode to describe how the upto 3 masks are combined with each other.
	int burst_in;					//!< @brief Number of input frames to capture in burst mode, 0 for continuous.
	int burst_out;					//!< @brief Number of monitor and mask frames to capture in burst mode, 0 for continuous.
	int monitor;					//!< @brief Mode for the monitor data stream
									//!< 0=>Off, 1=>Absolute difference from Prev1 block 2=> .. future expansion
	int save_mask;					//!< @brief Enable saving of the mask image.
	int noise_reject_mode;			//!< @brief Noise rejection filter mode
	int noise_reject_thres;			//!< @brief Noise rejection filter threshold
	int enb_bbox;					//!< @brief Enable Bounding box extraction.
	int skip_frames;				//!< @brief Process only 1 in n frames if this is > 0, if 0  disable skipping
	int skip_first;					//!< @brief If >0 skip this number of frames before the first processing
	int allow_lag;					//!< @brief Allow processing to lag input by up to this many frames, if more skip and catch up.
} FG_Processing;

//![FG_PROC_RUN_MODE]
#define FG_PROC_RM_RECORD			0		//!< Setup processing to record data only (No processing)
#define FG_PROC_RM_CUR_FROM_INPUT	1		//!< Setup processing to take the current data from the camera input. Processing must keep up with camera
#define FG_PROC_RM_CUR_FROM_MEM 	2		//!< Setup processing to take the current data from DRAM, so processing can expand into the frame to frame gap.
#define FG_PROC_RM_PLAYBACK			3		//!< Setup processing to playback data and disable recording of images to DRAM for testing.
//![FG_PROC_RUN_MODE]

#define FG_THRES_CONT_ABS(x)		((x)&0x3FF)		//!< Absolute threshold 0..255 for 8 bit camera data 0..1023 for 10 bit camera data
#define FG_THRES_CONT_SCALE(x)		(((x)&0x3FF)<<10)	//!< Relative threshold from current and prev 
#define FG_THRES_CONT_COMB_MODE(x)		(((x)&0xF)<<24)		//!< Combination mode 

#define FG_COMB_MODE_1ONLY			0
#define FG_COMB_MODE_1AND2			1
#define FG_COMB_MODE_1AND2AND3		2
#define FG_COMB_MODE_1AND2OR3		3
#define FG_COMB_MODE_1OR2			4
#define FG_COMB_MODE_1OR2OR3		5

#define FG_NOISE_REJECT_MODE(x)		((x)&0xF)		//!< @brief Set the noise reject mode.
													//!< See "snippet ml605fg_regs.h FG_NOISE_REJECT_MODES
#define FG_NOISE_REJECT_THRES(x)	(((x)&0xF)<<8)	//!< @brief Set the noise reject threshold when using FG_NOISE_REJECT_NUM_NEB.

//![FG_NOISE_REJECT_MODES]
#define FG_NOISE_REJECT_NONE		0	//!< @brief Pass mask data unmodified
#define FG_NOISE_REJECT_ANY_NEB		1	//!< @brief Lone dots are removed, anything with a neighbour is passed OK.
#define FG_NOISE_REJECT_LINE1		2	//!< @brief Line segments of width >=1 and length >=3 dot are preserved, shorter (2 dots) are removed
#define FG_NOISE_REJECT_LINE2		3	//!< @brief Line segments of width >=2 and length >= 2 are preserved
#define FG_NOISE_REJECT_LINE3		4	//!< @brief Line segments of width >=3 and length >= 3 are preserved
#define FG_NOISE_REJECT_NUM_NEB		5	//!< @brief Any dot with >= thres number of nearest neighbours is preserved.
//![FG_NOISE_REJECT_MODES]

//![FG_DATA_TYPES]
#define FRM_GRAB_DATA_UINT8		0
#define FRM_GRAB_DATA_UINT16	1
#define FRM_GRAB_DATA_UINT32	2
#define FRM_GRAB_DATA_UINT64	3

#define FRM_GRAB_DATA_1BIT		10
#define FRM_GRAB_DATA_8x1BIT	11
//![FG_DATA_TYPES]

#define FG_BBOX_MIN_X(a)		(((a)>>0)&0xFFF)
#define FG_BBOX_MIN_Y(a)		(((a)>>12)&0xFFF)
#define FG_BBOX_MAX_X(a)		(((a)>>24)&0xFFF)
#define FG_BBOX_MAX_Y(a)		(((a)>>36)&0xFFF)
#define FG_BBOX_PARENT(a)		(((a)>>48)&0x7FFF)
// #define FG_BBOX_VALID			(1L<<62)	// With 12 bit labels plus 8 stripes => 15 bits of label, so no space for valid.
#define FG_BBOX_HAS_PARENT		(1L<<63)

#endif
