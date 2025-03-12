/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUELGAUGE_BQ40Z50_GAUGE_H_
#define ZEPHYR_DRIVERS_FUELGAUGE_BQ40Z50_GAUGE_H_

#include <zephyr/drivers/i2c.h>

enum bq40z50_regs {
	BQ40Z50_MANUFACTURERACCESS = 0x00,      /* R/W */
	BQ40Z50_REMAININGCAPACITYALARM = 0x01,  /* R/W, Unit: mAh/cWh, Range: 0..700 */
	BQ40Z50_REMAININGTIMEALARM = 0x02,      /* R/W, Unit: minutes, Range: 0..30 */
	BQ40Z50_BATTERYMODE = 0x03,             /* R/W, Unit: ---, Range: 0x0000..0xFFFF */
	BQ40Z50_ATRATE = 0x04,                  /* R/W, Unit: mA, Range: -32768..32767 */
	BQ40Z50_ATRATETIMETOFULL = 0x05,        /* R/O, Unit: minutes, Range: 0..65535 */
	BQ40Z50_ATRATETIMETOEMPTY = 0x06,       /* R/O, Unit: minutes, Range: 0..65535 */
	BQ40Z50_ATRATEOK = 0x07,                /* R/O, Unit: ---, Range: 0..65535 */
	BQ40Z50_TEMPERATURE = 0x08,             /* R/O, Unit: 0.1 K, Range: 0..65535 */
	BQ40Z50_VOLTAGE = 0x09,                 /* R/O, Unit: mV, Range: 0..65535 */
	BQ40Z50_CURRENT = 0x0A,                 /* R/O, Unit: mA, Range: -32768..32767 */
	BQ40Z50_AVERAGECURRENT = 0x0B,          /* R/O, Unit: mA, Range: -32768..32767 */
	BQ40Z50_RELATIVESTATEOFCHARGE = 0x0D,   /* R/O, Unit: percent, Range: 0..100 */
	BQ40Z50_ABSOLUTESTATEOFCHARGE = 0x0E,   /* R/O, Unit: percent, Range: 0..100 */
	BQ40Z50_REMAININGCAPACITY = 0x0F,       /* R/O, Unit: mAh, Range: 0..65535 */
	BQ40Z50_FULLCHARGECAPACITY = 0x10,      /* R/O, Unit: mAh, Range: 0..65535 */
	BQ40Z50_RUNTIMETOEMPTY = 0x11,          /* R/O, Unit: minutes, Range: 0..65535 */
	BQ40Z50_AVERAGETIMETOEMPTY = 0x12,      /* R/O, Unit: minutes, Range: 0..65535 */
	BQ40Z50_CHARGINGCURRENT = 0x14,         /* R/O, Unit: mA, Range: 0..65535 */
	BQ40Z50_CHARGINGVOLTAGE = 0x15,         /* R/O, Unit: mV, Range: 0..65535 */
	BQ40Z50_BATTERYSTATUS = 0x16,           /* R/O, Unit: ---, Range: --- */
	BQ40Z50_CYCLECOUNT = 0x17,              /* R/O, Unit: cycles, Range: 0..65535 */
	BQ40Z50_DESIGNCAPACITY = 0x18,          /* R/O, Unit: mAh, Range: 0..65535 */
	BQ40Z50_DESIGNVOLTAGE = 0x19,           /* R/O, Unit: mV, Range: 7000..18000 */
	BQ40Z50_MANUFACTURERDATE = 0x1B,        /* R/O, Unit: ---, Range: 0..65535 */
	BQ40Z50_SERIALNUMBER = 0x1C,            /* R/O, Unit: ---, Range: 0..65535 */
	BQ40Z50_MANUFACTURERNAME = 0x20,        /* R/O, Unit: ASCII, Range: --- */
	BQ40Z50_DEVICENAME = 0x21,              /* R/O, Unit: ASCII, Range: --- */
	BQ40Z50_DEVICECHEMISTRY = 0x22,         /* R/O, Unit: ASCII, Range: --- */
	BQ40Z50_MANUFACTURERDATA = 0x23,        /* R/O, Unit: ---, Range: --- */
	BQ40Z50_AUTHENTICATE = 0x2F,            /* R/W, Unit: ---, Range: --- */
	BQ40Z50_MANUFACTURERBLOCKACCESS = 0x44, /* R/W */
	BQ40Z50_BTPDISCHARGE = 0x4A,            /* R/W, Unit: mAh, Range: 150..65535 */
	BQ40Z50_BTPCHARGE = 0x4B,               /* R/W, Unit: mAh, Range: 175..65535 */
	BQ40Z50_PFSTATUS = 0x53,                /* Cannot read in Sealed Mode */
	BQ40Z50_OPERATIONSTATUS = 0x54,         /* Cannot read in Sealed Mode */
	BQ40Z50_CHARGINGSTATUS = 0x55,          /* Cannot read in Sealed Mode */
	BQ40Z50_GAUGINGSSTATUS =
		0x56, /* Cannot read in Sealed Mode (same address as CHARGINGSTATUS) */
	BQ40Z50_MAXTURBOPWR = 0x59,  /* R/W, Unit: cW, Range: --- */
	BQ40Z50_SUSTURBOPWR = 0x5A,  /* R/W, Unit: cW, Range: --- */
	BQ40Z50_MAXTURBOCURR = 0x5E, /* R/W, Unit: mA, Range: --- */
	BQ40Z50_SUSTURBOCURR = 0x5F  /* R/W, Unit: mA, Range: --- */
};

#endif
