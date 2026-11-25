/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI interface of the ADE9000: command format and register map, taken
 *        from table 6 of the data sheet, Rev. B. Every register is 32 bit wide
 *        unless marked as 16 bit.
 */

#pragma once

#include <zephyr/sys/util.h>

/** Phase A current gain adjust */
#define ADE9000_REG_AIGAIN      0x000
/** Phase A multipoint gain correction factor, region 0 */
#define ADE9000_REG_AIGAIN0     0x001
/** Phase A multipoint gain correction factor, region 1 */
#define ADE9000_REG_AIGAIN1     0x002
/** Phase A multipoint gain correction factor, region 2 */
#define ADE9000_REG_AIGAIN2     0x003
/** Phase A multipoint gain correction factor, region 3 */
#define ADE9000_REG_AIGAIN3     0x004
/** Phase A multipoint gain correction factor, region 4 */
#define ADE9000_REG_AIGAIN4     0x005
/** Phase A multipoint phase correction factor, region 0 */
#define ADE9000_REG_APHCAL0     0x006
/** Phase A multipoint phase correction factor, region 1 */
#define ADE9000_REG_APHCAL1     0x007
/** Phase A multipoint phase correction factor, region 2 */
#define ADE9000_REG_APHCAL2     0x008
/** Phase A multipoint phase correction factor, region 3 */
#define ADE9000_REG_APHCAL3     0x009
/** Phase A multipoint phase correction factor, region 4 */
#define ADE9000_REG_APHCAL4     0x00A
/** Phase A voltage gain adjust */
#define ADE9000_REG_AVGAIN      0x00B
/** Phase A current rms offset for the filter-based AIRMS calculation */
#define ADE9000_REG_AIRMSOS     0x00C
/** Phase A voltage rms offset for the filter-based AVRMS calculation */
#define ADE9000_REG_AVRMSOS     0x00D
/** Phase A power gain adjust for the AWATT, AVA, AVAR, AFWATT, AFVA, and AFVAR calculations */
#define ADE9000_REG_APGAIN      0x00E
/** Phase A total active power offset correction for the AWATT calculation */
#define ADE9000_REG_AWATTOS     0x00F
/** Phase A total reactive power offset correction for the AVAR calculation */
#define ADE9000_REG_AVAROS      0x010
/** Phase A fundamental active power offset correction for the AFWATT calculation */
#define ADE9000_REG_AFWATTOS    0x011
/** Phase A fundamental reactive power offset correction for the AFVAR calculation */
#define ADE9000_REG_AFVAROS     0x012
/** Phase A current rms offset for the fundamental current rms, AIFRMS calculation */
#define ADE9000_REG_AIFRMSOS    0x013
/** Phase A voltage rms offset for the fundamental voltage rms, AVFRMS calculation */
#define ADE9000_REG_AVFRMSOS    0x014
/** Phase A voltage rms offset for the fast rms½ AVRMSONE calculation */
#define ADE9000_REG_AVRMSONEOS  0x015
/** Phase A current rms offset for the fast rms½ AIRMSONE calculation */
#define ADE9000_REG_AIRMSONEOS  0x016
/** Phase A voltage rms offset for the 10 cycle rms/12 cycle rms AVRMS1012 calculation */
#define ADE9000_REG_AVRMS1012OS 0x017
/** Phase A current rms offset for the 10 cycle rms/12 cycle rms AIRMS1012 calculation */
#define ADE9000_REG_AIRMS1012OS 0x018

/** Phase B current gain adjust */
#define ADE9000_REG_BIGAIN      0x020
/** Phase B multipoint gain correction factor, region 0 */
#define ADE9000_REG_BIGAIN0     0x021
/** Phase B multipoint gain correction factor, region 1 */
#define ADE9000_REG_BIGAIN1     0x022
/** Phase B multipoint gain correction factor, region 2 */
#define ADE9000_REG_BIGAIN2     0x023
/** Phase B multipoint gain correction factor, region 3 */
#define ADE9000_REG_BIGAIN3     0x024
/** Phase B multipoint gain correction factor, region 4 */
#define ADE9000_REG_BIGAIN4     0x025
/** Phase B multipoint phase correction factor, region 0 */
#define ADE9000_REG_BPHCAL0     0x026
/** Phase B multipoint phase correction factor, region 1 */
#define ADE9000_REG_BPHCAL1     0x027
/** Phase B multipoint phase correction factor, region 2 */
#define ADE9000_REG_BPHCAL2     0x028
/** Phase B multipoint phase correction factor, region 3 */
#define ADE9000_REG_BPHCAL3     0x029
/** Phase B multipoint phase correction factor, region 4 */
#define ADE9000_REG_BPHCAL4     0x02A
/** Phase B voltage gain adjust */
#define ADE9000_REG_BVGAIN      0x02B
/** Phase B current rms offset for the BIRMS calculation */
#define ADE9000_REG_BIRMSOS     0x02C
/** Phase B voltage rms offset for the BVRMS calculation */
#define ADE9000_REG_BVRMSOS     0x02D
/** Phase B power gain adjust for the BWATT, BVA, BVAR, BFWATT, BFVA, and BFVAR calculations */
#define ADE9000_REG_BPGAIN      0x02E
/** Phase B total active power offset correction for the BWATT calculation */
#define ADE9000_REG_BWATTOS     0x02F
/** Phase B total reactive power offset correction for the BVAR calculation */
#define ADE9000_REG_BVAROS      0x030
/** Phase B fundamental active power offset correction for the BFWATT calculation */
#define ADE9000_REG_BFWATTOS    0x031
/** Phase B fundamental reactive power offset correction for the BFVAR calculation */
#define ADE9000_REG_BFVAROS     0x032
/** Phase B current rms offset for the fundamental current rms BIFRMS calculation */
#define ADE9000_REG_BIFRMSOS    0x033
/** Phase B voltage rms offset for the fundamental voltage rms BVFRMS calculation */
#define ADE9000_REG_BVFRMSOS    0x034
/** Phase B voltage rms offset for the fast rms½ BVRMSONE calculation */
#define ADE9000_REG_BVRMSONEOS  0x035
/** Phase B current rms offset for the fast rms½ BIRMSONE calculation */
#define ADE9000_REG_BIRMSONEOS  0x036
/** Phase B voltage rms offset for the 10 cycle rms/12 cycle rms BVRMS1012 calculation */
#define ADE9000_REG_BVRMS1012OS 0x037
/** Phase B current rms offset for the 10 cycle rms/12 cycle rms BVRMS1012 calculation */
#define ADE9000_REG_BIRMS1012OS 0x038

