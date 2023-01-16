/*
 * fem.h
 *
 * Main header file for FEM card
 * Includes all base address definitions from xparameters.h
 *
 */

#ifndef FEM_H_
#define FEM_H_

//#include "xparameters.h"

// ****************************************************************************
// This define controls whether we are running on an ML507 dev. board, or a real FEM.
// For use on FEM, comment this line out!
// ****************************************************************************
//#define HW_PLATFORM_DEVBOARD
// ****************************************************************************

// Hardware base addresses, redefine xparameters.h

#if 0
// FEM devices
#define BADDR_I2C_LM82				XPAR_IIC_LM82_BASEADDR
#define BADDR_I2C_EEPROM			XPAR_IIC_EEPROM_BASEADDR
#define BADDR_I2C_SP3_TOP			XPAR_IIC_POWER_RHS_BASEADDR
#define BADDR_I2C_SP3_BOT			XPAR_IIC_POWER_LHS_BASEADDR
#define BADDR_MAC					XPAR_LLTEMAC_0_BASEADDR
#define BADDR_INTC					XPAR_XPS_INTC_2_BASEADDR
#define BADDR_RDMA					XPAR_RS232_UART_PPC2_RDMA_BASEADDR
#define BADDR_MBOX					XPAR_MAILBOX_0_IF_1_BASEADDR
#define XINTC_ID					XPAR_XPS_INTC_2_DEVICE_ID
#define XSYSACE_ID					XPAR_SYSACE_0_DEVICE_ID
#define MBOX_ID						XPAR_MAILBOX_0_IF_1_DEVICE_ID
#define MBOX_RECV_ID				XPAR_MAILBOX_0_IF_1_RECV_FSL
#define MBOX_SEND_ID				XPAR_MAILBOX_0_IF_1_SEND_FSL
#define MBOX_USE_FSL				XPAR_MAILBOX_0_IF_1_USE_FSL
// On XSPRESS3 to RHS power card I2C bus is used for the FMC.
#define BADDR_I2C_FMC				XPAR_IIC_POWER_RHS_BASEADDR
#endif

// DDR2 memory size
#define FEM_DDR2_START				0x00000000
#define FEM_DDR2_END				0xFFFFFFFF

// Enable / disable serial debugging output (comment to disable)


// Define an absolute maximum temperature threshold for the LM82.
// Overrides the value in EEPROM if (EEPROM_CRIT_TEMP > CRIT_TEMP_MAX)
#define CRIT_TEMP_MAX				90

// FEM Networking parameters
#define NET_MAX_CLIENTS				8

#define NET_SOCK_BACKLOG			NET_MAX_CLIENTS

#define NET_DEFAULT_TICK_SEC		2
#define NET_DEFAULT_TICK_USEC		0
#define NET_DEFAULT_TIMEOUT_LIMIT	5					// In ticks

// Self-test bits - if set these represent a failure during hardware init!
#define TEST_XPSTIMER_INIT			0x0001
#define TEST_TIMER_CALIB			0x0002
#define TEST_XINTC_INIT				0x0004
#define TEST_XINTC_START			0x0008
#define TEST_RDMA_UART_OK			0x0010
#define TEST_RDMA_READBACK			0x0020
#define TEST_SYSACE_INIT			0x0040
#define TEST_SYSACE_BIST			0x0080

#define TEST_EEPROM_CFG_READ		0x8000

#define XSPRESS3	1

#endif /* FEM_H_ */

