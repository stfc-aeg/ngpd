#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#else
#define ADQ_CLOCK_INT_INTREF 1
#define ADQ_SW_TRIGGER_MODE 1
#define ADQ_LEVEL_TRIGGER_ALL_CHANNELS 1

#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef LINUX
#include <stdlib.h>
#endif
#include "ngpd.h"
// #include "zynqmp_api.h"


int py_GetStreamOverflow()
{
    if (NGPDPath[0].valid == 0)
    {
        return -1; // ngpd card not set up
    }

    return ADQ_GetStreamOverflow(NGPDPath[0].adq_cu, NGPDPath[0].adq_num);

}