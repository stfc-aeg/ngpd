/*
	Client routines for talking to the multi-client server
	By Richard W.M. Jones
*/

#ifndef _CLIENT_H_
#define _CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----- initialization functions -----*/

/* Library initialization function. Call this once only at the beginning of
 * your code. You can pass the names of three error-handling functions, if
 * you want. If you pass NULL, they default to internal functions that print
 * the messages on stderr.
 *	errmsg_handler:		Called whenever the server prints a '!...' string.
 *	debugmsg_handler:	Called whenever the server prints a '#...' string.
 *	timebar_handler:	Called with a percentage when the server emits a
 *						timebar string.
 *	error_handler:		Called with a message whenever the client library has
 *						real trouble talking to the server. (Best to print the
 *						string and abort the current operation.) If the fatal
 *						code is set (3rd arg), then the client has probably
 *						disconnected.
 */
void	cln_init			(void (*errmsg_handler)(int,char*),
							 void (*debugmsg_handler)(int, char*),
							 void (*timebar_handler)(int, int, int, char *),
							 void (*error_handler)(int,char*, int fatal));

int		cln_connect			(const char *hostname, int port);
void	cln_disconnect		(int sn);
int		cln_init_data_port	(int sn);
int		cln_accept			(int sn);

/*----- top-level functions -----*/

int		cln_wait_prompt		(int sn);
int		cln_wait_response	(int sn, int *);
int		cln_wait_double_response (int sn, double *);
int		cln_wait_string_response (int sn, char *str_return);
void	cln_cmd_printf		(int sn, char *fs, ...);
int		cln_signal 			(int sn, int signum);
int		cln_release 		(int sn);

/*----- low-level function (not recommended that you call this directly) -----*/

int		cln_next			(int sn,
	char *errmsg_return,
	int *int_return, double *dbl_return, char *str_return,
	int *done, int * outoff );

#define CLN_NEXT_ERROR		-1			/* error, usually disconnection */
#define CLN_NEXT_PROMPT		0			/* '> ': at prompt */
#define CLN_NEXT_ERRMSG		1			/* '! ': read error message */
#define CLN_NEXT_DEBUGMSG	2			/* '# ': read debugging message */
#define CLN_NEXT_TIMEBAR	3			/* '@ ': time-bar message */
#define CLN_NEXT_UNKNOWN	4			/* unknown line */
#define CLN_NEXT_INTRET		5			/* '* ': read integer ret value */
#define CLN_NEXT_DBLRET		6			/* '* ': read double ret value */
#define CLN_NEXT_STRRET		7			/* '* ': read string ret value */

#ifdef __cplusplus
}
#endif

#endif /* _CLIENT_H_ */
