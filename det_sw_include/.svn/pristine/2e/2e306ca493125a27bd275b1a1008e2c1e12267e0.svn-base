/*-- Trace calls to malloc -- by Richard W.M. Jones --*/

#ifndef _TRACE_MALLOC_H_
#define _TRACE_MALLOC_H_
void *trace_malloc( int, char *, int);
void trace_free(void *, char *, int ); 
#define malloc(expression) trace_malloc ((expression),__FILE__,__LINE__)
#define free(expression) trace_free ((expression),__FILE__,__LINE__)

#endif /* _TRACE_MALLOC_H_ */
