/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AMD_VERSAL_SYSMON_H_
#define ZEPHYR_DRIVERS_SENSOR_AMD_VERSAL_SYSMON_H_

#include <zephyr/drivers/sensor.h>

#define SYSMON_MILLI_SCALE      1000
#define SYSMON_FRACTIONAL_SHIFT 7U

#define SYSMON_UPPER_SATURATION_SIGNED 32767
#define SYSMON_LOWER_SATURATION_SIGNED -32768
#define SYSMON_UPPER_SATURATION        65535
#define SYSMON_LOWER_SATURATION        0

#define SYSMON_NO_OF_EVENTS        32
#define SYSMON_NUM_SUPPLY_CHANNELS 62

#define SYSMON_EVENT_WORK_DELAY_MS 1000

#define SYSMON_SUPPLY_BASE 0x1040

#define SYSMON_NPI_LOCK 0x000C
#define SYSMON_CONFIG   0x0100

#define SYSMON_ISR        0x0044
#define SYSMON_IMR        0x0048
#define SYSMON_IER_OFFSET 0x004C
#define SYSMON_IDR_OFFSET 0x0050

#define SYSMON_TEMP_REG    0x1038
#define SYSMON_TEMP_RESULT 0x0200

#define SYSMON_ALARM_FLAG 0x1018
#define SYSMON_ALARM_REG  0x1940

#define SYSMON_SUPPLY_LOWER_THRESH 0x1980
#define SYSMON_SUPPLY_UPPER_THRESH 0x1C80

#define SYSMON_TEMP_LOWER_THRESH 0x1970
#define SYSMON_TEMP_UPPER_THRESH 0x1974

#define SYSMON_OT_TH_LOW 0x1978
#define SYSMON_OT_TH_UP  0x197C

#define SYSMON_TEMP_EV_CFG 0x1F84

#define SYSMON_MANTISSA_MASK 0xFFFF
#define SYSMON_FMT_MASK      0x10000
#define SYSMON_FMT_SHIFT     16

#define SYSMON_MODE_MASK  0x60000
#define SYSMON_MODE_SHIFT 17

#define SYSMON_MANTISSA_SIGN_SHIFT 15
#define SYSMON_SUPPLY_CONFIG_SHIFT 14

#define SYSMON_TEMP_INTR_MASK GENMASK(9, 8)

#define NPI_UNLOCK 0xF9E8D7C6

/**
 * @brief SysMon alarm bit positions.
 *
 * These values represent bit positions for various alarm conditions
 * reported by the SysMon hardware.
 */
enum sysmon_alarm_bit {
	SYSMON_BIT_ALARM0 = 0, /* Alarm 0 triggered */
	SYSMON_BIT_ALARM1 = 1, /* Alarm 1 triggered */
	SYSMON_BIT_ALARM2 = 2, /* Alarm 2 triggered */
	SYSMON_BIT_ALARM3 = 3, /* Alarm 3 triggered */
	SYSMON_BIT_ALARM4 = 4, /* Alarm 4 triggered */
	SYSMON_BIT_ALARM5 = 5, /* Alarm 5 triggered */
	SYSMON_BIT_ALARM6 = 6, /* Alarm 6 triggered */
	SYSMON_BIT_ALARM7 = 7, /* Alarm 7 triggered */
	SYSMON_BIT_OT = 8,     /* Overtemperature alarm triggered */
	SYSMON_BIT_TEMP = 9,   /* Temperature alarm triggered */
};

/**
 * @brief AMD Versal SysMon private sensor channels
 *
 * These channels represent voltage rails and temperature measurements
 * exposed by the Versal System Monitor (SysMon) hardware.
 * They extend Zephyr's sensor_channel enumeration using the private range.
 */
enum sensor_channel_versal_sysmon {
	/** Battery voltage channel */
	VERSAL_SYSMON_VCC_BAT = SENSOR_CHAN_PRIV_START,
	/** PMC supply voltage channel */
	VERSAL_SYSMON_VCC_PMC,
	/** PSFP supply voltage channel */
	VERSAL_SYSMON_VCC_PSFP,
	/** PSLP supply voltage channel */
	VERSAL_SYSMON_VCC_PSLP,
	/** RAM supply voltage channel */
	VERSAL_SYSMON_VCC_RAM,
	/** SOC supply voltage channel */
	VERSAL_SYSMON_VCC_SOC,
	/** Internal core voltage channel */
	VERSAL_SYSMON_VCCINT,
	/** Auxiliary voltage channel */
	VERSAL_SYSMON_VCCAUX,
	/** PMC auxiliary voltage channel */
	VERSAL_SYSMON_VCCAUX_PMC,
	/** SysMon auxiliary voltage channel */
	VERSAL_SYSMON_VCCAUX_SMON,
	/** VP/VN supply voltage channel */
	VERSAL_SYSMON_VP_VN,

