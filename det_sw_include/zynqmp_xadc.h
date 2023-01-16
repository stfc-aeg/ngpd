
#ifndef ZYNQMP_XADC_H_
#define ZYNQMP_XADC_H_

#define MAX_PATH_SIZE	300
#define MAX_NAME_SIZE	50
#define MAX_VALUE_SIZE  100

#define SYS_PATH_IIO	"/sys/bus/iio/devices"

#define ZYNQMP_NUM_XADC	2	// Allow for 1 in ZynqMP xadc and AMS

enum XADC_Param
{
	XADCParamVccInt,
  	XADCParamVccAux,
	XADCParamVccBRam,
	XADCParamVccPInt,	// PS7 Int
	XADCParamVccPAux,	// PS7 Aux
	XADCParamVccoDdr,	// PS7 Vccoddr
	XADCParamTemp,
	XADCParamVAux0,

	XADCParamMax
};

enum AMS_Param
{
	AMSParamPSTempLPD,
	AMSParamPSTempFPD,
	AMSParamPSVccIntLP,
	AMSParamPSVccIntFP,
	AMSParamPSVccAux,
	AMSParamPSVccDDR,
	AMSParamPSVccIO0,
	AMSParamPSVccIO1,
	AMSParamPSVccIO2,
	AMSParamPSVccIO3,
	AMSParamPSAVccMGTR,
	AMSParamPSAVttMGTR,
	AMSParamPSVccAMS,
	AMSParamPSVccPLL0,
	AMSParamPSVccBatt,
	AMSParamPLVccInt,
	AMSParamPLVccBRAM,
	AMSParamPLVccAux,
	AMSParamPSVccDDRPLL,
	AMSParamPSVccIntFPDDR,

	AMSParamMax
};

enum XADC_Alarm
{
	EAlarmVccInt_TH,
	EAlarmVccAux_TH,
	EAlarmVccBRam_TH,
	EAlarmTemp_TH,
	EAlarmMAX
};

struct Xadc_callback
{
	void(*func)(void *arg);
	void *arg;
};

enum XADC_Init_Type
{
	ZYNQMP_XADC_NOT_INITIALIZED,
	ZYNQMP_XADC_INIT_READ_ONLY,
	ZYNQMP_XADC_INIT_FULL
};

/*
 * int xadc_core_init(enum XADC_Init_Type type);
 *
 * This function should be called once in the beginning
 * of the program. It is responsible for finding the
 * XADC device nodes and to initialized global variables.
 * If called with init type FULL, it also spawn the event
 * handling thread which monitors the alarms and handle
 * the particular event.
 *
 * Argument		:
 * 			- 	type: enum value for the type of initialization.
 * 						READ_ONLY type, if event handling is not
 * 						needed. FULL type, if event handling is
 * 						also required.
 *
 * Return Value : [Integer]  0 : For success,
 *                         -1 : Device node not found
 *
 */
int zynqmp_xadc_init(enum XADC_Init_Type init_type);

/*
 * int xadc_core_deinit(void);
 *
 * This function should be called at the end of program.
 * It is responsible for release the resources.
 *
 * Argument: N/A
 * Return Value:[Integer] 0: Success
 */
int zynqmp_xadc_deinit(void);

/*
 * void xadc_update_stat(void);
 *
 * This function just updates the global caches for all
 * the statistic parameters. This should be called before
 * xadc_get_value() fucntion, which reads the statistic
 * from the cache.
 *
 * Argument: N/A
 * Return Value: N/A
 */
void xadc_update_stat(void);

/*
 * float xadc_get_value(enum XADC_Param parameter);
 *
 * This function returns the cached statistics value of
 * the given parameter. For voltage, the return value is
 * in mili-volt and for temperature its degree Celcius.
 *
 * Argument:
 * 		- parameter: enum value for the parameter whose
 * 					 value is needed.
 * 	Return Value:[float] cached statistic of the given
 * 	             parameter[ mV / degree C].
 *
 */
float xadc_get_value(enum XADC_Param parameter);	// read the parameter from the cache [not real time update]

/*
 * float xadc_touch(enum XADC_Param parameter);
 *
 * This function updates the global cache statistic for the
 * given parameter. It returns the realtime value of the
 * given parameter unlike xadc_get_value.
 *
 * Argument:
 * 		- parameter: enum value for the parameter whose value
 * 		             is needed.
 *
 * Return Value:[float] realtime value of the given parameter
 *                      [ mV / degree C].
 */
float xadc_touch(enum XADC_Param parameter);	// update the cache and read the value

/*
 * int xadc_set_threshold(enum XADC_Alarm alarm,float threshold_low, float threshold_high, struct Xadc_callback *callback);
 *
 * This function sets the threshold values for the given alarm.
 *
 * Argument:
	-	alarm: enum value for the alarm for which threshold are set.
	-	threshold_low: Low threshold value for the alarm [mV /degreeC]
    -	threshold_high: High threshold value for the alarm [mV /degreeC]
    -	callback: callback information for the event on the given alarm.
                  If NULL is passed in this arg, no call back will be
                  registered otherwise on event occurrence callback->func(arg)
                  will be called.

 * Return Value: [Integer] 0: Success, Non-zero: Error
 */
int xadc_set_threshold(enum XADC_Alarm alarm,float threshold_low, float threshold_high, struct Xadc_callback *callback);

/*
 * bool xadc_get_alarm_status(enum XADC_Alarm alarm);
 *
 * This function returns the current status of the event for the given alarm.
 * Valid only after setting the threshold. It is useful in case, one don't
 * need callback but just get status of the event at some stage.
 *
 * Argument:
 * 	- alarm: enum value for the alarm for which event status is needed.
 *
 * 	Return Value:[bool] true: Event Active , false: Event Inactive.
 *
 */
bool xadc_get_alarm_status(enum XADC_Alarm alarm);

enum SysMonType
{
	SysMonNone = 0,
	SysMonXADC = 1,
	SysMonAMS = 2
};
enum EConvType
{
	EConvType_None,
	EConvType_Raw_to_Scale,
	EConvType_Scale_to_Raw,
	EConvType_Max
};


struct Xadc_callback;

struct XadcThreshold
{
	const enum XADC_Alarm id;
	const char name[MAX_NAME_SIZE];
	float thres_low;
	float thres_high;
	int (* const conv_fn)(int, enum EConvType);
	bool isActive;
	struct Xadc_callback callback;
	pthread_mutex_t *lock;
};

#endif /* ZYNQMP_XADC_H_ */
