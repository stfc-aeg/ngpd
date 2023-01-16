#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "errors.h"
#include "ngpd.h"
#include "ngzmptest.h"
#include "zynqmp_xadc.h"

int list_sys_mons(int path);

int sys_mon_tests(int path)
{
	list_sys_mons(path);
	return 0;
}
int list_sys_mons(int path)
{
	int card =0;
	int rc;
	int io_errors = 0;
	int sys_mon_values[XADCParamMax+AMSParamMax];
	int n = XADCParamMax+AMSParamMax-1;
	int i;
	
	mprintf(1, "LIST-SYS-MON\n");
	if (zynqmp_xadc_init(ZYNQMP_XADC_INIT_FULL) < 0)
	{
		mprintf(MP_ERR, "ERROR initialising the AMS/XADC interface \n");
		io_errors++;
	}

	if (ngzmp_read_xadc(path, card, 0, n, (int32_t *)sys_mon_values) < 0)
	{
		mprintf(MP_ERR, "ERROR reading the AMS/XADC values \n");
		io_errors++;
	}
	for (i=0;i <n; i++)
		printf("%d\t%d\n", i, sys_mon_values[i]);
	
	if (io_errors > 0)
		mprintf(MP_ERR, "Found %d IO errors reading system monitorr values\n", io_errors);
	num_io_errors += io_errors;
	
	return 0;
}
