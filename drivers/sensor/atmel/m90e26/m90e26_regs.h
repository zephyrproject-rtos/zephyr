/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_REGS_H_

enum m90e26_reg_addr {
	/* Status and Special Registers */

	SOFTRESET = 0x00, /* Software Reset */
	SYSSTATUS = 0x01, /* System Status */
	FUNCEN = 0x02,    /* Function Enable */
	SAGTH = 0x03,     /* Voltage Sag Threshold */
	SMALLPMOD = 0x04, /* Small Power Mode */
	LASTDATA = 0x06,  /* Last Read/Write SPI/UART value */

	/* Metering Calibration and Configuration Registers */

	LSB = 0x08,      /* RMS/Power 16-bit LSB */
	CALSTART = 0x20, /* Calibration Start Command */
	PLCONSTH = 0x21, /* High Word of PL_Constant */
	PLCONSTL = 0x22, /* Low Word of PL_Constant */
	LGAIN = 0x23,    /* L Line Calibration Gain */
	LPHI = 0x24,     /* L Line Calibration Angle */
	NGAIN = 0x25,    /* N Line Calibration Gain */
	NPHI = 0x26,     /* N Line Calibration Angle */
	PSTARTTH = 0x27, /* Active Startup Power Threshold */
	PNOLTH = 0x28,   /* Active No-Load Power Threshold */
	QSTARTTH = 0x29, /* Reactive Startup Power Threshold */
	QNOLTH = 0x2A,   /* Reactive No-Load Power Threshold */
	MMODE = 0x2B,    /* Metering Mode Configuration */
	CS1 = 0x2C,      /* Checksum 1 */

	/* Measurement Calibration Registers */

	ADJSTART = 0x30, /* Measurement Adjustment Start Command */
	UGAIN = 0x31,    /* Voltage RMS Gain */
	IGAINL = 0x32,   /* L Line Current RMS Gain */
	IGAINN = 0x33,   /* N Line Current RMS Gain */
	UOFFSET = 0x34,  /* Voltage Offset */
	IOFFSETL = 0x35, /* L Line Current Offset */
	IOFFSETN = 0x36, /* N Line Current Offset */
	POFFSETL = 0x37, /* L Line Active Power Offset */
	QOFFSETL = 0x38, /* L Line Reactive Power Offset */
	POFFSETN = 0x39, /* N Line Active Power Offset */
	QOFFSETN = 0x3A, /* N Line Reactive Power Offset */
	CS2 = 0x3C,      /* Checksum 2 */

	/* Energy Registers */

	APENERGY = 0x40, /* Forward Active Energy */
	ANENERGY = 0x41, /* Reverse Active Energy */
	ATENERGY = 0x42, /* Total Active Energy */
	RPENERGY = 0x43, /* Forward (Inductive) Reactive Energy */
	RNENERGY = 0x44, /* Reverse (Capacitive) Reactive Energy */
	RTENERGY = 0x45, /* Absolute Reactive Energy */
	ENSTATUS = 0x46, /* Metering Status */

	/* Measurement Registers */

	IRMS = 0x48,    /* L Line Current RMS */
	URMS = 0x49,    /* Voltage RMS */
	PMEAN = 0x4A,   /* L Line Mean Active Power */
	QMEAN = 0x4B,   /* L Line Mean Reactive Power */
	FREQ = 0x4C,    /* Voltage Frequency */
	POWERF = 0x4D,  /* L Line Power Factor */
	PANGLE = 0x4E,  /* Phase Angle between Voltage and L Line Current */
	SMEAN = 0x4F,   /* L Line Mean Apparent Power */
	IRMS2 = 0x68,   /* N Line Current RMS */
	PMEAN2 = 0x6A,  /* N Line Mean Active Power */
	QMEAN2 = 0x6B,  /* N Line Mean Reactive Power */
	POWERF2 = 0x6D, /* N Line Power Factor */
	PANGLE2 = 0x6E, /* Phase Angle between Voltage and N Line Current */
	SMEAN2 = 0x6F,  /* N Line Mean Apparent Power */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_REGS_H_ */
