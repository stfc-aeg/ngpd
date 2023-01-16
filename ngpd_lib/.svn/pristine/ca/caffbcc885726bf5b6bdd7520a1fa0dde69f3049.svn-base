/*
 * commandProcessor.c
 *
 * Manages the decoding of received packets and generation
 * of response packets using LwIP library
 *
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

#include "fem.h"
#include "ngzmp.h"
#include "zynqmp_protocol.h"
#include "commandProcessor.h"
#include "ngzmp_dma_protocol.h"
#include "zynqmp_lib_driver.h"
/* Manages the connection of clients to the FEM-II and
 * the reception / validation / response generation of
 * packets.
 *
 */
u_int32_t commandHandler(struct zynqmp_protocol_header* pRxHeader, struct zynqmp_protocol_header* pTxHeader, u_int8_t* pRxPayload, u_int8_t* pTxPayload);

extern NGZMPFanControl fan_cont ;
extern int dev_board;

int msg_level = MSG_SPARSE;
//int msg_level = MSG_VERBOSE;
void command_processor()
{
	int listenerSocket, clientSocket, newFd, addrSize, maxFds, numBytesRead, numBytesToRead, i, j;
	struct sockaddr_in serverAddress, clientAddress;
	fd_set readSet, masterSet;
	struct timeval tv;
	int numConnectedClients = 0;
	int opt_val=1;
	int rc;
	// Setup and initialise client statuses to idle
	struct clientStatus state[NET_MAX_CLIENTS+3]; // Allow space ofr stdin, stdout, stderr
	struct clientStatus *pState;
	// RX and TX packet buffers
	// TODO: Tidy these and change signature of commandHandler()!
	u_int8_t* pTxBuffer = (u_int8_t*)malloc(sizeof(struct zynqmp_protocol_header)+ZYNQMP_FROM_FEM_BUFFER_MAX);
	struct zynqmp_protocol_header* pTxHeader = (struct zynqmp_protocol_header*)pTxBuffer;

	for (j=0; j<NET_MAX_CLIENTS+3; j++)
	{
		state[j].state = STATE_COMPLETE;
		state[j].hdr_bytes_read = 0;
		state[j].payload_bytes_read = 0;
		state[j].timeoutCount = 0;
		state[j].payloadBufferSz = 0;
		state[j].pPayload = NULL;
	}


	// Prepare file descriptor sets
	FD_ZERO(&readSet);
	FD_ZERO(&masterSet);
	maxFds = -1;

	DBGLEVEL(MSG_QUIET, "CmdProc: Thread starting...\r\n");

	// Open server socket, stream mode
	if ((listenerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		DBGLEVEL(MSG_ERROR, "CmdProc: Can't open socket, aborting...\r\n");
		DBGLEVEL(MSG_ERROR, "Terminating thread...\r\n");
		// TODO: Failing to open a socket is a critical failure!  We could put a retry loop here though...?
		return;
	}

	// Configure socket to receive any incoming connections
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(ZYNQMP_CMD_PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	memset(&(serverAddress.sin_zero), 0, sizeof(serverAddress.sin_zero));

	// Bind to address
	if (bind(listenerSocket, (struct sockaddr *)&serverAddress, sizeof (serverAddress)) < 0)
	{
		DBGLEVEL(MSG_ERROR, "CmdProc: Can't bind to socket %d!\n", ZYNQMP_CMD_PORT);
		DBGLEVEL(MSG_ERROR, "Terminating thread...\n");
		// TODO: Failing to bind to a socket is a critical failure!  Should we exit?
		return;
	}

	// Begin listening, register listener as FD of interest to read
	if (listen(listenerSocket, NET_SOCK_BACKLOG) < 0)
	{
		DBGLEVEL(MSG_ERROR, "CmdProc: Can't listen on socket %d!\n", ZYNQMP_CMD_PORT);
		DBGLEVEL(MSG_ERROR, "Terminating thread...\n");
		// TODO: Failing to listen on a socket is a critical failure!  Should we exit?
		return;
	}
	FD_SET(listenerSocket, &masterSet);
	maxFds = listenerSocket;

	DBGLEVEL(MSG_QUIET, "CmdProc: Socket on port %d ready, awaiting clients (max. clients = %d).\n", ZYNQMP_CMD_PORT, NET_MAX_CLIENTS);

	// ************************************************** MAIN SERVER LOOP ********************************************************
	while (1) {

		// Show our tick over
		//DBGLEVEL(".");

		// Copy master set over as readSet will be modified
		memcpy(&readSet, &masterSet, (size_t)sizeof(fd_set));

		// Reset timeval for select call as it may be overwritten
		tv.tv_sec =  NET_DEFAULT_TICK_SEC;
		tv.tv_usec = NET_DEFAULT_TICK_USEC;

		if ((rc=select(maxFds + 1, &readSet, NULL, NULL, &tv)) == -1)
		{
			DBGLEVEL(MSG_ERROR, "CmdProc: ERROR - Select failed!\n");
			// TODO: What to do here?
		}
		else if (rc == 0)
		{
			/* There has beena timeout. OK unless we have a descriptor waiting for a payload which is never comming */
			{
				for (i=1; i<=maxFds; i++)
				{
					pState = &(state[i-1]);
					if (pState->state == STATE_HDR_VALID || pState->state == STATE_START)
					{
						pState->timeoutCount++;
						if (pState->timeoutCount > NET_DEFAULT_TIMEOUT_LIMIT && 0)
						{
							// Client has timed out :(
							DBGLEVEL(MSG_ERROR, "CmdProc: Client #%d - timed out on operation.\n", j+1);
							pState->state = STATE_COMPLETE;
						}
					}
				}
			}
		}


		// Check file descriptors, see which one needs servicing
		for (i=0; i<=maxFds; i++)
		{
			if(FD_ISSET(i, &readSet))
			{

				// ----------------------------------------------------------------------------------------------------
				// This is a new connection
				if (i==listenerSocket)
				{

					// Accept connection
					addrSize = sizeof(clientAddress);
					newFd = accept(listenerSocket, (struct sockaddr*)&clientAddress, (socklen_t*)&addrSize);
					if (newFd == -1)
					{
						DBGLEVEL(MSG_ERROR, "CmdProc: Error accepting connection!");
						// TODO: Do we need further error handling here?
					}
					else
					{
						DBGLEVEL(MSG_SPARSE, "CmdProc: Received new connection! (Client #%d), fd is %d\n", numConnectedClients+1, newFd);
						// Check if we can accept more client connections
						if (numConnectedClients == NET_MAX_CLIENTS)
						{

							// Close connection
							DBGLEVEL(MSG_ERROR, "CmdProc: Client attempted to connect but can't accept any more connections!\n");
							close(newFd);

						}
						else
						{

							// Add to master list of FDs, update count
							FD_SET(newFd, &masterSet);
							if (newFd > maxFds)
							{
								maxFds = newFd;
							}

							// Add to client status list, malloc header and payload buffers (keep separate, simplifies pointer handling!)
							state[newFd-1].pHdr = malloc(sizeof(struct zynqmp_protocol_header));
							if (state[newFd-1].pHdr == NULL)
							{
								// Can't allocate payload space
								DBGLEVEL(MSG_ERROR, "CmdProc: Can't malloc header buffer for client %d!\n", newFd);
								DBGLEVEL(MSG_ERROR, "Terminating thread...\n");
								// TODO: Don't return, handle gracefully (NACK + disconnect?)
								return;
							}
							state[newFd-1].pPayload = malloc(ZYNQMP_TO_FEM_BUFFER_BYTES);
							if (state[newFd-1].pPayload == NULL)
							{
								// Can't allocate payload space
								DBGLEVEL(MSG_ERROR, "CmdProc: Can't malloc payload buffer for client %d!\n", newFd);
								DBGLEVEL(MSG_ERROR, "Terminating thread...\n");
								// TODO: Don't return, handle gracefully (NACK + disconnect?)
								return;
							}
							opt_val=1;
							if (setsockopt(newFd, IPPROTO_TCP, TCP_NODELAY, &opt_val, sizeof(int)) < 0)
								DBGLEVEL(MSG_ERROR, "CmdProc: Can't setsockopt client %d!\n", newFd);
							numConnectedClients++;

							state[newFd-1].state = STATE_COMPLETE;		// This causes reset at beginning of receive from existing client loop!

						}
					}

				}
				else
				// ----------------------------------------------------------------------------------------------------
				// Existing client is communicating with us
				{

					clientSocket = i;

					pState = &(state[i-1]);

					// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

					// Determine if the last packet was received from client, if so reset client status
					if (pState->state == STATE_COMPLETE)
					{

						// DBGLEVEL(MSG_VVERBOSE, "***** STATE_COMPLETE ****\n");

						// Check if previous transaction had an increased payload buffer size and free it if necessary
						if (pState->payloadBufferSz > ZYNQMP_TO_FEM_BUFFER_BYTES)
						{
							free(pState->pPayload);
							pState->pPayload = malloc(ZYNQMP_TO_FEM_BUFFER_BYTES);
							if (pState->pPayload == NULL)
							{
								// Can't allocate payload space
								DBGLEVEL(MSG_ERROR, "CmdProc: Can't re-malloc payload buffer for client after large packet!\n");
								DBGLEVEL(MSG_ERROR, "Terminating thread...\n");
								return;
							}
							else
							{
								DBGLEVEL(MSG_VERBOSE, "CmdProc: Resized payload buffer to %d\n", ZYNQMP_TO_FEM_BUFFER_BYTES);
							}
						}

						// Re-initialise client state
						pState->state = STATE_START;
						pState->payloadBufferSz = ZYNQMP_TO_FEM_BUFFER_BYTES;
						pState->hdr_bytes_read = 0;
						pState->payload_bytes_read = 0;
						pState->timeoutCount = 0;
						//DBGLEVEL(MSG_VVERBOSE, "CmdProc: Receiving from client #%d.\n", i);
					} // END if state == STATE_COMPLETE



					/* ------------------------------------------------------------------------------------------------
					 *
					 * Packet processing is achieved using a state machine with 5 states:
					 *
					 * 		STATE_START			A client is connected and we are ready to receive a request from it
					 * 			-> Client sends us a header
					 *
					 * 		STATE_GOT_HEADER	A client has begun communicating with us and we have received a header from it
					 * 			-> Header is validated
					 *
					 * 		STATE_HDR_VALID		A client send us a header, which we received, and we have verified it is valid
					 * 			-> Payload of size specified in header is sent
					 *
					 * 		STATE_GOT_PAYLD		A client has sent us a payload for the header we already received
					 * 			-> Command is processed either by commandProcessor, or handed off to the FPM for handling
					 *
					 * 		STATE_COMPLETE		We have finished receiving header + payload from the client, so become idle
					 *
					 * 		If errors are encountered during any state, it is handled and the client put into STATE_COMPLETE.
					 * 		New clients are also put into STATE_COMPLETE.
					 *
					 * ------------------------------------------------------------------------------------------------
					 */

					// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

					// Manage reception of header
					if (pState->state == STATE_START)
					{
						numBytesToRead = sizeof(struct zynqmp_protocol_header) - pState->hdr_bytes_read;
						DBGLEVEL(MSG_VERBOSE, "CmdProc: Trying to get %d bytes of header...\n", numBytesToRead);

						numBytesRead = recv(i, ((u_int8_t*)(pState->pHdr)) + pState->hdr_bytes_read, numBytesToRead, MSG_DONTWAIT);

						DBGLEVEL(MSG_VERBOSE, "Read %d bytes\n", numBytesRead);
						if (numBytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
							continue;	// This is a rare case when we get a dat ready from selectm but it is not there when we look for it, which is rare but can happen. No data so try the next fd in the select set.
						if (numBytesRead <= 0)
						{
							// Client has disconnected or an error
							disconnectClient(&state[i-1], &i, &masterSet, &numConnectedClients);
						}
						else if (numBytesRead < numBytesToRead)
						{
							///	pState->timeoutCount++;
						}

						pState->hdr_bytes_read += numBytesRead;

						// If we have full header move to next state
						if (pState->hdr_bytes_read == sizeof(struct zynqmp_protocol_header))
						{
							pState->state = STATE_GOT_HEADER;
							pState->payload_bytes_read = 0;
						}

					} // END if state == STATE_START

					// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

					// Validate header
					if (pState->state == STATE_GOT_HEADER)
					{

						//DBGLEVEL(MSG_VVERBOSE, "IN STATE_GOT_HEADER\n");
						// Validate
						if(validateHeaderContents(pState->pHdr)==0)
						{
							if (pState->pHdr->command == ZYNQMP_CMD_WRITE || pState->pHdr->command == ZYNQMP_CMD_PERSONALITY_WRITE)
								pState->payload_bytes_required = pState->pHdr->payload_sz;
							else
								pState->payload_bytes_required = 0;

							// Header is valid!
							if (pState->pHdr->bus_target==ZYNQMP_BUS_DIRECT && pState->pHdr->command == ZYNQMP_CMD_WRITE)
							{
								pState->BusDirectSize = pState->payload_bytes_required;
								pState->pBusDirect = (u_int8_t *)pState->pHdr->address;
								DBGLEVEL(MSG_VERBOSE, "CmdProc: Got a ZYNQMP_BUS_DIRECT, ZYNQMP_CMD_WRITE!\n");
							}
							pState->state = STATE_HDR_VALID;
							DBGLEVEL(MSG_VERBOSE, "CmdProc: Header is valid.\n");
						}
						else
						{
							// Header NOT valid
							DBGLEVEL(MSG_ERROR, "CmdProc: Header received but is invalid.\n");
							DUMPHDR(pState->pHdr);
							pState->state = STATE_COMPLETE;
						}

					} // END if state == STATE_GOT_HEADER

					// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

					// Header is OK so try to get payload for packet
					if (pState->state == STATE_HDR_VALID)
					{

						//DUMPHDR(pState->pHdr);

						// TODO: Check address is in valid range FEM_DDR2_START => addr => (FEM_DDR2_END-payload_sz)
						// TODO: We also need to reject an address of 0 as lwip sees this as a NULL pointer and doesn't do the rxbuf->appbuff copy!

						// Check if this is a direct memory receive, if so handle it
						if (pState->pHdr->bus_target==ZYNQMP_BUS_DIRECT && pState->pHdr->command == ZYNQMP_CMD_WRITE)
						{
							printf("Bus direct not fully supported yet\n");
							exit(1);
							DBGLEVEL(MSG_NORMAL, "CmdProc: Bus Direct Write: Trying to read %d bytes to 0x%p...\n", pState->BusDirectSize, pState->pBusDirect);
							// Read payload directly to DDR at specified address
							numBytesRead = recv(i, (void*)(pState->pBusDirect), pState->BusDirectSize, MSG_DONTWAIT);
							DBGLEVEL(MSG_VERBOSE, "CmdProc: Bus Direct Write: Read %d bytes...\n", numBytesRead);
							if (numBytesRead == EAGAIN || numBytesRead == EWOULDBLOCK)
								continue;	// No data so try the next fd in the select set.
							if (numBytesRead <= 0)
							{
								// Client has disconnected or an error
								disconnectClient(&state[i-1], &i, &masterSet, &numConnectedClients);
								continue;

							}
							pState->pBusDirect += numBytesRead;
							pState->BusDirectSize -= numBytesRead;
							if (pState->BusDirectSize == 0)
							{
								// Bypass normal reception of payload as we already have it
								pState->state = STATE_GOT_PYLD;
							}

						}
						else
						{

							if (pState->payload_bytes_required > ZYNQMP_TO_FEM_BUFFER_MAX)
							{
								DBGLEVEL(MSG_ERROR, "CmdProc: payload_sz %d exceeds maximum (%d), stopping processing packet.\n", pState->payload_bytes_required, ZYNQMP_TO_FEM_BUFFER_MAX);
								// TODO: Send NACK here!
								pState->state = STATE_COMPLETE;
							}
							else
							{

								if (pState->payload_bytes_required > pState->payloadBufferSz)
								{
									pState->pPayload = realloc(pState->pPayload, pState->payload_bytes_required);
									pState->payloadBufferSz = pState->payload_bytes_required;
									if (pState->pPayload == NULL)
									{
										DBGLEVEL(MSG_ERROR, "CmdProc: Fatal error - can't realloc rx buffer!\n");
										DBGLEVEL(MSG_ERROR, "Terminating thread...\n");
										// TODO: Send NACK here!
										return;
									}
									DBGLEVEL(MSG_VERBOSE, "CmdProc: Resized payload buffer to %d\n", pState->payloadBufferSz);

								}
								numBytesToRead = pState->payload_bytes_required - pState->payload_bytes_read;

								if (numBytesToRead != 0)
								{
									DBGLEVEL(MSG_VERBOSE, "CmdProc: Trying to get %d bytes of payload (payload_sz=%d)\n", numBytesToRead, pState->payload_bytes_required);
									numBytesRead = recv(i, pState->pPayload + pState->payload_bytes_read, numBytesToRead, MSG_DONTWAIT);

									if (numBytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
										continue;	// No data so try the next fd in the select set.

									if (numBytesRead <= 0)
									{
										// Client has disconnected or ERROR
										disconnectClient(&state[i-1], &i, &masterSet, &numConnectedClients);
									}
									else
									{
										u_int8_t * p = pState->pPayload + pState->payload_bytes_read;
										u_int8_t * q = pState->pPayload + pState->payload_bytes_read+numBytesRead-8;
										DBGLEVEL(MSG_VERBOSE, "CmdProc: Read %d bytes of %d as payload.\n", numBytesRead, numBytesToRead);
										if (numBytesRead > 8)
											DBGLEVEL(MSG_VERBOSE, "CmdProc: .... First bytes %02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X last byte %02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X\n", 
												p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7]);
											
									}

									pState->payload_bytes_read += numBytesRead;
								}

								// Check if this is the entire payload received
								if (pState->payload_bytes_read == pState->payload_bytes_required)
								{
									pState->state = STATE_GOT_PYLD;
									DBGLEVEL(MSG_VERBOSE, "CmdProc: Finished receiving payload!\n");
								}
								else
								{
									// Still more data to receive so re-enter select call
									continue;
								}
							}
						}

					} // END if state == STATE_HDR_VALID

					// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

					if (pState->state == STATE_GOT_PYLD)
					{
						int payloadBytes;
						int bytes_to_send, bytes_sent;
						u_int8_t *bp;

						DBGLEVEL(MSG_VERBOSE, "CmdProc: Received entire packet...\n");
						memcpy(pTxHeader, pState->pHdr, sizeof(struct zynqmp_protocol_header));

						// Generate response
						if (pState->pHdr->command == ZYNQMP_CMD_PERSONALITY_READ  || pState->pHdr->command == ZYNQMP_CMD_PERSONALITY_WRITE || pState->pHdr->command == ZYNQMP_CMD_PERSONALITY_WRITE_EMB)
						{
							// Hand off to personality module
							rc = handlePersonalityCommand(pState->pHdr, pTxHeader, pState->pPayload, pTxBuffer+sizeof(struct zynqmp_protocol_header));
						}
						else
						{
							// Process request ourselves
							rc = commandHandler(pState->pHdr, pTxHeader, pState->pPayload, pTxBuffer+sizeof(struct zynqmp_protocol_header));
						}
						pTxHeader->num_ops = rc;
						if (pState->pHdr->command == ZYNQMP_CMD_PERSONALITY_READ || pState->pHdr->command == ZYNQMP_CMD_READ || pState->pHdr->command == ZYNQMP_CMD_READ_INTL )
							payloadBytes = pTxHeader->payload_sz;	// Filled in by commandHandler, but with success is the same as the Received packet.
						else
							payloadBytes = 0;
					
						DBGLEVEL(MSG_VERBOSE, "CmdProc: Packet decoded, sending return code %d with %d bytes of return payload...\n", rc, payloadBytes);
						bytes_to_send = sizeof(struct zynqmp_protocol_header)+payloadBytes;
						bp = pTxBuffer;
						while (bytes_to_send > 0)
						{
							if ((bytes_sent = send(clientSocket, bp, bytes_to_send, MSG_NOSIGNAL)) < 0)
							{
								// Error occured during response
								DBGLEVEL(MSG_ERROR, "CmdProc: Error sending response packet! (has client disconnected?)\n");
								// TODO: Set FEM error state
								break;
							}
							bp += bytes_sent;
							bytes_to_send -= bytes_sent;
						}
						if (bytes_to_send > 0)
						{
							// TODO: Set FEM error state
							pState->state = STATE_COMPLETE;
						}
						else
						{
							// Everything OK!
							DBGLEVEL(MSG_VERBOSE, "CmdProc: Sent response size %zd OK.\n", sizeof(struct zynqmp_protocol_header)+payloadBytes );
							pState->state = STATE_COMPLETE;
						}

					} // END if state == STATE_GOT_PYLD

					// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

				}	// END else clause

			}	// END FD_ISSET(i)

		}	// END for (fd)




	}	// END while(1)
	// ************************************************ END MAIN SERVER LOOP ******************************************************

	// Code below here will never run...  but just in case output debug message
	DBGLEVEL(MSG_ERROR, "CmdProc: Exiting thread! [SHOULD NEVER EXECUTE!]\n");

}



/*
 * Cleanly disconnects a client and frees any payload buffers > NET_NOMINAL_RX_BUFFER_SZ
 *
 * @param pState pointer to client state struct of client that has disconnected
 * @param pIndex pointer to fd of client that is disconnecting
 * @param pFdSet pointer to fd_set of read file descriptors
 * @param pNumConnectedClients pointer to number of connected clients
 */
void disconnectClient(struct clientStatus* pState, int *pIndex, fd_set* pFdSet, int *pNumConnectedClients)
{

	DBGLEVEL(MSG_SPARSE, "DiscClient: Client #%d disconnected.\n", *pIndex);
	close(*pIndex);
	FD_CLR(*pIndex, pFdSet);
	(*pNumConnectedClients)--;

	if (pState->payloadBufferSz > 0)
	{
		free(pState->pPayload);
		pState->pPayload = NULL;
		pState->payloadBufferSz = 0;
	}
	pState->state = STATE_COMPLETE;
}


/*
 * Processes received commands.  It is assumed that packets are well formed
 * and have been checked before passing to this function.
 *
 * @param pRxHeader pointer to zynqmp_protocol_header for received packet
 * @param pTxHeader pointer to zynqmp_protocol_header for outbound packet, which is a copy of the inbound unless status now shows an error.
 * @param pRxPayload pointer to payload buffer of received packet
 * @param pTxPayload pointer to payload buffer for outbound packet
 */
u_int32_t commandHandler(struct zynqmp_protocol_header* pRxHeader, struct zynqmp_protocol_header* pTxHeader, u_int8_t* pRxPayload, u_int8_t* pTxPayload)
{

	int i;					// General use variable
	int responseSize = 0;	// Payload size for response packet in bytes
	u_int32_t retCode = 0;			// Return code, often number of requested operations performed, but not always. Use responseSize for read payload
	u_int8_t status = 0;
	// Native size pointers for various data widths
	u_int16_t* pTxPayload_16  = (u_int16_t*)pTxPayload;
	u_int16_t* pRxPayload_16  = (u_int16_t*)pRxPayload;
	u_int32_t* pTxPayload_32  = (u_int32_t*)pTxPayload;
	u_int32_t* pRxPayload_32  = (u_int32_t*)pRxPayload;
	int numRequestedOps=0;
	int slaveAddress;
	int busIndex;
	u_int32_t reg_addr;
	int adc, pair_sel, chan_sel, addr;
	int reg_addr_size;
	
	switch (pRxHeader->command)
	{
	case ZYNQMP_CMD_WRITE:
		numRequestedOps = pRxHeader->payload_sz/pRxHeader->data_width;
		break;

	case ZYNQMP_CMD_READ:
	case ZYNQMP_CMD_READ_INTL:
		numRequestedOps = pRxHeader->payload_sz/pRxHeader->data_width ;
		if (pRxHeader->payload_sz > ZYNQMP_FROM_FEM_BUFFER_MAX)
		{
			pTxHeader->payload_sz = 0;
			return ZYNQMP_STATUS_OVERFLOW;
		}
		break;

	case ZYNQMP_CMD_WRITE_EMB:
		numRequestedOps = pRxHeader->num_ops;
		pRxPayload = (u_int8_t *) (&(pRxHeader->payload_sz));
		pRxPayload_16 = (u_int16_t *) (&(pRxHeader->payload_sz));
		pRxPayload_32 = (u_int32_t *) (&(pRxHeader->payload_sz));
		break;

	case ZYNQMP_CMD_RMW:
		numRequestedOps = pRxHeader->num_ops;
		break;

	case ZYNQMP_CMD_MEM_ZERO:
		numRequestedOps = pRxHeader->num_ops;
		break;

	}

	switch(pRxHeader->bus_target)
	{
	case ZYNQMP_BUS_REGS:
	case ZYNQMP_BUS_JESD:
	case ZYNQMP_BUS_DMA:
	case ZYNQMP_BUS_PS_LOW:
	case ZYNQMP_BUS_PS_HIGH:
	case ZYNQMP_BUS_PL_RAM:
	case ZYNQMP_BUS_HIST:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "Driver Bus %d : Read offset 0x%x for 0x%X words\n", pRxHeader->bus_target, pRxHeader->address, (pRxHeader->payload_sz)/pRxHeader->data_width);
			i = zynqmp_read(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->address, numRequestedOps, pTxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32);
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			DBGLEVEL(MSG_NORMAL, "Driver Bus %d : Write offset 0x%x for 0x%X words, first=0x%08X\n", pRxHeader->bus_target, pRxHeader->address, numRequestedOps, *pRxPayload_32);

			i = zynqmp_write(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->address, numRequestedOps, pRxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				retCode = numRequestedOps;
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_RMW)
		{
			DBGLEVEL(MSG_NORMAL, "Driver Bus %d : Read-Modify-Write offset 0x%08X AND=0x%08X. OR=0x%08X\n", pRxHeader->bus_target, pRxHeader->address, pRxHeader->payload_sz, pRxHeader->extra1);
			i = zynqmp_rmw(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->address, pRxHeader->payload_sz, pRxHeader->extra1, &retCode);
			if (i < 0)
				status = ZYNQMP_STATUS_ERROR;
		}
		else if (pRxHeader->command == ZYNQMP_CMD_MEM_ZERO)
		{
			DBGLEVEL(MSG_NORMAL, "Driver Bus %d : Memory Zero offset 0x%x for 0x%X words\n", pRxHeader->bus_target, pRxHeader->address, numRequestedOps);
			i = zynqmp_memzero(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->address, numRequestedOps);
			if (i < 0)
				status = ZYNQMP_STATUS_ERROR;
			else
				retCode = numRequestedOps;
		}
		else if (pRxHeader->command == ZYNQMP_CMD_READ_INTL)
		{
			DBGLEVEL(MSG_NORMAL, "Driver Bus %d : Read Row offset 0x%x for 0x%X words\n", pRxHeader->bus_target, pRxHeader->address, (pRxHeader->payload_sz)/pRxHeader->data_width);
			i =  zynqmp_read_row(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->extra1, pRxHeader->extra2, pRxHeader->extra3, pRxHeader->address, numRequestedOps, pTxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32);
			}
		}
		else
		{
			status = ZYNQMP_STATUS_ERROR;
		}			
		break; // Busses via the device driver.

#if 0
	case ZYNQMP_BUS_READOUT_MODE:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "Driver internal statics Bus %d : Read offset 0x%x for 0x%X words\n", pRxHeader->bus_target, pRxHeader->address, (pRxHeader->payload_sz)/pRxHeader->data_width);
			if (pRxHeader->address == 0 && numRequestedOps == 1)
			{
				i = xsp3m_get_readout_mode(zynqmp_path, 0, pTxPayload_16);
				if (i < 0)
				{
					status = ZYNQMP_STATUS_ERROR;
					retCode = i;
				}
				else
				{
					responseSize = sizeof(u_int32_t) * numRequestedOps;
					retCode = numRequestedOps;
					DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32);
				}
			}
			else
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = -1;
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			DBGLEVEL(MSG_NORMAL, "Driver internal statics Bus %d : Write offset 0x%x for 0x%X words, first=0x%08X\n", pRxHeader->bus_target, pRxHeader->address, numRequestedOps, *pRxPayload_32);
			if (pRxHeader->address == 0 && numRequestedOps == 1)
			{
				i = xsp3m_set_readout_mode(zynqmp_path, 0, *pRxPayload_32);
				if (i < 0)
				{
					status = ZYNQMP_STATUS_ERROR;
					retCode = i;
				}
				else
				{
					retCode = numRequestedOps;
				}
			}
			else
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = -1;
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_RMW)
		{
			DBGLEVEL(MSG_NORMAL, "Driver internal statics Bus %d : Read-Modify-Write offset 0x%08X AND=0x%08X. OR=0x%08X\n", pRxHeader->bus_target, pRxHeader->address, pRxHeader->payload_sz, pRxHeader->extra1);
			if (pRxHeader->address == 0 && numRequestedOps == 1)
			{
				u_int32_t readout_mode;
			i = zynqmp_rmw(zynqmp_path, 0, pRxHeader->bus_target, pRxHeader->address, pRxHeader->payload_sz, pRxHeader->extra1, &retCode);
				i = xsp3m_get_readout_mode(zynqmp_path, 0, &readout_mode);
				if (i < 0)
				{
					status = ZYNQMP_STATUS_ERROR;
					retCode = -1;
				}
				else
				{
					readout_mode = (readout_mode &  pRxHeader->payload_sz) | pRxHeader->extra1;
					i = xsp3m_set_readout_mode(zynqmp_path, 0, readout_mode);
					if (i < 0)
					{
						status = ZYNQMP_STATUS_ERROR;
						retCode = -1;
					}
					else
					{
						retCode = readout_mode;
					}
				}
			}
			else
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = -1;
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_MEM_ZERO)
		{
			DBGLEVEL(MSG_NORMAL, "Driver internal static Bus %d : Memory Zero offset 0x%x for 0x%X words\n", pRxHeader->bus_target, pRxHeader->address, numRequestedOps);

			if (pRxHeader->address == 0 && numRequestedOps == 1)
			{
				i = xsp3m_set_readout_mode(zynqmp_path, 0, 0);
				if (i < 0)
				{
					status = ZYNQMP_STATUS_ERROR;
					retCode = i;
				}
				else
				{
					retCode = numRequestedOps;
				}
			}
			else
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = -1;
			}
		}
		else
		{
			status = ZYNQMP_STATUS_ERROR;
		}			
		break; // Busses via the device driver.