/** Phase C current gain adjust */
#define ADE9000_REG_CIGAIN      0x040
/** Phase C multipoint gain correction factor, region 0 */
#define ADE9000_REG_CIGAIN0     0x041
/** Phase C multipoint gain correction factor, region 1 */
#define ADE9000_REG_CIGAIN1     0x042
/** Phase C multipoint gain correction factor, region 2 */
#define ADE9000_REG_CIGAIN2     0x043
/** Phase C Multipoint gain correction factor, region 3 */
#define ADE9000_REG_CIGAIN3     0x044
/** Phase C Multipoint gain correction factor, region 4 */
#define ADE9000_REG_CIGAIN4     0x045
/** Phase C multipoint phase correction factor, region 0 */
#define ADE9000_REG_CPHCAL0     0x046
/** Phase C multipoint phase correction factor, region 1 */
#define ADE9000_REG_CPHCAL1     0x047
/** Phase C multipoint phase correction factor, region 2 */
#define ADE9000_REG_CPHCAL2     0x048
/** Phase C multipoint phase correction factor, region 3 */
#define ADE9000_REG_CPHCAL3     0x049
/** Phase C multipoint phase correction factor, region 4 */
#define ADE9000_REG_CPHCAL4     0x04A
/** Phase C voltage gain adjust */
#define ADE9000_REG_CVGAIN      0x04B
/** Phase C current rms offset for the CIRMS calculation */
#define ADE9000_REG_CIRMSOS     0x04C
/** Phase C voltage rms offset for the CVRMS calculation */
#define ADE9000_REG_CVRMSOS     0x04D
/** Phase C power gain adjust for the CWATT, CVA, CVAR, CFWATT, CFVA, and CFVAR calculations */
#define ADE9000_REG_CPGAIN      0x04E
/** Phase C total active power offset correction for the CWATT calculation */
#define ADE9000_REG_CWATTOS     0x04F
/** Phase C total reactive power offset correction for the CVAR calculation */
#define ADE9000_REG_CVAROS      0x050
/** Phase C fundamental active power offset correction for the CFWATT calculation */
#define ADE9000_REG_CFWATTOS    0x051
/** Phase C fundamental reactive power offset correction for the CFVAR calculation */
#define ADE9000_REG_CFVAROS     0x052
/** Phase C current rms offset for the fundamental current rms CIFRMS calculation */
#define ADE9000_REG_CIFRMSOS    0x053
/** Phase C voltage rms offset for the fundamental voltage rms CVFRMS calculation */
#define ADE9000_REG_CVFRMSOS    0x054
/** Phase C voltage rms offset for the fast rms½ CVRMSONE calculation */
#define ADE9000_REG_CVRMSONEOS  0x055
/** Phase C current rms offset for the fast rms½ CIRMSONE calculation */
#define ADE9000_REG_CIRMSONEOS  0x056
/** Phase C voltage rms offset for the 10 cycle rms/12 cycle rms CVRMS1012 calculation */
#define ADE9000_REG_CVRMS1012OS 0x057
/** Phase C current rms offset for the 10 cycle rms/12 cycle rms CIRMS1012 calculation */
#define ADE9000_REG_CIRMS1012OS 0x058

/** Configuration Register 0 */
#define ADE9000_REG_CONFIG0     0x060
/** Multipoint phase/gain threshold, region 0 */
#define ADE9000_REG_MTTHR_L0    0x061
/** Multipoint phase/gain threshold, region 1 */
#define ADE9000_REG_MTTHR_L1    0x062
/** Multipoint phase/gain threshold, region 2 */
#define ADE9000_REG_MTTHR_L2    0x063
/** Multipoint phase/gain threshold, region 3 */
#define ADE9000_REG_MTTHR_L3    0x064
/** Multipoint phase/gain threshold, region 4 */
#define ADE9000_REG_MTTHR_L4    0x065
/** Multipoint phase/gain threshold, region 0 */
#define ADE9000_REG_MTTHR_H0    0x066
/** Multipoint phase/gain threshold, region 1 */
#define ADE9000_REG_MTTHR_H1    0x067
/** Multipoint phase/gain threshold, region 2 */
#define ADE9000_REG_MTTHR_H2    0x068
/** Multipoint phase/gain threshold, region 3 */
#define ADE9000_REG_MTTHR_H3    0x069
/** Multipoint phase/gain threshold, region 4 */
#define ADE9000_REG_MTTHR_H4    0x06A
/** Neutral current rms offset for the NIRMS calculation */
#define ADE9000_REG_NIRMSOS     0x06B
/** Offset correction for the ISUMRMS calculation based on the sum of IA + IB + IC ± IN */
#define ADE9000_REG_ISUMRMSOS   0x06C
/** Neutral current gain adjust */
#define ADE9000_REG_NIGAIN      0x06D
/** Neutral current phase compensation */
#define ADE9000_REG_NPHCAL      0x06E
/** Neutral current rms offset for the fast rms½ NIRMSONE calculation */
#define ADE9000_REG_NIRMSONEOS  0x06F
/** Neutral current rms offset for the 10 cycle rms/12 cycle rms NIRMS1012 calculation */
#define ADE9000_REG_NIRMS1012OS 0x070
/** Nominal phase voltage rms used in the computation of apparent power, xVA, when the VNOMx_EN bit
 *  is set in the CONFIG0 register
 */
#define ADE9000_REG_VNOM        0x071
/** Value used in the digital integrator algorithm */
#define ADE9000_REG_DICOEFF     0x072
/** Threshold to compare ISUMRMS against */
#define ADE9000_REG_ISUMLVL     0x073

/** Instantaneous Phase A current channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_AI_PCF    0x20A
/** Instantaneous Phase A voltage channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_AV_PCF    0x20B
/** Phase A filter-based current rms value, updates at 8 kSPS. Read only */
#define ADE9000_REG_AIRMS     0x20C
/** Phase A filter-based voltage rms value, updates at 8 kSPS. Read only */
#define ADE9000_REG_AVRMS     0x20D
/** Phase A current fundamental rms, updates at 8 kSPS. Read only */
#define ADE9000_REG_AIFRMS    0x20E
/** Phase A voltage fundamental RMS, updates at 8 kSPS. Read only */
#define ADE9000_REG_AVFRMS    0x20F
/** Phase A low-pass filtered total active power, updated at 8 kSPS. Read only */
#define ADE9000_REG_AWATT     0x210
/** Phase A low-pass filtered total reactive power, updated at 8 kSPS. Read only */
#define ADE9000_REG_AVAR      0x211
/** Phase A total apparent power, updated at 8 kSPS. Read only */
#define ADE9000_REG_AVA       0x212
/** Phase A fundamental active power, updated at 8 kSPS. Read only */
#define ADE9000_REG_AFWATT    0x213
/** Phase A fundamental reactive power, updated at 8 kSPS. Read only */
#define ADE9000_REG_AFVAR     0x214
/** Phase A fundamental apparent power, updated at 8 kSPS. Read only */
#define ADE9000_REG_AFVA      0x215
/** Phase A power factor, updated every 1.024 sec. Read only */
#define ADE9000_REG_APF       0x216
/** Phase A voltage THD, updated every 1.024 sec. Read only */
#define ADE9000_REG_AVTHD     0x217
/** Phase A current THD, updated every 1.024 sec. Read only */
#define ADE9000_REG_AITHD     0x218
/** Phase A current fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_AIRMSONE  0x219
/** Phase A voltage fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_AVRMSONE  0x21A
/** Phase A current fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_AIRMS1012 0x21B
/** Phase A voltage fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_AVRMS1012 0x21C
/** If multipoint gain and phase compensation is enabled, with MTEN = 1 in the CONFIG0 register,
 *  this register indicate which AIGAINx and APHCALx is currently being used. Read only
 */
#define ADE9000_REG_AMTREGION 0x21D

/** Instantaneous Phase B current channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_BI_PCF    0x22A
/** Instantaneous Phase B voltage channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_BV_PCF    0x22B
/** Phase B filter-based current rms value, updates at 8 kSPS. Read only */
#define ADE9000_REG_BIRMS     0x22C
/** Phase B filter-based voltage rms value, updates at 8 kSPS. Read only */
#define ADE9000_REG_BVRMS     0x22D
/** Phase B current fundamental rms, updates at 8 kSPS. Read only */
#define ADE9000_REG_BIFRMS    0x22E
/** Phase B voltage fundamental rms, updates at 8 kSPS. Read only */
#define ADE9000_REG_BVFRMS    0x22F
/** Phase B low-pass filtered total active power, updated at 8 kSPS. Read only */
#define ADE9000_REG_BWATT     0x230
/** Phase B low-pass filtered total reactive power, updated at 8 kSPS. Read only */
#define ADE9000_REG_BVAR      0x231
/** Phase B total apparent power, updated at 8 kSPS. Read only */
#define ADE9000_REG_BVA       0x232
/** Phase B fundamental active power, updated at 8 kSPS. Read only */
#define ADE9000_REG_BFWATT    0x233
/** Phase B fundamental reactive power, updated at 8 kSPS. Read only */
#define ADE9000_REG_BFVAR     0x234
/** Phase B fundamental apparent power, updated at 8 kSPS. Read only */
#define ADE9000_REG_BFVA      0x235
/** Phase B power factor, updated every 1.024 sec. Read only */
#define ADE9000_REG_BPF       0x236
/** Phase B voltage THD, updated every 1.024 sec. Read only */
#define ADE9000_REG_BVTHD     0x237
/** Phase B current THD, updated every 1.024 sec. Read only */
#define ADE9000_REG_BITHD     0x238
/** Phase B current fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_BIRMSONE  0x239
/** Phase B voltage fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_BVRMSONE  0x23A
/** Phase B current fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_BIRMS1012 0x23B
/** Phase B voltage fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_BVRMS1012 0x23C
/** If multipoint gain and phase compensation is enabled, with MTEN = 1 in the CONFIG0 register,
 *  this register indicate which BIGAINx and BPHCALx is currently being used. Read only
 */
