#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
//#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#ifdef LINUX
#include <stdlib.h>
#endif


#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"
#include "hdf5.h"


static hid_t build_filter_type()
{
	hid_t ftype_id = H5Tcreate (H5T_COMPOUND, sizeof(NGPDFilter));

	H5Tinsert(ftype_id, "type", HOFFSET(NGPDFilter, type), H5T_NATIVE_INT);
	H5Tinsert(ftype_id, "iarg1", HOFFSET(NGPDFilter, iarg1), H5T_NATIVE_INT);
	H5Tinsert(ftype_id, "iarg2", HOFFSET(NGPDFilter, iarg2), H5T_NATIVE_INT);
	H5Tinsert(ftype_id, "darg", HOFFSET(NGPDFilter, darg), H5T_NATIVE_DOUBLE);
	return ftype_id;
}

int ngpd_save_settings(int path, char *file_name)
{
	hid_t file_id;
	hid_t dataset, dataspace, datatype, memspace, dataset_m, dataset_c;
	hsize_t dims[3], start[2], count[2], block[2];
	int buff_offset[NGPD_MAX_CHANS];
	float buff_float[NGPD_MAX_CHANS];
	u_int32_t *buff_u32=NULL;
	int rc=-1;
	int chan;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	NGPDFilter filt_buff[NGZMP_MAX_CHAN];
	NGPDMeasure *measure=NULL;
	int max_buff_size = NGPD_REGION_MAX_SIZE;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenZynqMP1)
		return ngzmp_save_settings(path, file_name);
	
	
	file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if (file_id < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot create file %s", file_name);
		return -1;
	}
	if (2*NGPD_MEASURE_TAIL_SUB_SIZE > max_buff_size)
		max_buff_size = 2*NGPD_MEASURE_TAIL_SUB_SIZE;
	buff_u32=(u_int32_t *)malloc(NGPDPath[path].num_chan*sizeof(u_int32_t)*max_buff_size);
	if (buff_u32 == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: out of memory");
		goto exit_error;
	}
	measure = (NGPDMeasure *)malloc(NGPDPath[path].num_chan*sizeof(NGPDMeasure));	
	if (measure == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: out of memory");
		goto exit_error;
	}
		
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_adc_get_range(path,  chan, buff_float+chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read ADC range : %s", old_msg);
			goto exit_error;
		}
		if ((rc=ngpd_adc_get_offset(path, chan, buff_offset+chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read offset level : %s", old_msg);
			goto exit_error;
		}
	}
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(1, dims, NULL);
	dataset = H5Dcreate(file_id, "/InputRange", H5T_NATIVE_FLOAT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_float);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write /InputRange dataset in file %s", file_name);
		H5Sclose(dataspace);
		goto exit_error;
	}
	dataset = H5Dcreate(file_id, "/InputOffset", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_offset);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write /InputOffset dataset in file %s", file_name);
		H5Sclose(dataspace);
		goto exit_error;
	}
	H5Sclose(dataspace);

	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_read_chan_regs(path, chan, NGPD_REGION_REGS, 0, NGPD_CHAN_NUM, buff_u32+NGPD_CHAN_NUM*chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read channel registers : %s", old_msg);
			goto exit_error;
		}
	}
	dims[1] = NGPD_CHAN_NUM;
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(2, dims, NULL);
	dataset = H5Dcreate(file_id, "/Registers", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
	H5Dclose(dataset);
	H5Sclose(dataspace);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write  /Registers dataset in file %s", file_name);
		goto exit_error;
	}

	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_read_measure(path, chan, measure+chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read measure setup and tables :%s", old_msg);
			goto exit_error;
		}
		if ((rc=ngpd_read_tail_subtract(path, chan, (int32_t *)buff_u32+2*NGPD_MEASURE_TAIL_SUB_SIZE*chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read tail subtract tables :%s", old_msg);
			goto exit_error;
		}
	}

	dims[1] = NGPD_MEASURE_TAIL_SUB_SIZE*2;
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(2, dims, NULL);
	dataset = H5Dcreate(file_id, "/TailSubEvenOdd", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
	H5Dclose(dataset);
	H5Sclose(dataspace);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write  TailSubEvenOdd dataset in file %s", file_name);
		goto exit_error;
	}
	dims[0] = NGPD_MEASURE_TAIL_THRES_SIZE;
	memspace = H5Screate_simple(1, dims, NULL);
	if (memspace < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot create dataspace for tail sum threshold in memory");
		goto exit_error;
	}
	start[0] = 0;
	count[0] = 1;
	block[0] = NGPD_MEASURE_TAIL_THRES_SIZE;
	H5Sselect_hyperslab(memspace, H5S_SELECT_SET, start, NULL, count, block);
	dims[1] = NGPD_MEASURE_TAIL_THRES_SIZE;
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(2, dims, NULL);
	if (memspace < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot create dataspace for tail sum threshold in file");
		H5Sclose(memspace);
		goto exit_error;
	}
	
	dataset_m = H5Dcreate(file_id, "/TailThresM", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	dataset_c = H5Dcreate(file_id, "/TailThresC", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		start[0] = chan;
		start[1] = 0;
		count[0] = 1;
		count[1] = 1;
		block[0] = 1;
		block[1] = NGPD_MEASURE_TAIL_THRES_SIZE;
		H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, NULL, count, block);
		rc = H5Dwrite(dataset_m, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, measure[chan].tail_thres_m);
		if (rc < 0)
			break;
		rc = H5Dwrite(dataset_c, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, measure[chan].tail_thres_c);
		if (rc < 0)
			break;
	}
	H5Dclose(dataset_m);
	H5Dclose(dataset_c);
	H5Sclose(dataspace);
	H5Sclose(memspace);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write tail measure settings dataset in file %s", file_name);
		goto exit_error;
	}
	if (0)
	{
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			if ((rc=ngpd_filter_read(path, chan, NGPD_FILT_NUM_COEFF, buff_u32+NGPD_FILT_NUM_COEFF*chan)) < 0)
			{
				strcpy(old_msg, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read filter coefficients : %s", old_msg);
				goto exit_error;
			}
		}
		dims[1] = NGPD_FILT_NUM_COEFF;
		dims[0] = NGPDPath[path].num_chan;
		dataspace = H5Screate_simple(2, dims, NULL);
		dataset = H5Dcreate(file_id, "/FilterCoefficients", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		rc = H5Dwrite(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
		H5Dclose(dataset);
		H5Sclose(dataspace);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write  Filter Coefficients dataset in file %s", file_name);
			goto exit_error;
		}
	}	
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_get_filter_type(path, chan, filt_buff+chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read filter type : %s", old_msg);
			goto exit_error;
		}
	}

	datatype = build_filter_type();
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(1, dims, NULL);
	dataset = H5Dcreate(file_id, "/FilterType", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, filt_buff);
	H5Tclose(datatype);
	H5Dclose(dataset);
	H5Sclose(dataspace);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write  Filter type dataset in file %s", file_name);
		goto exit_error;
	}
	rc = 0;

	free(buff_u32);
	free(measure);
	H5Fclose(file_id);
	return 0;

