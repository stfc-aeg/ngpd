// File: ADQ_simple_example.cpp
// Description: A simple example of how to use the ADQAPI.
// This example is generic for all SP Devices Data Acquisition Boards (ADQs)
// The example sets some basic settings and collects data.

#define _CRT_SECURE_NO_WARNINGS // This define removes warnings for printf

#include "ADQAPI.h"
#include <stdio.h>

#ifdef LINUX
#include <stdlib.h>
#endif

/** Parameters for the data collection **/
const unsigned int nof_samples = 1000;
const unsigned int nof_records = 4;
const unsigned int clock_source = ADQ_CLOCK_INT_INTREF;
const unsigned int trigger_mode = ADQ_SW_TRIGGER_MODE;

/** Level Trigger specific options **/
const int level_trigger_level = 10;
const int level_trigger_edge = 1;
const unsigned int level_trigger_channels = ADQ_LEVEL_TRIGGER_ALL_CHANNELS;

/** Internal Trigger specific options **/
const int internal_trigger_period = 1000;

int main()
{
  /** Declaration of variables for later user **/
  unsigned int success = 1;
  unsigned int nof_adq = 0;
  unsigned int adq_num = 1;
  unsigned int nof_failed_adq;
  int adq_type;
  char *serial_number;
  char *product_name;
  int api_rev = 0;
  int* fw_rev;
  void* target_buffers[ADQ_GETDATA_MAX_NOF_CHANNELS];
  unsigned int channel = 0;
  unsigned int record = 0;
  unsigned int sample = 0;
  FILE* outfile;
  char file_name[256];
  unsigned int nof_channels = 0;
  unsigned int bytes_per_sample = 0;
  int exit = 0;
  unsigned int ch_mask = 0xFF;

  /** Create a control unit **/
  void* adq_cu = CreateADQControlUnit();

  /** Enable Logging **/
  // Errors, Warnings and Info messages will be logged
  ADQControlUnit_EnableErrorTrace(adq_cu, LOG_LEVEL_INFO, ".");

  /* Enable Ethernet Scan
   * Ethernet communication. If activated, searches for ethernet connected
   * devices.
   * 0: Disabled
   * 1: Looks for ADQ14
   * 2: Looks for ADQ7
   */
  ADQControlUnit_EnableEthernetScan(adq_cu, 0);

  /** Find Devices **/
  // We will only connect to the first device in this example
  nof_adq = ADQControlUnit_FindDevices(adq_cu);
  printf("Number of ADQ devices found: %u\n", nof_adq);
  nof_failed_adq = ADQControlUnit_GetFailedDeviceCount(adq_cu);
  printf("Number of failed ADQ devices: %u\n", nof_failed_adq);

  if (nof_adq == 0)
  {
    printf("\nNo ADQ device found, aborting..\n");
  }
  else
  {

    /** Print product name, serial number and API revision **/
    api_rev = ADQAPI_GetRevision();
    fw_rev = ADQ_GetRevision(adq_cu, adq_num);
    serial_number = ADQ_GetBoardSerialNumber(adq_cu, adq_num);
    product_name = ADQ_GetBoardProductName(adq_cu, adq_num);
    printf("\nAPI revision:        %d\n", api_rev);
    printf("Firmware revision:   %d\n", fw_rev[0]);
    printf("Board serial number: %s\n", serial_number);
    printf("Board product name:  %s\n", product_name);

    // Checking for in-compatible firmware
    if (ADQ_HasFeature(adq_cu, adq_num, "FWATD") == 1)
    {
      printf("ERROR: This device is loaded with FWATD firmware and cannot be used for this example. Please see FWATD examples.\n");
      return -1;
    }
    if (ADQ_HasFeature(adq_cu, adq_num, "FWPD") == 1)
    {
      printf("ERROR: This device is loaded with FWPD firmware and cannot be used for this example. Please see FWPD examples.\n");
      return -1;
    }  

    /** Set Clock Source **/
    success = success && ADQ_SetClockSource(adq_cu, adq_num, clock_source);

    /** Set Trigger **/
    success = success && ADQ_SetTriggerMode(adq_cu, adq_num, trigger_mode);
    switch (trigger_mode)
    {
    case ADQ_LEVEL_TRIGGER_MODE:
      success = success && ADQ_SetLvlTrigLevel(adq_cu, adq_num, level_trigger_level);
      success = success && ADQ_SetLvlTrigEdge(adq_cu, adq_num, level_trigger_edge);
      success = success && ADQ_SetLvlTrigChannel(adq_cu, adq_num, level_trigger_channels);
      break;
    case ADQ_INTERNAL_TRIGGER_MODE:
      success = success && ADQ_SetInternalTriggerPeriod(adq_cu, adq_num, internal_trigger_period);
      break;
    default :
      break;
    }

    //Pre-trigger
    //ADQ_SetPreTrigSamples(adq_cu, adq_num, 128);

    //Hold-off
    //ADQ_SetTriggerHoldOﬀSamples (adq_cu, adq_num, 128);

    adq_type = ADQ_GetADQType(adq_cu, adq_num);

    //ADQ14 can use channel mask to selectively chose channel data to transfer to PC. I.E improved transfer time by channel selection.
    //Older products can only use channel mask when parsing already transfered data. I.E transfer time can not be improved even if only 1 channel is selected.
    if (adq_type == 714)
    {
      success = success && ADQ_MultiRecordSetChannelMask(adq_cu, adq_num, ch_mask);    
    }

    /** Set Up Collection with MultiRecord **/
    success = success && ADQ_MultiRecordSetup(adq_cu, adq_num, nof_records, nof_samples);

    /** Enable Trigger **/
    printf("\nStarting collection\n");
    success = success && ADQ_DisarmTrigger(adq_cu, adq_num);
    success = success && ADQ_ArmTrigger(adq_cu, adq_num);

    /** Verify that all records have been collected **/
    if (success)
    {
      while(!ADQ_GetAcquiredAll(adq_cu, adq_num))
      {
        if(trigger_mode == ADQ_SW_TRIGGER_MODE)
        {
          success = success && ADQ_SWTrig(adq_cu, adq_num);
        }
      }
    }

    /** Allocate memory for storing data **/
    nof_channels = ADQ_GetNofChannels(adq_cu, adq_num);
    success = success && ADQ_GetNofBytesPerSample(adq_cu, adq_num, &bytes_per_sample);
    for(channel = 0; channel < nof_channels; channel++)
    {
      target_buffers[channel] = malloc(nof_samples*nof_records*bytes_per_sample);
      success = success && (target_buffers[channel] != NULL); // Flag failure if memory allocation failed
    }

    /** Transfer Data from ADQ to host **/
    // Because the value left of '&&' is evaluated first, ADQ_GetData will not
    // be run if success is 0
    success = success && ADQ_GetData(adq_cu, adq_num, target_buffers, nof_samples*nof_records, bytes_per_sample,
      0,nof_records, ch_mask, 0, nof_samples, ADQ_TRANSFER_MODE_NORMAL);

    /** Close MultiRecord **/
    ADQ_MultiRecordClose(adq_cu, adq_num);

    /** Save data to files **/
    // The files may be drag-and-dropped into ADCaptureLab for convenient
    // plotting
    for(record = 0; record < nof_records; record++)
    {
      sprintf(file_name, "data_record%u.asc", record);
      outfile = fopen(file_name, "w");
      if (outfile != NULL)
      {
        for(sample = 0; sample < nof_samples; sample++)
        {
          for(channel = 0; channel < nof_channels; channel++)
          {
            if (bytes_per_sample == 2)
              fprintf(outfile, "%d\t", ((short*) target_buffers[channel])[record*nof_samples+sample]);
            else if (bytes_per_sample == 1)
              fprintf(outfile, "%d\t", ((char*) target_buffers[channel])[record*nof_samples+sample]);
            else
              success = 0; // Fail
          }
          fprintf(outfile, "\n"); //Print a new line to file
        }
        fclose(outfile);
      }
    }
    if (success)
      printf("All records were saved to file\n");
  }

  /** Exit gracefully **/
  DeleteADQControlUnit(adq_cu);
  for(channel = 0; channel < nof_channels; channel++)
  {
    if(target_buffers[channel] != NULL)
      free(target_buffers[channel]); // Only free if malloc succeeded
  }

  if (success == 0)
    printf("\nAn error occurred, please view the trace logs for more information\n");
  printf("\nType 0 and ENTER to exit.\n");
  scanf("%d", &exit);
  return 0;
}
