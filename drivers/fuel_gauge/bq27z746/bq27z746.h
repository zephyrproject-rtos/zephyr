/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUELGAUGE_BQ27Z746_GAUGE_H_
#define ZEPHYR_DRIVERS_FUELGAUGE_BQ27Z746_GAUGE_H_

#include <zephyr/drivers/i2c.h>

/* Registers */
#define BQ27Z746_MANUFACTURERACCESS    0x00 /* R/W */
#define BQ27Z746_ATRATE                0x02 /* R/W, Unit: mA, Range: -32768..32767 */
#define BQ27Z746_ATRATETIMETOEMPTY     0x04 /* R/O, Unit: minutes, Range: 0..65535 */
#define BQ27Z746_TEMPERATURE           0x06 /* R/O, Unit: 0.1 K, Range: 0..32767 */
#define BQ27Z746_VOLTAGE               0x08 /* R/O, Unit: mV, Range: 0..32767 */
#define BQ27Z746_BATTERYSTATUS         0x0A /* R/O, Unit: status bits */
#define BQ27Z746_CURRENT               0x0C /* R/O, Unit: mA, Range: -32768..32767 */
#define BQ27Z746_REMAININGCAPACITY     0x10 /* R/O, Unit: mAh, Range: 0..32767 */
#define BQ27Z746_FULLCHARGECAPACITY    0x12 /* R/O, Unit: mAh, Range: 0..32767 */
#define BQ27Z746_AVERAGECURRENT        0x14 /* R/O, Unit: mA, Range: -32768..32767 */
#define BQ27Z746_AVERAGETIMETOEMPTY    0x16 /* R/O, Unit: minutes, Range: 0..65535 */
#define BQ27Z746_AVERAGETIMETOFULL     0x18 /* R/O, Unit: minutes, Range: 0..65535 */
#define BQ27Z746_MAXLOADCURRENT        0x1E /* R/O, Unit: mA, Range: 0..65535 */
#define BQ27Z746_MAXLOADTIMETOEMPTY    0x20 /* R/O, Unit: minutes, Range: 0..65535 */
#define BQ27Z746_AVERAGEPOWER          0x22 /* R/O, Unit: mW, Range: -32768..32767 */
#define BQ27Z746_BTPDISCHARGESET       0x24 /* Datasheet unclear */
#define BQ27Z746_BTPCHARGESET          0x26 /* Datasheet unclear */
#define BQ27Z746_INTERNALTEMPERATURE   0x28 /* R/O, Unit: 0.1 K, Range: 0..32767 */
#define BQ27Z746_CYCLECOUNT            0x2A /* R/O, Unit: none, Range: 0..65535 */
#define BQ27Z746_RELATIVESTATEOFCHARGE 0x2C /* R/O, Unit: percent, Range: 0..100 */
#define BQ27Z746_STATEOFHEALTH         0x2E /* R/O, Unit: percent, Range: 0..100 */
#define BQ27Z746_CHARGINGVOLTAGE       0x30 /* R/O, Unit: mV, Range: 0..32767 */
#define BQ27Z746_CHARGINGCURRENT       0x32 /* R/O, Unit: mA, Range: 0..32767 */
#define BQ27Z746_TERMINATEVOLTAGE      0x34 /* R/W, Unit: mC, Range: 0..32767 */
#define BQ27Z746_TIMESTAMPUPPER        0x36 /* R/O, Unit: seconds, Range: 0..65535 */
#define BQ27Z746_TIMESTAMPLOWER        0x38 /* R/O, Unit: seconds, Range: 0..65535 */
#define BQ27Z746_QMAXCYCLES            0x3A /* R/O, Unit: none, Range: 0..65535 */
#define BQ27Z746_DESIGNCAPACITY                                                                    \
	0x3C /* R/O (sealed), R/W (unsealed or factory access), Unit: mAh, Range: 0..32767 */