exit_error:
	free(buff_u32);
	free(measure);
	H5Fclose(file_id);
	return (rc<0)?rc:-1;
}

static int dataset_open_and_check(hid_t file_id, const char *file_name, const char *name, hid_t *datasetP, int exp_rank, hsize_t exp_dim0, hsize_t exp_dim1, hid_t exp_dataclass, size_t exp_size)
{
	hid_t dataset, dataspace, datatype, dataclass;
	int rank;
	hsize_t dims[2];
	size_t size_bytes;
	
	dataset = H5Dopen (file_id, name, H5P_DEFAULT);
	if (dataset < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot open %s dataset in file %s", name, file_name);
		return -1;
	}
	dataspace = H5Dget_space(dataset);
	rank = H5Sget_simple_extent_ndims(dataspace);
	if (rank != exp_rank)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected %s dataset rank to be %d, not %d in file %s", name, exp_rank, rank, file_name);
		H5Dclose(dataset);
		H5Sclose(dataspace);
		return -1;
	}
	rank = H5Sget_simple_extent_dims(dataspace, dims, NULL);
	H5Sclose(dataspace);
	if (rank == 1 && (dims[0] != exp_dim0))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected %s dataset size to be %" PRIu64 ", not %" PRIu64 " in file %s", name, 
					(uint64_t)exp_dim0, (uint64_t)(dims[0]), file_name);
		H5Dclose(dataset);
		return -1;
	}
	if (rank == 2 && (dims[0] != exp_dim0 || dims[1] != exp_dim1))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected %s dataset size to be (%" PRIu64 ", %" PRIu64 ") not (%" PRIu64 ", %" PRIu64 ") in file %s", 
				name, (uint64_t)exp_dim0, (uint64_t)exp_dim1, (uint64_t)(dims[0]), (uint64_t)(dims[1]), file_name);
		H5Dclose(dataset);
		return -1;
	}
	datatype = H5Dget_type(dataset);
	dataclass = H5Tget_class(datatype);
	size_bytes =  H5Tget_size(datatype); 
	H5Tclose(datatype);
	if (dataclass != exp_dataclass || size_bytes != exp_size)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected %s dataset to contain class %" PRIu64 " not class %" PRId64 ", size %zu not %zu in file %s", 
				name, (int64_t)dataclass, (int64_t)exp_dataclass, size_bytes, exp_size, file_name);
		H5Dclose(dataset);
		return -1;
	}
	*datasetP = dataset;
	return 0;
}

