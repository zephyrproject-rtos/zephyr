#include "common.h"

static int result = 0;

void saadc_init(void)
{
	NRF_SAADC->CH[1].CONFIG |= (1 << 24) | (0 << 20) | (5 << 16) | (1 << 12) | (2 << 8) | (0 << 4) | (0 << 0);

	NRF_SAADC->CH[1].PSELP = 0x00000002;
	NRF_SAADC->CH[1].PSELN = 0x00000000;

	NRF_SAADC->RESOLUTION = 0x00000000; // 8-bit

	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&result;

	NRF_SAADC->OVERSAMPLE = 0x00000008;

	NRF_SAADC->SAMPLERATE = 0x00000000;

	NRF_SAADC->ENABLE = 0x00000001;

	NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
	while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0);
	NRF_SAADC->EVENTS_CALIBRATEDONE = 0;

	while(NRF_SAADC->STATUS == 0x01);
}

int saadc_read(void)
{
	int tmp = 0;

	NRF_SAADC->TASKS_START = 1;
	while (NRF_SAADC->EVENTS_STARTED == 0);
	NRF_SAADC->EVENTS_STARTED = 0;

	// Do a SAADC sample, will put the result in the configured RAM buffer.
	NRF_SAADC->TASKS_SAMPLE = 1;
	while (NRF_SAADC->EVENTS_END == 0);
	NRF_SAADC->EVENTS_END = 0;

	// Stop the SAADC, since it's not used anymore.
	NRF_SAADC->TASKS_STOP = 1;
	while (NRF_SAADC->EVENTS_STOPPED == 0);
	NRF_SAADC->EVENTS_STOPPED = 0;

	tmp = (int)(result*100.0/256.0) ;

	return tmp;	
}