#define BQ27Z746_ALTMANUFACTURERACCESS 0x3E /* R/W */
#define BQ27Z746_MACDATA               0x40 /* R/O, MAC data */
#define BQ27Z746_MACDATASUM            0x60 /* R/O, Checksum over MAC command and data */
#define BQ27Z746_MACDATALEN            0x61 /* R/O, Length of the MAC data */
#define BQ27Z746_VOLTHISETTHRESHOLD    0x62 /* R/W, Unit: mV, Range: 0..5000 */
#define BQ27Z746_VOLTHICLEARTHRESHOLD  0x64 /* R/W, Unit: mV, Range: 0..5000 */
#define BQ27Z746_VOLTLOSETTHRESHOLD    0x66 /* R/W, Unit: mV, Range: 0..5000 */
#define BQ27Z746_VOLTLOCLEARTHRESHOLD  0x68 /* R/W, Unit: mV, Range: 0..5000 */
#define BQ27Z746_TEMPHISETTHRESHOLD    0x6A /* R/W, Unit: degree celsius, Range: -128..127 */
#define BQ27Z746_TEMPHICLEARTHRESHOLD  0x6B /* R/W, Unit: degree celsius, Range: -128..127 */
#define BQ27Z746_TEMPLOSETTHRESHOLD    0x6C /* R/W, Unit: degree celsius, Range: -128..127 */
#define BQ27Z746_TEMPLOCLEARTHRESHOLD  0x6D /* R/W, Unit: degree celsius, Range: -128..127 */
#define BQ27Z746_INTERRUPTSTATUS       0x6E /* R/O, Unit: status bits */
#define BQ27Z746_SOCDELTASETTHRESHOLD  0x6F /* R/W, Unit: percent, Range: 0..100 */