#define ADE9000_REG_BMTREGION 0x23D

/** Instantaneous Phase C current channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_CI_PCF    0x24A
/** Instantaneous Phase C voltage channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_CV_PCF    0x24B
/** Phase C filter-based current rms value, updates at 8 kSPS. Read only */
#define ADE9000_REG_CIRMS     0x24C
/** Phase C filter-based voltage rms value, updates at 8 kSPS. Read only */
#define ADE9000_REG_CVRMS     0x24D
/** Phase C current fundamental rms, updates at 8 kSPS. Read only */
#define ADE9000_REG_CIFRMS    0x24E
/** Phase C voltage fundamental rms, updates at 8 kSPS. Read only */
#define ADE9000_REG_CVFRMS    0x24F
/** Phase C low-pass filtered total active power, updated at 8 kSPS. Read only */
#define ADE9000_REG_CWATT     0x250
/** Phase C low-pass filtered total reactive power, updated at 8 kSPS. Read only */
#define ADE9000_REG_CVAR      0x251
/** Phase C total apparent power, updated at 8 kSPS. Read only */
#define ADE9000_REG_CVA       0x252
/** Phase C fundamental active power, updated at 8 kSPS. Read only */
#define ADE9000_REG_CFWATT    0x253
/** Phase C fundamental reactive power, updated at 8 kSPS. Read only */
#define ADE9000_REG_CFVAR     0x254
/** Phase C fundamental apparent power, updated at 8 kSPS. Read only */
#define ADE9000_REG_CFVA      0x255
/** Phase C power factor, updated every 1.024 sec. Read only */
#define ADE9000_REG_CPF       0x256
/** Phase C voltage THD, updated every 1.024 sec. Read only */
#define ADE9000_REG_CVTHD     0x257
/** Phase C current total THD, updated every 1.024 sec. Read only */
#define ADE9000_REG_CITHD     0x258
/** Phase C current fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_CIRMSONE  0x259
/** Phase C voltage fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_CVRMSONE  0x25A
/** Phase C current fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_CIRMS1012 0x25B
/** Phase C voltage fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_CVRMS1012 0x25C
/** If multipoint gain and phase compensation is enabled, with MTEN = 1 in the CONFIG0 register,
 *  these bits indicate which CIGAINx and CPHCALx is currently being used. Read only
 */
#define ADE9000_REG_CMTREGION 0x25D

/** Instantaneous neutral current channel waveform processed by the DSP at 8 kSPS. Read only */
#define ADE9000_REG_NI_PCF    0x265
/** Neutral current filter-based rms value. Read only */
#define ADE9000_REG_NIRMS     0x266
/** Neutral current fast rms½ calculation, one cycle rms updated every half cycle. Read only */
#define ADE9000_REG_NIRMSONE  0x267
/** Neutral current fast 10 cycle rms/12 cycle rms calculation. Read only */
#define ADE9000_REG_NIRMS1012 0x268
/** Filter-based rms based on the sum of IA + IB + IC ± IN. Read only */
#define ADE9000_REG_ISUMRMS   0x269
/** This register indicates the version of the metrology algorithms after the user writes run = 1 to
 *  start the measurements. Read only
 */
#define ADE9000_REG_VERSION2  0x26A

/** Phase A accumulated total active power, LSB, Updated after PWR_TIME 8 kSPS samples. Read only */
#define ADE9000_REG_AWATT_ACC_LO 0x2E4
/** Phase A accumulated total active power, MSB. Read only */
#define ADE9000_REG_AWATT_ACC_HI 0x2E5
/** Phase A accumulated total active energy, LSB. Read only */
#define ADE9000_REG_AWATTHR_LO   0x2E6
/** Phase A accumulated total active energy, MSB. Read only */
#define ADE9000_REG_AWATTHR_HI   0x2E7

/** Phase A accumulated total reactive power, LSB. Read only */
#define ADE9000_REG_AVAR_ACC_LO 0x2EE
/** Phase A accumulated total reactive power, MSB. Read only */
#define ADE9000_REG_AVAR_ACC_HI 0x2EF
/** Phase A accumulated total reactive energy, LSB. Read only */
#define ADE9000_REG_AVARHR_LO   0x2F0
/** Phase A accumulated total reactive energy, MSB. Read only */
#define ADE9000_REG_AVARHR_HI   0x2F1

/** Phase A accumulated total apparent power, LSB. Read only */
#define ADE9000_REG_AVA_ACC_LO 0x2F8
/** Phase A accumulated total apparent power, MSB. Read only */
#define ADE9000_REG_AVA_ACC_HI 0x2F9
/** Phase A accumulated total apparent energy, LSB. Read only */
#define ADE9000_REG_AVAHR_LO   0x2FA
/** Phase A accumulated total apparent energy, LSB. Read only */
#define ADE9000_REG_AVAHR_HI   0x2FB

/** Phase A accumulated fundamental active power, LSB. Read only */
#define ADE9000_REG_AFWATT_ACC_LO 0x302
/** Phase A accumulated fundamental active power, MSB. Read only */
#define ADE9000_REG_AFWATT_ACC_HI 0x303
/** Phase A accumulated fundamental active energy, LSB. Read only */
#define ADE9000_REG_AFWATTHR_LO   0x304
/** Phase A accumulated fundamental active energy, MSB. Read only */
#define ADE9000_REG_AFWATTHR_HI   0x305

/** Phase A accumulated fundamental reactive power, LSB. Read only */
#define ADE9000_REG_AFVAR_ACC_LO 0x30C
/** Phase A accumulated fundamental reactive power, MSB. Read only */
#define ADE9000_REG_AFVAR_ACC_HI 0x30D
/** Phase A accumulated fundamental reactive energy, LSB. Read only */
#define ADE9000_REG_AFVARHR_LO   0x30E
/** Phase A accumulated fundamental reactive energy, MSB. Read only */
#define ADE9000_REG_AFVARHR_HI   0x30F

/** Phase A accumulated fundamental apparent power, LSB. Read only */
#define ADE9000_REG_AFVA_ACC_LO 0x316
/** Phase A accumulated fundamental apparent power, MSB. Read only */
#define ADE9000_REG_AFVA_ACC_HI 0x317
/** Phase A accumulated fundamental apparent energy, LSB. Read only */
#define ADE9000_REG_AFVAHR_LO   0x318
/** Phase A accumulated fundamental apparent energy, MSB. Read only */
#define ADE9000_REG_AFVAHR_HI   0x319

/** Phase B accumulated total active power, LSB. Read only */
#define ADE9000_REG_BWATT_ACC_LO 0x320
/** Phase B accumulated total active power, MSB. Read only */
#define ADE9000_REG_BWATT_ACC_HI 0x321
/** Phase B accumulated total active energy, LSB. Read only */
#define ADE9000_REG_BWATTHR_LO   0x322
/** Phase B accumulated total active energy, MSB. Read only */
#define ADE9000_REG_BWATTHR_HI   0x323