#endif


	case ZYNQMP_BUS_DMA_STATUS:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "DMA Status block Read offset 0x%x for 0x%X words\n",  pRxHeader->address, (pRxHeader->payload_sz)/pRxHeader->data_width);
			i = zynqmp_read_dma_status_block(zynqmp_path, 0, pRxHeader->address, numRequestedOps, pTxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32);
			}
		}
		else
		{
			DBGLEVEL(MSG_ERROR, "DMA Status block: only read supported, not %d \n",  pRxHeader->command);
			status = ZYNQMP_STATUS_ERROR;
			retCode = -1;
		}
		break;

	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_PLAYBACK0:
	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_PLAYBACK1:
	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_SCOPE0:
	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_SCOPE1: 
	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_SCOPE2: 
	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_HIST_TEST:
	case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_HIST_READ:
	case ZYNQMP_BUS_DMA_BUFF+ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G:
	case ZYNQMP_BUS_DMA_BUFF+ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "Driver DMA Buff %d : Read offset 0x%x for 0x%X words\n", pRxHeader->bus_target-ZYNQMP_BUS_DMA_BUFF, pRxHeader->address, (pRxHeader->payload_sz)/pRxHeader->data_width);
			i = zynqmp_read_dma_buff(zynqmp_path, 0, pRxHeader->bus_target-ZYNQMP_BUS_DMA_BUFF, pRxHeader->address, numRequestedOps, pTxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32);
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			DBGLEVEL(MSG_NORMAL, "Driver DMA Buff %d : Seq =%d, Write offset 0x%x for 0x%X words, first=0x%08X, last=%08X\n", pRxHeader->bus_target-ZYNQMP_BUS_DMA_BUFF, pRxHeader->sequence, pRxHeader->address, numRequestedOps, *pRxPayload_32, pRxPayload_32[numRequestedOps-1]);

			i = zynqmp_write_dma_buff(zynqmp_path, 0, pRxHeader->bus_target-ZYNQMP_BUS_DMA_BUFF, pRxHeader->address, numRequestedOps, pRxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				retCode = numRequestedOps;
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_MEM_ZERO)
		{
			DBGLEVEL(MSG_NORMAL, "Driver DMA Buff %d : Memory Zero offset 0x%x for 0x%X words\n", pRxHeader->bus_target-ZYNQMP_BUS_DMA_BUFF, pRxHeader->address, numRequestedOps);
			i = zynqmp_zero_dma_buff(zynqmp_path, 0, pRxHeader->bus_target-ZYNQMP_BUS_DMA_BUFF, pRxHeader->address, numRequestedOps);
			if (i < 0)
				status = ZYNQMP_STATUS_ERROR;
			else
				retCode = numRequestedOps;
		}
		else
		{
			status = ZYNQMP_STATUS_ERROR;
		}			
		break; // Busses via the device driver.


	case ZYNQMP_BUS_FAN_CONT:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL,"Fan Cont: Read offset=%08X, Num=%0X\n", pRxHeader->address, numRequestedOps);
			if (pRxHeader->address+numRequestedOps > sizeof(NGZMPFanControl)/sizeof(u_int32_t))
			{
				DBGLEVEL(MSG_ERROR, "CmdDisp: Fan Control read at Lword offset %d, size %d is outside fan control block size %zd bytes", pRxHeader->address,  numRequestedOps, sizeof(NGZMPFanControl));
				status = ZYNQMP_STATUS_ERROR;
			}
			else
			{
				u_int32_t *p = (u_int32_t *)&fan_cont;
				p += pRxHeader->address;
				for (i=0; i<numRequestedOps; i++)
				{
					*(pTxPayload_32+i) = *p++;
				}
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			DBGLEVEL(MSG_NORMAL, "FAN_CONT: Write Addr=%08X, Num=%0X, First=%08X\n", pRxHeader->address, numRequestedOps, *pRxPayload_32);
			if (pRxHeader->address+numRequestedOps > sizeof(NGZMPFanControl)/sizeof(u_int32_t))
			{
				DBGLEVEL(MSG_ERROR, "CmdDisp: Fan Control Write at Lword offset %d, size %d is outside fan control block size %zd bytes", pRxHeader->address,  numRequestedOps, sizeof(NGZMPFanControl));
				status = ZYNQMP_STATUS_ERROR;
			}
			else
			{
				u_int32_t *p = (u_int32_t *)&fan_cont;
				p += pRxHeader->address;
				for (i=0; i<numRequestedOps; i++)
					*p++ = *(pRxPayload_32+i);
				retCode = numRequestedOps;
			}
		}
		else
		{
			status = ZYNQMP_STATUS_ERROR;
		}			
		break; // ZYNQMP_BUS_FAN_CONT

	case ZYNQMP_BUS_SPI_DGA:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "DGA SPI : Read Start chan=%d for %d chans\n", pRxHeader->address, numRequestedOps);
			i =  ngzmp_spi_read_dga(zynqmp_path, pRxHeader->address, numRequestedOps,  pTxPayload_16);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int16_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%04X\n", *pTxPayload_16 );
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			DBGLEVEL(MSG_NORMAL, "DGA SPI : Write chan=%d for %d chan, first=0x%04X\n", pRxHeader->address, numRequestedOps, *pRxPayload_16);
			i =  ngzmp_spi_write_dga(zynqmp_path, pRxHeader->address, numRequestedOps,  pRxPayload_16);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				retCode = numRequestedOps;
			}
		}
		break;

	case ZYNQMP_BUS_SPI_ADC:
		addr = NGZMP_SPI_ADC_ADDR_GET(pRxHeader->address);
		adc = NGZMP_SPI_ADC_ADC_GET(pRxHeader->address);
		pair_sel = NGZMP_SPI_ADC_PAIR_GET(pRxHeader->address);
		chan_sel = NGZMP_SPI_ADC_CHAN_GET(pRxHeader->address);
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			if (adc == 3)
				adc = 1;
			DBGLEVEL(MSG_NORMAL, "ADC SPI : Read ADC=%d, pair_sel=%d, chan_sel=%d, addr=0x%04X for 0x%X words\n", adc, pair_sel, chan_sel, addr, numRequestedOps);
			i =  ngzmp_spi_read_adc(zynqmp_path, 0, adc, pair_sel, chan_sel, addr, numRequestedOps, pTxPayload);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int8_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%02X\n", *pTxPayload );
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			if (adc == 3)
				adc = -1;
			DBGLEVEL(MSG_NORMAL, "ADC SPI : Write ADC=%d, pair_sel=%d, chan_sel=%d, addr=0x%04X for 0x%X words: First data=0x%02X\n", adc, pair_sel, chan_sel, addr, numRequestedOps, *pRxPayload);
			i =  ngzmp_spi_write_adc(zynqmp_path, 0, adc, pair_sel, chan_sel, addr, numRequestedOps, pRxPayload);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				retCode = numRequestedOps;
			}
		}
		break;

	case ZYNQMP_BUS_SPI_CLK:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "CLK SPI : Read offset 0x%x for 0x%X words\n", pRxHeader->address, numRequestedOps);

			i = ngzmp_spi_read_clk(zynqmp_path, 0, pRxHeader->address, numRequestedOps, pTxPayload);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int8_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%02X\n", *pTxPayload );
			}
		}
		else if (pRxHeader->command == ZYNQMP_CMD_WRITE || pRxHeader->command == ZYNQMP_CMD_WRITE_EMB)
		{
			DBGLEVEL(MSG_NORMAL, "Clk SPI : Write offset 0x%x for 0x%X words, first=0x%02X\n",  pRxHeader->address, numRequestedOps, *pRxPayload);

			i = ngzmp_spi_write_clk(zynqmp_path, 0, pRxHeader->address, numRequestedOps, pRxPayload);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				retCode = numRequestedOps;
			}
		}
		break;

		// --------------------------------------------------------------------
		case ZYNQMP_BUS_I2C:

			/*
			 * Decode address field
			 * Least significant word (lower byte) = I2C slave address
			 * Most significant word (lower byte)  = Bus index  {@link NGZMP_I2C_BUS_}
			 */
			slaveAddress = (pRxHeader->address & 0xFF);
			busIndex = (pRxHeader->address & 0xFF00) >> 8;
			reg_addr = (pRxHeader->address & 0x0FF0000) >> 16;
			reg_addr_size = (pRxHeader->address & 0x7000000) >> 24;
			DBGLEVEL(MSG_NORMAL, "CmdDisp: Processing I2C operation combined address field = %08X\n", pRxHeader->address);
			DBGLEVEL(MSG_NORMAL, "CmdDisp: .... bus index %d, I2C address %d\n", busIndex, slaveAddress);
			if (pRxHeader->command == ZYNQMP_CMD_READ)
			{
				if (pRxHeader->address & ZYNQMP_I2C_USE_REG_ADDR)
				{
					if (reg_addr_size)
						i = ngzmp_i2c_read_reg_addr_padded(zynqmp_path, 0, busIndex, slaveAddress, reg_addr, reg_addr_size, numRequestedOps, pTxPayload);
					else
						i = ngzmp_i2c_read_reg_addr(zynqmp_path, 0, busIndex, slaveAddress, reg_addr, numRequestedOps, pTxPayload);
				}
				else
					i = ngzmp_i2c_read_reg(zynqmp_path, 0, busIndex, slaveAddress, numRequestedOps, pTxPayload);
				if (i != numRequestedOps)
				{
					// I2C operation failed, set NACK
					DBGLEVEL(MSG_ERROR, "I2C_R - only got %d bytes instead of %d!\n", i, numRequestedOps);
					status = ZYNQMP_STATUS_ERROR;
					retCode = i;	// Consider what to do with that a partial read?
				}
				else
				{
					responseSize = i;
					retCode = i;
				}

			}
			else
			{
				if (pRxHeader->address & ZYNQMP_I2C_USE_REG_ADDR)
					i = ngzmp_i2c_write_reg_addr(zynqmp_path, 0, busIndex, slaveAddress, reg_addr, numRequestedOps, pRxPayload);
				else
					i = ngzmp_i2c_write_reg(zynqmp_path, 0, busIndex, slaveAddress, numRequestedOps, pRxPayload);
				if (i != numRequestedOps)
				{
					// I2C operation failed, set NACK
					DBGLEVEL(MSG_ERROR, "I2C_W - bus %d, slaveAddress=%d, only sent %d bytes instead of %d!\n", busIndex, slaveAddress, i, numRequestedOps);
					status = ZYNQMP_STATUS_ERROR;
					retCode = i;	// Consider what to do with that a partial read?
				}
				else
				{
					retCode = i;	// Consider what to do with that a partial read?
				}
			}
			break; // ZYNQMP_BUS_I2C

	case ZYNQMP_BUS_XADC:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			DBGLEVEL(MSG_NORMAL, "XADC : Read offset 0x%x for 0x%X words\n", pRxHeader->address, (pRxHeader->payload_sz)/pRxHeader->data_width);

			i = ngzmp_read_xadc(zynqmp_path, 0, pRxHeader->address, numRequestedOps, (int32_t *)pTxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32 );
			}
		}
		break;

	case ZYNQMP_BUS_UCD90160:
		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{
			int chip = (pRxHeader->address >> 8) & 0xFF;
			int page = (pRxHeader->address) & 0xFF;
			DBGLEVEL(MSG_NORMAL, "UCD90160 : Read chip=0x%x page = %0X for 0x%X words\n", chip, page, (pRxHeader->payload_sz)/pRxHeader->data_width);
			retCode = 0;
			i = ngzmp_i2c_read_ucd90160_vout(zynqmp_path, 0, chip, page, numRequestedOps, (int32_t *)pTxPayload_32);
			if (i < 0)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = sizeof(u_int32_t) * numRequestedOps;
				retCode = numRequestedOps;
				DBGLEVEL(MSG_VERBOSE, ".... First data =0x%08X\n", *pTxPayload_32 );
			}
		}
		break;