int ngpd_restore_settings(int path, char *file_name)
{
	hid_t file_id;
	hid_t dataset, dataspace, datatype, dataclass, localtype, dataset_m, dataset_c, memspace;
	int rank;
	hsize_t dims[3], start[2], count[2];
	u_int32_t *buff_u32=NULL;
	size_t size_bytes;
	int rc=0;
	int chan;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	NGPDFilter filt_buff[NGZMP_MAX_CHAN];
	int buff_offset[NGPD_MAX_CHANS];
	float buff_float[NGPD_MAX_CHANS];
	int max_buff_size = NGPD_REGION_MAX_SIZE;
	NGPDMeasure measure;
	int need_filter_coeff;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
		return ngzmp_restore_settings(path, file_name);

	
	file_id = H5Fopen(file_name, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot open file %s", file_name);
		return -1;
	}

	if (2*NGPD_MEASURE_TAIL_SUB_SIZE > max_buff_size)
		max_buff_size = 2*NGPD_MEASURE_TAIL_SUB_SIZE;
	buff_u32=(u_int32_t *)malloc(NGPDPath[path].num_chan*sizeof(u_int32_t)*max_buff_size);
	if (buff_u32 == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: out of memory");
		goto exit_error;
	}
		
	if (dataset_open_and_check(file_id, file_name, "/InputRange", &dataset, 1, NGPDPath[path].num_chan, 0, H5T_FLOAT, sizeof(float)) < 0)
		goto exit_error;

	rc = H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_float);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /InputRange dataset from file %s", file_name);
		H5Dclose(dataset);
		goto exit_error;
	}
	H5Dclose(dataset);

	if (dataset_open_and_check(file_id, file_name, "/InputOffset", &dataset, 1, NGPDPath[path].num_chan, 0, H5T_INTEGER, sizeof(int32_t)) < 0)
		goto exit_error;

	rc = H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_offset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /InputOffset dataset from file %s", file_name);
		H5Dclose(dataset);
		goto exit_error;
	}
	H5Dclose(dataset);
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_adc_set_range(path,  chan, (double)(buff_float[chan]))) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot set adc range : %s", old_msg);
			goto exit_error;
		}
		if ((rc=ngpd_adc_set_offset(path, chan, buff_offset[chan])) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot set offset level : %s", old_msg);
			goto exit_error;
		}
	}

	if (dataset_open_and_check(file_id, file_name, "/Registers", &dataset, 2, NGPDPath[path].num_chan, NGPD_CHAN_NUM, H5T_INTEGER, sizeof(uint32_t)) < 0)
		goto exit_error;
	rc = H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read %s dataset from file %s", "/Registers", file_name);
		H5Dclose(dataset);
		goto exit_error;
	}
	H5Dclose(dataset);
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, 0, 1, buff_u32+NGPD_CHAN_NUM*chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot write register : %s", old_msg);
			goto exit_error;
		}
		/* Do not write to channel register 1 as this is the filter register and it leaves it 1 value out of step (on ADQ14) */
		if ((rc=ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, 2, NGPD_CHAN_NUM-2, buff_u32+NGPD_CHAN_NUM*chan+2)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot write register : %s", old_msg);
			goto exit_error;
		}
	}

	dims[1] = NGPD_MEASURE_TAIL_SUB_SIZE*2;
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(2, dims, NULL);

	if (dataset_open_and_check(file_id, file_name, "/TailSubEvenOdd", &dataset, 2, NGPDPath[path].num_chan, NGPD_MEASURE_TAIL_SUB_SIZE*2, H5T_INTEGER, sizeof(uint32_t)) < 0)
		goto exit_error;
	rc = H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /TailSubEvenOdd dataset from file %s", file_name);
		H5Dclose(dataset);
		goto exit_error;
	}
	H5Dclose(dataset);

	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_write_tail_subtract(path,  chan, (int32_t *)buff_u32+2*NGPD_MEASURE_TAIL_SUB_SIZE*chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write tail subtract tables : %s", old_msg);
			goto exit_error;
		}
	}



	if (dataset_open_and_check(file_id, file_name, "/TailThresM", &dataset_m, 2, NGPDPath[path].num_chan, NGPD_MEASURE_TAIL_THRES_SIZE, H5T_INTEGER, sizeof(uint32_t)) < 0)
		goto exit_error;
	if (dataset_open_and_check(file_id, file_name, "/TailThresC", &dataset_c, 2, NGPDPath[path].num_chan, NGPD_MEASURE_TAIL_THRES_SIZE, H5T_INTEGER, sizeof(uint32_t)) < 0)
	{
		H5Dclose(dataset_m);
		goto exit_error;
	}
	dataspace = H5Dget_space(dataset_m);
	dims[0] = NGPD_MEASURE_TAIL_THRES_SIZE;
	memspace = H5Screate_simple(1, dims, NULL);
	start[0] = 0;
	count[0] = NGPD_MEASURE_TAIL_THRES_SIZE;
	rc =  H5Sselect_hyperslab(memspace, H5S_SELECT_SET, start, NULL, count, NULL);
	
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_read_measure(path, chan, &measure)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot read measure setup and tables :%s", old_msg);
			H5Dclose(dataset_m);
			H5Dclose(dataset_c);
			goto exit_error;
		}
		start[0] = chan; 
		start[1] = 0;
		count[0] = 1;
		count[1] = NGPD_MEASURE_TAIL_THRES_SIZE;
		rc =  H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, NULL, count, NULL);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read set hyperslab into /TailThresM" );
			H5Dclose(dataset_m);
			H5Dclose(dataset_c);
			goto exit_error;
		}
		rc = H5Dread(dataset_m, H5T_NATIVE_INT32, memspace, dataspace, H5P_DEFAULT, measure.tail_thres_m);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /TailThresM dataset from file %s", file_name);
			H5Dclose(dataset_m);
			H5Dclose(dataset_c);
			goto exit_error;
		}
		rc = H5Dread(dataset_c, H5T_NATIVE_INT32, memspace, dataspace, H5P_DEFAULT, measure.tail_thres_c);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /TailThresC dataset from file %s", file_name);
			H5Dclose(dataset_m);
			H5Dclose(dataset_c);
			goto exit_error;
		}
		if ((rc=ngpd_write_measure(path, chan, &measure)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_settings: Cannot write measure setup and tables :%s", old_msg);
			H5Dclose(dataset_m);
			H5Dclose(dataset_c);
			goto exit_error;
		}
		
	}
	H5Dclose(dataset_m);
	H5Dclose(dataset_c);

	if (dataset_open_and_check(file_id, file_name, "/FilterType", &dataset, 1, NGPDPath[path].num_chan, 0, H5T_COMPOUND, sizeof(NGPDFilter)) < 0)
		goto exit_error;
	localtype = build_filter_type();
	rc = H5Dread(dataset, localtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, filt_buff);
	H5Tclose(localtype);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /FilterType dataset from file %s", file_name);
		goto exit_error;
	}
	need_filter_coeff = 0;
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		rc = 0;
		switch (filt_buff[chan].type)
		{
		case NGPDFilter_Rectangle:
			rc = ngpd_filter_load_rect(path, chan, filt_buff[chan].iarg1);
			break;

		case NGPDFilter_Gaussian:
			rc = ngpd_filter_load_gaus(path, chan, filt_buff[chan].darg);
			break;

		case NGPDFilter_Exponential:
			rc = ngpd_filter_load_exp(path, chan, filt_buff[chan].darg);
			break;

		case NGPDFilter_Trapezoidal:
			rc = ngpd_filter_load_trapezoid(path, chan, filt_buff[chan].iarg1, filt_buff[chan].iarg2);
			break;
		case NGPDFilter_Custom:
			need_filter_coeff = 1;
			break;
		default:
			rc = 0;
			break;
		}
		if (rc<0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot generate filter type %d : %s", filt_buff[chan].type, old_msg);
			goto exit_error;
		}
	}
	
	if (0)
	{
		dataset = H5Dopen (file_id, "/FilterCoefficients", H5P_DEFAULT);
		if (dataset < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot open /FilterCoefficients dataset in file %s", file_name);
			goto exit_error;
		}
		dataspace = H5Dget_space(dataset);
		rank = H5Sget_simple_extent_ndims(dataspace);
		if (rank != 2)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected /FilterCoefficients dataset rank to be 2, not %d in file %s", 
								rank, file_name);
			H5Dclose(dataset);
			H5Sclose(dataspace);
			goto exit_error;
		}
		rank = H5Sget_simple_extent_dims(dataspace, dims, NULL);
		H5Sclose(dataspace);
		if (dims[0] != NGPDPath[path].num_chan || dims[1] != NGPD_FILT_NUM_COEFF)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected /FilterCoefficients dataset shape (%d, %d), not (%" PRIu64 ", %" PRIu64 ") in file %s", 
								NGPDPath[path].num_chan, NGPD_FILT_NUM_COEFF, (uint64_t)(dims[0]), (uint64_t)(dims[1]), file_name);
			H5Dclose(dataset);
			goto exit_error;
		}
		datatype = H5Dget_type(dataset);
		dataclass = H5Tget_class(datatype);
		size_bytes =  H5Tget_size(datatype); 
		H5Tclose(datatype);
		if (dataclass != H5T_INTEGER || size_bytes != 4)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Expected /FilterCoefficients dataset to contain 32 bit ints not class %" PRId64 ", size %zu in file %s", 
								(int64_t)dataclass, size_bytes, file_name);
			H5Dclose(dataset);
			goto exit_error;
		}
		rc = H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot read /FilterCoefficients dataset from file %s", file_name);
			H5Dclose(dataset);
			goto exit_error;
		}
		H5Dclose(dataset);
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			if (filt_buff[chan].type == NGPDFilter_Custom)
			{
				if ((rc=ngpd_filter_load_integer(path, chan, NGPD_FILT_NUM_COEFF, buff_u32+NGPD_FILT_NUM_COEFF*chan, NULL)) < 0)
				{
					strcpy(old_msg, ngpd_error_message);
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_restore_settings: Cannot write filter coefficients : %s", old_msg);
					goto exit_error;
				}
			}
		}
	}	
	free(buff_u32);
	H5Fclose(file_id);
	return 0;