/** Phase B accumulated total reactive power, LSB. Read only */
#define ADE9000_REG_BVAR_ACC_LO 0x32A
/** Phase B accumulated total reactive power, MSB. Read only */
#define ADE9000_REG_BVAR_ACC_HI 0x32B
/** Phase B accumulated total reactive energy, LSB. Read only */
#define ADE9000_REG_BVARHR_LO   0x32C
/** Phase B accumulated total reactive energy, MSB. Read only */
#define ADE9000_REG_BVARHR_HI   0x32D

/** Phase B accumulated total apparent power, LSB. Read only */
#define ADE9000_REG_BVA_ACC_LO 0x334
/** Phase B accumulated total apparent power, MSB. Read only */
#define ADE9000_REG_BVA_ACC_HI 0x335
/** Phase B accumulated total apparent energy, LSB. Read only */
#define ADE9000_REG_BVAHR_LO   0x336
/** Phase B accumulated total apparent energy, MSB. Read only */
#define ADE9000_REG_BVAHR_HI   0x337

/** Phase B accumulated fundamental active power, LSB. Read only */
#define ADE9000_REG_BFWATT_ACC_LO 0x33E
/** Phase B accumulated fundamental active power, MSB. Read only */
#define ADE9000_REG_BFWATT_ACC_HI 0x33F
/** Phase B accumulated fundamental active energy, LSB. Read only */
#define ADE9000_REG_BFWATTHR_LO   0x340
/** Phase B accumulated fundamental active energy, MSB. Read only */
#define ADE9000_REG_BFWATTHR_HI   0x341

/** Phase B accumulated fundamental reactive power, LSB. Read only */
#define ADE9000_REG_BFVAR_ACC_LO 0x348
/** Phase B accumulated fundamental reactive power, MSB. Read only */
#define ADE9000_REG_BFVAR_ACC_HI 0x349
/** Phase B accumulated fundamental reactive energy, LSB. Read only */
#define ADE9000_REG_BFVARHR_LO   0x34A
/** Phase B accumulated fundamental reactive energy, MSB. Read only */
#define ADE9000_REG_BFVARHR_HI   0x34B

/** Phase B accumulated fundamental apparent power, LSB. Read only */
#define ADE9000_REG_BFVA_ACC_LO 0x352
/** Phase B accumulated fundamental apparent power, MSB. Read only */
#define ADE9000_REG_BFVA_ACC_HI 0x353
/** Phase B accumulated fundamental apparent energy, LSB. Read only */
#define ADE9000_REG_BFVAHR_LO   0x354
/** Phase B accumulated fundamental apparent energy, MSB. Read only */
#define ADE9000_REG_BFVAHR_HI   0x355

/** Phase C accumulated total active power, LSB. Read only */
#define ADE9000_REG_CWATT_ACC_LO 0x35C
/** Phase C accumulated total active power, MSB. Read only */
#define ADE9000_REG_CWATT_ACC_HI 0x35D
/** Phase C accumulated total active energy, LSB. Read only */
#define ADE9000_REG_CWATTHR_LO   0x35E
/** Phase C accumulated total active energy, MSB. Read only */
#define ADE9000_REG_CWATTHR_HI   0x35F

/** Phase C accumulated total reactive power, LSB updated after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_CVAR_ACC_LO 0x366
/** Phase C accumulated total reactive power, MSB updated after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_CVAR_ACC_HI 0x367
/** Phase C accumulated total reactive energy, LSB. Read only */
#define ADE9000_REG_CVARHR_LO   0x368
/** Phase C accumulated total reactive energy, MSB. Read only */
#define ADE9000_REG_CVARHR_HI   0x369

/** Phase C accumulated total apparent power, LSB. Read only */
#define ADE9000_REG_CVA_ACC_LO 0x370
/** Phase C accumulated total apparent power, MSB. Read only */
#define ADE9000_REG_CVA_ACC_HI 0x371
/** Phase C accumulated total apparent energy, LSB. Read only */
#define ADE9000_REG_CVAHR_LO   0x372
/** Phase C accumulated total apparent energy, MSB. Read only */
#define ADE9000_REG_CVAHR_HI   0x373

/** Phase C accumulated fundamental active power, LSB. Read only */
#define ADE9000_REG_CFWATT_ACC_LO 0x37A
/** Phase C accumulated fundamental active power, MSB. Read only */
#define ADE9000_REG_CFWATT_ACC_HI 0x37B
/** Phase C accumulated fundamental active energy, LSB. Read only */
#define ADE9000_REG_CFWATTHR_LO   0x37C
/** Phase C accumulated fundamental active energy, MSB. Read only */
#define ADE9000_REG_CFWATTHR_HI   0x37D

/** Phase C accumulated fundamental reactive power, LSB. Read only */
#define ADE9000_REG_CFVAR_ACC_LO 0x384
/** Phase C accumulated fundamental reactive power, MSB. Read only */
#define ADE9000_REG_CFVAR_ACC_HI 0x385
/** Phase C accumulated fundamental reactive energy, LSB. Read only */
#define ADE9000_REG_CFVARHR_LO   0x386
/** Phase C accumulated fundamental reactive energy, MSB. Read only */
#define ADE9000_REG_CFVARHR_HI   0x387

/** Phase C accumulated fundamental apparent power, LSB. Read only */
#define ADE9000_REG_CFVA_ACC_LO 0x38E
/** Phase C accumulated fundamental apparent power, MSB. Read only */
#define ADE9000_REG_CFVA_ACC_HI 0x38F
/** Phase C accumulated fundamental apparent energy, LSB. Read only */
#define ADE9000_REG_CFVAHR_LO   0x390
/** Phase C accumulated fundamental apparent energy, MSB. Read only */
#define ADE9000_REG_CFVAHR_HI   0x391

/** Accumulated positive total active power, LSB, from AWATT, BWATT, and CWATT registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_PWATT_ACC_LO 0x396
/** Accumulated positive total active power, MSB, from AWATT, BWATT, and CWATT registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_PWATT_ACC_HI 0x397

/** Accumulated Negative total active power, LSB, from AWATT, BWATT, and CWATT registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_NWATT_ACC_LO 0x39A
/** Accumulated Negative total active power, MSB, from AWATT, BWATT, and CWATT registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_NWATT_ACC_HI 0x39B

/** Accumulated positive total reactive power, LSB, from AVAR, BVAR, and CVAR registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_PVAR_ACC_LO 0x39E
/** Accumulated positive total reactive power, MSB, from AVAR, BVAR, and CVAR registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_PVAR_ACC_HI 0x39F

/** Accumulated Negative total reactive power, LSB, from AVAR, BVAR, and CVAR registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_NVAR_ACC_LO 0x3A2
/** Accumulated Negative total reactive power, MSB, from AVAR, BVAR, and CVAR registers, updated
 *  after PWR_TIME 8 kSPS samples. Read only
 */
#define ADE9000_REG_NVAR_ACC_HI 0x3A3

/** Current peak register. Read only */
#define ADE9000_REG_IPEAK        0x400
/** Voltage peak register. Read only */
#define ADE9000_REG_VPEAK        0x401
/** Status Register 0 */
#define ADE9000_REG_STATUS0      0x402
/** Status Register 1 */
#define ADE9000_REG_STATUS1      0x403
/** Event status register. Read only */
#define ADE9000_REG_EVENT_STATUS 0x404
/** Interrupt Enable Register 0 */
#define ADE9000_REG_MASK0        0x405
/** Interrupt Enable Register 1 */
#define ADE9000_REG_MASK1        0x406
/** Event enable register */
#define ADE9000_REG_EVENT_MASK   0x407

