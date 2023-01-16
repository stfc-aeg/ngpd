
#ifndef _XDMA_HBM_HIST_H
#define _XDMA_HBM_HIST_H 1
#define XDmaHbmHistMaxChannels	4
#include <exception>
#include <stdint.h>
#include <stdarg.h>
#include "axi_hist.h"

#define HBM_HIST_MEM_BYPASS_SIZE (0x100000000L)
#define HBM_HIST_USER_SIZE (0x1000000L)
#define HBM_NUM_PORTS (16)
#define HBM_TOTAL_PORTS (32)

#define HBM_HIST_REGS_BASE 	 (0x60000)
#define HBM_HIST_TESTER_BASE (0x70000)

struct HistTesterRegs
{
	uint32_t histEnable;
	uint32_t control;
	uint32_t spare_rw0[6];
	uint32_t magic;
	uint32_t spare1;
	uint32_t histBusy;
	uint32_t testPatBusy;
	uint64_t histTimer[32];
};
#define HBM_HT_CONT_USE_LIST 	1
#define HBM_HT_CONT_DIS_UPPER	2

using namespace std;

class XDmaHbmException : public exception
{
	string m_errMsg;
public :
	XDmaHbmException(string s) : m_errMsg(s) {}
	// XDmaHbmException(const char *cp) : m_errMsg(cp) {}
	XDmaHbmException(const char * format, ...)
	{
		va_list args, args_copy;
		va_start(args, format);
		va_copy(args_copy, args);

		int len = vsnprintf(nullptr, 0, format, args);
		if (len <= 0)
		{
			m_errMsg = "vsnprintf failed";
		}
		else
		{
			m_errMsg.resize(len);
			vsnprintf(&m_errMsg[0], len+1, format, args_copy); // or m_errMsg.data() in C++17 and later...
		}
		va_end(args_copy);
		va_end(args);
	}

	virtual const char * what() const throw()
	{
		return m_errMsg.c_str(); 
	}
};

class XDmaMMap {
	string m_devName;
		int m_fd;
		size_t m_len;
	public :
		uint32_t *m_base;
		XDmaMMap();
		XDmaMMap(const char *deviceName, size_t size);
		~XDmaMMap();
};

class XDmaHbmHist {
	string m_devName;
	int m_num_h2c;
	int m_h2c_fd[XDmaHbmHistMaxChannels];
	int m_h2c_alignment[XDmaHbmHistMaxChannels];
	int m_num_c2h;
	int m_c2h_fd[XDmaHbmHistMaxChannels];
	int m_c2h_alignment[XDmaHbmHistMaxChannels];
	XDmaMMap *m_regsBAR;
	XDmaMMap *m_memBAR;
public :
	XDmaHbmHist (int);
	~XDmaHbmHist();
	void startClearStream(uint32_t portMask, uint32_t stream, uint32_t startWord, uint32_t numWords);
	void startClearAll(uint32_t portMask, uint32_t startWord, uint32_t numWords);
	void startClear(uint32_t portMask, uint32_t startWord, uint32_t num_words, int allStreams);
	void startClearTPG(uint32_t portMask, uint32_t startWord, uint32_t num_words, int allStreams);
	void readStreamDma(int port, int stream, uint32_t offset, uint32_t size, uint32_t *data, int dmaChan = 0, int useDmaReset=0);
	void writeStreamDma(int port, int stream, uint32_t offset, uint32_t size, uint32_t *data, int dmaChan = 0, int useDmaReset=0);
	void readStreamCpu(int port, int stream, uint32_t offset, uint32_t size, uint32_t *data);
	void writeStreamCpu(int port, int stream, uint32_t offset, uint32_t size, uint32_t *data);
	void startAXISReadStream(uint32_t portMask, uint32_t stream, uint32_t startWord, uint32_t numWords);
	void startAXISReadSeq(uint32_t portMask, uint32_t startWord, uint32_t numWords);

	void readAlignedDma(char *buffer, uint64_t numBytes, uint64_t base, int dmaChan=0 );
	void writeAlignedDma(char *buffer, uint64_t numBytes, uint64_t base, int dmaChan=0 );

	void waitClear();

	AXIHistConfig m_histConf;
	volatile struct AXIHistRegs *m_histRegs;
	volatile struct HistTesterRegs *m_histTester;
	uint32_t *m_histMemBase;
	int m_hasMemBAR;
	
	};

#endif
