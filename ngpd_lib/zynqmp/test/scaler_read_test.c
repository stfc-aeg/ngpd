/*
 * scaler_read_test.c
 *
 *  Created on: Nov 21, 2012
 *      Author: gm
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xspress3.h"
#include "xspress3test.h"
#include "errors.h"

int scaler_read_test(int xsp3_handle_) {
	char *functionName = "scaler_read_test";
	int xsp3_status;
	int xsp3_num_cards = 1;
	int frame_count;
	int framesToReadOut;
	int lastFrameCount=0;
	int frameCounter;
	int maxNumFrames = 4096;
	int numChannels = 4;
	int acquire = 0;;
	int stillBusy = 0;
	int dumpOffset = 0;
	u_int32_t *pSCA;
	int numFrames;
	int card, frame, chan, sca;
	xsp3_histogram_start(xsp3_handle_, 1);

	while(1) {
	//Read how many data frames have been transferred.
//	getIntegerParam(xsp3NumCardsParam, &xsp3_num_cards);
	for (card = 0; card < xsp3_num_cards; card++) {
		xsp3_status = xsp3_scaler_check_desc(xsp3_handle_, card);
		if (xsp3_status < XSP3_OK) {
//			checkStatus(xsp3_status, "xsp3_dma_check_desc", functionName);
//			status = asynError;
			return -1;
		}
		frame_count = xsp3_status;
		printf("%s frame_count: %d.\n", functionName, frame_count);
	}

	if ((!acquire) && (stillBusy == 1)) {
		frame_count = 0;
		framesToReadOut = 0;
//		callParamCallbacks();
	}

	//Take the value from the last card for now
//	setIntegerParam(xsp3FrameCountParam, frame_count);

	if (frame_count > lastFrameCount) {
		framesToReadOut = frame_count - lastFrameCount;
		lastFrameCount = frame_count;

//		getIntegerParam(NDArrayCounter, &frameCounter);
		frameCounter += framesToReadOut;
		int remainingFrames = framesToReadOut;
		//Check we are not overflowing or reading too many frames.
		if (frameCounter >= maxNumFrames) {
			remainingFrames = maxNumFrames - (frameCounter - framesToReadOut);
			printf("%s ERROR: Stopping Acquisition. We Reached The Max Num Of Frames.\n", functionName);
//			setStringParam(ADStatusMessage, "Stopped. Max Frames Reached.");
//			setIntegerParam(ADAcquire, 0);
			acquire = 0;
//			setIntegerParam(ADStatus, ADStatusAborted);
		} else if (frameCounter >= numFrames) {
			remainingFrames = numFrames - (frameCounter - framesToReadOut);
//			setStringParam(ADStatusMessage, "Completed Acqusition.");
//			setIntegerParam(ADAcquire, 0);
//			setIntegerParam(ADStatus, ADStatusIdle);
			acquire = 0;
		}

		int frameOffset = frameCounter - framesToReadOut;
		if (acquire == 0) {
			frameCounter = frameOffset + remainingFrames;
		}

		printf("%s frame_count: %d.\n", functionName, frame_count);
		printf("%s framesToReadOut: %d.\n", functionName, framesToReadOut);
		printf("%s frameCounter: %d.\n", functionName, frameCounter);
		printf("%s remainingFrames: %d.\n", functionName, remainingFrames);
		printf("%s frameOffset: %d.\n", functionName, frameOffset);

		//epicsThreadSleep(0.05);
		u_int32_t *pData = NULL;
		//Readout multiple frames of scaler data here into local array.

		if (!stillBusy) {
			pData = pSCA + (frameOffset * (XSP3_SW_NUM_SCALERS * numChannels));
			xsp3_status = xsp3_scaler_read(xsp3_handle_, pData, 0, 0, frameOffset, XSP3_SW_NUM_SCALERS, numChannels,
					remainingFrames);
			if (xsp3_status < XSP3_OK) {
//				checkStatus(xsp3_status, "xsp3_scaler_read", functionName);
				//What to do here?
				return -1;
			}
		}

		//Dump data for testing
		u_int32_t *pDumpData = pSCA;
		for (frame = frameOffset; frame < frameCounter; frame++) {
			for (chan = 0; chan < numChannels; ++chan) {
				for (sca = 0; sca < XSP3_SW_NUM_SCALERS; sca++) {
					printf(" frame: %d chan: %d sca: %d data[%d]: %d\n", frame, chan, sca, dumpOffset,
							*(pDumpData + dumpOffset));
					++dumpOffset;
				}
			}
		}
	}
	}
	return 0;
}