#if 0

#if 0

	// --------------------------------------------------------------------
	case ZYNQMP_BUS_EEPROM:

		if (pRxHeader->command == ZYNQMP_CMD_READ)
		{

			i = readFromEEPROM(pRxHeader->address, pTxPayload, numRequestedOps);
			if (i != numRequestedOps)
			{
				// EEPROM read failed, set error code
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			}
			else
			{
				responseSize = i;
				retCode = i;
			}
		}
		else
		{
			i = writeToEEPROM(pRxHeader->address, pRxPayload, numRequestedOps);
			if (i != numRequestedOps)
			{
				status = ZYNQMP_STATUS_ERROR;
				retCode = i;
			} 
			else
			{
				SBIT(state, STATE_ACK);
				retCode = i;
			}

		}
		break; // ZYNQMP_BUS_EEPROM
#endif


#if 0

	// * some of these will need reinstating modifying as done for I2C and SPI
		// --------------------------------------------------------------------
		case ZYNQMP_BUS_DIRECT:
			switch (pRxHeader->data_width)
			{
			case	ZYNQMP_WIDTH_BYTE: dataWidth = 1; break;
			case	ZYNQMP_WIDTH_WORD: dataWidth = 2; break;
			case	ZYNQMP_WIDTH_LONG: dataWidth = 4; break;
			default: DBGLEVEL(MSG_ERROR, "Unsupported width code %d in ZYNQMP_BUS_DIRECT access\n", pRxHeader->data_width);
					dataWidth = 0;
					break;
			}
			if (dataWidth == 0)
				SBIT(state, STATE_NACK);
			else
			{
				numOps = pRxHeader->payload_sz/dataWidth;
				SBIT(state, STATE_ACK);
			}
			XCache_FlushDCacheRange((unsigned int) (pRxHeader->address), pRxHeader->payload_sz);
			break;

				// --------------------------------------------------------------------

			case ZYNQMP_BUS_FEM_CONFIG:

				dataWidth = sizeof(u_int8_t);

				// Make sure return packet will not violate ZYNQMP_FROM_FEM_BUFFER_MAX (write operations should never be too long if ZYNQMP_FROM_FEM_BUFFER_MAX => 4!)
				if (CMPBIT(state, STATE_READ)) {
					if ( (sizeof(u_int32_t) + (dataWidth * numRequestedReads)) > ZYNQMP_FROM_FEM_BUFFER_MAX )
					{
						DBGLEVEL(MSG_ERROR, "CmdDisp: Response would be too large! (BUS=0x%x, WDTH=0x%x, STAT=0x%x, PYLDSZ=0x%x)\n", pRxHeader->bus_target, pRxHeader->data_width, pRxHeader->state, pRxHeader->payload_sz);
						SBIT(state, STATE_NACK);
						// TODO: Set FEM error state
						break;
					}
				}

				if (CMPBIT(state, STATE_READ)) // READ OPERATION
				{
					DBGLEVEL(MSG_NORMAL,"FEM_CONFIG: Read offset=%08X, Num=%0X\n", pRxHeader->address, *pRxPayload_32);
					if (pRxHeader->address+numRequestedReads > sizeof(struct fem_config))
					{
						DBGLEVEL(MSG_ERROR, "CmdDisp: FEM Config read at Byte offset %d, size %d is outside FEM config block size %d bytes", pRxHeader->address,  numRequestedReads, sizeof(struct fem_config));
					}
					else
					{
						u_int8_t *p = (u_int8_t *)&femConfig;
						p += pRxHeader->address;
						for (i=0; i<*pRxPayload_32; i++)
						{
							// DBGLEVEL(MSG_VVERBOSE, "CmdDisp: Read ADDR 0x%x", pRxHeader->address + (i*dataWidth));
							*(pTxPayload+4+i) = *p++;
							//DBGLEVEL(MSG_VVERBOSE, " VALUE 0x%x\n", readRegister_32(pRxHeader->address + (i*dataWidth)));
							responseSize += dataWidth;
							numOps++;
						}
					}
					SBIT(state, STATE_ACK);

				}
				else if (CMPBIT(state, STATE_WRITE)) // WRITE OPERATION
				{
					pRxPayload_32 = (u_int32_t*)pRxPayload;
					DBGLEVEL(MSG_NORMAL, "FEM_CONFIG: Write Addr=%08X, Num=%0X, First=%08X\n", pRxHeader->address, pRxHeader->payload_sz/dataWidth, *pRxPayload_32);

					if (pRxHeader->address+pRxHeader->payload_sz/dataWidth > sizeof(struct fem_config))
					{
						DBGLEVEL(MSG_ERROR, "CmdDisp: FEM Config Write at Byte offset %d, size %d is outside FEM COnfig block size %d bytes", pRxHeader->address,  pRxHeader->payload_sz/dataWidth, sizeof(struct fem_config));
					}
					else
					{
						u_int8_t *p = (u_int8_t *)&femConfig;
						p += pRxHeader->address;
						for (i=0; i<((pRxHeader->payload_sz)/dataWidth); i++)
						{
							//DBGLEVEL(MSG_VVERBOSE, "CmdDisp: Write ADDR 0x%x VALUE 0x%x\n", pRxHeader->address + (i*dataWidth), *(pRxPayload_32+i));
							*p++ = *(pRxPayload+i);
							numOps++;
						}
						if (pRxHeader->address+pRxHeader->payload_sz/dataWidth == sizeof(struct fem_config))
						{
							/* If hit last word, write to EEPROM */
							writeConfigToEEPROM(0, &femConfig);
						}
					}
					SBIT(state, STATE_ACK);
				}
				else
				{
					// Neither R or W bits set, can't process request
					DBGLEVEL(MSG_ERROR, "CmdDisp: Error, can't determine ZYNQMP_BUS_FEM_CONFIG operation mode, status was 0x%x\n", state);

					SBIT(state, STATE_NACK);
					// TODO: Set FEM error state
				}

				break; // ZYNQMP_BUS_FEM_CONFIG