	/** Supply voltage of I/O Bank 306 channel */
	VERSAL_SYSMON_VCCO_306,
	/** Supply voltage of I/O Bank 406 channel */
	VERSAL_SYSMON_VCCO_406,
	/** Supply voltage of I/O Bank 500 channel */
	VERSAL_SYSMON_VCCO_500,
	/** Supply voltage of I/O Bank 501 channel */
	VERSAL_SYSMON_VCCO_501,
	/** Supply voltage of I/O Bank 502 channel */
	VERSAL_SYSMON_VCCO_502,
	/** Supply voltage of I/O Bank 503 channel */
	VERSAL_SYSMON_VCCO_503,
	/** Supply voltage of I/O Bank 700 channel */
	VERSAL_SYSMON_VCCO_700,
	/** Supply voltage of I/O Bank 701 channel */
	VERSAL_SYSMON_VCCO_701,
	/** Supply voltage of I/O Bank 702 channel */
	VERSAL_SYSMON_VCCO_702,
	/** Supply voltage of I/O Bank 703 channel */
	VERSAL_SYSMON_VCCO_703,
	/** Supply voltage of I/O Bank 704 channel */
	VERSAL_SYSMON_VCCO_704,
	/** Supply voltage of I/O Bank 705 channel */
	VERSAL_SYSMON_VCCO_705,
	/** Supply voltage of I/O Bank 706 channel */
	VERSAL_SYSMON_VCCO_706,
	/** Supply voltage of I/O Bank 707 channel */
	VERSAL_SYSMON_VCCO_707,
	/** Supply voltage of I/O Bank 708 channel */
	VERSAL_SYSMON_VCCO_708,
	/** Supply voltage of I/O Bank 709 channel */
	VERSAL_SYSMON_VCCO_709,
	/** Supply voltage of I/O Bank 710 channel */
	VERSAL_SYSMON_VCCO_710,
	/** Supply voltage of I/O Bank 711 channel */
	VERSAL_SYSMON_VCCO_711,

	/** Analog supply of GTY block in region 103 channel */
	VERSAL_SYSMON_GTY_AVCC_103,
	/** Analog supply of GTY block in region 104 channel */
	VERSAL_SYSMON_GTY_AVCC_104,
	/** Analog supply of GTY block in region 105 channel */
	VERSAL_SYSMON_GTY_AVCC_105,
	/** Analog supply of GTY block in region 106 channel */
	VERSAL_SYSMON_GTY_AVCC_106,
	/** Analog supply of GTY block in region 200 channel */
	VERSAL_SYSMON_GTY_AVCC_200,
	/** Analog supply of GTY block in region 201 channel */
	VERSAL_SYSMON_GTY_AVCC_201,
	/** Analog supply of GTY block in region 202 channel */
	VERSAL_SYSMON_GTY_AVCC_202,
	/** Analog supply of GTY block in region 203 channel */
	VERSAL_SYSMON_GTY_AVCC_203,
	/** Analog supply of GTY block in region 204 channel */
	VERSAL_SYSMON_GTY_AVCC_204,
	/** Analog supply of GTY block in region 205 channel */
	VERSAL_SYSMON_GTY_AVCC_205,
	/** Analog supply of GTY block in region 206 channel */
	VERSAL_SYSMON_GTY_AVCC_206,

	/** Auxiliary supply of GTY block in region 103 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_103,
	/** Auxiliary supply of GTY block in region 104 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_104,
	/** Auxiliary supply of GTY block in region 105 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_105,
	/** Auxiliary supply of GTY block in region 106 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_106,
	/** Auxiliary supply of GTY block in region 200 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_200,
	/** Auxiliary supply of GTY block in region 201 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_201,
	/** Auxiliary supply of GTY block in region 202 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_202,
	/** Auxiliary supply of GTY block in region 203 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_203,
	/** Auxiliary supply of GTY block in region 204 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_204,
	/** Auxiliary supply of GTY block in region 205 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_205,
	/** Auxiliary supply of GTY block in region 206 channel */
	VERSAL_SYSMON_GTY_AVCCAUX_206,

	/** Termination supply of GTY block in region 103 channel */
	VERSAL_SYSMON_GTY_AVTT_103,
	/** Termination supply of GTY block in region 104 channel */
	VERSAL_SYSMON_GTY_AVTT_104,
	/** Termination supply of GTY block in region 105 channel */
	VERSAL_SYSMON_GTY_AVTT_105,
	/** Termination supply of GTY block in region 106 channel */
	VERSAL_SYSMON_GTY_AVTT_106,
	/** Termination supply of GTY block in region 200 channel */
	VERSAL_SYSMON_GTY_AVTT_200,
	/** Termination supply of GTY block in region 201 channel */
	VERSAL_SYSMON_GTY_AVTT_201,
	/** Termination supply of GTY block in region 202 channel */
	VERSAL_SYSMON_GTY_AVTT_202,
	/** Termination supply of GTY block in region 203 channel */
	VERSAL_SYSMON_GTY_AVTT_203,
	/** Termination supply of GTY block in region 204 channel */
	VERSAL_SYSMON_GTY_AVTT_204,
	/** Termination supply of GTY block in region 205 channel */
	VERSAL_SYSMON_GTY_AVTT_205,
	/** Termination supply of GTY block in region 206 channel */
	VERSAL_SYSMON_GTY_AVTT_206,

	/** Die temperature channel for threshold temperature */
	VERSAL_SYSMON_DIE_TEMP,
	/** Die temperature channel for over temperature */
	VERSAL_SYSMON_DIE_TEMP_OT,
};

/**
 * @brief Extended SysMon sensor attributes
 *
 * These attributes extend Zephyr's standard sensor attributes and
 * provide support for over-temperature alarm configuration and
 * temperature statistics specific to the Versal SysMon hardware.
 */
enum versal_sysmon_ext_attr {
	/** Temperature above which OT alarm is triggered */
	SENSOR_ATTR_UPPER_THRESH_OT = SENSOR_ATTR_PRIV_START,
	/** Temperature below which OT alarm is cleared */
	SENSOR_ATTR_LOWER_THRESH_OT,
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AMD_VERSAL_SYSMON_H_ */