exit_error:
	free(buff_u32);
	H5Fclose(file_id);
	return (rc<0)?rc:-1;
}

int ngzmp_save_settings(int path, char *file_name)
{
	hid_t file_id;
	hid_t dataset, dataspace, datatype;
	hsize_t dims[3];
	u_int16_t buff_u16[NGZMP_MAX_CHAN];
	u_int32_t *buff_u32=NULL;
	int rc;
	int chan, region;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	NGPDFilter filt_buff[NGZMP_MAX_CHAN];
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation != NGPDGenZynqMP1)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Called on generation type=%d, not NGZynqMP ", NGPDPath[path].generation);
		return -1;
	}
	
	file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if (file_id < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot create file %s", file_name);
		return -1;
	}
	buff_u32=(u_int32_t *)malloc(NGPDPath[path].num_chan*sizeof(u_int32_t)*NGPD_REGION_MAX_SIZE);
	if (buff_u32 == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: out of memory");
		goto exit_error;
	}
		
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(1, dims, NULL);
	if ((rc=ngzmp_spi_read_dga(path, 0, NGPDPath[path].num_chan, buff_u16)) < 0)
	{
		strcpy(old_msg, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot read DGA values : %s", old_msg);
		goto exit_error;
	}
	dataset = H5Dcreate(file_id, "/DGAGain", H5T_NATIVE_INT16, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u16);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot write /DGAGain dataset in file %s", file_name);
		H5Sclose(dataspace);
		goto exit_error;
	}

	if ((rc=ngpd_i2c_read_preamp_offset(path, 0, NGPDPath[path].num_chan, buff_u16)) < 0)
	{
		strcpy(old_msg, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot read preamp offset values : %s", old_msg);
		goto exit_error;
	}
	dataset = H5Dcreate(file_id, "/Offsets", H5T_NATIVE_INT16, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u16);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot write /Offsets dataset in file %s", file_name);
		H5Sclose(dataspace);
		goto exit_error;
	}
	H5Sclose(dataspace);

	for (region=0; region < NGZMP_NUM_REGIONS; region++)
	{		
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			if ((rc=ngpd_read_chan_regs(path, chan, region, 0, ngzmp_bram_size_table[region], buff_u32+ngzmp_bram_size_table[region]*chan)) < 0)
			{
				strcpy(old_msg, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot read region %d : %s", region, old_msg);
				goto exit_error;
			}
		}
		dims[1] = ngzmp_bram_size_table[region];
		dims[0] = NGPDPath[path].num_chan;
		dataspace = H5Screate_simple(2, dims, NULL);
		dataset = H5Dcreate(file_id, ngzmp_dataset_name[region], H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		rc = H5Dwrite(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
		H5Dclose(dataset);
		H5Sclose(dataspace);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot write  %s dataset in file %s", ngzmp_dataset_name[region], file_name);
			goto exit_error;
		}
	}
	
	if (1)
	{
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			if ((rc=ngpd_filter_read(path, chan, NGPD_FILT_NUM_COEFF, buff_u32+NGPD_FILT_NUM_COEFF*chan)) < 0)
			{
				strcpy(old_msg, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot read filter coefficients : %s", old_msg);
				goto exit_error;
			}
		}
		dims[1] = NGPD_FILT_NUM_COEFF;
		dims[0] = NGPDPath[path].num_chan;
		dataspace = H5Screate_simple(2, dims, NULL);
		dataset = H5Dcreate(file_id, "/FilterCoefficients", H5T_NATIVE_INT32, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		rc = H5Dwrite(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
		H5Dclose(dataset);
		H5Sclose(dataspace);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot write  Filter Coefficients dataset in file %s", file_name);
			goto exit_error;
		}
	}	
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		if ((rc=ngpd_get_filter_type(path, chan, filt_buff+chan)) < 0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot read filter type : %s", old_msg);
			goto exit_error;
		}
	}

	datatype = build_filter_type();
	dims[0] = NGPDPath[path].num_chan;
	dataspace = H5Screate_simple(1, dims, NULL);
	dataset = H5Dcreate(file_id, "/FilterType", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	rc = H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, filt_buff);
	H5Tclose(datatype);
	H5Dclose(dataset);
	H5Sclose(dataspace);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_save_settings: Cannot write  Filter type dataset in file %s", file_name);
		goto exit_error;
	}
	free(buff_u32);
	H5Fclose(file_id);
	return 0;

