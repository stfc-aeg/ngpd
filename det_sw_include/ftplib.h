/***********************************************************************
 *                            ftplib.h                                 *
 ***********************************************************************
 Purpose: ftplib header file
 Author:  K.S.Turner
 Updates: 
 01/11/90 KST Initial implementation
 26/11/92 GRM Updated to include ansi prototyping
*/

#include <stdio.h>

#if !defined(FTPLIB_H)
#define FTPLIB_H

#define FTP_PP_MARKER			-62	/* Restart marker reply		*/
#define FTP_PP_SWAIT			-61	/* Service rdy in nnn mins	*/
#define FTP_PP_DOPEN			FTP_SUCCESS /*	-60	/* Data connection open		*/
#define FTP_PP_FOK				FTP_SUCCESS	/*	-59	/* File status okay			*/

#define FTP_PC_OK				FTP_SUCCESS	/*	-58	/* Command okay					*/
#define FTP_PC_IGNORE			FTP_SUCCESS	/*	-57	/* Command superfluous			*/
#define FTP_PC_SSTAT			FTP_SUCCESS	/*	-56	/* System status / help reply	*/
#define FTP_PC_DSTAT			FTP_SUCCESS	/*	-55	/* Directory status				*/
#define FTP_PC_FSTAT			FTP_SUCCESS	/*	-54	/* File status					*/
#define FTP_PC_HELPM			FTP_SUCCESS	/*	-53	/* Help message					*/
#define FTP_PC_NAME				FTP_SUCCESS	/*	-52	/* NAME system type				*/
#define FTP_PC_READY			FTP_SUCCESS	/*	-51	/* Service ready				*/
#define FTP_PC_CLOSE			FTP_SUCCESS	/*	-50	/* Closing service connection	*/
#define FTP_PC_DOPEN			FTP_SUCCESS	/*	-49	/* Data connection open			*/
#define FTP_PC_DCLOSE			FTP_SUCCESS	/*	-48	/* Closing data connection		*/
#define FTP_PC_PASSV			FTP_SUCCESS	/*	-47	/* Entering passive mode		*/
#define FTP_PC_LOGIN			FTP_SUCCESS	/*	-46	/* User logged in				*/
#define FTP_PC_FILEOK			FTP_SUCCESS	/*	-45	/* File action okay				*/
#define FTP_PC_PATH				FTP_SUCCESS	/*	-44	/* pathname created				*/

#define FTP_PI_PASSREQ			-43	/* Username okay, need password	*/
#define FTP_PI_ACCOUNT			-42	/* Need account for login		*/
#define FTP_PI_INFOREQ			-41	/* File action needs more info	*/

#define FTP_TN_NSERVICE			-40	/* Service not available		*/
#define FTP_TN_DOPEN			-39	/* Cannot open data connection	*/
#define FTP_TN_TABORT			-38	/* Transfer aborted				*/
#define FTP_TN_FUNAVAIL			-37	/* File unavailable				*/
#define FTP_TN_LERROR			-36	/* Local error					*/
#define FTP_TN_QUOTA			-35	/* Not enough storage space		*/

#define FTP_PN_ESYNTAX			-34	/* Syntax error					*/
#define FTP_PN_EPARAM			-33	/* Syntax error in param or arg	*/
#define FTP_PN_ECOMMAND			-32	/* Command not implemented		*/
#define FTP_PN_BADSEQU			-31	/* Bad command sequence			*/
#define FTP_PN_BADPARM			-30	/* Bad param for command		*/
#define FTP_PN_NOLOGIN			-29	/* Not logged in				*/
#define FTP_PN_NOACCT			-28	/* Account required for			*/
#define FTP_PN_FUNAVAIL			-27	/* File unavailable				*/
#define FTP_PN_PAGETYPE			-26	/* Page type unknown			*/
#define FTP_PN_QUOTA			-25	/* Exceeded storage allocation	*/
#define FTP_PN_FNAME			-24	/* File name not allowed		*/

#define FTP_LCL_READ			-99	/* error reading local file */
#define FTP_LCL_MALLOC			-23	/* cannot allocate memory */
#define FTP_LCL_MODE			-22	/* mode must be r,w,a */
#define FTP_LCL_EOF				-21	/* end-of-file on local file */
#define FTP_LCL_WRITE			-20	/* error writing to local file */

