/*
 * scaler.h         (C) Copyright CLRC Daresbury Laboratory
 */

#ifndef _SCALER_H
#define _SCALER_H

/*
 * EC738 parameters
 */

#define EC738_TOTSCA	32	/* Total nos. of scaler    - defined by H/W */
#define EC738_MEMSIZE	32768	/* Scaler on-board memory  - defined by H/W */
#define EC738_MAXFRAME	1024	/* Maximum time frames     - defined by H/W */
#define EC738_WORDSIZE	24 	/* Number of bits per scaler word           */

#define EC738_HLFSCA	16	/* Total nos. of scaler when mode to 16 48 bit scalers   - defined by H/W */

/* Status register layout */

#define ENBTST	0x1		/* Put scaler into test mode */
#define VETO	0x2		/* Veto inputs */
#define ENBIRQ	0x4		/* Enable interrupts */
#define INPTYP	0x8		/* Scaler input setting TTL or ECL */
#define LEMOTYP	0x10		/* Front panel lemo setting NIM or TTL */
#define FPVETO	0x20		/* Status of front panel veto */
#define MEMHF	0x40		/* Memory half full */
#define SCALHF	0x80		/* Scaler buffer half full */

#define ERROR (-1)
#define SUCCESS 0

/* mcslib routine definitions */

#if defined(__STDC__) || defined (_ANSI_EXT)
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
error_code mcs_clrmem  (path_id);
error_code mcs_disable (path_id);
error_code mcs_enable  (path_id);
error_code mcs_init    (path_id);
error_code mcs_irqdis  (path_id);
error_code mcs_irqenb  (path_id, process_id, signal_code);
error_code mcs_rdcol   (path_id, u_int32, u_int32, u_int32*);
error_code mcs_rdmem   (path_id, u_int32, u_int32, u_int32*);
error_code mcs_rdrows  (path_id, u_int32, u_int32, u_int32, u_int32, u_int32*);
error_code mcs_rdscal  (path_id, u_int32, u_int32, u_int32*);
error_code mcs_tstclk  (path_id, u_int32);
error_code mcs_wrmem   (path_id, u_int32, u_int32, u_int32*);
error_code mcs_wrrows  (path_id, u_int32, u_int32, u_int32, u_int32, u_int32*);
error_code mcs_xfer    (path_id);
int mcs_prcsig  (path_id);
int mcs_rdstat  (path_id);
#if defined(__cplusplus)
}
#endif /* __cplusplus */
#else
extern error_code mcs_clrmem (),  mcs_enable (), mcs_init (),   mcs_irqdis ();
extern error_code mcs_irqenb (),  mcs_rdcol (),  mcs_rdmem ();
extern error_code mcs_rdrows (),  mcs_rdscal (), mcs_tstclk ();
extern error_code mcs_disable (), mcs_wrmem (),  mcs_xfer ();
extern int mcs_prcsig (), mcs_rdstat ();
#endif

#endif /* _SCALER_H */
