// UltraTCP.h : header file
//

#define TECPOWERMASK  0x01
#define HEADPOWERMASK 0x02
#define BIASENABLEMASK 0x04
#define SYNCENABLEMASK 0x80000000
#define CALENABLEMASK 0x01
#define EN8PCMASK 0x06
#define TECOVERTEMPMASK 0x80000000

#define HEAD_SILICON 0
#define HEAD_INGAAS 1
#define HEAD_MCT 2
#define HEAD_MCT_512 3
#define HEAD_GAAS_POS 4
#define HEAD_GAAS_NEG 5

#define ULTRA_OK 0
#define ULTRA_ERROR (-1)

#define ULTRA_NUMPIXELS 512
#define ULTRA_WORDSIZE 2
#define ULTRA_UDP_PKT_DATA_OFFSET 6
#define ULTRA_DATA_SIZE (ULTRA_WORDSIZE*ULTRA_NUMPIXELS)
#define ULTRA_PKT_SIZE (ULTRA_DATA_SIZE+ULTRA_UDP_PKT_DATA_OFFSET)
#define ULTRA_NUMFRAMES 10000

#define TELNETPORT 7
#define HEADPORT 5000
#define HOSTPORT 5005
#define XH_MAX_TIMING_GROUPS 1024
#define XH_NUM_HEAD_PCB 1
#define XH_NUM_READOUT_CHAN 16
#define XH_DMA_CONT_16BIT 1

#define XH_S1_PW_MIN 5
#define XH_S1_PW_MAX 0xFFFF
#define XH_S2_PW_MIN 5
#define XH_S2_PW_MAX 0xFFFF
#define XH_RST_MIN   2

#define XH_XCLK_HP_MIN 1
#define XH_XCLK_HP_MAX 255
#define XH_XCLK_STALL_MAX 255
#define XH_CYCLES_MAX 0x40000

#define XH_TRIG_CONT_TRIG_MUX_SEL(x)	(((x)&0xF)<<0)
#define XH_TRIG_CONT_ENB_GROUP_TRIG		(1<<4)
#define XH_TRIG_CONT_ENB_FRAME_TRIG		(1<<5)
#define XH_TRIG_CONT_ENB_SCAN_TRIG		(1<<6)
#define XH_TRIG_CONT_TRIG_FALLING		(1<<7)
#define XH_TRIG_CONT_ORBIT_MUX_SEL(x)	(((x)&3)<<8)
#define XH_TRIG_CONT_ENB_GROUP_ORBIT	(1<<10)
#define XH_TRIG_CONT_ENB_FRAME_ORBIT	(1<<11)
#define XH_TRIG_CONT_ENB_SCAN_ORBIT		(1<<12)
#define XH_TRIG_CONT_GET_TRIG_MUX_SEL(x)	(((x)>>0)&0xF)

#define XH_TRIG_MAX_TRIG_MUX_SEL  9
#define XH_TRIG_MAX_ORBIT_MUX_SEL 3

#define XH_TRIG_SEL_ORBIT_DELAYED 8
#define XH_TRIG_SEL_SOFTWARE      9

#define XH_TRIG_STAT_IDLE 0
#define XH_TRIG_STAT_RUNNING 1
#define XH_TRIG_STAT_PAUSED 2


#define XH_HIDE_GET_MODE(x) ((x)&0xF)
#define XH_HIDE_MODE_NONE       		0
#define XH_HIDE_MODE_MUXRESET   		1
#define XH_HIDE_MODE_RST_S1S2F  		2
#define XH_HIDE_MODE_ALL_EDGES  		3
#define XH_HIDE_MODE_NO_OVERLAP 		4
#define XH_HIDE_MODE_DELAY_MUXRESET 	5
#define XH_HIDE_MODE_DEL_MR_ALIGN   	6
#define XH_HIDE_MODE_34_XCLK_PRE    	7
#define XH_HIDE_MODE_33_XCLK_PRE    	8
#define XH_HIDE_MODE_33_XCLK_PRE_ALIGN  9
#define XH_HIDE_MODE_FULLY_OVERLAPPED   10
#define XH_HIDE_MODE_RST_S1S2F_STALL	11
#define XH_HIDE_MODE_DEL_MR_STALL   	12
#define XH_HIDE_MODE_DEL_MR_NO_OVERLAP  13
#define XH_HIDE_MODE_DEL_MR_STALL_SHORT 14
#define XH_HIDE_MODE_MAX  XH_HIDE_MODE_DEL_MR_STALL_SHORT

#define XH_HIDE_LONG_S2			0x10
#define XH_HIDE_RST_FIRST		0x20
#define XH_HIDE_NO_ADJUST		0x40

#define XH_TIMING_MAX_FIXED_HIGH	31			//!< Max length of fixed high
#define XH_MAX_AUX_DELAY 0xFFFF

#define XH_RETRIGGER_LATENCY 7

// This class is exported 