#define FTP_NET_ACCEPT			-98 /* accept */
#define FTP_NET_HOST			-10	/* hostname */
#define FTP_NET_GETSOCKNAME		-9	/* getsockname */
#define FTP_NET_CONNECT			-8	/* connect */
#define FTP_NET_GETSERVBYNAME	-7	/* getservbyname */
#define FTP_NET_SOCKET			-6	/* socket */
#define FTP_NET_BIND			-5	/* bind */
#define FTP_NET_SETSOCKOPT		-4	/* setsockopt */
#define FTP_NET_LISTEN			-3	/* listen */
#define FTP_NET_READ			-2	/* network read error */
#define FTP_NET_WRITE			-1	/* network write error */

#define FTP_UNKNOWN				-97 /* Unknown error (serious) */
#define FTP_SUCCESS				0	/* successful function */

#if defined(__STDC__) || defined(_ANSI_EXT)
int  ftpAuth (void *, char *, char *);
void ftpClose (void *);
int  ftpCloseData (void *);
int  ftpCwd (void *, char *);
void ftpSetDebugLevel (int);
void ftpClrError (void);
void ftpSetError (int);
int  ftpGetError (void);
void ftpSetErrorFile (FILE *);
char *ftpErrorMessage (int);
FILE *GetErrorFile (void);
void ftpLogPrintf (int, char *, ...);
void ftpLogError (int, const char *);
int  ftpReplyCodeMajor (void *);
int  ftpReplyCodeClass (void *);
int  ftpReplyCodeMinor (void *);
char *ReplyCodeMessage (void *);
int  ftpList (void *, char *);
void *ftpOpen (char *);
int  ftpOpenData (void *, char *, char *);
int  ftpPasswd (void *, char *);
int  ftpPwd (void *, char *);
int  ftpDelete (void *, char *);
long ftpReadAscii (void *, FILE *);
long ftpReadBinary (void *, FILE *);
int  ftpReadReply (void *);
int  ftpPutAsciiFile (char *, char *, char *, char *, char *);
int  ftpPutBinaryFile (char *, char *, char *, char *, char *);
int  ftpGetAsciiFile (char *, char *, char *, char *, char *);
int  ftpGetBinaryFile (char *, char *, char *, char *, char *);
int  ftpPutAsciiApndFile (char *, char *, char *, char *, char *);
int  ftpSetAscii (void *);
int  ftpSetBinary (void *);
int  ftpUser (void *, char *);
long ftpWriteAscii (void *, FILE *);
long ftpWriteBinary (void *, FILE *);
int  ftpWriteCmd (void *, char *, char *);
FILE *ftpGetLogFile (void);
void ftpSetDebugLevel (int dbg);
void ftpSetLogFile (FILE *fp);
int ftpGetDSock(void *vptr);


#else
extern void ftpClose ();
extern int  ftpCwd (), ftpGetError (), ftpAuth (), ftpCloseData ();
extern void ftpSetDebugLevel (), ftpClrError (), ftpSetError ();
extern void ftpSetErrorFile ();
extern FILE *GetErrorFile (void);
extern void ftpLogPrintf (), ftpLogError ();
extern int  ftpReplyCodeMajor (), ftpReplyCodeClass ();
extern int  ftpReplyCodeMinor ();
extern char *ReplyCodeMessage ();
extern int  ftpList ();
extern void *ftpOpen ();	
extern int  ftpOpenData (), ftpPasswd (), ftpPwd ();
extern long ftpReadAscii (), long ftpReadBinary ();
extern int  ftpReadReply (), ftpPutAsciiFile ();
extern int  ftpPutBinaryFile (), ftpGetAsciiFile ();
extern int  ftpGetBinaryFile (), ftpPutAsciiApndFile ();
extern int  ftpSetAscii (), ftpSetBinary (), ftpUser ();
extern long ftpWriteAscii (), ftpWriteBinary ();
extern int  ftpWriteCmd ();
extern char *ftpErrorMessage ();
extern int ftpGetDSock();

#endif

#endif /* FTPLIB_H  */

