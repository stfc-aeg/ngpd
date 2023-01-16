#include <pthread.h>
#include <stdio.h>
#include "xspress3.h"

#define NUM_THREADS 4
#define MAX_BRAM_SIZE 16384
#define CHUNK 256

XSP3Path Xsp3Sys[XSP3_MAX_PATH];
char error_message[NGPD_MAX_ERROR_MESSAGE + 1];
int max_errors=200;

int testBRAMMemory(void *femHandle, int chan) {
	int j,k,rc;
	u_int32_t *send_buf, *recv_buf, mask, x;
	int mem_size;
	int errors = 0;
	int io_errors = 0;

	if ((send_buf = (u_int32_t *) malloc(MAX_BRAM_SIZE * sizeof(u_int32_t))) == NULL) {
		fprintf(stderr, "No memory\n");
		exit(1);
	}
	if ((recv_buf = (u_int32_t *) malloc(MAX_BRAM_SIZE * sizeof(u_int32_t))) == NULL) {
		fprintf(stderr, "No memory\n");
		exit(1);
	}
	for (j = 1; j <= XSP3_REGION_RAM_MAX; j++) { // all possible regions
		if ((mem_size = xsp3_bram_size_table[j]) != 0) {
			if (xsp3_bram_width[j] == 32)
				mask = 0xFFFFFFFF;
			else
				mask = (1 << xsp3_bram_width[j]) - 1;
			printf("Writing chan %d, region %d, function %s\n", chan, j, xsp3_bram_name[j]);
			x = chan * 9 + j;
			for (k = 0; k < mem_size; k++) // all possible BRAM locations
					{
				send_buf[k] = mask & x;
				x++;
			}

			rc = xspress3FemSetIntArray(femHandle, chan, j, 0, mem_size, send_buf);
			if (rc != FEM_RTN_OK) {
				printf("IO Error on writing BRAM at channel %d region %d offset %d: %d\n", chan, j, k, rc);
				io_errors++;
			}
		}
	}
	for (j = 1; j <= XSP3_REGION_RAM_MAX; j++) { // all possible regions
		if ((mem_size = xsp3_bram_size_table[j]) != 0) {
			if (xsp3_bram_width[j] == 32)
				mask = 0xFFFFFFFF;
			else
				mask = (1 << xsp3_bram_width[j]) - 1;
			x = chan * 9 + j;
			for (k = 0; k < mem_size; k++) // all possible BRAM locations
					{
				send_buf[k] = mask & x;
				x++;
			}

			rc = xspress3FemGetIntArray(femHandle, chan, j, 0, mem_size, recv_buf);
			if (rc != FEM_RTN_OK) {
				printf("Error on reading BRAM at channel %d region %d offset %d: %d\n", chan, j, k, rc);
				io_errors++;
				break;
			}
			for (k = 0; k < mem_size; k++) // all possible BRAM locations
					{
				if (send_buf[k] != recv_buf[k]) {
					errors++;
					if (errors < max_errors)
						printf("ERROR: Chan  %d, region %d, offset %d. Wrote %08X, read %08X\n", chan, j, k,
								send_buf[k], recv_buf[k]);
					else if (errors == max_errors)
						printf("Too Many Errors, counting\n");

				}
			}
		}

	}
	free(send_buf);
	free(recv_buf);
	return errors;
}

void *func0(void *vpath) {
	long path;
	path = (long) vpath;
	void* femHandle = Xsp3Sys[path].femHandle;
	int chan = 0;
	int i;
	for (i = 0; i < 10; i++) {
		printf("Testing channel 0 using path %ld\n", path);
		testBRAMMemory(femHandle, chan);
	}

	pthread_exit(NULL);
}

void *func1(void *vpath) {
	long path;
	path = (long) vpath;
	void* femHandle = Xsp3Sys[path].femHandle;
	int chan = 1;
	int i;
	for (i = 0; i < 10; i++) {
		printf("Testing channel 1 using path %ld\n", path);
		testBRAMMemory(femHandle, chan);
	}
	pthread_exit(NULL);
}
void *func2(void *vpath) {
	long path;
	path = (long) vpath;
	void* femHandle = Xsp3Sys[path].femHandle;
	int chan = 2;
	int i;
	for (i = 0; i < 10; i++) {
		printf("Testing channel 2 using path %ld\n", path);
		testBRAMMemory(femHandle, chan);
	}
	pthread_exit(NULL);
}
void *func3(void *vpath) {
	long path;
	path = (long) vpath;
	void* femHandle = Xsp3Sys[path].femHandle;
	int chan = 3;
	int i;
	for (i = 0; i < 10; i++) {
		printf("Testing channel 3 using path %ld\n", path);
		testBRAMMemory(femHandle, chan);
	}
	pthread_exit(NULL);
}

//int thread_tests(int path)

int main(int argc, char *argv[]) {
	long path;
	char femHostName[][XSP3_MAX_IP_CHARS] = { "192.168.0.2" };
	int ncards = 1;
	int nchan = ncards * 4;
	int femPort = 30123;
	int rc;
	int debug = 1;

	pthread_t threads[NUM_THREADS];

	path = xsp3_config_tcp(femHostName, femPort, ncards, nchan, debug);
	if (path < 0) {
		printf(xsp3_get_error_message());
		return 1;
	}
	int errors = 0;

	printf("In main: creating thread 0\n");
	rc = pthread_create(&threads[0], NULL, func0, (void *) path);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		return -1;
	}
	printf("In main: creating thread 1\n");
	rc = pthread_create(&threads[1], NULL, func1, (void *) path);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		return -1;
	}
	printf("In main: creating thread 2\n");
	rc = pthread_create(&threads[2], NULL, func2, (void *) path);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		return -1;
	}
	printf("In main: creating thread 3\n");
	rc = pthread_create(&threads[3], NULL, func3, (void *) path);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		return -1;
	}
	pthread_exit(NULL);
	return errors;
}
