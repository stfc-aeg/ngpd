/*
 * fan_control.h
 *
 *  Created on: 20 Feb 2013
 *      Author: wih73
 */

#ifndef FAN_CONTROL_H_
#define FAN_CONTROL_H_
typedef s16 int16_t;
typedef s32 int32_t;
typedef struct _fan_log {
	int16_t temp; 	// Signed int in 0.5 deg C units
	int16_t speed; // 0 = off, 4000 = full speed
} FanLog;
#define XSP3_FAN_LOG_POINTS 10000
#define XSP3_FAN_MODE_OFF				0
#define XSP3_FAN_MODE_MONITOR_LOOP 		1
#define XSP3_FAN_MODE_MONITOR_ONESHOT 	2
#define XSP3_FAN_MODE_CONTROL 			3

#define XSP3_FAN_CONT_NUM_AVERAGE 10
#define XSP3_FAN_CONT_FRAC_PRECISION 10

typedef struct _fan_cont
{
	int32_t mode;
	int32_t start_run;
	int32_t cur_temp[4];
	int32_t cur_max;
	int32_t target, p_const, i_const;
	int32_t cur_point;
	FanLog log[XSP3_FAN_LOG_POINTS];
} FanControl;
extern FanControl fan_cont;

#endif /* FAN_CONTROL_H_ */