/** Over current detection threshold level */
#define ADE9000_REG_OILVL        0x409
/** Phase A overcurrent rms½ value. Read only */
#define ADE9000_REG_OIA          0x40A
/** Phase B overcurrent rms½ value. Read only */
#define ADE9000_REG_OIB          0x40B
/** Phase C overcurrent rms½ value. Read only */
#define ADE9000_REG_OIC          0x40C
/** Neutral current overcurrent rms½ value. Read only */
#define ADE9000_REG_OIN          0x40D
/** User configured line period value used for resampling, fast rms½ and 10 cycle rms/ 12 cycle rms
 *  when the UPERIOD_SEL bit in the CONFIG2 register is set
 */
#define ADE9000_REG_USER_PERIOD  0x40E
/** Register used in the algorithm that computes the fundamental active, reactive, and apparent
 *  powers as well as the fundamental IRMS and VRMS values
 */
#define ADE9000_REG_VLEVEL       0x40F
/** Voltage RMS½ dip detection threshold level */
#define ADE9000_REG_DIP_LVL      0x410
/** Phase A voltage rms½ value during a dip condition. Read only */
#define ADE9000_REG_DIPA         0x411
/** Phase B voltage rms½ value during a dip condition. Read only */
#define ADE9000_REG_DIPB         0x412
/** Phase C voltage rms½ value during a dip condition. Read only */
#define ADE9000_REG_DIPC         0x413
/** Voltage rms½ swell detection threshold level */
#define ADE9000_REG_SWELL_LVL    0x414
/** Phase A voltage rms½ value during a swell condition. Read only */
#define ADE9000_REG_SWELLA       0x415
/** Phase B voltage rms½ value during a swell condition. Read only */
#define ADE9000_REG_SWELLB       0x416
/** Phase C voltage rms½ value during a swell condition. Read only */
#define ADE9000_REG_SWELLC       0x417
/** Line period on Phase A voltage. Read only */
#define ADE9000_REG_APERIOD      0x418
/** Line period on Phase B voltage. Read only */
#define ADE9000_REG_BPERIOD      0x419
/** Line period on Phase C voltage. Read only */
#define ADE9000_REG_CPERIOD      0x41A
/** Line period measurement on combined signal from Phase A, Phase B, and Phase C voltages. Read
 *  only
 */
#define ADE9000_REG_COM_PERIOD   0x41B
/** No load threshold in the total and fundamental active power datapath */
#define ADE9000_REG_ACT_NL_LVL   0x41C
/** No load threshold in the total and fundamental reactive power datapath */
#define ADE9000_REG_REACT_NL_LVL 0x41D
/** No load threshold in the total and fundamental apparent power datapath */
#define ADE9000_REG_APP_NL_LVL   0x41E
/** Phase no load register. Read only */
#define ADE9000_REG_PHNOLOAD     0x41F
/** Sets the maximum output rate from the digital to frequency converter for the total and
 *  fundamental active power for the CFx calibration pulse output
 */
#define ADE9000_REG_WTHR         0x420
/** Sets the maximum output rate from the digital to frequency converter for the total and
 *  fundamental reactive power for the CFx calibration pulse output
 */
#define ADE9000_REG_VARTHR       0x421
/** Sets the maximum output rate from the digital to frequency converter for the total and
 *  fundamental apparent power for the CFx calibration pulse output
 */
#define ADE9000_REG_VATHR        0x422
/** This register holds the data read or written during the last 32-bit transaction on the SPI port.
 *  Read only
 */
#define ADE9000_REG_LAST_DATA_32 0x423
/** This register allows any ADC output to be redirected to any digital datapath */
#define ADE9000_REG_ADC_REDIRECT 0x424
/** CFx calibration pulse width configuration register */
#define ADE9000_REG_CF_LCFG      0x425

/** This register identifies the IC. Read only */
#define ADE9000_REG_PART_ID 0x472

/** Temperature sensor gain and offset, calculated during the manufacturing process. Read only */
#define ADE9000_REG_TEMP_TRIM 0x474

/** Write this register to 1 to start the measurements. 16 bit */
#define ADE9000_REG_RUN        0x480
/** Configuration Register 1. 16 bit */
#define ADE9000_REG_CONFIG1    0x481
/** Time between positive to negative zero crossings on Phase A and Phase B voltages. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_VA_VB 0x482
/** Time between positive to negative zero crossings on Phase B and Phase C voltages. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_VB_VC 0x483
/** Time between positive to negative zero crossings on Phase A and Phase C voltages. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_VA_VC 0x484
/** Time between positive to negative zero crossings on Phase A voltage and current. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_VA_IA 0x485
/** Time between positive to negative zero crossings on Phase B voltage and current. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_VB_IB 0x486
/** Time between positive to negative zero crossings on Phase C voltage and current. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_VC_IC 0x487
/** Time between positive to negative zero crossings on Phase A and Phase B current. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_IA_IB 0x488
/** Time between positive to negative zero crossings on Phase B and Phase C current. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_IB_IC 0x489
/** Time between positive to negative zero crossings on Phase A and Phase C current. 16 bit, read
 *  only
 */
#define ADE9000_REG_ANGL_IA_IC 0x48A
/** Voltage rms½ dip detection cycle configuration. 16 bit */
#define ADE9000_REG_DIP_CYC    0x48B
/** Voltage rms½ swell detection cycle configuration. 16 bit */
#define ADE9000_REG_SWELL_CYC  0x48C

/** Overcurrent status register. 16 bit, read only */
#define ADE9000_REG_OISTATUS  0x48F
/** CFx configuration register. 16 bit */
#define ADE9000_REG_CFMODE    0x490
/** Computation mode register. 16 bit */
#define ADE9000_REG_COMPMODE  0x491
/** Accumulation mode register. 16 bit */
#define ADE9000_REG_ACCMODE   0x492
/** Configuration Register 3. 16 bit */
#define ADE9000_REG_CONFIG3   0x493
/** CF1 denominator register. 16 bit */
#define ADE9000_REG_CF1DEN    0x494
/** CF2 denominator register. 16 bit */
#define ADE9000_REG_CF2DEN    0x495
/** CF3 denominator register. 16 bit */
#define ADE9000_REG_CF3DEN    0x496
/** CF4 denominator register. 16 bit */
#define ADE9000_REG_CF4DEN    0x497
/** Zero-crossing timeout configuration register. 16 bit */
#define ADE9000_REG_ZXTOUT    0x498
/** Voltage channel zero-crossing threshold register. 16 bit */
#define ADE9000_REG_ZXTHRSH   0x499
/** This register selects which zero crossing and which line period measurement are used for other
 *  calculations. 16 bit
 */
#define ADE9000_REG_ZX_LP_SEL 0x49A

/** Number of line cycles used for phase sequence detection. 16 bit */
#define ADE9000_REG_SEQ_CYC 0x49C
/** Power sign register. 16 bit, read only */
#define ADE9000_REG_PHSIGN  0x49D

/** Waveform buffer configuration register. 16 bit */
#define ADE9000_REG_WFB_CFG      0x4A0
/** This register enables interrupts to occur after specific pages of the waveform buffer are
 *  filled. 16 bit
 */
#define ADE9000_REG_WFB_PG_IRQEN 0x4A1
/** This register enables events to trigger a capture in the waveform buffer. 16 bit */
#define ADE9000_REG_WFB_TRG_CFG  0x4A2
/** This register indicates the last page that was filled in the waveform buffer and the location of
 *  trigger events. 16 bit
 */
#define ADE9000_REG_WFB_TRG_STAT 0x4A3

/** This register holds the CRC of the configuration registers. 16 bit, read only */
#define ADE9000_REG_CRC_RSLT 0x4A8
/** This register holds the 16-bit CRC of the data sent out on the MOSI pin during the last SPI
 *  register read. 16 bit, read only
 */
#define ADE9000_REG_CRC_SPI  0x4A9

/** This register holds the data read or written during the last 16-bit transaction on the SPI port.
 *  16 bit, read only
 */
#define ADE9000_REG_LAST_DATA_16 0x4AC

/** This register holds the address and read/write operation request (CMD_HDR) for the last
 *  transaction on the SPI port. 16 bit, read only
 */
