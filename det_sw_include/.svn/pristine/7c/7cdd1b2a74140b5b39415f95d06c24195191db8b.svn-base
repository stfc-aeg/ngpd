/***********************************************************************
 *                            ftpdef.h                                 *
 ***********************************************************************
 Purpose: internal definitions ftplib header file
 Author:  K.S.Turner
 Updates: 
 01/11/90 KST Initial implementation
 26/11/92 GRM Updated to ansi
*/
#if !defined(FTPDEF_H)
#define FTPDEF_H

#if 0 && defined(__MINGW32__)

/* Dummy version to allow non-socket version to compile (not link) */
#define BUFLEN 120

#define CR '\015'
#define LF '\012'

#define FTP_MAJOR_PP 1
#define FTP_MAJOR_PC 2
#define FTP_MAJOR_PI 3
#define FTP_MAJOR_TN 4
#define FTP_MAJOR_PN 5

#define FTP_CLASS_SYN 0
#define FTP_CLASS_INF 1
#define FTP_CLASS_CON 2
#define FTP_CLASS_ACC 3
#define FTP_CLASS_USP 4
#define FTP_CLASS_FIL 5

typedef struct {
	int client;
	int server;
	int socket;
	int dsock;
	char reply[BUFLEN];
	char buff[BUFSIZ];
	char *bptr;
	int nleft;
} FTP;

enum {DEBUG1=1, DEBUG2=2, DEBUG3=3};

#else

#if defined(_OSK) || defined(_OS9000)
#include <types.h>
#include <inet/socket.h>
#include <inet/netdb.h>
#include <inet/in.h>
#else
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#define socket_close closesocket
#else
#define socket_close close
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif
#endif

#define BUFLEN 120

#define CR '\015'
#define LF '\012'

#define FTP_MAJOR_PP 1
#define FTP_MAJOR_PC 2
#define FTP_MAJOR_PI 3
#define FTP_MAJOR_TN 4
#define FTP_MAJOR_PN 5

#define FTP_CLASS_SYN 0
#define FTP_CLASS_INF 1
#define FTP_CLASS_CON 2
#define FTP_CLASS_ACC 3
#define FTP_CLASS_USP 4
#define FTP_CLASS_FIL 5

typedef struct {
	struct sockaddr_in client;
	struct sockaddr_in server;
	int socket;
	int dsock;
	char reply[BUFLEN];
	char buff[BUFSIZ];
	char *bptr;
	int nleft;
} FTP;

enum {DEBUG1=1, DEBUG2=2, DEBUG3=3};
#endif

#endif /* FTPDEF_H  */
