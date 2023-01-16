/*
 * fan_speed.c
 *
 *  Created on: 20 Feb 2013
 *      Author: wih73
 *   Zynq Version: 13/10/2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <time.h>

#include "fem.h"
#include "xspress3.h"
#include "zynqmp_protocol.h"
#include "commandProcessor.h"
#include "xspress3_dma_protocol.h"
#include "xspress4_lib_driver.h"

extern int dev_board;

X3mFanControl fan_cont;
void init_fan_cont()

{
#if 0
	fan_cont.mode = XSP3_FAN_MODE_MONITOR_LOOP;
#else
	fan_cont.mode = XSP3_FAN_MODE_CONTROL;
#endif
	fan_cont.target = 55*2;
	fan_cont.p_const = 18*XSP3_FAN_CONT_FRAC_PRECISION;
	fan_cont.i_const = (int)(0.6*XSP3_FAN_CONT_FRAC_PRECISION);
	fan_cont.cur_point = 0;
	fan_cont.start_run = 1;

}
void *fanSpeedControl(void * unused)
{
	int temperature[XSP4_NUM_LM75];	// Bigger of 2 buffer sizes in case called on xspress4, unlikely
	int chip;
	int maxTemp;
	int fan_speed;
	int integral = 0;
	int prev_mode = 0;
	int prev_integral;
	int ave_temp;
	int ave_count;
	struct timespec interval, remaining;
	int32_t zynq_temp=0, zynq_ave_temp;
	int test_mode=0;
	int test_temp = 0;
	u_int32_t offtime = 0;
	
	init_fan_cont();
	if (dev_board == 0)
	{
		if (xsp4_i2c_open_adc() < 0)
		{
#if 0
			printf("Cannot open i2C paths, so assuming dev card version, exiting fan speed\n");
			return NULL;
#else
			printf("Cannot open i2C paths, so assuming dev card version, enabling test mode\n");
			test_mode = 1;
#endif
		} 
	}
	else
		test_mode = 1;

	if (!dev_board)
	{
		xsp3m_write_fan_speed(xsp_path, 0, 255); // Library makes 0 = slow, 255 = full
		xsp4_write_glob_reg(xsp_path, 0, XSP34_GLOB_BC_FAN_SPEED, 1, &offtime);
	}
	fan_speed = 255;

	ave_temp = 0;
	ave_count = 0;
	zynq_ave_temp = 0;
	prev_integral = 0;

	while (1)
	{
		if (fan_cont.mode != prev_mode || fan_cont.start_run)
		{
			integral = 0;
			fan_cont.cur_point = 0;
			fan_cont.start_run = 0;
		}
		prev_mode = fan_cont.mode;

		if (fan_cont.mode != XSP3_FAN_MODE_OFF)
		{
			if (test_mode == 0)
			{
				 xsp3_i2c_read_adc_temp_int(xsp_path, 0, temperature);
			}
			else
			{
				for (chip=0; chip<XSP3M_NUM_LM75; chip++)
					temperature[chip] = test_temp+chip*2;
				test_temp ++;
				if (test_temp > 200)
					test_temp = 0;
			}

#if 0
			printf("Temperatures:");
			for (chip=0; chip<4; chip++)
				printf(" %d", temperature[chip]/2);
			printf(" MAX=%d\n", maxTemp/2);
#endif
			maxTemp = -100;
			for (chip=0; chip<XSP3M_NUM_LM75; chip++)
			{
				if (temperature[chip] > maxTemp)
					maxTemp = temperature[chip];
			}
			fan_cont.cur_max = maxTemp;
			for (chip=0; chip<4; chip++)
				fan_cont.cur_temp[chip] = temperature[chip];
			xsp3_read_xadc(xsp_path, 0, XSP3_XADC_ZYNQ_TEMP, 1, (u_int32_t *)&zynq_temp);
#if 0
			if (zynq_temp/500) > max_temp;	// ZYnq temperature is returned in mili degrees.
				maxTemp = zynq_temp/500;
#endif
			ave_temp += maxTemp;
			zynq_ave_temp += zynq_temp;
			ave_count ++;

			if (ave_count == XSP3_FAN_CONT_NUM_AVERAGE)
			{
				ave_count = 0;
				if (fan_cont.mode == XSP3_FAN_MODE_CONTROL)
				{
					int err = ave_temp - fan_cont.target*XSP3_FAN_CONT_NUM_AVERAGE;
					int zynq_err = zynq_ave_temp/500 - 70*2*XSP3_FAN_CONT_NUM_AVERAGE; // in 0.5 degree units.
//					printf("ave_temp=%d, aynq_ave_temp=%d, err=%d, zynq_err=%d\n", ave_temp, zynq_ave_temp/500, err, zynq_err);
					if (zynq_err > err)
						err = zynq_err;
					// Proportional term. If board is 10 degree too hot, turn fans on full?
					// so 20 units error needs to make a fan sped of 4000, x200. is 1 in 200 resolution, OK with integer
					// Integral term will accumulate error of order max 10 c = 20 units, hopefully for 10 minutes, until controlled
					// to 20*10*60*10 = 120,000, so fine in 32 bit int
					// To get order of magnitude for integral term, consider 10 deg C error for 10 seconds would ramp integral to 200. Allow this to scale fan speed to full (4000),
					// would have to be multiplied by 20..
					// Try integer prec with 0..200 range.
					prev_integral = integral;
					if (integral < 2000000 && integral > -2000000)
						integral += err;

					fan_speed = (fan_cont.p_const*err)/(XSP3_FAN_CONT_NUM_AVERAGE*XSP3_FAN_CONT_FRAC_PRECISION) + (integral*fan_cont.i_const)/(XSP3_FAN_CONT_FRAC_PRECISION*XSP3_FAN_CONT_NUM_AVERAGE);
					if (fan_speed > 255)
					{
						fan_speed = 255;
						integral = prev_integral; // If saturated, limit non-linear behaviour
					}
					else if (fan_speed < 0)
					{
						fan_speed = 0;
						integral = prev_integral; // If saturated, limit non-linear behaviour
					}
					if (zynq_temp > 80*1000)	// Override fan speed if Zynq is too hot
						fan_speed = 255;
					if (test_mode == 0)
					{
						xsp3m_write_fan_speed(xsp_path, 0, fan_speed);
						offtime = 16*(255-fan_speed);		// We write off time to the hardware, the signal is inverted on its way out of the FPGA and then inverted by the MOSFET on the adapter board.
						if (offtime >3270) 
							offtime = 3270;		// only allow to drop to 20 % duty cycle.
						xsp4_write_glob_reg(xsp_path, 0, XSP34_GLOB_BC_FAN_SPEED, 1, &offtime);
					}
				}
				else
				{
					if (test_mode == 0)
						xsp3m_read_fan_speed(xsp_path, 0, &fan_speed);
				}
				fan_cont.log[fan_cont.cur_point].speed = fan_speed;
				fan_cont.log[fan_cont.cur_point].temp = ave_temp;
				fan_cont.log[fan_cont.cur_point].zynq_temp = zynq_ave_temp/500;	// Change Zynq milidegrees into 0.5 degree steps
				fan_cont.cur_point++;
				if (fan_cont.cur_point == XSP3_FAN_LOG_POINTS)
				{
					if (fan_cont.mode == XSP3_FAN_MODE_MONITOR_ONESHOT)
						fan_cont.cur_point = XSP3_FAN_LOG_POINTS-1;
					else
						fan_cont.cur_point = 0;
				}
				ave_temp = 0;
				zynq_ave_temp = 0;
			}
		}
		interval.tv_sec = 0;
		interval.tv_nsec = 500*1000*1000;	// Cannot readout faster than 300 ms
		while (nanosleep(&interval, &remaining) == -1 && errno == EINTR)
			interval = remaining;	
	}
}

