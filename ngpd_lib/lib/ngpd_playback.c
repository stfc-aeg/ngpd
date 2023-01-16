#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef _MSC_VER
#include <windows.h>
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#ifdef LINUX
#include <stdlib.h>
#endif
#if !defined( __MINGW32__) && !defined(_MSC_VER)
#define O_BINARY 0
#endif

#include "ngpd.h"

int ngpd_playback_generate(int path, int card, NGPDTestPattern *test_pat)
{
	int rc, i, t;
	u_int16_t *wr_buff, *p;
	int num_t = NGPD_PLAYBACK_NUM_T;
	int first_card, last_card;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_generate: Invalid path=%d\n", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
		num_t = NGPDPath[path].playback_num_bytes/sizeof(u_int16_t);
	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_generate: illegal card specified (%d)", card);
		return -1;
	}
	else
	{
		first_card = card;
		last_card = card;
	}

	if (test_pat->num_t != 0)
		num_t = test_pat->num_t;
	NGPDPath[path].playback_num_t = num_t;
	wr_buff = (u_int16_t *)malloc(num_t*sizeof(u_int16_t));
	if (wr_buff == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_generate: Out of memory\n");
		return -1;
	}
	if (NGPDPath[path].debug > 0)
		printf("Generating test pattern type %d, i_biasline=%d, i_height=%d, i_period=%d\n", test_pat->type, test_pat->i_baseline, test_pat->i_height, test_pat->i_period);
	switch (test_pat->type)
	{
	case NGPDTP_Ramp:
		p = wr_buff;
		for (t=0; t<num_t; t++)
			*p++ = t;
		break;

	case NGPDTP_Delta:
		p = wr_buff;
		for (t=0; t<num_t; t++)
		{
			if (t % test_pat->i_period == test_pat->i_period/2)
				*p++ = test_pat->i_baseline+test_pat->i_height;
			else
				*p++ = test_pat->i_baseline;
		}
		break;
	
	case NGPDTP_Square:
		p = wr_buff;
		for (t=0; t<num_t;)
		{
			for (i=0; i<test_pat->i_period/2 && t < num_t; i++, t++)
				*p++ = test_pat->i_baseline;
			for (i=0; i<test_pat->i_period/2 && t < num_t; i++, t++)
				*p++ = test_pat->i_baseline+test_pat->i_height;
		}
		break;
	default:
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_generate: test pattern type %d not known", test_pat->type);
		free (wr_buff);
		return -1;
	}

	if (NGPDPath[path].debug > 0)
		printf("Writing test pattern to ADQ14\n");
	rc=0;
	for (card=first_card; rc==0&&card<=last_card; card++)
	{
		rc = ngpd_dma_upload(path, card, 0, num_t/2, (u_int32_t*)wr_buff);
	}
	free (wr_buff);
	NGPDPath[path].playback_num_streams = 1;
	if (rc != 0)
	{
		char old_message[NGPD_MAX_ERROR_MESSAGE+2];
		strcpy(old_message, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_generate: Error writing pattern: %s", old_message);
	}
	return rc;
}

/**
 * @ingroup debug
 * Load Playback data from a single ADQ14 Neutron Gamma Discriminator scope mode file.
 *

 * The ADQ14 scope mode file should contain 1 stream, though the code has place holder to extend this. 

 * @param path 			A handle to the top level of the NGPD system returned from {@link xsp3_config()}.
 * @param card 			The number of the card in the NGPD system,
 *        				0 for a single card system 
 * @param filename 		File name to load
 * @param max_num_t		Limit the number of points to read or 0 to use the entire file.
 * @return 				0 or a negative error code.
 */
int ngpd_playback_load(int path, int card, char *filename, int max_num_t) 
{
	int rc = 0;
	u_int16_t *read_buff=NULL;
	char *cp;
	size_t n;
	ssize_t bytes_read;
	int fd=-1;
	struct stat fileStat;
	size_t file_num_t, fw_num_t=NGPD_PLAYBACK_NUM_T;
	int first_card, last_card;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_load: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
		fw_num_t = NGPDPath[path].playback_num_bytes/sizeof(u_int16_t);

	// Do sanity check on card number versus total in system
	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_playback_load: illegal card specified (%d)", card);
		return -1;
	}
	else
	{
		first_card = card;
		last_card = card;
	}

	if (stat(filename, &fileStat) < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,"ngpd_playback_load:  Can't determine the size of file name='%s'\n", filename);
		return NGPD_ERROR;
	}
	fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,"ngpd_playback_load: Can't open file '%s'\n", filename);
		return NGPD_ERROR;
	}
	file_num_t = fileStat.st_size/sizeof(u_int16_t);
	file_num_t = 32*(file_num_t/32);
	
	if (NGPDPath[path].debug)
		printf("ngpd_playback_load: file_num_t=%zu, fw_num_t=%zu\n", file_num_t, fw_num_t);
	
	if (max_num_t > fw_num_t)
		max_num_t = fw_num_t;
	if (max_num_t == 0)
	{
		/* Just use file_num_t */
		if (file_num_t > fw_num_t)
			file_num_t = fw_num_t;
	}
	else
	{
		max_num_t = 32*(max_num_t/32);
		if (max_num_t > file_num_t)
		{
			printf("Warning, file num samples %zd is shorter than specified num_t, limit num_t to file size\n", file_num_t);
		}
		else
			file_num_t = max_num_t;
	}
//	printf("... Using file_num_t=%zu\n", file_num_t);
	n = file_num_t * sizeof(u_int16_t);
	read_buff = (u_int16_t*) malloc(n);
	if (read_buff == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,"ngpd_playback_load: Out of memeory allcoating buffer size %zu\n", n);
		close (fd);
		return NGPD_ERROR;
	}
		
	cp = (char *)read_buff;
	do
	{
		bytes_read = read(fd, cp, n);
		if (bytes_read <= 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,"ngpd_playback_load: reading data from file '%s', errno=%d\n", filename, errno);
			free(read_buff);
			close (fd);
			return NGPD_ERROR;
		}
		n -= bytes_read;
		cp += bytes_read;
	} while (n > 0);
	close(fd);
	rc=0;
	for (card=first_card; rc==0&&card<=last_card; card++)
	{
		rc = ngpd_dma_upload(path, card, 0, file_num_t/2, (u_int32_t*)read_buff);
	}
	free(read_buff);
	NGPDPath[path].playback_num_t = file_num_t;
	NGPDPath[path].playback_num_streams = 1;
	printf("ngpd_playback_load: file_num_t=%zd, playback_num_bytes=%zd, playback_num_t=%d, rc=%d\n", file_num_t, NGPDPath[path].playback_num_bytes, NGPDPath[path].playback_num_t, rc);
	return rc;
}