#define ADE9000_REG_LAST_CMD 0x4AE
/** Configuration Register 2. 16 bit */
#define ADE9000_REG_CONFIG2  0x4AF
/** Energy and power accumulation configuration. 16 bit */
#define ADE9000_REG_EP_CFG   0x4B0
/** Power update time configuration. 16 bit */
#define ADE9000_REG_PWR_TIME 0x4B1
/** Energy accumulation update time configuration. 16 bit */
#define ADE9000_REG_EGY_TIME 0x4B2

/** This register forces an update of the CRC of configuration registers. 16 bit */
#define ADE9000_REG_CRC_FORCE 0x4B4
/** This register selects which registers are optionally included in the configuration register CRC
 *  feature. 16 bit
 */
#define ADE9000_REG_CRC_OPTEN 0x4B5
/** Temperature sensor configuration register. 16 bit */
#define ADE9000_REG_TEMP_CFG  0x4B6
/** Temperature measurement result. 16 bit, read only */
#define ADE9000_REG_TEMP_RSLT 0x4B7

/** This register configures the PGA gain for each ADC. 16 bit */
#define ADE9000_REG_PGA_GAIN 0x4B9
/** ADC channel enable/disable. 16 bit */
#define ADE9000_REG_CHNL_DIS 0x4BA

/** This register enables the configuration lock feature. 16 bit */
#define ADE9000_REG_WR_LOCK 0x4BF

/** Enables/disables total reactive power calculation. 16 bit */
#define ADE9000_REG_VAR_DIS 0x4E0

/** This register is reserved. 16 bit, read only */
#define ADE9000_REG_RESERVED1 0x4F0

/** Version of the ADE9000 IC. 16 bit, read only */
#define ADE9000_REG_VERSION 0x4FE

/* Burst readable from here on with CONFIG1.BURST_EN */
/** Current channel A ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_AI_SINC_DAT 0x500
/** Voltage channel A ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_AV_SINC_DAT 0x501
/** Current channel B ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_BI_SINC_DAT 0x502
/** Voltage channel B ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_BV_SINC_DAT 0x503
/** Current channel C ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_CI_SINC_DAT 0x504
/** Voltage channel C ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_CV_SINC_DAT 0x505
/** Neutral current channel ADC waveforms from the sinc4 output at 32 kSPS. Read only */
#define ADE9000_REG_NI_SINC_DAT 0x506

/** Current channel A ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_AI_LPF_DAT 0x510
/** Voltage channel A ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_AV_LPF_DAT 0x511
/** Current channel B ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_BI_LPF_DAT 0x512
/** Voltage channel B ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_BV_LPF_DAT 0x513
/** Current channel C ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_CI_LPF_DAT 0x514
/** Voltage channel C ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_CV_LPF_DAT 0x515
/** Neutral current channel ADC waveforms from the sinc4 + IIR LPF output at 8 kSPS. Read only */
#define ADE9000_REG_NI_LPF_DAT 0x516

/** AV_PCF in the burst read block ordered by function */
#define ADE9000_REG_AV_PCF_1    0x600
/** BV_PCF in the burst read block ordered by function */
#define ADE9000_REG_BV_PCF_1    0x601
/** CV_PCF in the burst read block ordered by function */
#define ADE9000_REG_CV_PCF_1    0x602
/** NI_PCF in the burst read block ordered by function */
#define ADE9000_REG_NI_PCF_1    0x603
/** AI_PCF in the burst read block ordered by function */
#define ADE9000_REG_AI_PCF_1    0x604
/** BI_PCF in the burst read block ordered by function */
#define ADE9000_REG_BI_PCF_1    0x605
/** CI_PCF in the burst read block ordered by function */
#define ADE9000_REG_CI_PCF_1    0x606
/** AIRMS in the burst read block ordered by function */
#define ADE9000_REG_AIRMS_1     0x607
/** BIRMS in the burst read block ordered by function */
#define ADE9000_REG_BIRMS_1     0x608
/** CIRMS in the burst read block ordered by function */
#define ADE9000_REG_CIRMS_1     0x609
/** AVRMS in the burst read block ordered by function */
#define ADE9000_REG_AVRMS_1     0x60A
/** BVRMS in the burst read block ordered by function */
#define ADE9000_REG_BVRMS_1     0x60B
/** CVRMS in the burst read block ordered by function */
#define ADE9000_REG_CVRMS_1     0x60C
/** NIRMS in the burst read block ordered by function */
#define ADE9000_REG_NIRMS_1     0x60D
/** AWATT in the burst read block ordered by function */
#define ADE9000_REG_AWATT_1     0x60E
/** BWATT in the burst read block ordered by function */
#define ADE9000_REG_BWATT_1     0x60F
/** CWATT in the burst read block ordered by function */
#define ADE9000_REG_CWATT_1     0x610
/** AVA in the burst read block ordered by function */
#define ADE9000_REG_AVA_1       0x611
/** BVA in the burst read block ordered by function */
#define ADE9000_REG_BVA_1       0x612
/** CVA in the burst read block ordered by function */
#define ADE9000_REG_CVA_1       0x613
/** AVAR in the burst read block ordered by function */
#define ADE9000_REG_AVAR_1      0x614
/** BVAR in the burst read block ordered by function */
#define ADE9000_REG_BVAR_1      0x615
/** CVAR in the burst read block ordered by function */
#define ADE9000_REG_CVAR_1      0x616
/** AFVAR in the burst read block ordered by function */
#define ADE9000_REG_AFVAR_1     0x617
/** BFVAR in the burst read block ordered by function */
#define ADE9000_REG_BFVAR_1     0x618
/** CFVAR in the burst read block ordered by function */
#define ADE9000_REG_CFVAR_1     0x619
/** APF in the burst read block ordered by function */
#define ADE9000_REG_APF_1       0x61A
/** BPF in the burst read block ordered by function */
#define ADE9000_REG_BPF_1       0x61B
/** CPF in the burst read block ordered by function */
#define ADE9000_REG_CPF_1       0x61C
/** AVTHD in the burst read block ordered by function */
#define ADE9000_REG_AVTHD_1     0x61D
/** BVTHD in the burst read block ordered by function */
#define ADE9000_REG_BVTHD_1     0x61E
/** CVTHD in the burst read block ordered by function */
#define ADE9000_REG_CVTHD_1     0x61F
/** AITHD in the burst read block ordered by function */
#define ADE9000_REG_AITHD_1     0x620
/** BITHD in the burst read block ordered by function */
#define ADE9000_REG_BITHD_1     0x621
/** CITHD in the burst read block ordered by function */
#define ADE9000_REG_CITHD_1     0x622
/** AFWATT in the burst read block ordered by function */
#define ADE9000_REG_AFWATT_1    0x623
/** BFWATT in the burst read block ordered by function */
#define ADE9000_REG_BFWATT_1    0x624
/** CFWATT in the burst read block ordered by function */
#define ADE9000_REG_CFWATT_1    0x625
/** AFVA in the burst read block ordered by function */
#define ADE9000_REG_AFVA_1      0x626
/** BFVA in the burst read block ordered by function */
#define ADE9000_REG_BFVA_1      0x627
/** CFVA in the burst read block ordered by function */
#define ADE9000_REG_CFVA_1      0x628
/** AFIRMS in the burst read block ordered by function */
#define ADE9000_REG_AFIRMS_1    0x629
/** BFIRMS in the burst read block ordered by function */
#define ADE9000_REG_BFIRMS_1    0x62A
/** CFIRMS in the burst read block ordered by function */
#define ADE9000_REG_CFIRMS_1    0x62B
/** AFVRMS in the burst read block ordered by function */
#define ADE9000_REG_AFVRMS_1    0x62C
/** BFVRMS in the burst read block ordered by function */
#define ADE9000_REG_BFVRMS_1    0x62D
/** CFVRMS in the burst read block ordered by function */
#define ADE9000_REG_CFVRMS_1    0x62E
/** AIRMSONE in the burst read block ordered by function */
#define ADE9000_REG_AIRMSONE_1  0x62F
/** BIRMSONE in the burst read block ordered by function */
#define ADE9000_REG_BIRMSONE_1  0x630
/** CIRMSONE in the burst read block ordered by function */
#define ADE9000_REG_CIRMSONE_1  0x631
/** AVRMSONE in the burst read block ordered by function */
#define ADE9000_REG_AVRMSONE_1  0x632
/** BVRMSONE in the burst read block ordered by function */
#define ADE9000_REG_BVRMSONE_1  0x633
/** CVRMSONE in the burst read block ordered by function */
#define ADE9000_REG_CVRMSONE_1  0x634
/** NIRMSONE in the burst read block ordered by function */
#define ADE9000_REG_NIRMSONE_1  0x635
/** AIRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_AIRMS1012_1 0x636
/** BIRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_BIRMS1012_1 0x637
/** CIRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_CIRMS1012_1 0x638
/** AVRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_AVRMS1012_1 0x639
/** BVRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_BVRMS1012_1 0x63A
/** CVRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_CVRMS1012_1 0x63B
/** NIRMS1012 in the burst read block ordered by function */
#define ADE9000_REG_NIRMS1012_1 0x63C

