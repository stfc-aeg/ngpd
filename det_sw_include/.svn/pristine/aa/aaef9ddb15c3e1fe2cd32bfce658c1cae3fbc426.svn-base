// UltraTCP.h : header file
//

#define TECPOWERMASK  0x01l
#define HEADPOWERMASK 0x02l
#define BIASENABLEMASK 0x04l
#define SYNCENABLEMASK 0x80000000l
#define CALENABLEMASK 0x01l
#define EN8PCMASK 0x06l
#define TECOVERTEMPMASK 0x80000000l

#define HEAD_SILICON 0
#define HEAD_INGAAS 1
#define HEAD_MCT 2

#define ULTRA_OK 0
#define ULTRA_ERROR -1

#define ULTRA_NUMPIXELS 512
#define ULTRA_WORDSIZE 2
#define ULTRA_UDP_PKT_DATA_OFFSET 6
#define ULTRA_DATA_SIZE (ULTRA_WORDSIZE*ULTRA_NUMPIXELS)
#define ULTRA_PKT_SIZE (ULTRA_DATA_SIZE+ULTRA_UDP_PKT_DATA_OFFSET)
#define ULTRA_NUMFRAMES 10000


// This class is exported 

//class __declspec(dllexport) UltraTCP  : public CSocket {
//public:
//	UltraTCP(void);
//	virtual ~UltraTCP();
//	void hello();
	int UltraTCP_InitialiseLink(char *, char *, unsigned int);
	void UltraTCP_CloseLink(void);
	int UltraTCP_GetHeadColdTemp(float *);
	int UltraTCP_GetHeadHotTemp(float *);
	int UltraTCP_GetAdcRef(unsigned int, int, float *);
	int UltraTCP_GetAdcOff(unsigned int, int, float *);
	int UltraTCP_SetAdcGain(unsigned int, float, int);
	int UltraTCP_SetAdcOffset(unsigned int, float, int);
	void UltraTCP_AdcChanLookup(int, int, int *, int *);
	int UltraTCP_GetHeadVdd(float *);
	int UltraTCP_GetHeadADCVdd(float *);
	int UltraTCP_GetHeadVref(float *);
	int UltraTCP_GetHeadVrefc(float *);
	int UltraTCP_GetHeadVpupref(float *);
	int UltraTCP_GetHeadVclamp(float *);
	int UltraTCP_GetHeadVres1(float *);
	int UltraTCP_GetHeadVres2(float *);
	int UltraTCP_GetHeadVTrip(float *);
	int UltraTCP_SetHeadVdd(float);
	int UltraTCP_SetHeadVref(float);
	int UltraTCP_SetHeadVrefc(float);
	int UltraTCP_SetHeadVpupref(float);
	int UltraTCP_SetHeadVclamp(float);
	int UltraTCP_SetHeadVres1(float);
	int UltraTCP_SetHeadVres2(float);
	int UltraTCP_SetHeadVTrip(float);
	int UltraTCP_GetAux1(unsigned int *, unsigned int *);
	int UltraTCP_GetAux2(unsigned int *, unsigned int *);
	int UltraTCP_GetTecColdTemp(float *);
	int UltraTCP_GetTecSupplyVolts(float *);
	int UltraTCP_GetAdcPosSupplyVolts(float *);
	int UltraTCP_GetAdcNegSupplyVolts(float *);
	int UltraTCP_GetVinPosSupplyVolts(float *);
	int UltraTCP_GetVinNegSupplyVolts(float *);
	int UltraTCP_SetAux1(unsigned int, unsigned int);
	int UltraTCP_SetAux2(unsigned int, unsigned int);
	int UltraTCP_GetFpgaPwrReg(unsigned int *);
	int UltraTCP_GetFpgaSyncReg(unsigned int *);
	int UltraTCP_GetFpgaAdcReg(unsigned int *);
	int UltraTCP_GetFpgaXchipReg(unsigned int *);
	int UltraTCP_GetFrameCount(unsigned int *);
	int UltraTCP_GetFrameErrorCount(unsigned int *);
	int UltraTCP_SetFpgaPwrReg(unsigned int);
	int UltraTCP_SetFpgaSyncReg(unsigned int);
	int UltraTCP_SetFpgaAdcReg(unsigned int);
	int UltraTCP_SetFpgaXchipReg(unsigned int);
	int UltraTCP_IsTecPowerEnabled(void);
	int UltraTCP_IsHeadPowerEnabled(void);
	int UltraTCP_IsBiasEnabled(void);
	int UltraTCP_IsSyncEnabled(void);
	int UltraTCP_IsCalibEnabled(void);
	int UltraTCP_Is8pCEnabled(void);
	int UltraTCP_IsTecOverTemp(void);
	int UltraTCP_EnableTecPower(unsigned int);
	int UltraTCP_EnableHeadPower(unsigned int);
	int UltraTCP_EnableBias(unsigned int);
	int UltraTCP_EnableSync(unsigned int);
	int UltraTCP_EnableCalib(unsigned int);
	int UltraTCP_Enable8pC(unsigned int);
	int UltraTCP_GetXchipTiming(unsigned int *, unsigned int *, unsigned int *,
			   unsigned int *, unsigned int *, unsigned int *, unsigned int *,
			   unsigned int, unsigned int *, unsigned int *);
	int UltraTCP_SetXchipTiming(unsigned int, unsigned int, unsigned int,
			   unsigned int, unsigned int, unsigned int, unsigned int,
			   unsigned int, unsigned int);
	int UltraTCP_SaveConfiguration(void);
	int UltraTCP_RecallConfiguration(void);
	int UltraTCP_ReadHeadType(void);

	int UltraUDP_InitialiseLink(char *, unsigned int);
	int UltraUDP_InitialiseModule(void);
	void *UltraUDP_ThreadRx_function(void *ptr);

	int UltraTCP_TimingStart(void);
	int UltraTCP_TimingStop(void);
	int UltraTCP_SetFrames(unsigned int);
	int UltraTCP_TimingReadStatus(unsigned int *completedFrames, int *collectingStatus);
	static int getValue(const char *cmd, float *value);


//protected:
//	char recvBuf[100];
//	int sock;

//private:
//	float getValue(char*);
//	int setValue(char *cmd);
//	int getRegisterValue(char *cmd, unsigned int *value);
//	int getUnsignedValues(char *cmd, unsigned int *value1, unsigned int *value2);

//};
//W32DLLTESTPROJ2_API int fnW32dlltestproj2(void);