exit_error:
	free(buff_u32);
	H5Fclose(file_id);
	return (rc<0)?rc:-1;
}

int ngzmp_restore_settings(int path, char *file_name)
{
	hid_t file_id;
	hid_t dataset, dataspace, datatype, dataclass, localtype;
	int rank;
	hsize_t dims[3];
	u_int16_t buff_u16[NGZMP_MAX_CHAN];
	u_int32_t *buff_u32=NULL;
	size_t size_bytes;
	int rc;
	int chan, region;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	NGPDFilter filt_buff[NGZMP_MAX_CHAN];
	int need_filter_coeff=0;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Invalid path=%d", path);
		return -1;
	}
	
	if (NGPDPath[path].generation != NGPDGenZynqMP1)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Called on generation type=%d, not NGZynqMP ", NGPDPath[path].generation);
		return -1;
	}

	file_id = H5Fopen(file_name, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot open file %s", file_name);
		return -1;
	}
	buff_u32=(u_int32_t *)malloc(NGPDPath[path].num_chan*sizeof(u_int32_t)*NGPD_REGION_MAX_SIZE);
	if (buff_u32 == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: out of memory");
		goto exit_error;
	}
		
	if (dataset_open_and_check(file_id, file_name, "/DGAGain", &dataset, 1, NGPDPath[path].num_chan, 0, H5T_INTEGER, sizeof(uint16_t)) < 0)
		goto exit_error;
	rc = H5Dread(dataset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u16);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read/DGAGain dataset from file %s", file_name);
		goto exit_error;
	}
	
	if ((rc=ngzmp_spi_write_dga(path, 0, NGPDPath[path].num_chan, buff_u16)) < 0)
	{
		strcpy(old_msg, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read DGA values : %s", old_msg);
		goto exit_error;
	}

	if (dataset_open_and_check(file_id, file_name, "/Offsets", &dataset, 1, NGPDPath[path].num_chan, 0, H5T_INTEGER, sizeof(uint16_t)) < 0)
		goto exit_error;
	rc = H5Dread(dataset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u16);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read/Offsets dataset from file %s", file_name);
		goto exit_error;
	}	
	if ((rc=ngpd_i2c_write_preamp_offset(path, 0, NGPDPath[path].num_chan, buff_u16)) < 0)
	{
		strcpy(old_msg, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read preamp offset values : %s", old_msg);
		goto exit_error;
	}
	
	for (region=0; region < NGZMP_NUM_REGIONS; region++)
	{		
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			if (dataset_open_and_check(file_id, file_name, ngzmp_dataset_name[region], &dataset, 2, NGPDPath[path].num_chan, ngzmp_bram_size_table[region], H5T_INTEGER, sizeof(uint32_t)) < 0)
				goto exit_error;
			rc = H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
			H5Dclose(dataset);
			if (rc < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read %s dataset from file %s", ngzmp_dataset_name[region], file_name);
				goto exit_error;
			}
			
			if (region == NGZMP_REGION_REGS)
			{
				if ((rc=ngpd_write_chan_regs(path, chan, region, 0, 1, buff_u32+ngzmp_bram_size_table[region]*chan)) < 0)
				{
					strcpy(old_msg, ngpd_error_message);
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot write region %d : %s", region, old_msg);
					goto exit_error;
				}
				/* Omit filter register as pushing single value here gets filter load out of step */
				if ((rc=ngpd_write_chan_regs(path, chan, region, 2, ngzmp_bram_size_table[region]-2, buff_u32+ngzmp_bram_size_table[region]*chan+2)) < 0)
				{
					strcpy(old_msg, ngpd_error_message);
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot write region %d : %s", region, old_msg);
					goto exit_error;
				}
			}
			else
			{
				if ((rc=ngpd_write_chan_regs(path, chan, region, 0, ngzmp_bram_size_table[region], buff_u32+ngzmp_bram_size_table[region]*chan)) < 0)
				{
					strcpy(old_msg, ngpd_error_message);
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot write region %d : %s", region, old_msg);
					goto exit_error;
				}
			}
		}
	}

	if (dataset_open_and_check(file_id, file_name, "/FilterType", &dataset, 1, NGPDPath[path].num_chan, 0, H5T_COMPOUND, sizeof(NGPDFilter)) < 0)
		goto exit_error;
	localtype = build_filter_type();
	rc = H5Dread(dataset, localtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, filt_buff);
	H5Tclose(localtype);
	H5Dclose(dataset);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read /FilterType dataset from file %s", file_name);
		H5Dclose(dataset);
		goto exit_error;
	}
	for (chan=0; chan<NGPDPath[path].num_chan; chan++)
	{
		rc = 0;
		switch (filt_buff[chan].type)
		{
		case NGPDFilter_Rectangle:
			rc = ngpd_filter_load_rect(path, chan, filt_buff[chan].iarg1);
			break;

		case NGPDFilter_Gaussian:
			rc = ngpd_filter_load_gaus(path, chan, filt_buff[chan].darg);
			break;

		case NGPDFilter_Exponential:
			rc = ngpd_filter_load_exp(path, chan, filt_buff[chan].darg);
			break;

		case NGPDFilter_Trapezoidal:
			rc = ngpd_filter_load_trapezoid(path, chan, filt_buff[chan].iarg1, filt_buff[chan].iarg2);
			break;
		case NGPDFilter_Custom:
			need_filter_coeff = 1;
			break;
		default:
			rc = 0;
			break;
		}
		if (rc<0)
		{
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot generate filter type %d : %s", filt_buff[chan].type, old_msg);
			goto exit_error;
		}
	}
	
	if (need_filter_coeff)
	{
		dataset = H5Dopen (file_id, "/FilterCoefficients", H5P_DEFAULT);
		if (dataset < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot open /FilterCoefficients dataset in file %s", file_name);
			goto exit_error;
		}
		dataspace = H5Dget_space(dataset);
		rank = H5Sget_simple_extent_ndims(dataspace);
		if (rank != 2)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Expected /FilterCoefficients dataset rank to be 2, not %d in file %s", 
								rank, file_name);
			H5Dclose(dataset);
			H5Sclose(dataspace);
			goto exit_error;
		}
		rank = H5Sget_simple_extent_dims(dataspace, dims, NULL);
		H5Sclose(dataspace);
		if (dims[0] != NGPDPath[path].num_chan || dims[1] != NGPD_FILT_NUM_COEFF)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Expected /FilterCoefficients dataset shape (%d, %d), not (%" PRIu64 ", %" PRIu64 ") in file %s", 
								NGPDPath[path].num_chan, NGPD_FILT_NUM_COEFF, (uint64_t)(dims[0]), (uint64_t)(dims[1]), file_name);
			H5Dclose(dataset);
			goto exit_error;
		}
		datatype = H5Dget_type(dataset);
		dataclass = H5Tget_class(datatype);
		size_bytes =  H5Tget_size(datatype); 
		H5Tclose(datatype);
		if (dataclass != H5T_INTEGER || size_bytes != 4)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Expected /FilterCoefficients dataset to contain 32 bit ints not class %" PRId64 ", size %zu in file %s", 
								(int64_t)dataclass, size_bytes, file_name);
			H5Dclose(dataset);
			goto exit_error;
		}
		rc = H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff_u32);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot read /FilterCoefficients dataset from file %s", file_name);
			H5Dclose(dataset);
			goto exit_error;
		}
		H5Dclose(dataset);
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			if (filt_buff[chan].type == NGPDFilter_Custom)
			{
				if ((rc=ngpd_filter_load_integer(path, chan, NGPD_FILT_NUM_COEFF, buff_u32+NGPD_FILT_NUM_COEFF*chan, NULL)) < 0)
				{
					strcpy(old_msg, ngpd_error_message);
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_restore_settings: Cannot write filter coefficients : %s", old_msg);
					goto exit_error;
				}
			}
		}
	}	

	free(buff_u32);
	H5Fclose(file_id);
	return 0;

exit_error:
	free(buff_u32);
	H5Fclose(file_id);
	return (rc<0)?rc:-1;
}

