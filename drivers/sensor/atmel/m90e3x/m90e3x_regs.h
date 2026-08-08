/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_REGS_H_

enum m90e3x_reg_addr {

	/* Energy Registers */

	APENERGYT = 0x80,  /* Total Forward Active Energy */
	APENERGYA = 0x81,  /* Phase A Forward Active Energy */
	APENERGYB = 0x82,  /* Phase B Forward Active Energy */
	APENERGYC = 0x83,  /* Phase C Forward Active Energy */
	ANENERGYT = 0x84,  /* Total Reverse Active Energy */
	ANENERGYA = 0x85,  /* Phase A Reverse Active Energy */
	ANENERGYB = 0x86,  /* Phase B Reverse Active Energy */
	ANENERGYC = 0x87,  /* Phase C Reverse Active Energy */
	RPENERGYT = 0x88,  /* Total Forward Reactive Energy */
	RPENERGYA = 0x89,  /* Phase A Forward Reactive Energy */
	RPENERGYB = 0x8A,  /* Phase B Forward Reactive Energy */
	RPENERGYC = 0x8B,  /* Phase C Forward Reactive Energy */
	RNENERGYT = 0x8C,  /* Total Reverse Reactive Energy */
	RNENERGYA = 0x8D,  /* Phase A Reverse Reactive Energy */
	RNENERGYB = 0x8E,  /* Phase B Reverse Reactive Energy */
	RNENERGYC = 0x8F,  /* Phase C Reverse Active Energy */
	SAENERGYT = 0x90,  /* Total (Arithmetic Sum) Apparent Energy */
	SENERGYA = 0x91,   /* Phase A Apparent Energy */
	SENERGYB = 0x92,   /* Phase B Apparent Energy */
	SENERGYC = 0x93,   /* Phase C Apparent Energy */
	SVENERGYT = 0x94,  /* Total (Vector Sum) Apparent Energy (M90E36AS only) */
	ENSTATUS0 = 0x95,  /* Metering Status 0 (M90E36AS only) */
	ENSTATUS1 = 0x96,  /* Metering Status 1 (M90E36AS only) */
	SVMEANT = 0x98,    /* Total (Vector Sum) Apparent Power (M90E36AS only) */
	SVMEANTLSB = 0x99, /* Lower Word of Total (Vector Sum) Apparent Power (M90E36AS only) */

	/* Fundamental / Harmonic Energy Register*/

	APENERGYTF = 0xA0, /* Total Forward Active Fundamental Energy */
	APENERGYAF = 0xA1, /* Phase A Forward Active Fundamental Energy */
	APENERGYBF = 0xA2, /* Phase B Forward Active Fundamental Energy */
	APENERGYCF = 0xA3, /* Phase C Forward Active Fundamental Energy */
	ANENERGYTF = 0xA4, /* Total Reverse Active Fundamental Energy */
	ANENERGYAF = 0xA5, /* Phase A Reverse Active Fundamental Energy */
	ANENERGYBF = 0xA6, /* Phase B Reverse Active Fundamental Energy */
	ANENERGYCF = 0xA7, /* Phase C Reverse Active Fundamental Energy */
	APENERGYTH = 0xA8, /* Total Forward Active Harmonic Energy */
	APENERGYAH = 0xA9, /* Phase A Forward Active Harmonic Energy */
	APENERGYBH = 0xAA, /* Phase B Forward Active Harmonic Energy */
	APENERGYCH = 0xAB, /* Phase C Forward Active Harmonic Energy */
	ANENERGYTH = 0xAC, /* Total Reverse Active Harmonic Energy */
	ANENERGYAH = 0xAD, /* Phase A Reverse Active Harmonic Energy */
	ANENERGYBH = 0xAE, /* Phase B Reverse Active Harmonic Energy */
	ANENERGYCH = 0xAF, /* Phase C Reverse Active Harmonic Energy */

	/* Power and Power Factor Registers */

