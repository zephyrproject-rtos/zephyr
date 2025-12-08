/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E32AS_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E32AS_REGS_H_

enum m90e32as_reg_addr
{

  /* Status and Special Registers */

  MeterEn         = 0x00,  // Metering Enable
  ChannelMapI     = 0x01,  // Current Channel Mapping Configuration
  ChannelMapU     = 0x02,  // Voltage Channel Mapping Configuration
  SagPeakDetCfg   = 0x05,  // Sag and Peak Detector Period Configuration
  OVthCfg         = 0x06,  // Over Voltage Threshold
  ZXConfig        = 0x07,  // Zero-Crossing Configuration (Configuration of ZX0/1/2 pins' source)
  SagTh           = 0x08,  // Voltage Sag Threshold
  PhaseLossTh     = 0x09,  // Voltage Phase Losing Threshold (Similar to Voltage Sag Threshold register)
  InWarnTh        = 0x0A,  // Neutral Current (Calculated) Warning Threshold
  OIth            = 0x0B,  // Over Current Threshold
  FreqLoTh        = 0x0C,  // Low Threshold for Frequency Detection
  FreqHiTh        = 0x0D,  // High Threshold for Frequency Detection
  PMPwrCtrl       = 0x0E,  // Partial Measurement Mode Power Control
  IRQ0MergeCfg    = 0x0F,  // IRQ0 Merge Configuration (Refer to 4.2.2 Reliability Enhancement Feature)

  /* Low Power Mode Registers */

  DetectCtrl      = 0x10,  // Current Detect Control
  DetectTh1       = 0x11,  // Channel 1 Current Threshold in Detection Mode
  DetectTh2       = 0x12,  // Channel 2 Current Threshold in Detection Mode
  DetectTh3       = 0x13,  // Channel 3 Current Threshold in Detection Mode
  IDCoffsetA      = 0x14,  // Phase A Current DC offset
  IDCoffsetB      = 0x15,  // Phase B Current DC offset
  IDCoffsetC      = 0x16,  // Phase C Current DC offset
  UDCoffsetA      = 0x17,  // Voltage DC offset for Channel A
  UDCoffsetB      = 0x18,  // Voltage DC offset for Channel B
  UDCoffsetC      = 0x19,  // Voltage DC offset for Channel C
  UGainTAB        = 0x1A,  // Voltage Gain Temperature Compensation for Phase A/B
  UGainTC         = 0x1B,  // Voltage Gain Temperature Compensation for Phase C
  PhiFreqComp     = 0x1C,  // Phase Compensation for Frequency
  LOGIrms0        = 0x20,  // Current (Log Irms0) Configuration for Segment Compensation
  LOGIrms1        = 0x21,  // Current (Log Irms1) Configuration for Segment Compensation
  F0              = 0x22,  // Nominal Frequency
  T0              = 0x23,  // Nominal Temperature
  PhiAIrms01      = 0x24,  // Phase A Phase Compensation for Current Segment 0 and 1
  PhiAIrms2       = 0x25,  // Phase A Phase Compensation for Current Segment 2
  GainAIrms01     = 0x26,  // Phase A Gain Compensation for Current Segment 0 and 1
  GainAIrms2      = 0x27,  // Phase A Gain Compensation for Current Segment 2
  PhiBIrms01      = 0x28,  // Phase B Phase Compensation for Current Segment 0 and 1
  PhiBIrms2       = 0x29,  // Phase B Phase Compensation for Current Segment 2
  GainBIrms01     = 0x2A,  // Phase B Gain Compensation for Current Segment 0 and 1
  GainBIrms2      = 0x2B,  // Phase B Gain Compensation for Current Segment 2
  PhiCIrms01      = 0x2C,  // Phase C Phase Compensation for Current Segment 0 and 1
  PhiCIrms2       = 0x2D,  // Phase C Phase Compensation for Current Segment 2
  GainCIrms01     = 0x2E,  // Phase C Gain Compensation for Current Segment 0 and 1
  GainCIrms2      = 0x2F,  // Phase C Gain Compensation for Current Segment 2

  /* Configuration Registers */