//class __declspec(dllexport) UltraTCP  : public CSocket {
//public:
//	UltraTCP(void);
//	virtual ~UltraTCP();
//	void hello();
	int UltraTCP_InitialiseLink(int sys, char *, char *, unsigned int);
	void UltraTCP_CloseLink(int sys);
	int UltraTCP_GetHeadColdTemp(int sys, float *);
	int UltraTCP_GetHeadHotTemp(int sys, float *);
	int UltraTCP_GetAdcOff(int sys, unsigned int Channel, float *value);
	int UltraTCP_GetAdcRef(int sys, unsigned int Channel, float *value);
	int UltraTCP_SetAdcGain(int sys, unsigned int Channel, float Gain);
	int UltraTCP_SetAdcOffset(int sys, unsigned int Channel, float Offset);

	void UltraTCP_AdcChanLookup(int sys, int, int *, int *);
	int UltraTCP_GetHeadVdd(int sys, float *);
	int UltraTCP_GetHeadADCVdd(int sys, float *);
	int UltraTCP_GetHeadVref(int sys, float *);
	int UltraTCP_GetHeadVrefc(int sys, float *);
	int UltraTCP_GetHeadVpupref(int sys, float *);
	int UltraTCP_GetHeadVclamp(int sys, float *);
	int UltraTCP_GetHeadVres1(int sys, float *);
	int UltraTCP_GetHeadVres2(int sys, float *);
	int UltraTCP_GetHeadVTrip(int sys, float *);
	int UltraTCP_SetHeadVdd(int sys, float);
	int UltraTCP_SetHeadVref(int sys, float);
	int UltraTCP_SetHeadVrefc(int sys, float);
	int UltraTCP_SetHeadVpupref(int sys, float);
	int UltraTCP_SetHeadVclamp(int sys, float);
	int UltraTCP_SetHeadVres1(int sys, float);
	int UltraTCP_SetHeadVres2(int sys, float);
	int UltraTCP_SetHeadVTrip(int sys, float);
	int UltraTCP_GetAux1(int sys, unsigned int *, unsigned int *);
	int UltraTCP_GetAux2(int sys, unsigned int *, unsigned int *);
	int UltraTCP_GetTecColdTemp(int sys, float *);
	int UltraTCP_GetTecSupplyVolts(int sys, float *);
	int UltraTCP_GetAdcPosSupplyVolts(int sys, float *);
	int UltraTCP_GetAdcNegSupplyVolts(int sys, float *);
	int UltraTCP_GetVinPosSupplyVolts(int sys, float *);
	int UltraTCP_GetVinNegSupplyVolts(int sys, float *);
	int UltraTCP_SetAux1(int sys, unsigned int, unsigned int);
	int UltraTCP_SetAux2(int sys, unsigned int, unsigned int);
	int UltraTCP_GetFpgaPwrReg(int sys, unsigned int *);
	int UltraTCP_GetFpgaSyncReg(int sys, unsigned int *);
	int UltraTCP_GetFpgaAdcReg(int sys, unsigned int *);
	int UltraTCP_GetFpgaXchipReg(int sys, unsigned int *);
	int UltraTCP_GetFrameCount(int sys, unsigned int *);
	int UltraTCP_GetFrameErrorCount(int sys, unsigned int *);
	int UltraTCP_SetFpgaPwrReg(int sys, unsigned int);
	int UltraTCP_SetFpgaSyncReg(int sys, unsigned int);
	int UltraTCP_SetFpgaAdcReg(int sys, unsigned int);
	int UltraTCP_SetFpgaXchipReg(int sys, unsigned int);
	int UltraTCP_IsTecPowerEnabled(int sys);
	int UltraTCP_IsHeadPowerEnabled(int sys);
	int UltraTCP_IsBiasEnabled(int sys);
	int UltraTCP_IsSyncEnabled(int sys);
	int UltraTCP_IsCalibEnabled(int sys);
	int UltraTCP_Is8pCEnabled(int sys);
	int UltraTCP_IsTecOverTemp(int sys);
	int UltraTCP_EnableTecPower(int sys, unsigned int);
	int UltraTCP_EnableHeadPower(int sys, unsigned int);
	int UltraTCP_EnableBias(int sys, unsigned int);
	int UltraTCP_EnableSync(int sys, unsigned int);
	int UltraTCP_EnableCalib(int sys, unsigned int);
	int UltraTCP_Enable8pC(int sys, unsigned int);
	int UltraTCP_GetXchipTiming(int sys, unsigned int *, unsigned int *, unsigned int *,
			   unsigned int *, unsigned int *, unsigned int *, unsigned int *,
			   unsigned int, unsigned int *, unsigned int *);
	int UltraTCP_SetXchipTiming(int sys, int gnum, int force);
	int UltraTCP_SaveConfiguration(int sys);
	int UltraTCP_RecallConfiguration(int sys);
	int UltraTCP_ReadHeadType(int sys);

	int UltraUDP_InitialiseLink(int sys, char *, unsigned int);
	int UltraUDP_InitialiseModule(int sys, int num_t);
	void *UltraUDP_ThreadRx_function(void *ptr);

	XHState UltraTCP_TimingStart(int sys);
	int UltraTCP_TimingStop(int sys);
	int UltraTCP_TimingReadStatus(int sys, unsigned int *completedFrames, int *collectingStatus);
	XHState UltraTCP_TimingContinue(int sys);


