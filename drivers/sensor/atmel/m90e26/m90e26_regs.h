/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_REGS_H_

enum m90e26_reg_addr
{
  /* Status and Special Registers */

  SoftReset       = 0x00, // Software Reset
  SysStatus       = 0x01, // System Status
  FuncEn          = 0x02, // Function Enable
  SagTh           = 0x03, // Voltage Sag Threshold
  SmallPMod       = 0x04, // Small Power Mode
  LastData        = 0x06, // Last Read/Write SPI/UART value

  /* Metering Calibration and Configuration Registers */

  LSB             = 0x08, // RMS/Power 16-bit LSB
  CalStart        = 0x20, // Calibration Start Command
  PLconstH        = 0x21, // High Word of PL_Constant
  PLconstL        = 0x22, // Low Word of PL_Constant
  Lgain           = 0x23, // L Line Calibration Gain
  Lphi            = 0x24, // L Line Calibration Angle
  Ngain           = 0x25, // N Line Calibration Gain
  Nphi            = 0x26, // N Line Calibration Angle
  PStartTh        = 0x27, // Active Startup Power Threshold
  PNolTh          = 0x28, // Active No-Load Power Threshold
  QStartTh        = 0x29, // Reactive Startup Power Threshold
  QNolTh          = 0x2A, // Reactive No-Load Power Threshold
  MMode           = 0x2B, // Metering Mode Configuration
  CS1             = 0x2C, // Checksum 1

  /* Measurement Calibration Registers */

  AdjStart       = 0x30, // Measurement Adjustment Start Command
  Ugain          = 0x31, // Voltage RMS Gain
  IgainL         = 0x32, // L Line Current RMS Gain
  IgainN         = 0x33, // N Line Current RMS Gain
  Uoffset        = 0x34, // Voltage Offset
  IoffsetL       = 0x35, // L Line Current Offset
  IoffsetN       = 0x36, // N Line Current Offset
  PoffsetL       = 0x37, // L Line Active Power Offset
  QoffsetL       = 0x38, // L Line Reactive Power Offset
  PoffsetN       = 0x39, // N Line Active Power Offset
  QoffsetN       = 0x3A, // N Line Reactive Power Offset
  CS2            = 0x3C, // Checksum 2

  /* Energy Registers */

  APenergy       = 0x40,  // Forward Active Energy
  ANenergy       = 0x41,  // Reverse Active Energy
  ATenergy       = 0x42,  // Total Active Energy
  RPenergy       = 0x43,  // Forward (Inductive) Reactive Energy
  RNenergy       = 0x44,  // Reverse (Capacitive) Reactive Energy
  RTenergy       = 0x45,  // Absolute Reactive Energy
  EnStatus       = 0x46,  // Metering Status

  /* Measurement Registers */

  Irms           = 0x48,  // L Line Current RMS
  Urms           = 0x49,  // Voltage RMS
  Pmean          = 0x4A,  // L Line Mean Active Power
  Qmean          = 0x4B,  // L Line Mean Reactive Power
  Freq           = 0x4C,  // Voltage Frequency
  PowerF         = 0x4D,  // L Line Power Factor
  Pangle         = 0x4E,  // Phase Angle between Voltage and L Line Current
  Smean          = 0x4F,  // L Line Mean Apparent Power
  Irms2          = 0x68,  // N Line Current RMS
  Pmean2         = 0x6A,  // N Line Mean Active Power
  Qmean2         = 0x6B,  // N Line Mean Reactive Power
  PowerF2        = 0x6D,  // N Line Power Factor
  Pangle2        = 0x6E,  // Phase Angle between Voltage and N Line Current
  Smean2         = 0x6F,  // N Line Mean Apparent Power
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_REGS_H_ */