/* MAC commands */
#define BQ27Z746_MAC_CMD_DEVICETYPE                  0x0001
#define BQ27Z746_MAC_CMD_FIRMWAREVERSION             0x0002
#define BQ27Z746_MAC_CMD_HARDWAREVERSION             0x0003
#define BQ27Z746_MAC_CMD_IFCHECKSUM                  0x0004
#define BQ27Z746_MAC_CMD_STATICDFSIGNATURE           0x0005
#define BQ27Z746_MAC_CMD_CHEMID                      0x0006
#define BQ27Z746_MAC_CMD_PREV_MACWRITE               0x0007
#define BQ27Z746_MAC_CMD_STATICCHEMDFSIGNATURE       0x0008
#define BQ27Z746_MAC_CMD_ALLDFSIGNATURE              0x0009
#define BQ27Z746_MAC_CMD_SHELFENABLE                 0x000B
#define BQ27Z746_MAC_CMD_SHELFDISABLE                0x000C
#define BQ27Z746_MAC_CMD_SHUTDOWNMODE                0x0010
#define BQ27Z746_MAC_CMD_RESET1                      0x0012
#define BQ27Z746_MAC_CMD_SHIPMODEENABLE              0x0015
#define BQ27Z746_MAC_CMD_SHIPMODEDISABLE             0x0016
#define BQ27Z746_MAC_CMD_QMAX_DAY                    0x0017
#define BQ27Z746_MAC_CMD_CHARGEFETTOGGLE             0x001F
#define BQ27Z746_MAC_CMD_DISCHARGEFETTOGGLE          0x0020
#define BQ27Z746_MAC_CMD_GAUGING_IT_ENABLE           0x0021
#define BQ27Z746_MAC_CMD_FET_ENABLE                  0x0022
#define BQ27Z746_MAC_CMD_LIFETIMEDATACOLLECTION      0x0023
#define BQ27Z746_MAC_CMD_LIFETIMEDATARESET           0x0028
#define BQ27Z746_MAC_CMD_CALIBRATIONMODE             0x002D
#define BQ27Z746_MAC_CMD_LIFETIMEDATAFLUSH           0x002E
#define BQ27Z746_MAC_CMD_LIFETIMEDATASPEEDUPMODE     0x002F
#define BQ27Z746_MAC_CMD_SEALDEVICE                  0x0030
#define BQ27Z746_MAC_CMD_SECURITYKEYS                0x0035
#define BQ27Z746_MAC_CMD_RESET2                      0x0041
#define BQ27Z746_MAC_CMD_TAMBIENTSYNC                0x0047
#define BQ27Z746_MAC_CMD_DEVICE_NAME                 0x004A
#define BQ27Z746_MAC_CMD_DEVICE_CHEM                 0x004B
#define BQ27Z746_MAC_CMD_MANUFACTURER_NAME           0x004C
#define BQ27Z746_MAC_CMD_MANUFACTURE_DATE            0x004D
#define BQ27Z746_MAC_CMD_SERIAL_NUMBER               0x004E
#define BQ27Z746_MAC_CMD_SAFETYALERT                 0x0050
#define BQ27Z746_MAC_CMD_SAFETYSTATUS                0x0051
#define BQ27Z746_MAC_CMD_OPERATIONSTATUS             0x0054
#define BQ27Z746_MAC_CMD_CHARGINGSTATUS              0x0055
#define BQ27Z746_MAC_CMD_GAUGINGSTATUS               0x0056
#define BQ27Z746_MAC_CMD_MANUFACTURINGSTATUS         0x0057
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK1          0x0060
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK2          0x0061
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK3          0x0062
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK4          0x0063
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK6          0x0065
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK7          0x0065
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK8          0x0067
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK9          0x0068
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK10         0x0069
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK11         0x006A
#define BQ27Z746_MAC_CMD_LIFETIMEDATABLOCK12         0x006B
#define BQ27Z746_MAC_CMD_MANUFACTURERINFO            0x0070
#define BQ27Z746_MAC_CMD_DASTATUS1                   0x0071
#define BQ27Z746_MAC_CMD_DASTATUS2                   0x0072
#define BQ27Z746_MAC_CMD_ITSTATUS1                   0x0073
#define BQ27Z746_MAC_CMD_ITSTATUS2                   0x0074
#define BQ27Z746_MAC_CMD_ITSTATUS3                   0x0075
#define BQ27Z746_MAC_CMD_FCC_SOH                     0x0077
#define BQ27Z746_MAC_CMD_FILTERED_CAPACITY           0x0078
#define BQ27Z746_MAC_CMD_MANUFACTURERINFOB           0x007A
#define BQ27Z746_MAC_CMD_MANUFACTURERINFOC           0x007B
#define BQ27Z746_MAC_CMD_FET_CONTROL_OVERRIDE        0x0097
#define BQ27Z746_MAC_CMD_SYSTEM_RESET_ENABLE         0x00A3
#define BQ27Z746_MAC_CMD_SYSTEM_RESET                0x00A4
#define BQ27Z746_MAC_CMD_BATTSENSEOUTPUT             0x00B1
#define BQ27Z746_MAC_CMD_RATABLECELL0                0x00E0
#define BQ27Z746_MAC_CMD_ROMMODE                     0x0F00
#define BQ27Z746_MAC_CMD_DATAFLASHACCESS             0x4000
#define BQ27Z746_MAC_CMD_SWITCHTOHDQ                 0x7C40
#define BQ27Z746_MAC_CMD_EXITCALIBRATIONOUTPUT       0xF080
#define BQ27Z746_MAC_CMD_OUTPUTCCANDADCFORCALIBRATIO 0xF081
#define BQ27Z746_MAC_CMD_OUTPUTTEMPERATURECAL        0xF083
#define BQ27Z746_MAC_CMD_PROTECTORCALIBRATION        0xF0A0
#define BQ27Z746_MAC_CMD_PROTECTORIMAGE1             0xF0A1
#define BQ27Z746_MAC_CMD_PROTECTORIMAGE2             0xF0A2
#define BQ27Z746_MAC_CMD_PROTECTORIMAGESAVE          0xF0A3
#define BQ27Z746_MAC_CMD_PROTECTORIMAGELOCK          0xF0A4
#define BQ27Z746_MAC_CMD_PROTECTORFACTORYCONFIG      0xF0A5

struct bq27z746_config {
	struct i2c_dt_spec i2c;
};

#endif
