/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E32AS_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E32AS_REGS_H_

enum m90e32as_reg_addr {
	/* Status and Special Registers */

	METEREN = 0x00,       /* Metering Enable */
	CHANNELMAPI = 0x01,   /* Current Channel Mapping Configuration */
	CHANNELMAPU = 0x02,   /* Voltage Channel Mapping Configuration */
	SAGPEAKDETCFG = 0x05, /* Sag and Peak Detector Period Configuration */
	OVTHCFG = 0x06,       /* Over Voltage Threshold */
	ZXCONFIG = 0x07, /* Zero-Crossing Configuration (Configuration of ZX0/1/2 pins' source) */
	SAGTH = 0x08,    /* Voltage Sag Threshold */
	PHASELOSSTH = 0x09,  /* Voltage Phase Losing Threshold */
	INWARNTH = 0x0A,     /* Neutral Current (Calculated) Warning Threshold */
	OITH = 0x0B,         /* Over Current Threshold */
	FREQLOTH = 0x0C,     /* Low Threshold for Frequency Detection */
	FREQHITH = 0x0D,     /* High Threshold for Frequency Detection */
	PMPWRCTRL = 0x0E,    /* Partial Measurement Mode Power Control */
	IRQ0MERGECFG = 0x0F, /* IRQ0 Merge Configuration */

	/* Low Power Mode Registers */

	DETECTCTRL = 0x10,  /* Current Detect Control */
	DETECTTH1 = 0x11,   /* Channel 1 Current Threshold in Detection Mode */
	DETECTTH2 = 0x12,   /* Channel 2 Current Threshold in Detection Mode */
	DETECTTH3 = 0x13,   /* Channel 3 Current Threshold in Detection Mode */
	IDCOFFSETA = 0x14,  /* Phase A Current DC offset */
	IDCOFFSETB = 0x15,  /* Phase B Current DC offset */
	IDCOFFSETC = 0x16,  /* Phase C Current DC offset */
	UDCOFFSETA = 0x17,  /* Voltage DC offset for Channel A */
	UDCOFFSETB = 0x18,  /* Voltage DC offset for Channel B */
	UDCOFFSETC = 0x19,  /* Voltage DC offset for Channel C */
	UGAINTAB = 0x1A,    /* Voltage Gain Temperature Compensation for Phase A/B */
	UGAINTC = 0x1B,     /* Voltage Gain Temperature Compensation for Phase C */
	PHIFREQCOMP = 0x1C, /* Phase Compensation for Frequency */
	LOGIRMS0 = 0x20,    /* Current (Log Irms0) Configuration for Segment Compensation */
	LOGIRMS1 = 0x21,    /* Current (Log Irms1) Configuration for Segment Compensation */
	F0 = 0x22,          /* Nominal Frequency */
	T0 = 0x23,          /* Nominal Temperature */
	PHIAIRMS01 = 0x24,  /* Phase A Phase Compensation for Current Segment 0 and 1 */
	PHIAIRMS2 = 0x25,   /* Phase A Phase Compensation for Current Segment 2 */
	GAINAIRMS01 = 0x26, /* Phase A Gain Compensation for Current Segment 0 and 1 */
	GAINAIRMS2 = 0x27,  /* Phase A Gain Compensation for Current Segment 2 */
	PHIBIRMS01 = 0x28,  /* Phase B Phase Compensation for Current Segment 0 and 1 */
	PHIBIRMS2 = 0x29,   /* Phase B Phase Compensation for Current Segment 2 */
	GAINBIRMS01 = 0x2A, /* Phase B Gain Compensation for Current Segment 0 and 1 */
	GAINBIRMS2 = 0x2B,  /* Phase B Gain Compensation for Current Segment 2 */
	PHICIRMS01 = 0x2C,  /* Phase C Phase Compensation for Current Segment 0 and 1 */
	PHICIRMS2 = 0x2D,   /* Phase C Phase Compensation for Current Segment 2 */
	GAINCIRMS01 = 0x2E, /* Phase C Gain Compensation for Current Segment 0 and 1 */
	GAINCIRMS2 = 0x2F,  /* Phase C Gain Compensation for Current Segment 2 */

	/* Configuration Registers */