	PMEANT = 0xB0,     /* Total (all-phase-sum) Active Power */
	PMEANA = 0xB1,     /* Phase A Active Power */
	PMEANB = 0xB2,     /* Phase B Active Power */
	PMEANC = 0xB3,     /* Phase C Active Power */
	QMEANT = 0xB4,     /* Total (all-phase-sum) Reactive Power */
	QMEANA = 0xB5,     /* Phase A Reactive Power */
	QMEANB = 0xB6,     /* Phase B Reactive Power */
	QMEANC = 0xB7,     /* Phase C Reactive Power */
	SMEANT = 0xB8,     /* Total (Arithmetic Sum) Apparent Power */
	SMEANA = 0xB9,     /* Phase A Apparent Power */
	SMEANB = 0xBA,     /* Phase B Apparent Power */
	SMEANC = 0xBB,     /* Phase C Apparent Power */
	PFMEANT = 0xBC,    /* Total Power Factor */
	PFMEANA = 0xBD,    /* Phase A Power Factor */
	PFMEANB = 0xBE,    /* Phase B Power Factor */
	PFMEANC = 0xBF,    /* Phase C Power Factor */
	PMEANTLSB = 0xC0,  /* Lower Word of Total (all-phase-sum) Active Power */
	PMEANALSB = 0xC1,  /* Lower Word of Phase A Active Power */
	PMEANBLSB = 0xC2,  /* Lower Word of Phase B Active Power */
	PMEANCLSB = 0xC3,  /* Lower Word of Phase C Active Power */
	QMEANTLSB = 0xC4,  /* Lower Word of Total (all-phase-sum) Reactive Power */
	QMEANALSB = 0xC5,  /* Lower Word of Phase A Reactive Power */
	QMEANBLSB = 0xC6,  /* Lower Word of Phase B Reactive Power */
	QMEANCLSB = 0xC7,  /* Lower Word of Phase C Reactive Power */
	SAMEANTLSB = 0xC8, /* Lower Word of Total (Arithmetic Sum) Apparent Power */
	SMEANALSB = 0xC9,  /* Lower Word of Phase A Apparent Power */
	SMEANBLSB = 0xCA,  /* Lower Word of Phase B Apparent Power */
	SMEANCLSB = 0xCB,  /* Lower Word of Phase C Apparent Power */

	/* Fundamental / Harmonic Power and Voltage / Current RMS Registers */

	PMEANTF = 0xD0,    /* Total Active Fundamental Power */
	PMEANAF = 0xD1,    /* Phase A Active Fundamental Power */
	PMEANBF = 0xD2,    /* Phase B Active Fundamental Power */
	PMEANCF = 0xD3,    /* Phase C Active Fundamental Power */
	PMEANTH = 0xD4,    /* Total Active Harmonic Power */
	PMEANAH = 0xD5,    /* Phase A Active Harmonic Power */
	PMEANBH = 0xD6,    /* Phase B Active Harmonic Power */
	PMEANCH = 0xD7,    /* Phase C Active Harmonic Power */
	IRMSN1 = 0xD8,     /* N Line Sampled Current RMS (M90E36AS only) */
	URMSA = 0xD9,      /* Phase A Voltage RMS */
	URMSB = 0xDA,      /* Phase B Voltage RMS */
	URMSC = 0xDB,      /* Phase C Voltage RMS */
	IRMSN = 0xDC,      /* N Line Calculated Current RMS */
	IRMSA = 0xDD,      /* Phase A Current RMS */
	IRMSB = 0xDE,      /* Phase B Current RMS */
	IRMSC = 0xDF,      /* Phase C Current RMS */
	PMEANTFLSB = 0xE0, /* Lower Word of Total Active Fundamental Power */
	PMEANAFLSB = 0xE1, /* Lower Word of Phase A Active Fundamental Power */
	PMEANBFLSB = 0xE2, /* Lower Word of Phase B Active Fundamental Power */
	PMEANCFLSB = 0xE3, /* Lower Word of Phase C Active Fundamental Power */
	PMEANTHLSB = 0xE4, /* Lower Word of Total Active Harmonic Power */
	PMEANAHLSB = 0xE5, /* Lower Word of Phase A Active Harmonic Power */
	PMEANBHLSB = 0xE6, /* Lower Word of Phase B Active Harmonic Power */
	PMEANCHLSB = 0xE7, /* Lower Word of Phase C Active Harmonic Power */
	URMSALSB = 0xE9,   /* Lower Word of Phase A Voltage RMS */
	URMSBLSB = 0xEA,   /* Lower Word of Phase B Voltage RMS */
	URMSCLSB = 0xEB,   /* Lower Word of Phase C Voltage RMS */
	IRMSALSB = 0xED,   /* Lower Word of Phase A Current RMS */
	IRMSBLSB = 0xEE,   /* Lower Word of Phase B Current RMS */
	IRMSCLSB = 0xEF,   /* Lower Word of Phase C Current RMS */

	/* Frequency, Angle and Temperature Registers */

	FREQ = 0xF8,    /* Frequency */
	PANGLEA = 0xF9, /* Phase A Mean Phase Angle */
	PANGLEB = 0xFA, /* Phase B Mean Phase Angle */
	PANGLEC = 0xFB, /* Phase C Mean Phase Angle */
	TEMP = 0xFC,    /* Measured Temperature */
	UANGLEA = 0xFD, /* Phase A Voltage Phase Angle */
	UANGLEB = 0xFE, /* Phase B Voltage Phase Angle */
	UANGLEC = 0xFF, /* Phase C Voltage Phase Angle */
};

#endif /* _ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_REGS_H_ */
