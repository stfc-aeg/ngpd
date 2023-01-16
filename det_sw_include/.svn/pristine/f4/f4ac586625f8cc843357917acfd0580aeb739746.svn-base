/***********************************************************************
 *                             libtac.h                                 *
 ***********************************************************************
 Purpose : Header file for C binding library only
 Author  : W.Helsby
 Returns : 
 Updates :
 18/01/93 WIH Initial implementation
*/

#define ERROR -1

/*
 * CSR bits are available to the user to confirm that the system is in
 * the correct mode.
 */

/* Now the Data Control Register Bits */

#define DCR_XSHIFT(xshift)	(xshift)
#define DCR_YSHIFT(yshift)	((yshift)<<3)
#define DCR_OPMODE(op)		((op)<<6)
#define DCR_NUMZOOM(num)	((num)<<8)
#define DCR_NUMENG(num)		((num)<<10)

/* Shift values for 1D and burst modes */
#define XSHIFT_BURST 0
#define YSHIFT_1D	 6
#define YSHIFT_BURST 7

/* Some details of the lookup RAM */

#define TAC_IDSHIFT 13 
#define TAC_LOOKSIZE 8192

int tac_WrCSR(int path, short value);
int tac_WrDCR(int path, short value);
int tac_WrCycles(int path, short value);
int tac_WrXTest(int path, short value);
int tac_WrYTest(int path, short value);
int tac_WrTime(int path, short value);
int tac_RdCSR(int path, short *ptr);
int tac_RdDCR(int path, short *ptr);
int tac_RdCycles(int path, short *ptr);
int tac_RdXTest(int path, short *ptr);
int tac_RdYTest(int path, short *ptr);
void tac_StartBurstIRQ(int path, int num_cycles, short procid, short signum);
void tac_StartBurst(int path, int num_cycles);
int tac_RdOutBst(int path,int  num, unsigned long *data);
int tac_WrMemBst(int path, int start, int num, unsigned long *data);
int tac_EnbIRQ(int path, short procid, short signum);
int tac_DisIRQ(int path);
int tac_WrXMem(int path, int start, int num, short *data);
int tac_WrYMem(int path, int start, int num, short *data);
int tac_RdXMem(int path, int start, int num, short *data);
int tac_RdYMem(int path, int start, int num, short *data);
int tac_XSync(int path, int num, int waits, short *data);
int tac_YSync(int path, int num, int waits, short *data);
int tac_XYSync(int path, int num, int waits, long *data);
int tac_TSync(int path, int num, int waits, short *data);
int tac_XSyncRead(int path, int num, int rep, short *data, long *result);
int tac_YSyncRead(int path, int num, int rep, short *data, long *result);
int tac_XYSyncRead(int path, int num, int rep, long *data, long *result);
int tac_TSyncRead(int path, int num, int rep, short *data, long *result);

#define MEMNUMWORDS 32768
/* MAX Number of time frames from tfg  */
#define MAXTIME 1024
#define MAXWIN 4
#define MAXPASS 4

typedef struct  {
	short inp_low,inp_high;
} WIN_DESC1D;


typedef struct {
	short xl,xh;
	short yl,yh;
} WIN_DESC2D;

/* The following is a list of currently supported zoom styles for 2 D operation */

#define WIN_DIFFXY  1
#define WIN_SAMEX  2
#define WIN_SAMEY  3
#define WIN_SAMEXY 4


typedef struct _map_desc {
	short inp_low;    /*  Input low boundary   */
	short inp_high;   /*  Input high boundary  */
	short out_low;    /*  Output low boundary  */
	short out_high;   /*  Output low boundary  */
	short zoom;       /*  integer zoom value setup by validate_map */
	short id;         /*  Code to identify which Window of several          */
	short num_bits;   /* Number of bits of output to create                 */
} MAP_DESC;

typedef struct
{
	short num_win;
	WIN_DESC1D win[MAXWIN];
	MAP_DESC map[MAXWIN];
} PASS_1D;

typedef struct
{
	short num_pass;
	PASS_1D pass[MAXPASS];
	short num_bits;	/* number of bits in output composite time frame */
	short num_eng;
	short use_y;
	short use_tfg;
	short test_mode;
} TAC_DEF1D;

typedef struct
{
	short num_win;
	short overlap_type;
	double fraction;		/* fraction 0.0 - 1.0 of total image to use */
	WIN_DESC2D win[MAXWIN];
	MAP_DESC map[2*MAXWIN];
} PASS_2D;

typedef struct
{
	short num_pass;
	PASS_2D pass[MAXPASS];
	short nbits_x, nbits_y;	/* number of bits in output composite time frame */
	short num_eng;
	short use_tfg;
	short test_mode;
} TAC_DEF2D;

/* Tranfser function description for linearised TAC setups */
#define TF_STATE_UNDEFINED 0
#define TF_STATE_FLOOD     1
#define TF_STATE_MASK      2
#define TF_STATE_BOTH      3

#define MASK_MAX_ROWS   50
#define MASK_MAX_PEAKS	200
#define MAX_WIRES		400
typedef struct
{
	int state;
	int tac_hole_left, tac_hole_right;
	int perp_hole_left, perp_hole_right;
	float perp_threshold;
	struct _limits {int low, high; } integrate_limits[MASK_MAX_ROWS];
	double left, right;
	double peak[MASK_MAX_PEAKS];
	double peak_total[MASK_MAX_PEAKS];  /* Total in this peak */
	double total[MASK_MAX_PEAKS];  /* Total from flood between this peak and next */
	double h[MASK_MAX_PEAKS];	/* Estimate of intensity at (before) this peak */
	double wire_posn[MAX_WIRES];
	double wire_amp[MAX_WIRES];
	double wire_rms;
	int num_wires;
	float average;
	int num_peaks;
	double peak_sep;
	double x0;				/* used as c in check tf */
	double ls_m;	/* Least squares fit 'm' */
	double ls_c;	/* Least squares fit 'c' */
	int use_wire_posns;	/* use wires instead of peaks */
} TransferFunction;


#define TAC_REMAP_X 1
#define TAC_REMAP_Y 2
#define TAC_REMAP_BOTH 3

/* 

     SETUP     Mid clock is generated evertime a test clock is given.
     RUN       Normal operation from
     BURST     Test mode reads bursts of data from lookup rams
     SINGLE    Test mode where data is written into the input.
     READ_ADC  Enable ADC to be read directly.
     READ_TEST Test mode for Enable ADC to be read directly.
*/

/* Control register bits */

#define MODE_SETUP     0
#define MODE_RUN       1
#define MODE_BURST     2
#define MODE_SINGLE    3
#define MODE_REMAP_TEST 4
#define MODE_REMAP	   5
#define MODE_READ_ADC  6
#define MODE_READ_TEST 7

#define CSR_2D			0x08
#define CSR_YT			0x10
#define CSR_INTT		0x20
#define CSR_IRQENB		0x40
#define CSR_RUNNING		0x100 
#define CSR_NEWDATA		0x200 

int tac_setup_linearised2d_zoom(int path, int npix_x, int npix_y, int enb_tfg,
	int mode, int x_low, int x_high, int y_low, int y_high);
int tac_setup_linearised2d(int path, int npix_x, int npix_y, int enb_tfg,
	int mode);
int tac_default1d(int path, int num_pix, int use_tfg);
int tac_default2d(int path, int num_x, int num_y, int use_tfg);
int tac_setup1d(int path, TAC_DEF1D *def);
int tac_setup2d(int path, TAC_DEF2D *def);


