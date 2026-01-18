/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_REGS_H_

enum m90e3x_reg_addr
{

  /* Energy Registers */

  APenergyT       = 0x80,  // Total Forward Active Energy
  APenergyA       = 0x81,  // Phase A Forward Active Energy
  APenergyB       = 0x82,  // Phase B Forward Active Energy
  APenergyC       = 0x83,  // Phase C Forward Active Energy
  ANenergyT       = 0x84,  // Total Reverse Active Energy
  ANenergyA       = 0x85,  // Phase A Reverse Active Energy
  ANenergyB       = 0x86,  // Phase B Reverse Active Energy
  ANenergyC       = 0x87,  // Phase C Reverse Active Energy
  RPenergyT       = 0x88,  // Total Forward Reactive Energy
  RPenergyA       = 0x89,  // Phase A Forward Reactive Energy
  RPenergyB       = 0x8A,  // Phase B Forward Reactive Energy
  RPenergyC       = 0x8B,  // Phase C Forward Reactive Energy
  RNenergyT       = 0x8C,  // Total Reverse Reactive Energy
  RNenergyA       = 0x8D,  // Phase A Reverse Reactive Energy
  RNenergyB       = 0x8E,  // Phase B Reverse Reactive Energy
  RNenergyC       = 0x8F,  // Phase C Reverse Reactive Energy
  SAenergyT       = 0x90,  // Total (Arithmetic Sum) Apparent Energy
  SenergyA        = 0x91,  // Phase A Apparent Energy
  SenergyB        = 0x92,  // Phase B Apparent Energy
  SenergyC        = 0x93,  // Phase C Apparent Energy
  SVenergyT       = 0x94,  // Total (Vector Sum) Apparent Energy (M90E36AS only)
  EnStatus0       = 0x95,  // Metering Status 0 (M90E36AS only)
  EnStatus1       = 0x96,  // Metering Status 1 (M90E36AS only)
  SVmeanT         = 0x98,  // Total (Vector Sum) Apparent Power (M90E36AS only)
  SVmeanTLSB      = 0x99,  // Lower Word of Total (Vector Sum) Apparent Power (M90E36AS only)

  /* Fundamental / Harmonic Energy Register*/

  APenergyTF      = 0xA0,  // Total Forward Active Fundamental Energy
  APenergyAF      = 0xA1,  // Phase A Forward Active Fundamental Energy
  APenergyBF      = 0xA2,  // Phase B Forward Active Fundamental Energy
  APenergyCF      = 0xA3,  // Phase C Forward Active Fundamental Energy
  ANenergyTF      = 0xA4,  // Total Reverse Active Fundamental Energy
  ANenergyAF      = 0xA5,  // Phase A Reverse Active Fundamental Energy
  ANenergyBF      = 0xA6,  // Phase B Reverse Active Fundamental Energy
  ANenergyCF      = 0xA7,  // Phase C Reverse Active Fundamental Energy
  APenergyTH      = 0xA8,  // Total Forward Active Harmonic Energy
  APenergyAH      = 0xA9,  // Phase A Forward Active Harmonic Energy
  APenergyBH      = 0xAA,  // Phase B Forward Active Harmonic Energy
  APenergyCH      = 0xAB,  // Phase C Forward Active Harmonic Energy
  ANenergyTH      = 0xAC,  // Total Reverse Active Harmonic Energy
  ANenergyAH      = 0xAD,  // Phase A Reverse Active Harmonic Energy
  ANenergyBH      = 0xAE,  // Phase B Reverse Active Harmonic Energy
  ANenergyCH      = 0xAF,  // Phase C Reverse Active Harmonic Energy

  /* Power and Power Factor Registers */