/** AV_PCF in the burst read block ordered by phase */
#define ADE9000_REG_AV_PCF_2    0x680
/** AI_PCF in the burst read block ordered by phase */
#define ADE9000_REG_AI_PCF_2    0x681
/** AIRMS in the burst read block ordered by phase */
#define ADE9000_REG_AIRMS_2     0x682
/** AVRMS in the burst read block ordered by phase */
#define ADE9000_REG_AVRMS_2     0x683
/** AWATT in the burst read block ordered by phase */
#define ADE9000_REG_AWATT_2     0x684
/** AVA in the burst read block ordered by phase */
#define ADE9000_REG_AVA_2       0x685
/** AVAR in the burst read block ordered by phase */
#define ADE9000_REG_AVAR_2      0x686
/** AFVAR in the burst read block ordered by phase */
#define ADE9000_REG_AFVAR_2     0x687
/** APF in the burst read block ordered by phase */
#define ADE9000_REG_APF_2       0x688
/** AVTHD in the burst read block ordered by phase */
#define ADE9000_REG_AVTHD_2     0x689
/** AITHD in the burst read block ordered by phase */
#define ADE9000_REG_AITHD_2     0x68A
/** AFWATT in the burst read block ordered by phase */
#define ADE9000_REG_AFWATT_2    0x68B
/** AFVA in the burst read block ordered by phase */
#define ADE9000_REG_AFVA_2      0x68C
/** AFIRMS in the burst read block ordered by phase */
#define ADE9000_REG_AFIRMS_2    0x68D
/** AFVRMS in the burst read block ordered by phase */
#define ADE9000_REG_AFVRMS_2    0x68E
/** AIRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_AIRMSONE_2  0x68F
/** AVRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_AVRMSONE_2  0x690
/** AIRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_AIRMS1012_2 0x691
/** AVRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_AVRMS1012_2 0x692
/** BV_PCF in the burst read block ordered by phase */
#define ADE9000_REG_BV_PCF_2    0x693
/** BI_PCF in the burst read block ordered by phase */
#define ADE9000_REG_BI_PCF_2    0x694
/** BIRMS in the burst read block ordered by phase */
#define ADE9000_REG_BIRMS_2     0x695
/** BVRMS in the burst read block ordered by phase */
#define ADE9000_REG_BVRMS_2     0x696
/** BWATT in the burst read block ordered by phase */
#define ADE9000_REG_BWATT_2     0x697
/** BVA in the burst read block ordered by phase */
#define ADE9000_REG_BVA_2       0x698
/** BVAR in the burst read block ordered by phase */
#define ADE9000_REG_BVAR_2      0x699
/** BFVAR in the burst read block ordered by phase */
#define ADE9000_REG_BFVAR_2     0x69A
/** BPF in the burst read block ordered by phase */
#define ADE9000_REG_BPF_2       0x69B
/** BVTHD in the burst read block ordered by phase */
#define ADE9000_REG_BVTHD_2     0x69C
/** BITHD in the burst read block ordered by phase */
#define ADE9000_REG_BITHD_2     0x69D
/** BFWATT in the burst read block ordered by phase */
#define ADE9000_REG_BFWATT_2    0x69E
/** BFVA in the burst read block ordered by phase */
#define ADE9000_REG_BFVA_2      0x69F
/** BFIRMS in the burst read block ordered by phase */
#define ADE9000_REG_BFIRMS_2    0x6A0
/** BFVRMS in the burst read block ordered by phase */
#define ADE9000_REG_BFVRMS_2    0x6A1
/** BIRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_BIRMSONE_2  0x6A2
/** BVRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_BVRMSONE_2  0x6A3
/** BIRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_BIRMS1012_2 0x6A4
/** BVRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_BVRMS1012_2 0x6A5
/** CV_PCF in the burst read block ordered by phase */
#define ADE9000_REG_CV_PCF_2    0x6A6
/** CI_PCF in the burst read block ordered by phase */
#define ADE9000_REG_CI_PCF_2    0x6A7
/** CIRMS in the burst read block ordered by phase */
#define ADE9000_REG_CIRMS_2     0x6A8
/** CVRMS in the burst read block ordered by phase */
#define ADE9000_REG_CVRMS_2     0x6A9
/** CWATT in the burst read block ordered by phase */
#define ADE9000_REG_CWATT_2     0x6AA
/** CVA in the burst read block ordered by phase */
#define ADE9000_REG_CVA_2       0x6AB
/** CVAR in the burst read block ordered by phase */
#define ADE9000_REG_CVAR_2      0x6AC
/** CFVAR in the burst read block ordered by phase */
#define ADE9000_REG_CFVAR_2     0x6AD
/** CPF in the burst read block ordered by phase */
#define ADE9000_REG_CPF_2       0x6AE
/** CVTHD in the burst read block ordered by phase */
#define ADE9000_REG_CVTHD_2     0x6AF
/** CITHD in the burst read block ordered by phase */
#define ADE9000_REG_CITHD_2     0x6B0
/** CFWATT in the burst read block ordered by phase */
#define ADE9000_REG_CFWATT_2    0x6B1
/** CFVA in the burst read block ordered by phase */
#define ADE9000_REG_CFVA_2      0x6B2
/** CFIRMS in the burst read block ordered by phase */
#define ADE9000_REG_CFIRMS_2    0x6B3
/** CFVRMS in the burst read block ordered by phase */
#define ADE9000_REG_CFVRMS_2    0x6B4
/** CIRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_CIRMSONE_2  0x6B5
/** CVRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_CVRMSONE_2  0x6B6
/** CIRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_CIRMS1012_2 0x6B7
/** CVRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_CVRMS1012_2 0x6B8
/** NI_PCF in the burst read block ordered by phase */
#define ADE9000_REG_NI_PCF_2    0x6B9
/** NIRMS in the burst read block ordered by phase */
#define ADE9000_REG_NIRMS_2     0x6BA
/** NIRMSONE in the burst read block ordered by phase */
#define ADE9000_REG_NIRMSONE_2  0x6BB
/** NIRMS1012 in the burst read block ordered by phase */
#define ADE9000_REG_NIRMS1012_2 0x6BC

/**
 * @brief  First location of the waveform buffer, 2048 locations of 32 bit
 *         read with burst reads. See ADE9000_WFB_SIZE.
 */
#define ADE9000_WFB_BASE 0x800

/**
 * @brief         SPI command word: address in bits 15 to 4, bit 3 set for a read.
 * @param addr    Register address
 * @param read    true for a read, false for a write
 */
#define ADE9000_CMD(addr, read) ((uint16_t)(((addr) << 4) | ((read) ? BIT(3) : 0)))

