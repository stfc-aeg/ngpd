/***********************************************************************
 *                              camlib.h                               *
 ***********************************************************************
 Purpose : Camac function definitions
 Author  : G.Mant & G.J.Milne
 Updates : 
 27/11/89 GRM Initial C implementation
 22/12/89 GJM VME modifications to incorparate routine definitions
 25/09/90 GRM Change structure BRANCH to Branch 
 17/10/90 GJM Added cfubr, csubr, cfubc, csubc
*/

#define RD1   0                          /* Camac Read                */
#define RD2   1                          /* Camac Read                */
#define RC1   2                          /* Camac Read & clear        */
#define RCM   3                          /* Camac Read                */
#define RC2   4                          /* Camac Read                */
#define TLM   8                          /* Camac Test LAM            */
#define CL1   9                          /* Camac Clear               */
#define CLM   10                         /* Camac Clear LAM           */
#define CL2   11                         /* Camac Clear               */
#define WT1   16                         /* Camac Write               */
#define WT2   17                         /* Camac Write               */
#define SS1   18                         /* Camac Selective set       */
#define SS2   19                         /* Camac Selective set       */
#define BS2   20                         /* Camac Bit Select          */
#define SC1   21                         /* Camac Selective Clear     */
#define BC2   22                         /* Camac Selective Bit Clear */
#define SC2   23                         /* Camac Selective Clear     */
#define DIS   24                         /* Camac Disable             */
#define XEQ   25                         /* Camac Initialise          */
#define ENB   26                         /* Camac Enable              */
#define TST   27                         /* Camac Test LAM            */

typedef struct
{
	unsigned char acr;
	unsigned char irr;
	unsigned char imr;
	unsigned char isr;
} AMD_DEVICE;

typedef struct
{
	AMD_DEVICE amd1;
	AMD_DEVICE amd2;
	AMD_DEVICE amd3;
	unsigned short cpids [7];			/* PIDs booking crates			*/
	unsigned short lpids [24];			/* PIDs booking LAMs			*/
	unsigned short lcrts [24];			/* Crates associated with LAMs	*/
	unsigned char  errcnt;				/* Error Counter				*/
	unsigned int   intcnt;				/* Total number of interrupts	*/
} Branch;

/* Control	 	*/
extern void cccc  (),	cccd  (),	ccci  (),	cccz  (),	cclc  ();
extern void cclm  (),	cculk ();
extern int  cclnk ();

/* Definition 	*/
extern void cdlam (),	cdreg ();

/* Action 		*/
extern void cfsa  (),	cfubc (),	cfubr (),	cssa (),	csubc ();
extern void csubr ();

/* Analysis 	*/
extern void cglam (),	cgreg ();

/* Test 		*/
extern void ctcd  (),	ctci  (),	ctgl  (),	ctlm  ();

/* Other 		*/
extern void ccboo (),	ccfl  (),	ccinf (),	ccrgl (),	ccrls ();
extern void cinit ();
extern int  ctstat();