#endif 
#endif	
		default:
			status = ZYNQMP_STATUS_ERROR;
			// TODO: Set FEM error state
			break;

	} // END switch(bus)

	// Build response header (all fields other status byte stay same as received packet)
	if (pRxHeader->command == ZYNQMP_CMD_READ)	// For read command specify the read paylaod size, fow write leave this field unchanged.
		pTxHeader->payload_sz = responseSize;	
	pTxHeader->state = status;	// Overwrite the status, which hopefully syas at 0 (no error).

	return retCode;
}



/*
 * Validates the fields of a header for consistency
 * @param pHeader pointer to header
 *
 * @return 0 if header logically correct, -1 if not
 */
int validateHeaderContents(struct zynqmp_protocol_header *pHeader)
{
	// Verify magic
	if (pHeader->magic != ZYNQMP_PROTOCOL_MAGIC_WORD)
	{
		DBGLEVEL(MSG_ERROR, "validateHeader: Header magic does not match. Read %08X, expected %08X\n", pHeader->magic, ZYNQMP_PROTOCOL_MAGIC_WORD);
		return -1;
	}

	// Logical checks
	if (pHeader->command == ZYNQMP_CMD_READ || pHeader->command == ZYNQMP_CMD_WRITE || pHeader->command == ZYNQMP_CMD_WRITE_EMB || pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
	{
		switch(pHeader->bus_target)
		{
		case ZYNQMP_BUS_REGS:
		case ZYNQMP_BUS_JESD:
		case ZYNQMP_BUS_DMA:
		case ZYNQMP_BUS_PS_LOW:
		case ZYNQMP_BUS_PS_HIGH:
		case ZYNQMP_BUS_PL_RAM:
		case ZYNQMP_BUS_HIST:
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 32bit for  RAW or RDMA access! (%d)\n",pHeader->data_width);
				return -1;
			}
			break;

#if 0
		case ZYNQMP_BUS_READOUT_MODE:
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 32bit for  driver static bus accesses! (%d)\n",pHeader->data_width);
				return -1;
			}
			break;
#endif
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_PLAYBACK0:
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_PLAYBACK1:
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_SCOPE0:
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_SCOPE1: 
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_SCOPE2: 
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_HIST_TEST:
		case ZYNQMP_BUS_DMA_BUFF+NGZMP_DMA_STREAM_BNUM_HIST_READ:
		case ZYNQMP_BUS_DMA_BUFF+ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G:
		case ZYNQMP_BUS_DMA_BUFF+ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM:
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 32bit for  DMA buffer access! (%d)\n",pHeader->data_width);
				return -1;
			}
			if (pHeader->command == ZYNQMP_CMD_RMW)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported on DMA buffers\n");
				return -1;
			}
			break;

		case ZYNQMP_BUS_SPI_DGA:
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported on SPI DGA\n");
				return -1;
			}

			// Data width must always be 8bit
			if (pHeader->data_width!=ZYNQMP_WIDTH_WORD)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width must always be 16bit for SPI DGA access!\n");
				return -1;
			}
			break;

		case ZYNQMP_BUS_SPI_ADC:
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported on SPI ADC\n");
				return -1;
			}

			// Data width must always be 8bit
			if (pHeader->data_width!=ZYNQMP_WIDTH_BYTE)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width must always be 8bit for SPI ADC access!\n");
				return -1;
			}
			break;
		case ZYNQMP_BUS_SPI_CLK:
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported on SPI CLK\n");
				return -1;
			}

			// Data width must always be 8bit
			if (pHeader->data_width!=ZYNQMP_WIDTH_BYTE)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width must always be 8bit for SPI CLK access!\n");
				return -1;
			}
			break;

		case ZYNQMP_BUS_I2C:
			// Data width must always be 8bit
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported bus I2C\n");
				return -1;
			}
			if (pHeader->data_width!=ZYNQMP_WIDTH_BYTE)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width must always be 8bit for I2C access!\n");
				return -1;
			}
			// Address most significant byte must be 0-4 only
			if ( (((pHeader->address & 0xFF00) >> 16)<0) || (((pHeader->address & 0xFF00) >> 16)>NGZMP_I2C_BUS_MAX) )
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Address bus number (most significant byte) must be only 0...%d for I2C access!\n", NGZMP_I2C_BUS_MAX);
				return -1;
			}
			break;