/**
 * @name   Distances between register blocks
 * @{
 */
/** The registers of phase B and C follow those of phase A at this distance */
#define ADE9000_STRIDE_PHASE        0x020
/** The energy registers of phase B and C follow those of phase A at this distance */
#define ADE9000_STRIDE_ENERGY_PHASE 0x3C
/** Distance between the energy types of one phase, xWATTHR to xVARHR etc. */
#define ADE9000_STRIDE_ENERGY_TYPE  0x0A
/** @} */

/**
 * @name   PART_ID fields
 * @{
 */
/** Set for an ADE9000 */
#define ADE9000_PART_ID_ADE9000 BIT(20)
/** @} */

/**
 * @name   RUN fields
 * @{
 */
/** Starts the DSP and the measurements */
#define ADE9000_RUN_ON BIT(0)
/** @} */

/**
 * @name   CONFIG0 fields
 * @{
 */
/** ISUM calculation, whether NI_PCF is added to or subtracted from the phase currents */
#define ADE9000_CONFIG0_ISUM_CFG    GENMASK(1, 0)
/** Turns off the high-pass filter of all voltage and current channels */
#define ADE9000_CONFIG0_HPFDIS      BIT(3)
/** Multipoint gain and phase compensation on */
#define ADE9000_CONFIG0_MTEN        BIT(4)
/** Digital integrator of the phase current channels on, for Rogowski coils */
#define ADE9000_CONFIG0_INTEN       BIT(5)
/** Zero crossing detection before the high-pass filter, integrator and phase compensation */
#define ADE9000_CONFIG0_ZX_SRC_SEL  BIT(6)
/** Rms½ and 10/12 cycle rms from the ADC samples before the high-pass filter and integrator */
#define ADE9000_CONFIG0_RMS_SRC_SEL BIT(7)
/** VNOMA_EN to VNOMC_EN, apparent power of the phase from VNOM instead of xVRMS */
#define ADE9000_CONFIG0_VNOM_EN     GENMASK(10, 8)
/** Digital integrator of the neutral current channel on */
#define ADE9000_CONFIG0_ININTEN     BIT(11)
/** Turns off the low-pass filter of the total active power */
#define ADE9000_CONFIG0_DISAPLPF    BIT(12)
/** Turns off the low-pass filter of the total reactive power */
#define ADE9000_CONFIG0_DISRPLPF    BIT(13)
/** @} */

/**
 * @name   CONFIG1 fields
 * @{
 */
/** Software reset, clears itself */
#define ADE9000_CONFIG1_SWRST              BIT(0)
/** CF3/ZX pin outputs the zero crossing selected in ZX_LP_SEL */
#define ADE9000_CONFIG1_CF3_CFG            BIT(1)
/** CF4/EVENT/DREADY pin function, one of ADE9000_CONFIG1_CF4_CFG_* */
#define ADE9000_CONFIG1_CF4_CFG            GENMASK(3, 2)
/** CF4_CFG value: CF4 energy pulse output */
#define ADE9000_CONFIG1_CF4_CFG_CF         0
/** CF4_CFG value: EVENT output */
#define ADE9000_CONFIG1_CF4_CFG_EVENT      2
/** CF4_CFG value: DREADY output */
#define ADE9000_CONFIG1_CF4_CFG_DREADY     3
/** Interrupt mode of the dip and swell events */
#define ADE9000_CONFIG1_DIP_SWELL_IRQ_MODE BIT(10)
/** Burst reads of the registers from 0x500 on, also turns off the CRC appended to reads */
#define ADE9000_CONFIG1_BURST_EN           BIT(11)
/** STATUS0 events also pull IRQ1 low */
#define ADE9000_CONFIG1_IRQ0_ON_IRQ1       BIT(12)
/** Use the external reference on REFIN instead of the internal one */
#define ADE9000_CONFIG1_EXT_REF            BIT(15)
/** @} */

/**
 * @name   CONFIG2 fields
 * @{
 */
/** Corner frequency of the high-pass filter */
#define ADE9000_CONFIG2_HPF_CRN     GENMASK(11, 9)
/** Line period from USER_PERIOD for rms½, 10/12 cycle rms and resampling */
#define ADE9000_CONFIG2_UPERIOD_SEL BIT(12)
/** @} */

/**
 * @name   ACCMODE fields
 * @{
 */
/** Accumulation mode of the active power: signed, absolute, positive or negative */
#define ADE9000_ACCMODE_WATTACC GENMASK(1, 0)
/** Accumulation mode of the reactive power: signed, absolute, positive or negative */
#define ADE9000_ACCMODE_VARACC  GENMASK(3, 2)
/** Network configuration, see ADE9000_DT_VCONSEL_* */
#define ADE9000_ACCMODE_VCONSEL GENMASK(6, 4)
/** IB = -IA - IC, for three wire systems with only two current transformers */
#define ADE9000_ACCMODE_ICONSEL BIT(7)
/** 60 Hz line frequency, clear for 50 Hz */
#define ADE9000_ACCMODE_SELFREQ BIT(8)
/** @} */

/**
 * @name   EP_CFG fields
 * @{
 */
/** Energy and power accumulation on, also needed for the power factor and THD */
#define ADE9000_EP_CFG_EGY_PWR_EN BIT(0)
/** Number of 8 kSPS samples the no load condition is evaluated over */
#define ADE9000_EP_CFG_NOLOAD_TMR GENMASK(15, 13)
/** @} */

/**
 * @name   PGA_GAIN fields, two bits per ADC: 0 is a gain of 1, 1 of 2, 2 of 4
 * @{
 */
/** PGA gain of IA */
#define ADE9000_PGA_GAIN_IA GENMASK(1, 0)
/** PGA gain of IB */
#define ADE9000_PGA_GAIN_IB GENMASK(3, 2)
/** PGA gain of IC */
#define ADE9000_PGA_GAIN_IC GENMASK(5, 4)
/** PGA gain of IN */
#define ADE9000_PGA_GAIN_IN GENMASK(7, 6)
/** PGA gain of VA */
#define ADE9000_PGA_GAIN_VA GENMASK(9, 8)
/** PGA gain of VB */
#define ADE9000_PGA_GAIN_VB GENMASK(11, 10)
/** PGA gain of VC */
#define ADE9000_PGA_GAIN_VC GENMASK(13, 12)
/** @} */

/**
 * @name   AMTREGION, BMTREGION and CMTREGION fields
 * @{
 */
/** Active multipoint region, 0 to 4, 15 without multipoint compensation */
#define ADE9000_MTREGION_REGION GENMASK(3, 0)
/** @} */

/**
 * @name   VAR_DIS fields
 * @{
 */
/** Turns off the total reactive power calculation */
#define ADE9000_VAR_DIS_VARDIS BIT(0)
/** @} */

/**
 * @name   STATUS1 fields
 * @{
 */
/**
 * @brief         ZXTOVA, ZXTOVB, ZXTOVC: no zero crossing of the phase voltage
 *                within ZXTOUT.
 * @param phase   0 for phase A, 1 for B, 2 for C
 */
#define ADE9000_STATUS1_ZXTOV(phase) BIT(6 + (phase))
/** Set once the IC is ready after a reset, pulls IRQ1 low and cannot be masked */
#define ADE9000_STATUS1_RSTDONE      BIT(16)
/** @} */

/**
 * @name   Register values
 * @{
 */
/** DICOEFF for the digital integrator of Rogowski coils */
#define ADE9000_DICOEFF_INTEGRATOR 0xFFFFE000
/** TEMP_CFG with TEMP_EN, TEMP_START and 256 samples: a new result every 320 ms */
#define ADE9000_TEMP_CFG_START     0x000D
/** VLEVEL for a nominal input at 1/4 of full scale, the reset value */
#define ADE9000_VLEVEL_DEFAULT     286021
/** @} */
