/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUELGAUGE_BQ4XZ50_GAUGE_H_
#define ZEPHYR_DRIVERS_FUELGAUGE_BQ4XZ50_GAUGE_H_

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>

/*
 * SBS command set shared by the bq40z50 and bq41z50. Both parts expose the same commands at the
 * same addresses. Some status registers differ in their bit assignments, but not in the fields
 * this driver reads.
 */
enum bq4xz50_regs {
	BQ4XZ50_MANUFACTURERACCESS = 0x00,      /* R/W */
	BQ4XZ50_REMAININGCAPACITYALARM = 0x01,  /* R/W, Unit: mAh/cWh, Range: 0..700 */
	BQ4XZ50_REMAININGTIMEALARM = 0x02,      /* R/W, Unit: minutes, Range: 0..30 */
	BQ4XZ50_BATTERYMODE = 0x03,             /* R/W, Unit: ---, Range: 0x0000..0xFFFF */
	BQ4XZ50_ATRATE = 0x04,                  /* R/W, Unit: mA/10mW, Range: -32768..32767 */
	BQ4XZ50_ATRATETIMETOFULL = 0x05,        /* R/O, Unit: minutes, Range: 0..65535 */
	BQ4XZ50_ATRATETIMETOEMPTY = 0x06,       /* R/O, Unit: minutes, Range: 0..65535 */
	BQ4XZ50_ATRATEOK = 0x07,                /* R/O, Unit: ---, Range: 0..65535 */
	BQ4XZ50_TEMPERATURE = 0x08,             /* R/O, Unit: 0.1 K, Range: 0..65535 */
	BQ4XZ50_VOLTAGE = 0x09,                 /* R/O, Unit: mV, Range: 0..65535 */
	BQ4XZ50_CURRENT = 0x0A,                 /* R/O, Unit: mA, Range: -32768..32767 */
	BQ4XZ50_AVERAGECURRENT = 0x0B,          /* R/O, Unit: mA, Range: -32768..32767 */
	BQ4XZ50_MAXERROR = 0x0C,                /* R/O, Unit: percent, Range: 0..100 */
	BQ4XZ50_RELATIVESTATEOFCHARGE = 0x0D,   /* R/O, Unit: percent, Range: 0..100 */
	BQ4XZ50_ABSOLUTESTATEOFCHARGE = 0x0E,   /* R/O, Unit: percent, Range: 0..100 */
	BQ4XZ50_REMAININGCAPACITY = 0x0F,       /* R/O, Unit: mAh/10mWh, Range: 0..65535 */
	BQ4XZ50_FULLCHARGECAPACITY = 0x10,      /* R/O, Unit: mAh/10mWh, Range: 0..65535 */
	BQ4XZ50_RUNTIMETOEMPTY = 0x11,          /* R/O, Unit: minutes, Range: 0..65535 */
	BQ4XZ50_AVERAGETIMETOEMPTY = 0x12,      /* R/O, Unit: minutes, Range: 0..65535 */
	BQ4XZ50_AVERAGETIMETOFULL = 0x13,       /* R/O, Unit: minutes, Range: 0..65535 */
	BQ4XZ50_CHARGINGCURRENT = 0x14,         /* R/O, Unit: mA, Range: 0..65535 */
	BQ4XZ50_CHARGINGVOLTAGE = 0x15,         /* R/O, Unit: mV, Range: 0..65535 */
	BQ4XZ50_BATTERYSTATUS = 0x16,           /* R/O, Unit: ---, Range: --- */
	BQ4XZ50_CYCLECOUNT = 0x17,              /* R/O, Unit: cycles, Range: 0..65535 */
	BQ4XZ50_DESIGNCAPACITY = 0x18,          /* R/O, Unit: mAh/10mWh, Range: 0..65535 */
	BQ4XZ50_DESIGNVOLTAGE = 0x19,           /* R/O, Unit: mV, Range: 7000..18000 */
	BQ4XZ50_SPECIFICATIONINFO = 0x1A,       /* R/O, Unit: ---, Range: --- */
	BQ4XZ50_MANUFACTURERDATE = 0x1B,        /* R/O, Unit: ---, Range: 0..65535 */
	BQ4XZ50_SERIALNUMBER = 0x1C,            /* R/O, Unit: ---, Range: 0..65535 */
	BQ4XZ50_MANUFACTURERNAME = 0x20,        /* R/O, Unit: ASCII, Range: --- */
	BQ4XZ50_DEVICENAME = 0x21,              /* R/O, Unit: ASCII, Range: --- */
	BQ4XZ50_DEVICECHEMISTRY = 0x22,         /* R/O, Unit: ASCII, Range: --- */
	BQ4XZ50_MANUFACTURERDATA = 0x23,        /* R/O, Unit: ---, Range: --- */
	BQ4XZ50_AUTHENTICATE = 0x2F,            /* R/W, Unit: ---, Range: --- */
	BQ4XZ50_CELLVOLTAGE4 = 0x3C,            /* R/O, Unit: mV, Range: 0..65535 */
	BQ4XZ50_CELLVOLTAGE3 = 0x3D,            /* R/O, Unit: mV, Range: 0..65535 */
	BQ4XZ50_CELLVOLTAGE2 = 0x3E,            /* R/O, Unit: mV, Range: 0..65535 */
	BQ4XZ50_CELLVOLTAGE1 = 0x3F,            /* R/O, Unit: mV, Range: 0..65535 */
	BQ4XZ50_MANUFACTURERBLOCKACCESS = 0x44, /* R/W */
	BQ4XZ50_BTPDISCHARGE = 0x4A,            /* R/W, Unit: mAh, Range: 150..65535 */
	BQ4XZ50_BTPCHARGE = 0x4B,               /* R/W, Unit: mAh, Range: 175..65535 */
	BQ4XZ50_STATEOFHEALTH = 0x4F,           /* R/O, Unit: percent, Range: 0..100 */
	BQ4XZ50_SAFETYALERT = 0x50,             /* Cannot read in Sealed Mode */
	BQ4XZ50_SAFETYSTATUS = 0x51,            /* Cannot read in Sealed Mode */
	BQ4XZ50_PFALERT = 0x52,                 /* Cannot read in Sealed Mode */
	BQ4XZ50_PFSTATUS = 0x53,                /* Cannot read in Sealed Mode */
	BQ4XZ50_OPERATIONSTATUS = 0x54,         /* Cannot read in Sealed Mode */
	BQ4XZ50_CHARGINGSTATUS = 0x55,          /* Cannot read in Sealed Mode */
	BQ4XZ50_GAUGINGSTATUS = 0x56,           /* Cannot read in Sealed Mode */
	BQ4XZ50_MANUFACTURINGSTATUS = 0x57,     /* Cannot read in Sealed Mode */
	BQ4XZ50_AFEREG = 0x58,                  /* Cannot read in Sealed Mode */
	BQ4XZ50_MAXTURBOPWR = 0x59,             /* R/W, Unit: cW, Range: --- */
	BQ4XZ50_SUSTURBOPWR = 0x5A,             /* R/W, Unit: cW, Range: --- */
	BQ4XZ50_TURBOPACKR = 0x5B,              /* R/W, Unit: mOhm, Range: --- */
	BQ4XZ50_TURBOSYSR = 0x5C,               /* R/W, Unit: mOhm, Range: --- */
	BQ4XZ50_TURBOEDV = 0x5D,                /* R/W, Unit: mV, Range: --- */
	BQ4XZ50_MAXTURBOCURR = 0x5E,            /* R/W, Unit: mA, Range: --- */
	BQ4XZ50_SUSTURBOCURR = 0x5F             /* R/W, Unit: mA, Range: --- */
};

struct bq4xz50_config {
	struct i2c_dt_spec i2c;
};

int bq4xz50_init(const struct device *dev);

extern const struct fuel_gauge_driver_api bq4xz50_driver_api;

#define BQ4XZ50_DEFINE(inst)                                                                       \
	static const struct bq4xz50_config bq4xz50_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &bq4xz50_init, NULL, NULL, &bq4xz50_config_##inst, POST_KERNEL, \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &bq4xz50_driver_api);

#endif