	PLCONSTH = 0x31, /* High Word of PL_Constant */
	PLCONSTL = 0x32, /* Low Word of PL_Constant */
	MMODE0 = 0x33,   /* Metering Method Configuration */
	MMODE1 = 0x34,   /* PGA Gain Configuration */
	PSTARTTH = 0x35, /* Active Startup Power Threshold */
	QSTARTTH = 0x36, /* Reactive Startup Power Threshold */
	SSTARTTH = 0x37, /* Apparent Startup Power Threshold */
	PPHASETH = 0x38, /* Startup Power Threshold for Any Phase (Active Energy Accumulation) */
	QPHASETH = 0x39, /* Startup Power Threshold for Any Phase (Reactive Energy Accumulation) */
	SPHASETH = 0x3A, /* Startup Power Threshold for Any Phase (Apparent Energy Accumulation) */

	/* Calibration Registers */

	POFFSETA = 0x41, /* Phase A Active Power offset */
	QOFFSETA = 0x42, /* Phase A Reactive Power offset */
	POFFSETB = 0x43, /* Phase B Active Power offset */
	QOFFSETB = 0x44, /* Phase B Reactive Power offset */
	POFFSETC = 0x45, /* Phase C Active Power offset */
	QOFFSETC = 0x46, /* Phase C Reactive Power offset */
	PQGAINA = 0x47,  /* Phase A Calibration Gain */
	PHIA = 0x48,     /* Phase A Calibration Phase Angle */
	PQGAINB = 0x49,  /* Phase B Calibration Gain */
	PHIB = 0x4A,     /* Phase B Calibration Phase Angle */
	PQGAINC = 0x4B,  /* Phase C Calibration Gain */
	PHIC = 0x4C,     /* Phase C Calibration Phase Angle */

	/* Fundamental / Harmonic Energy Calibration Registers */

	POFFSETAF = 0x51, /* Phase A Fundamental Active Power offset */
	POFFSETBF = 0x52, /* Phase B Fundamental Active Power offset */
	POFFSETCF = 0x53, /* Phase C Fundamental Active Power offset */
	PGAINAF = 0x54,   /* Phase A Fundamental Calibration Gain */
	PGAINBF = 0x55,   /* Phase B Fundamental Calibration Gain */
	PGAINCF = 0x56,   /* Phase C Fundamental Calibration Gain */

	/* Measurement Calibration Registers */

	UGAINA = 0x61,   /* Phase A Voltage RMS Gain */
	IGAINA = 0x62,   /* Phase A Current RMS Gain */
	UOFFSETA = 0x63, /* Phase A Voltage RMS offset */
	IOFFSETA = 0x64, /* Phase A Current RMS offset */
	UGAINB = 0x65,   /* Phase B Voltage RMS Gain */
	IGAINB = 0x66,   /* Phase B Current RMS Gain */
	UOFFSETB = 0x67, /* Phase B Voltage RMS offset */
	IOFFSETB = 0x68, /* Phase B Current RMS offset */
	UGAINC = 0x69,   /* Phase C Voltage RMS Gain */
	IGAINC = 0x6A,   /* Phase C Current RMS Gain */
	UOFFSETC = 0x6B, /* Phase C Voltage RMS offset */
	IOFFSETC = 0x6C, /* Phase C Current RMS offset */

	/* EMM Status Registers */

	SOFTRESET = 0x70,    /* Software Reset */
	EMMSTATE0 = 0x71,    /* EMM State 0 */
	EMMSTATE1 = 0x72,    /* EMM State 1 */
	EMMINTSTATE0 = 0x73, /* EMM Interrupt Status 0 */
	EMMINTSTATE1 = 0x74, /* EMM Interrupt Status 1 */
	EMMINTEN0 = 0x75,    /* EMM Interrupt Enable 0 */
	EMMINTEN1 = 0x76,    /* EMM Interrupt Enable 1 */
	LASTSPIDATA = 0x78,  /* Last Read/Write SPI Value */
	CRCERRSTATUS = 0x79, /* CRC Error Status */
	CRCDIGEST = 0x7A,    /* CRC Digest */
	CFGREGACCEN = 0x7F,  /* Configure Register Access Enable */

	/* Peak Voltage/Current Registers */

	UPEAKA = 0xF1, /* Channel A Voltage Peak */
	UPEAKB = 0xF2, /* Channel B Voltage Peak */
	UPEAKC = 0xF3, /* Channel C Voltage Peak */
	IPEAKA = 0xF5, /* Channel A Current Peak */
	IPEAKB = 0xF6, /* Channel B Current Peak */
	IPEAKC = 0xF7, /* Channel C Current Peak */
};

#endif /* _ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E32AS_REGS_H_ */
