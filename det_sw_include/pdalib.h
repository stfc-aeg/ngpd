/***********************************************************************
 *                              pdalib.h                               *
 ***********************************************************************
 Purpose: 
 Author:  C.Ramsdale & G.Mant
 Returns: Void
 Updates: 
 24/06/91 GRM Initial C implementation
*/

/* PDA memory system parameters */

#define PDA_MemSize	1024*512					/* Memory size in bytes */
#define PDA_MemSizeLong	PDA_MemSize/sizeof(long)	/* longword size    */
#define PDA_MemSizeShort PDA_MemSize/sizeof(short)	/* Shortword  size  */

/* Status/Command register layout */

#define Sequential	0x0			/* 16 bit overwrite - no segmentation    */
#define SingleScan	0x1			/* 16 bit overwrite - with segmentation  */
#define Looping		0x2			/* 32 bit accumulate - no segmentation   */
#define Accumulate	0x3			/* 32 bit accumulate - with segmentation */
#define Hist	0x4			/* Set histogram mode                    */
#define Pix512	0x0			/* 512  pixel selection                  */
#define Pix1024	0x8			/* 1024 pixel selection                  */
#define Pix2048	0x10		/* 2048 pixel selection                  */
#define Pix4096	0x18		/* 4096 pixel selection                  */
#define Go		0x20		/* Go/stop                               */
#define TstMode	0x40		/* Set in test mode                      */
#define LogData	0x80		/* Data logging in final pixel of frame  */

/* IRQ status register (read/write) */

#define FlyReg	0x1			/* Request access to fly register        */
#define IRQEnb	0x2			/* Overall enable interrupts             */
#define IRQFrm	0x4			/* Enable IRQ at start of frame          */
#define IRQLoop	0x8			/* Enable IRQ for signal at front panel  */
#define IRQDis	0x10		/* Disable IRQ for signal at front panel */

#define NoIRQ	0x0			/* No interrupt                   */ 
#define FrACK	0x20		/* Frame request acknowledge      */
#define FrStart	0x40		/* New frame started              */
#define FPDis	0x60		/* Front panel disable activated  */
#define FPLoop	0x80		/* Front panel loopnow activated  */
#define MemFull	0xa0		/* Memory full                    */
#define OvFlow	0xc0		/* 32 bit accumulation overflow   */
#define RESRV	0xe0		/* RESERVED                       */


#define SS_Init		1024		/* Initialise                 */
#define SS_Enable	1025		/* Enable mode status bits    */
#define SS_Disable	1026		/* Disable mode status bits   */
#define SS_WrMem32	1027		/* Write long word to memory  */
#define SS_WrMem16	1028		/* Write short word to memory */
#define SS_WrTest	1029		/* Write to the test register */
#define SS_EnbIRQ	1030		/* Enable interrupts          */
#define SS_DisIRQ	1031		/* Disable interrupts         */
#define SS_RdMem32	1032		/* Read memory in longwords   */
#define SS_RdMem16	1033		/* Read memory in shortwords  */
#define SS_Go		1034		/* Set data acquisition mode  */
#define SS_Halt		1035		/* Stop data acquisitio mode  */
#define SS_ClrMem	1036		/* Clear PDA memory           */
#define SS_Frame	1037		/* Set frame pointer		  */

#define GS_Status	1038		/* Read PDA IRQ stat register */
#define GS_PrcSig	1039		/* Read process ID to signal  */
#define GS_RdFrame	1040		/* Read current frame         */
#define GS_Mode		1041		/* Read PDA mode register     */
#define GS_Vec		1042		/* Read PDA interrupt vector  */
#define GS_Signal	1043		/* Read user signal value     */

#define SS_Rd16to32	1044		/* Read memory, extend words -> longwords */

typedef struct {
	long *buff;					/* Buffer address             */
	unsigned long addr;			/* Memory offset              */
	unsigned long npts;			/* Number of data points      */
	unsigned short testreg;		/* Value for test register    */
	unsigned short procid;		/* Process ID to signal       */
	unsigned char status;		/* IRQ status register        */
	unsigned char mode;			/* Mode status register       */
	unsigned char frame; 		/* Frame pointer              */
} PDA;

#define ERROR	-1
