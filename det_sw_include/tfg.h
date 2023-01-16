/*
 * tfg.h         (C) Copyright CLRC Daresbury Laboratory
 */

#ifndef _TFG_H
#define _TFG_H
#ifdef __LINUX__
#else
#include <types.h>
#endif
/* EC740 parameters */

#define EC740_MAXFRAME	1024	/* Maximum time frames    - defined by H/W */
#define EC740_MAXCYCLE	4095	/* Maximum cycle count    - defined by H/W */
#define EC740_MINCYCLE     0
#define EC740_MEMSIZE	4096	/* Memory size            - defined by H/W */
#define EC740_MEMWORDSIZE 16    /* short word             - defined by H/W */
#define EC740_MAXFCOUNT 1023	/* maximum frame count */

/* Status write register layout */

#define FPANELENB	0x1	/* Start front panel enable      */
#define EXTINHENB	0x2	/* External inhibit enable       */
#define IRQEXTINH	0x4	/* Interrupt on external inhibit */
#define IRQPAUSE	0x8	/* Interrupt on pause            */
#define IRQENDCYCLE	0x10	/* Interrupt on end of cycle     */
#define IRQENDRUN	0x20	/* Interrupt on end of run       */

/* Status read only register layout */

#define MODULERUNNING	0x40	/* Module running */
#define MODULEPAUSED	0x80	/* Module paused  */
#define INHIBITSET	0x100	/* Latched, External Inhibit set    */
#define PAUSESET	0x200	/* Latched, Paused                  */
#define ENDCYCLESET	0x400	/* Latched, At end of cycle         */
#define ENDRUNSET	0x800	/* Latched, All TFG cycles complete */

/* Memory control bit layout */
 
#define MEMPAUSE    0x100
#define MEMEOF      0x200

/*
 * For Backwards compatability, before someone knobbed about with the case
 * for no good reason at all !
 */

/* Status write register layout */

#define FPanelEnb	0x1	/* Start front panel enable      */
#define ExtInhEnb	0x2	/* External inhibit enable       */
#define IRQExtInh	0x4	/* Interrupt on external inhibit */
#define IRQPause	0x8	/* Interrupt on pause            */
#define IRQEndCycle	0x10	/* Interrupt on end of cycle     */
#define IRQEndRun	0x20	/* Interrupt on end of run       */

/* Status read only register layout */

#define ModuleRunning	0x40	/* Module running */
#define ModulePaused	0x80	/* Module paused  */
#define InhibitSet	0x100	/* Latched, External Inhibit set    */
#define PauseSet	0x200	/* Latched, Paused                  */
#define EndCycleSet	0x400	/* Latched, At end of cycle         */
#define EndRunSet	0x800	/* Latched, All TFG cycles complete */

/* Memory control bit layout */
 
#define MemPause    0x100
#define MemEOF      0x200

#define ERROR (-1)
#define SUCCESS 0

/* tfglib routine definitions */

#if defined(__STDC__) || defined(_ANSI_EXT)
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
error_code tfg_disable (path_id);
error_code tfg_enable  (path_id, int32);
error_code tfg_init    (path_id);
error_code tfg_irqdis  (path_id);
#ifdef linux
error_code tfg_irqenb (int path, int32 status, int procid, int signum);
#else
#if defined(_MPFPOWERPC)
error_code tfg_irqenb  (path_id, int32, process_id, signal_code);
#else
error_code tfg_irqenb  (path_id, int32, int32, int16);
#endif
#endif
error_code tfg_pause   (path_id);
error_code tfg_rdmem   (path_id, int32, u_int16*);
error_code tfg_start   (path_id);
error_code tfg_wrcycle (path_id, int32);
error_code tfg_wrmem   (path_id, int32, u_int16*);
int tfg_prcsig  (path_id);
int tfg_rdcycle (path_id);
int tfg_rdframe (path_id);
int tfg_rdstat  (path_id);
#if defined(__cplusplus)
}
#endif /* __cplusplus */
#else
extern error_code tfg_enable (),  tfg_init (),  tfg_irqdis ();
extern error_code tfg_irqenb (),  tfg_rdmem ();
extern error_code tfg_disable (), tfg_wrmem (), tfg_wrcycle ();
extern error_code tfg_start  (),  tfg_pause ();
extern int tfg_rdcycle (), tfg_prcsig(), tfg_rdframe (), tfg_rdstat ();
#endif

#endif /* TFG_H */