#if 0

		case ZYNQMP_BUS_FEM_CONFIG:
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported for FEM Config access\n");
				return -1;
			}
			if (pHeader->data_width!=ZYNQMP_WIDTH_BYTE)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 8bit for FEM Config access! (%d)\n",pHeader->data_width);
				return -1;
			}
			break;
#endif
		case ZYNQMP_BUS_DMA_STATUS:
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 32bit for DMA status! (%d)\n",pHeader->data_width);
				return -1;
			}
			break;

		case ZYNQMP_BUS_XADC:
			// Data width must always be  32 bit
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported bus XADC\n");
				return -1;
			}
			if (pHeader->command == ZYNQMP_CMD_WRITE || pHeader->command == ZYNQMP_CMD_WRITE_EMB)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Write and Write embedded not supported bus XADC\n");
				return -1;
			}
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width must always be 32 bit for XADC access!\n");
				return -1;
			}
			break;

		case ZYNQMP_BUS_UCD90160:
			// Data width must always be  32 bit
			if (pHeader->command == ZYNQMP_CMD_RMW || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: RMW and memzero command not supported bus UCD90160 read\n");
				return -1;
			}
			if (pHeader->command == ZYNQMP_CMD_WRITE || pHeader->command == ZYNQMP_CMD_WRITE_EMB)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Write and Write embedded not supported bus UCD90160 read\n");
				return -1;
			}
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width must always be 32 bit for UCD90160 read access!\n");
				return -1;
			}
			break;

		case ZYNQMP_BUS_FAN_CONT:
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG || pHeader->command == ZYNQMP_CMD_MEM_ZERO)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 32bit for fan control! (%d)\n",pHeader->data_width);
				return -1;
			}
			break;

		default:
			// Never valid!
			DBGLEVEL(MSG_ERROR, "validateHeader: Unsupported bus target %d!\n", pHeader->bus_target);
			return -1;
		}
	}
	else if (pHeader->command == ZYNQMP_CMD_PERSONALITY_READ || pHeader->command == ZYNQMP_CMD_PERSONALITY_WRITE || pHeader->command == ZYNQMP_CMD_PERSONALITY_WRITE_EMB)
	{
		// TODO - Implement ZYNQMP_CMD_PERSONALITY checks!
		return 0;		// For the meantime, pass any old packets to the personality modules...
	}
	else if (pHeader->command == ZYNQMP_CMD_READ_INTL)
	{
		switch (pHeader->bus_target)
		{
		case ZYNQMP_BUS_REGS:
		case ZYNQMP_BUS_JESD:
		case ZYNQMP_BUS_DMA:
		case ZYNQMP_BUS_PS_LOW:
		case ZYNQMP_BUS_PS_HIGH:
		case ZYNQMP_BUS_PL_RAM:
		case ZYNQMP_BUS_HIST:
			if (pHeader->data_width!=ZYNQMP_WIDTH_LONG)
			{
				DBGLEVEL(MSG_ERROR, "validateHeader: Data width should always be 32bit for  RAW or RDMA access! (%d)\n",pHeader->data_width);
				return -1;
			}
			break;
		default:
			DBGLEVEL(MSG_ERROR, "validateHeader: Command ZYNQMP_CMD_READ_INTL only supported on driver address spaces. Not bus=%d)\n", pHeader->bus_target);
			return -1;
		}
	}
	else
	{
		DBGLEVEL(MSG_ERROR, "validateHeader: Unsupported command %d!\n", pHeader->command);
		return -1;
	}

	// ...otherwise, header is valid!
	return 0;
}

/* Generates and sends a BADPKT response packet which signals
 * to a client that the last packet received was not
 * well formed or it was not possible to decode it correctly.
 *
 * @param pHeader pointer to header object
 * @param clientSocket socket identifier for client
 */
void generateBadPacketResponse(struct zynqmp_protocol_header *pHeader, int clientSocket)
{
	pHeader->address = 0;
	pHeader->bus_target = 0;
	pHeader->command = 0;
	pHeader->data_width = 0;
	pHeader->magic = ZYNQMP_PROTOCOL_MAGIC_WORD;
	pHeader->payload_sz = 0;
	pHeader->state = 0xF8;

	if (send(clientSocket, pHeader, sizeof(struct zynqmp_protocol_header), MSG_NOSIGNAL) == -1)
	{
		DBGLEVEL(MSG_NORMAL, "gBPR: Error sending BADPKT response packet!\n");
	}
}