  PmeanT          = 0xB0,  // Total (all-phase-sum) Active Power
  PmeanA          = 0xB1,  // Phase A Active Power
  PmeanB          = 0xB2,  // Phase B Active Power
  PmeanC          = 0xB3,  // Phase C Active Power
  QmeanT          = 0xB4,  // Total (all-phase-sum) Reactive Power
  QmeanA          = 0xB5,  // Phase A Reactive Power
  QmeanB          = 0xB6,  // Phase B Reactive Power
  QmeanC          = 0xB7,  // Phase C Reactive Power
  SmeanT          = 0xB8,  // Total (Arithmetic Sum) Apparent Power
  SmeanA          = 0xB9,  // Phase A Apparent Power
  SmeanB          = 0xBA,  // Phase B Apparent Power
  SmeanC          = 0xBB,  // Phase C Apparent Power
  PFmeanT         = 0xBC,  // Total Power Factor
  PFmeanA         = 0xBD,  // Phase A Power Factor
  PFmeanB         = 0xBE,  // Phase B Power Factor
  PFmeanC         = 0xBF,  // Phase C Power Factor
  PmeanTLSB       = 0xC0,  // Lower Word of Total (all-phase-sum) Active Power
  PmeanALSB       = 0xC1,  // Lower Word of Phase A Active Power
  PmeanBLSB       = 0xC2,  // Lower Word of Phase B Active Power
  PmeanCLSB       = 0xC3,  // Lower Word of Phase C Active Power
  QmeanTLSB       = 0xC4,  // Lower Word of Total (all-phase-sum) Reactive Power
  QmeanALSB       = 0xC5,  // Lower Word of Phase A Reactive Power
  QmeanBLSB       = 0xC6,  // Lower Word of Phase B Reactive Power
  QmeanCLSB       = 0xC7,  // Lower Word of Phase C Reactive Power
  SAmeanTLSB      = 0xC8,  // Lower Word of Total (Arithmetic Sum) Apparent Power
  SmeanALSB       = 0xC9,  // Lower Word of Phase A Apparent Power
  SmeanBLSB       = 0xCA,  // Lower Word of Phase B Apparent Power
  SmeanCLSB       = 0xCB,  // Lower Word of Phase C Apparent Power

  /* Fundamental / Harmonic Power and Voltage / Current RMS Registers */

  PmeanTF         = 0xD0,  // Total Active Fundamental Power
  PmeanAF         = 0xD1,  // Phase A Active Fundamental Power
  PmeanBF         = 0xD2,  // Phase B Active Fundamental Power
  PmeanCF         = 0xD3,  // Phase C Active Fundamental Power
  PmeanTH         = 0xD4,  // Total Active Harmonic Power
  PmeanAH         = 0xD5,  // Phase A Active Harmonic Power
  PmeanBH         = 0xD6,  // Phase B Active Harmonic Power
  PmeanCH         = 0xD7,  // Phase C Active Harmonic Power
  IrmsN1          = 0xD8,  // N Line Sampled Current RMS (M90E36AS only)
  UrmsA           = 0xD9,  // Phase A Voltage RMS
  UrmsB           = 0xDA,  // Phase B Voltage RMS
  UrmsC           = 0xDB,  // Phase C Voltage RMS
  IrmsN           = 0xDC,  // N Line Calculated Current RMS
  IrmsA           = 0xDD,  // Phase A Current RMS
  IrmsB           = 0xDE,  // Phase B Current RMS
  IrmsC           = 0xDF,  // Phase C Current RMS
  PmeanTFLSB      = 0xE0,  // Lower Word of Total Active Fundamental Power
  PmeanAFLSB      = 0xE1,  // Lower Word of Phase A Active Fundamental Power
  PmeanBFLSB      = 0xE2,  // Lower Word of Phase B Active Fundamental Power
  PmeanCFLSB      = 0xE3,  // Lower Word of Phase C Active Fundamental Power
  PmeanTHLSB      = 0xE4,  // Lower Word of Total Active Harmonic Power
  PmeanAHLSB      = 0xE5,  // Lower Word of Phase A Active Harmonic Power
  PmeanBHLSB      = 0xE6,  // Lower Word of Phase B Active Harmonic Power
  PmeanCHLSB      = 0xE7,  // Lower Word of Phase C Active Harmonic Power
  UrmsALSB        = 0xE9,  // Lower Word of Phase A Voltage RMS
  UrmsBLSB        = 0xEA,  // Lower Word of Phase B Voltage RMS
  UrmsCLSB        = 0xEB,  // Lower Word of Phase C Voltage RMS
  IrmsALSB        = 0xED,  // Lower Word of Phase A Current RMS
  IrmsBLSB        = 0xEE,  // Lower Word of Phase B Current RMS
  IrmsCLSB        = 0xEF,  // Lower Word of Phase C Current RMS

  /* Frequency, Angle and Temperature Registers */

  Freq            = 0xF8,  // Frequency
  PAngleA         = 0xF9,  // Phase A Mean Phase Angle
  PAngleB         = 0xFA,  // Phase B Mean Phase Angle
  PAngleC         = 0xFB,  // Phase C Mean Phase Angle
  Temp            = 0xFC,  // Measured Temperature
  UangleA         = 0xFD,  // Phase A Voltage Phase Angle
  UangleB         = 0xFE,  // Phase B Voltage Phase Angle
  UangleC         = 0xFF,  // Phase C Voltage Phase Angle

};

#endif /* _ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_REGS_H_ */