  PLconstH        = 0x31,  // High Word of PL_Constant
  PLconstL        = 0x32,  // Low Word of PL_Constant
  MMode0          = 0x33,  // Metering Method Configuration
  MMode1          = 0x34,  // PGA Gain Configuration
  PStartTh        = 0x35,  // Active Startup Power Threshold
  QStartTh        = 0x36,  // Reactive Startup Power Threshold
  SStartTh        = 0x37,  // Apparent Startup Power Threshold
  PPhaseTh        = 0x38,  // Startup Power Threshold for Any Phase (Active Energy Accumulation)
  QPhaseTh        = 0x39,  // Startup Power Threshold for Any Phase (Reactive Energy Accumulation)
  SPhaseTh        = 0x3A,  // Startup Power Threshold for Any Phase (Apparent Energy Accumulation)

  /* Calibration Registers */

  PoffsetA        = 0x41,  // Phase A Active Power offset
  QoffsetA        = 0x42,  // Phase A Reactive Power offset
  PoffsetB        = 0x43,  // Phase B Active Power offset
  QoffsetB        = 0x44,  // Phase B Reactive Power offset
  PoffsetC        = 0x45,  // Phase C Active Power offset
  QoffsetC        = 0x46,  // Phase C Reactive Power offset
  PQGainA         = 0x47,  // Phase A Calibration Gain
  PhiA            = 0x48,  // Phase A Calibration Phase Angle
  PQGainB         = 0x49,  // Phase B Calibration Gain
  PhiB            = 0x4A,  // Phase B Calibration Phase Angle
  PQGainC         = 0x4B,  // Phase C Calibration Gain
  PhiC            = 0x4C,  // Phase C Calibration Phase Angle

  /* Fundamental / Harmonic Energy Calibration Registers */

  PoffsetAF       = 0x51,  // Phase A Fundamental Active Power offset
  PoffsetBF       = 0x52,  // Phase B Fundamental Active Power offset
  PoffsetCF       = 0x53,  // Phase C Fundamental Active Power offset
  PGainAF         = 0x54,  // Phase A Fundamental Calibration Gain
  PGainBF         = 0x55,  // Phase B Fundamental Calibration Gain
  PGainCF         = 0x56,  // Phase C Fundamental Calibration Gain

  /* Measurement Calibration Registers */

  UgainA          = 0x61,  // Phase A Voltage RMS Gain
  IgainA          = 0x62,  // Phase A Current RMS Gain
  UoffsetA        = 0x63,  // Phase A Voltage RMS offset
  IoffsetA        = 0x64,  // Phase A Current RMS offset
  UgainB          = 0x65,  // Phase B Voltage RMS Gain
  IgainB          = 0x66,  // Phase B Current RMS Gain
  UoffsetB        = 0x67,  // Phase B Voltage RMS offset
  IoffsetB        = 0x68,  // Phase B Current RMS offset
  UgainC          = 0x69,  // Phase C Voltage RMS Gain
  IgainC          = 0x6A,  // Phase C Current RMS Gain
  UoffsetC        = 0x6B,  // Phase C Voltage RMS offset
  IoffsetC        = 0x6C,  // Phase C Current RMS offset

  /* EMM Status Registers */

  SoftReset       = 0x70,  // Software Reset
  EMMState0       = 0x71,  // EMM State 0
  EMMState1       = 0x72,  // EMM State 1
  EMMIntState0    = 0x73,  // EMM Interrupt Status 0
  EMMIntState1    = 0x74,  // EMM Interrupt Status 1
  EMMIntEn0       = 0x75,  // EMM Interrupt Enable 0
  EMMIntEn1       = 0x76,  // EMM Interrupt Enable 1
  LastSPIData     = 0x78,  // Last Read/Write SPI Value
  CRCErrStatus    = 0x79,  // CRC Error Status
  CRCDigest       = 0x7A,  // CRC Digest
  CfgRegAccEn     = 0x7F,  // Configure Register Access Enable

  /* Peak Voltage/Current Registers */

  UPeakA          = 0xF1,  // Channel A Voltage Peak
  UPeakB          = 0xF2,  // Channel B Voltage Peak
  UPeakC          = 0xF3,  // Channel C Voltage Peak
  IPeakA          = 0xF5,  // Channel A Current Peak
  IPeakB          = 0xF6,  // Channel B Current Peak
  IPeakC          = 0xF7,  // Channel C Current Peak

};

#endif /* _ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E32AS_REGS_H_ */