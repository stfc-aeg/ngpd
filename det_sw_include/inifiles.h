/*
	INI-Files library
	by Richard W.M. Jones

	Header file.

	See READ_ME for information.
*/

#ifndef _INIFILES_H_
#define _INIFILES_H_

#define MAX_INI_FIELDS 16

typedef struct {

	/* in C++, these fields would be public: */

	char *name, *type;
	void *ptr;

	/* in C++, these fields would be private: */

	int nr_fields;			/* number of fields in 'type' string */
	struct {
		char t;				/* type: 'd', 'f', 'g', etc. */
		int low;			/* lower bound for count */
		int high;			/* upper bound for count */
	} f[MAX_INI_FIELDS];

} INIF;

#ifdef __STDC__
void ini_init(void);
void ini_select(INIF*);
int  ini_read(char*,int);
#else
void ini_init();
void ini_select();
int  ini_read();
#endif

#endif
