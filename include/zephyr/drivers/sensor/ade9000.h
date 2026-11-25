/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended sensor API of the ADE9000: custom channels, triggers and
 *        attributes, plus the waveform buffer which does not fit the sensor
 *        API.
 *
 * Gains, offsets and the phase compensation are written from the devicetree at
 * init and can be changed at run time, e.g. with values from a calibration
 * stored in flash. A value set at run time holds until the next reset of the
 * IC. Calibration attributes on the custom channels:
 * - SENSOR_ATTR_GAIN sets and reads the gain as a factor, e.g. 1.02, from
 *   -15.0 to 17.0. A negative factor also turns the phase by 180 degrees.
 *   xIGAIN on IRMS and IFRMS, xVGAIN on VRMS and VFRMS, xPGAIN on WATT, VAR,
 *   FWATT and FVAR, NIGAIN on NIRMS. Channels that share a register share the
 *   gain. The multipoint gains xIGAIN0 to xIGAIN4 come from the devicetree or
 *   ade9000_multipoint_set().
 * - SENSOR_ATTR_OFFSET sets and reads the offset as a raw code in val1:
 *   xIRMSOS, xVRMSOS, xIFRMSOS, xVFRMSOS, xWATTOS, xVAROS, xFWATTOS, xFVAROS,
 *   NIRMSOS and ISUMRMSOS on the channel of the same name.
 * - SENSOR_ATTR_ADE9000_OFFSET_ONE and SENSOR_ATTR_ADE9000_OFFSET_1012 set and
 *   read the offsets of the one cycle and 10/12 cycle rms, xIRMSONEOS and
 *   xIRMS1012OS on IRMS, xVRMSONEOS and xVRMS1012OS on VRMS, NIRMSONEOS and
 *   NIRMS1012OS on NIRMS.
 * - SENSOR_ATTR_ADE9000_PHCAL sets and reads the phase compensation in
 *   degrees, xPHCAL0 on IRMS and NPHCAL on NIRMS. A positive angle corrects a
 *   current that lags the voltage. The range is -15 to +2.25 degrees at 50 Hz
 *   and -15 to +2.7 degrees at 60 Hz, data sheet page 31. The conversion uses
 *   the line frequency of SENSOR_ATTR_ADE9000_LINE_FREQUENCY, so set the phase
 *   compensation again after a change of the line frequency. The multipoint
 *   registers xPHCAL1 to xPHCAL4 come from the devicetree or
 *   ade9000_multipoint_set().
 * - SENSOR_ATTR_OFFSET on the energy channels sets and reads the start value of
 *   the energy counters in Wh, varh or VAh, e.g. the meter reading restored
 *   from flash after a reboot.
 * On a three phase channel these attributes take an array of
 * ADE9000_PHASE_COUNT values, one per phase.
 *
 * The operating mode attributes SENSOR_ATTR_ADE9000_LINE_FREQUENCY,
 * SENSOR_ATTR_ADE9000_VCONSEL and SENSOR_ATTR_ADE9000_ICONSEL work on
 * SENSOR_CHAN_ALL and change ACCMODE at run time, the devicetree sets the value
 * at init.
 *
 * The register attributes from SENSOR_ATTR_ADE9000_EVENT_MASK on work on
 * SENSOR_CHAN_ALL and pass raw register values in val1. Attributes marked with
 * a count take or return an array of that many values, one per register.
 *
 * Rms and power channels are scaled with voltage-full-scale-microvolt and
 * current-full-scale-microampere from the devicetree.
 *
 * The energy channels count in software with 64 bit: each fetch adds the change
 * of the 32 bit xWATTHR_HI etc. registers since the last fetch. Fetch energy at
 * least every 100 s, at full scale the registers wrap after about 106 s. The
 * registers update every energy-time + 1 samples, once a second by default.
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ade9000.h>
#include <zephyr/sys/util.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief        Number of 32 bit locations in the waveform buffer.
 */
#define ADE9000_WFB_SIZE 2048

/**
 * @brief  Number of 32 bit locations in one page of the waveform buffer.
 */
#define ADE9000_WFB_PAGE_SIZE 128

/**
 * @brief  Number of pages of the waveform buffer.
 */
#define ADE9000_WFB_PAGE_COUNT 16

/**
 * @brief  Locations of one sample set with fixed data rate: IA, VA, IB, VB, IC,
 *         VC, IN and one unused. A burst read returns only the channels
 *         selected with ADE9000_WFB_CFG_BURST_CHAN.
 */
#define ADE9000_WFB_SET_SIZE 8

/**
 * @brief  Channel order in a burst with ADE9000_WFB_CFG_BURST_CHAN_ALL and
 *         ADE9000_WFB_CFG_WF_IN_EN disabled.
 */
enum ade9000_wfb_channel {
	ADE9000_WFB_CHANNEL_IA,
	ADE9000_WFB_CHANNEL_VA,
	ADE9000_WFB_CHANNEL_IB,
	ADE9000_WFB_CHANNEL_VB,
	ADE9000_WFB_CHANNEL_IC,
	ADE9000_WFB_CHANNEL_VC,
	ADE9000_WFB_CHANNEL_COUNT,
};

/** Number of pages in one half of the waveform buffer */
#define ADE9000_WFB_HALF_PAGES (ADE9000_WFB_PAGE_COUNT / 2)

/** First location of the second half of the waveform buffer */
#define ADE9000_WFB_HALF_INDEX (ADE9000_WFB_SIZE / 2)

/** Number of samples per channel in one half of the fixed-rate waveform buffer */
#define ADE9000_WFB_HALF_SAMPLES (ADE9000_WFB_HALF_INDEX / ADE9000_WFB_SET_SIZE)

/** Number of words returned for one half with BURST_CHAN_ALL and WF_IN_EN disabled */
#define ADE9000_WFB_HALF_WORDS (ADE9000_WFB_HALF_SAMPLES * ADE9000_WFB_CHANNEL_COUNT)

/** Burst starts one location after the start of each half */
#define ADE9000_WFB_BURST_OFFSET 1

/**
 * @brief  Sample rate in Hz of the waveform buffer with
 *         ADE9000_WFB_CFG_WF_SRC_SINC4.
 */
#define ADE9000_WFB_RATE_SINC4 32000

/**
 * @brief  Sample rate in Hz of the waveform buffer with
 *         ADE9000_WFB_CFG_WF_SRC_SINC4_IIR and ADE9000_WFB_CFG_WF_SRC_PCF.
 */
#define ADE9000_WFB_RATE_PCF 8000

/**
 * @brief  xV_PCF and xI_PCF code at the peak of a full-scale sine. Also scales
 *         the waveform buffer with ADE9000_WFB_CFG_WF_SRC_PCF: v = code /
 *         ADE9000_FULL_SCALE_WAVE * voltage-full-scale * sqrt(2), the same for
 *         currents.
 *
 * The rms full scale 52,702,092 times sqrt(2), so a sine gives the same value
 * from the samples as from xVRMS and xIRMS and the gains calibrated on the rms
 * registers apply to the samples as well. The nominal 74,770,000 of the data
 * sheet (page 33) is 0.32 % more; with this value the rms of the samples
 * matches xVRMS within 0.03 %.
 */
#define ADE9000_FULL_SCALE_WAVE 74532013

/**
 * @name   STATUS0 and MASK0 fields, for SENSOR_ATTR_ADE9000_STATUS0 and
 *         SENSOR_ATTR_ADE9000_MASK0
 * @{
 */
/** The energy registers were updated, every EGY_TIME + 1 samples */
#define ADE9000_STATUS0_EGYRDY    BIT(0)
/** An event enabled in WFB_TRG_CFG stopped or marked the waveform buffer */
#define ADE9000_STATUS0_WFB_TRIG  BIT(16)
/** A page enabled in WFB_PG_IRQEN of the waveform buffer is full */
#define ADE9000_STATUS0_PAGE_FULL BIT(17)
/** @} */

/**
 * @name   WFB_CFG fields, for SENSOR_ATTR_ADE9000_WFB_CFG
 * @{
 */
/** Channels a burst read returns, one of ADE9000_WFB_CFG_BURST_CHAN_* */
#define ADE9000_WFB_CFG_BURST_CHAN          GENMASK(3, 0)
/** BURST_CHAN value: all channels */
#define ADE9000_WFB_CFG_BURST_CHAN_ALL      0x0
/** BURST_CHAN value: IA and VA */
#define ADE9000_WFB_CFG_BURST_CHAN_IA_VA    0x1
/** BURST_CHAN value: IB and VB */
#define ADE9000_WFB_CFG_BURST_CHAN_IB_VB    0x2
/** BURST_CHAN value: IC and VC */
#define ADE9000_WFB_CFG_BURST_CHAN_IC_VC    0x3
/** BURST_CHAN value: IA */
#define ADE9000_WFB_CFG_BURST_CHAN_IA       0x8
/** BURST_CHAN value: VA */
#define ADE9000_WFB_CFG_BURST_CHAN_VA       0x9
/** BURST_CHAN value: IB */
#define ADE9000_WFB_CFG_BURST_CHAN_IB       0xA
/** BURST_CHAN value: VB */
#define ADE9000_WFB_CFG_BURST_CHAN_VB       0xB
/** BURST_CHAN value: IC */
#define ADE9000_WFB_CFG_BURST_CHAN_IC       0xC
/** BURST_CHAN value: VC */
#define ADE9000_WFB_CFG_BURST_CHAN_VC       0xD
/** BURST_CHAN value: IN, needs ADE9000_WFB_CFG_WF_IN_EN */
#define ADE9000_WFB_CFG_BURST_CHAN_IN       0xE
/** BURST_CHAN value: single address reads, burst reads off */
#define ADE9000_WFB_CFG_BURST_CHAN_SINGLE   0xF
/** A capture starts when this bit goes from 0 to 1 */
#define ADE9000_WFB_CFG_WF_CAP_EN           BIT(4)
/** Fill the buffer with fixed data rate samples, clear for resampled data */
#define ADE9000_WFB_CFG_WF_CAP_SEL          BIT(5)
/** Fill and trigger mode with fixed data rate, one of ADE9000_WFB_CFG_WF_MODE_* */
#define ADE9000_WFB_CFG_WF_MODE             GENMASK(7, 6)
/** WF_MODE value: stop when the buffer is full */
#define ADE9000_WFB_CFG_WF_MODE_STOP_FULL   0x0
/** WF_MODE value: fill continuously, stop on an enabled trigger event */
#define ADE9000_WFB_CFG_WF_MODE_STOP_TRIG   0x1
/** WF_MODE value: fill continuously, center the capture around an enabled trigger event */
#define ADE9000_WFB_CFG_WF_MODE_CENTER_TRIG 0x2
/** WF_MODE value: fill continuously, save the address of enabled trigger events */
#define ADE9000_WFB_CFG_WF_MODE_CONTINUOUS  0x3
/** Source of the samples, one of ADE9000_WFB_CFG_WF_SRC_* */
#define ADE9000_WFB_CFG_WF_SRC              GENMASK(9, 8)
/** WF_SRC value: sinc4 output at 32 kSPS */
#define ADE9000_WFB_CFG_WF_SRC_SINC4        0x0
/** WF_SRC value: sinc4 + IIR low-pass filter output at 8 kSPS */
#define ADE9000_WFB_CFG_WF_SRC_SINC4_IIR    0x2
/** WF_SRC value: xI_PCF and xV_PCF processed by the DSP at 8 kSPS */
#define ADE9000_WFB_CFG_WF_SRC_PCF          0x3
/** IN samples are read out of the buffer */
#define ADE9000_WFB_CFG_WF_IN_EN            BIT(12)
/** @} */

/**
 * @brief  Continuous fixed-rate PCF waveform capture, bursting IA, VA, IB, VB,
 *         IC and VC without IN. Capture remains disabled until WF_CAP_EN is set.
 */
#define ADE9000_WFB_CFG_PCF_ALL_CONTINUOUS                                                         \
	(FIELD_PREP(ADE9000_WFB_CFG_WF_SRC, ADE9000_WFB_CFG_WF_SRC_PCF) |                          \
	 FIELD_PREP(ADE9000_WFB_CFG_WF_MODE, ADE9000_WFB_CFG_WF_MODE_CONTINUOUS) |                 \
	 ADE9000_WFB_CFG_WF_CAP_SEL |                                                              \
	 FIELD_PREP(ADE9000_WFB_CFG_BURST_CHAN, ADE9000_WFB_CFG_BURST_CHAN_ALL))

/**
 * @name   WFB_PG_IRQEN fields, for SENSOR_ATTR_ADE9000_WFB_PG_IRQEN
 * @{
 */
/**
 * @brief        Raise ADE9000_STATUS0_PAGE_FULL when this page is full.
 * @param page   0 to ADE9000_WFB_PAGE_COUNT - 1
 */
#define ADE9000_WFB_PG_IRQEN_PAGE(page) BIT(page)
/** @} */

/** PAGE_FULL interrupt after the last page of each half of the waveform buffer */
#define ADE9000_WFB_HALF_PG_IRQEN                                                                  \
	(ADE9000_WFB_PG_IRQEN_PAGE(ADE9000_WFB_HALF_PAGES - 1) |                                   \
	 ADE9000_WFB_PG_IRQEN_PAGE(ADE9000_WFB_PAGE_COUNT - 1))

/**
 * @name   WFB_TRG_STAT fields, for SENSOR_ATTR_ADE9000_WFB_TRG_STAT
 * @{
 */
/** Location of the last trigger event in the buffer */
#define ADE9000_WFB_TRG_STAT_WFB_TRIG_ADDR GENMASK(10, 0)
/** Last page that was filled, 0 to ADE9000_WFB_PAGE_COUNT - 1 */
#define ADE9000_WFB_TRG_STAT_WFB_LAST_PAGE GENMASK(15, 12)
/** @} */

/**
 * @brief  Number of values sensor_channel_get() returns for a three phase
 *         channel: phase A, B and C in val[0], val[1] and val[2].
 */
#define ADE9000_PHASE_COUNT 3

/**
 * @brief  Number of multipoint regions: gains xIGAIN0 to xIGAIN4, phase
 *         compensation xPHCAL0 to xPHCAL4 and thresholds MTTHR_L0 to MTTHR_L4
 *         and MTTHR_H0 to MTTHR_H4.
 */
#define ADE9000_MT_REGION_COUNT 5

/**
 * @brief  Custom channels. Unless marked as single, a channel returns
 *         ADE9000_PHASE_COUNT values, so sensor_channel_get() needs an array of
 *         that size.
 */
enum sensor_channel_ade9000 {
	/** Current rms in A, from xIRMS */
	SENSOR_CHAN_ADE9000_IRMS = SENSOR_CHAN_PRIV_START,
	/** Voltage rms in V, from xVRMS */
	SENSOR_CHAN_ADE9000_VRMS,
	/** Single: neutral current rms in A, from NIRMS */
	SENSOR_CHAN_ADE9000_NIRMS,
	/** Single: rms of the sum of the phase currents in A, from ISUMRMS */
	SENSOR_CHAN_ADE9000_ISUMRMS,
	/** Fundamental current rms in A, from xIFRMS */
	SENSOR_CHAN_ADE9000_IFRMS,
	/** Fundamental voltage rms in V, from xVFRMS */
	SENSOR_CHAN_ADE9000_VFRMS,

	/** Total active power in W, from xWATT */
	SENSOR_CHAN_ADE9000_WATT,
	/** Total reactive power in var, from xVAR */
	SENSOR_CHAN_ADE9000_VAR,
	/** Total apparent power in VA, from xVA */
	SENSOR_CHAN_ADE9000_VA,
	/** Fundamental active power in W, from xFWATT */
	SENSOR_CHAN_ADE9000_FWATT,
	/** Fundamental reactive power in var, from xFVAR */
	SENSOR_CHAN_ADE9000_FVAR,
	/** Fundamental apparent power in VA, from xFVA */
	SENSOR_CHAN_ADE9000_FVA,

	/** Power factor, -1 to 1, from xPF. Needs EP_CFG.EGY_PWR_EN */
	SENSOR_CHAN_ADE9000_PF,
	/** Voltage THD in percent, from xVTHD, updated once per second */
	SENSOR_CHAN_ADE9000_VTHD,
	/** Current THD in percent, from xITHD, updated once per second */
	SENSOR_CHAN_ADE9000_ITHD,

	/** Line frequency in Hz, from xPERIOD. 0 for a phase with STATUS1.ZXTOVx set */
	SENSOR_CHAN_ADE9000_FREQ,

	/** Angles VA-VB, VB-VC and VA-VC in degrees. Like all angles scaled with
	 *  the measured xPERIOD of phase A, B and C, with the nominal line
	 *  frequency for a phase with STATUS1.ZXTOVx set
	 */
	SENSOR_CHAN_ADE9000_ANGL_VV,
	/** Angles VA-IA, VB-IB and VC-IC in degrees */
	SENSOR_CHAN_ADE9000_ANGL_VI,
	/** Angles IA-IB, IB-IC and IA-IC in degrees */
	SENSOR_CHAN_ADE9000_ANGL_II,

	/** Instantaneous current in A, one 8 kSPS sample from xI_PCF. Use the
	 *  waveform buffer for coherent waveforms
	 */
	SENSOR_CHAN_ADE9000_I_PCF,
	/** Instantaneous voltage in V, one 8 kSPS sample from xV_PCF */
	SENSOR_CHAN_ADE9000_V_PCF,
	/** Single: instantaneous neutral current in A, one 8 kSPS sample from NI_PCF */
	SENSOR_CHAN_ADE9000_NI_PCF,
	/** Multipoint region in use, 0 to 4, from xMTREGION. 15 without multi-point-compensation */
	SENSOR_CHAN_ADE9000_MTREGION,

	/** One cycle current rms in A, from xIRMSONE, updated every half cycle */
	SENSOR_CHAN_ADE9000_IRMSONE,
	/** One cycle voltage rms in V, from xVRMSONE, updated every half cycle */
	SENSOR_CHAN_ADE9000_VRMSONE,
	/** Single: one cycle neutral current rms in A, from NIRMSONE */
	SENSOR_CHAN_ADE9000_NIRMSONE,
	/** 10/12 cycle current rms in A, from xIRMS1012 */
	SENSOR_CHAN_ADE9000_IRMS1012,
	/** 10/12 cycle voltage rms in V, from xVRMS1012 */
	SENSOR_CHAN_ADE9000_VRMS1012,
	/** Single: 10/12 cycle neutral current rms in A, from NIRMS1012 */
	SENSOR_CHAN_ADE9000_NIRMS1012,

	/** Total active energy in Wh, from xWATTHR_HI */
	SENSOR_CHAN_ADE9000_WATTHR,
	/** Total reactive energy in varh, from xVARHR_HI */
	SENSOR_CHAN_ADE9000_VARHR,
	/** Total apparent energy in VAh, from xVAHR_HI */
	SENSOR_CHAN_ADE9000_VAHR,
	/** Fundamental active energy in Wh, from xFWATTHR_HI */
	SENSOR_CHAN_ADE9000_FWATTHR,
	/** Fundamental reactive energy in varh, from xFVARHR_HI */
	SENSOR_CHAN_ADE9000_FVARHR,
	/** Fundamental apparent energy in VAh, from xFVAHR_HI */
	SENSOR_CHAN_ADE9000_FVAHR,

	/** End marker, not a channel */
	SENSOR_CHAN_ADE9000_MAX,
};

/**
 * @brief  Triggers of the ADE9000 for sensor_trigger_set(). Need
 *         CONFIG_ADE9000_TRIGGER and the IRQ line in the devicetree, with
 *         irq0-on-irq1 IRQ1 also serves the IRQ0 trigger. The handler runs in
 *         thread context, after the driver has read and cleared STATUS0 or
 *         STATUS1. SENSOR_ATTR_ADE9000_STATUS0 and SENSOR_ATTR_ADE9000_STATUS1
 *         return the events that fired.
 */
enum sensor_trigger_type_ade9000 {
	/** IRQ0 fired for one of the events enabled with SENSOR_ATTR_ADE9000_MASK0 */
	SENSOR_TRIG_ADE9000_IRQ0 = SENSOR_TRIG_PRIV_START,
	/** IRQ1 fired for one of the events enabled with SENSOR_ATTR_ADE9000_MASK1 */
	SENSOR_TRIG_ADE9000_IRQ1,
};

/**
 * @brief  Attributes of the ADE9000. Register values are in val1.
 */
enum sensor_attribute_ade9000 {
	/** MASK0, events that pull IRQ0 low. SENSOR_CHAN_ALL, set and get */
	SENSOR_ATTR_ADE9000_MASK0 = SENSOR_ATTR_PRIV_START,
	/** MASK1, events that pull IRQ1 low. SENSOR_CHAN_ALL, set and get */
	SENSOR_ATTR_ADE9000_MASK1,
	/** STATUS0 as read and cleared by the last IRQ0 trigger, without
	 *  CONFIG_ADE9000_TRIGGER by the last fetch. SENSOR_CHAN_ALL, get only
	 */
	SENSOR_ATTR_ADE9000_STATUS0,
	/** STATUS1 as read and cleared by the last IRQ1 trigger, without
	 *  CONFIG_ADE9000_TRIGGER by the last fetch. SENSOR_CHAN_ALL, get only
	 */
	SENSOR_ATTR_ADE9000_STATUS1,
	/** WFB_CFG, source and mode of the waveform buffer. SENSOR_CHAN_ALL, set and get */
	SENSOR_ATTR_ADE9000_WFB_CFG,
	/** WFB_PG_IRQEN, pages that raise PAGE_FULL. SENSOR_CHAN_ALL, set and get */
	SENSOR_ATTR_ADE9000_WFB_PG_IRQEN,
	/** One cycle rms offset xIRMSONEOS, xVRMSONEOS, NIRMSONEOS. IRMS, VRMS, NIRMS */
	SENSOR_ATTR_ADE9000_OFFSET_ONE,
	/** 10/12 cycle rms offset xIRMS1012OS, xVRMS1012OS, NIRMS1012OS. IRMS, VRMS, NIRMS */
	SENSOR_ATTR_ADE9000_OFFSET_1012,
	/** Phase compensation xPHCAL0 on IRMS, NPHCAL on NIRMS, in degrees. Set and get */
	SENSOR_ATTR_ADE9000_PHCAL,
	/** Line frequency in Hz, 50 or 60 (ACCMODE.SELFREQ). SENSOR_CHAN_ALL, set and get */
	SENSOR_ATTR_ADE9000_LINE_FREQUENCY,
	/** Network configuration, one of ADE9000_DT_VCONSEL_* (ACCMODE.VCONSEL).
	 *  SENSOR_CHAN_ALL, set and get
	 */
	SENSOR_ATTR_ADE9000_VCONSEL,
	/** 1 calculates IB = -IA - IC, 0 measures IB (ACCMODE.ICONSEL). SENSOR_CHAN_ALL,
	 *  set and get
	 */
	SENSOR_ATTR_ADE9000_ICONSEL,
	/** EVENT_MASK, events that drive the EVENT pin. Set and get */
	SENSOR_ATTR_ADE9000_EVENT_MASK,
	/** EVENT_STATUS. Get only */
	SENSOR_ATTR_ADE9000_EVENT_STATUS,
	/** OILVL, overcurrent threshold. Set and get */
	SENSOR_ATTR_ADE9000_OILVL,
	/** OIA, OIB, OIC, OIN, overcurrent values. Count 4, get only */
	SENSOR_ATTR_ADE9000_OI,
	/** OISTATUS, phases in overcurrent. Get only */
	SENSOR_ATTR_ADE9000_OISTATUS,
	/** DIP_LVL, dip threshold. Set and get */
	SENSOR_ATTR_ADE9000_DIP_LVL,
	/** DIP_CYC, dip duration in half cycles. Set and get */
	SENSOR_ATTR_ADE9000_DIP_CYC,
	/** DIPA, DIPB, DIPC, voltage during a dip. Count 3, get only */
	SENSOR_ATTR_ADE9000_DIP,
	/** SWELL_LVL, swell threshold. Set and get */
	SENSOR_ATTR_ADE9000_SWELL_LVL,
	/** SWELL_CYC, swell duration in half cycles. Set and get */
	SENSOR_ATTR_ADE9000_SWELL_CYC,
	/** SWELLA, SWELLB, SWELLC, voltage during a swell. Count 3, get only */
	SENSOR_ATTR_ADE9000_SWELL,
	/** IPEAK, peak current and its phase. Get only */
	SENSOR_ATTR_ADE9000_IPEAK,
	/** VPEAK, peak voltage and its phase. Get only */
	SENSOR_ATTR_ADE9000_VPEAK,
	/** CONFIG3, peak detection (PEAKSEL) and overcurrent (OC_EN). Set and get */
	SENSOR_ATTR_ADE9000_CONFIG3,
	/** ZXTOUT, zero crossing timeout. Set and get */
	SENSOR_ATTR_ADE9000_ZXTOUT,
	/** ZXTHRSH, zero crossing threshold. Set and get */
	SENSOR_ATTR_ADE9000_ZXTHRSH,
	/** ZX_LP_SEL, source of the zero crossing and the line period. Set and get */
	SENSOR_ATTR_ADE9000_ZX_LP_SEL,
	/** SEQ_CYC, line cycles for the phase sequence error. Set and get */
	SENSOR_ATTR_ADE9000_SEQ_CYC,
	/** USER_PERIOD, line period used with use-user-period. Set and get */
	SENSOR_ATTR_ADE9000_USER_PERIOD,
	/** COM_PERIOD, line period of the combined voltages. Get only */
	SENSOR_ATTR_ADE9000_COM_PERIOD,
	/** ISUMLVL, threshold for the MISMTCH event. Set and get */
	SENSOR_ATTR_ADE9000_ISUMLVL,
	/** ACT_NL_LVL, REACT_NL_LVL, APP_NL_LVL, no load thresholds. Count 3, set and get */
	SENSOR_ATTR_ADE9000_NL_LVL,
	/** PHNOLOAD, phases in no load. Get only */
	SENSOR_ATTR_ADE9000_PHNOLOAD,
	/** PHSIGN, sign of the powers. Get only */
	SENSOR_ATTR_ADE9000_PHSIGN,
	/** PWR_TIME, update interval of the _ACC power registers. Set and get */
	SENSOR_ATTR_ADE9000_PWR_TIME,
	/** WFB_TRG_CFG, events that stop or mark the waveform buffer. Set and get */
	SENSOR_ATTR_ADE9000_WFB_TRG_CFG,
	/** WFB_TRG_STAT, last page and trigger address. Get only */
	SENSOR_ATTR_ADE9000_WFB_TRG_STAT,
	/** CFMODE, energy type and enable of CF1 to CF4. Set and get */
	SENSOR_ATTR_ADE9000_CFMODE,
	/** COMPMODE, phases summed into CF1 to CF4. Set and get */
	SENSOR_ATTR_ADE9000_COMPMODE,
	/** CF1DEN to CF4DEN, pulse dividers. Count 4, set and get */
	SENSOR_ATTR_ADE9000_CFDEN,
	/** CF_LCFG, CF pulse width and latching. Set and get */
	SENSOR_ATTR_ADE9000_CF_LCFG,
	/** WTHR, VARTHR, VATHR, CF energy thresholds. Count 3, set and get */
	SENSOR_ATTR_ADE9000_CF_THR,
	/** CRC_OPTEN, optional registers covered by the configuration CRC. Set and get */
	SENSOR_ATTR_ADE9000_CRC_OPTEN,
	/** CRC_FORCE, write 1 to update CRC_RSLT now. Set and get */
	SENSOR_ATTR_ADE9000_CRC_FORCE,
	/** CRC_RSLT, CRC of the configuration registers. Get only */
	SENSOR_ATTR_ADE9000_CRC_RSLT,
	/** CRC_SPI, CRC of the last SPI read. Get only */
	SENSOR_ATTR_ADE9000_CRC_SPI,
	/** LAST_CMD, last SPI command. Get only */
	SENSOR_ATTR_ADE9000_LAST_CMD,
	/** LAST_DATA_16, data of the last 16 bit SPI transfer. Get only */
	SENSOR_ATTR_ADE9000_LAST_DATA_16,
	/** LAST_DATA_32, data of the last 32 bit SPI transfer. Get only */
	SENSOR_ATTR_ADE9000_LAST_DATA_32,
	/** ADC_REDIRECT, which ADC feeds each input of the DSP, e.g. to swap VB and VC.
	 *  Set and get
	 */
	SENSOR_ATTR_ADE9000_ADC_REDIRECT,
	/** WR_LOCK, write the lock or unlock key from the data sheet. Set and get */
	SENSOR_ATTR_ADE9000_WR_LOCK,
};

/**
 * @brief  Multipoint gain and phase calibration of all phases, in units, for
 *         ade9000_multipoint_set() and ade9000_multipoint_get().
 *
 * The xIRMS current selects the region, data sheet figure 59: region n spans
 * thr_low[n] to thr_high[n]. Rising, the current leaves region n above
 * thr_high[n], falling it leaves region n + 1 below thr_low[n + 1], so
 * thr_low[n + 1] to thr_high[n] is the hysteresis. Typically thr_low[0] is 0
 * and thr_high[4] the full-scale current. In the active region the gain
 * xIGAINn applies on top of xIGAIN, and xPHCALn replaces xPHCAL0.
 */
struct ade9000_multipoint {
	/** Multipoint compensation on (CONFIG0.MTEN). Only an enabled table has
	 *  to have its regions in order
	 */
	bool enable;
	/** Lower thresholds MTTHR_L0 to MTTHR_L4 in A, shared by all phases */
	struct sensor_value thr_low[ADE9000_MT_REGION_COUNT];
	/** Upper thresholds MTTHR_H0 to MTTHR_H4 in A, shared by all phases */
	struct sensor_value thr_high[ADE9000_MT_REGION_COUNT];
	/** Calibration of phase A, B and C */
	struct {
		/** Gains xIGAIN0 to xIGAIN4 as a factor, e.g. 1.002 */
		struct sensor_value gain[ADE9000_MT_REGION_COUNT];
		/** Phase compensation xPHCAL0 to xPHCAL4 in degrees */
		struct sensor_value phcal[ADE9000_MT_REGION_COUNT];
	} phase[ADE9000_PHASE_COUNT];
};

/**
 * @brief        Read samples out of the waveform buffer with one burst read.
 *               Each location holds one 32 bit sample, or two 16 bit samples
 *               when the buffer is filled with resampled data.
 * @param dev    ADE9000 device
 * @param index  First location, 0 to ADE9000_WFB_SIZE - 1
 * @param buf    Takes the samples in CPU byte order
 * @param count  Number of locations to read
 * @return       0 on success, -EINVAL if the range leaves the buffer, negative
 *               errno of the SPI transfer otherwise
 */
int ade9000_waveform_read(const struct device *dev, uint16_t index, int32_t *buf, size_t count);

/**
 * @brief        Volts or amperes per xV_PCF, xI_PCF or NI_PCF code, also per
 *               sample of the waveform buffer with ADE9000_WFB_CFG_WF_SRC_PCF:
 *               full-scale * sqrt(2) / ADE9000_FULL_SCALE_WAVE.
 * @param dev    ADE9000 device
 * @param chan   SENSOR_CHAN_ADE9000_V_PCF for V per code,
 *               SENSOR_CHAN_ADE9000_I_PCF or SENSOR_CHAN_ADE9000_NI_PCF for A
 *               per code
 * @param scale  Takes the factor
 * @return       0 on success, -EINVAL for another channel or a NULL scale
 */
int ade9000_wave_scale(const struct device *dev, enum sensor_channel chan, float *scale);

#ifdef CONFIG_ADE9000_MULTIPOINT
/**
 * @brief        Set the multipoint calibration of all phases. All values are
 *               checked first, so an invalid table leaves every register
 *               unchanged. The compensation is off while the registers are
 *               written. Holds until the next reset of the IC. Like
 *               SENSOR_ATTR_ADE9000_PHCAL the angles use the current line
 *               frequency.
 * @param dev    ADE9000 device
 * @param mp     Calibration table
 * @return       0 on success, -EINVAL for a value out of range or, with
 *               enable set, regions out of order, negative errno of the SPI
 *               transfer otherwise
 */
int ade9000_multipoint_set(const struct device *dev, const struct ade9000_multipoint *mp);

/**
 * @brief        Read back the multipoint calibration of all phases.
 * @param dev    ADE9000 device
 * @param mp     Takes the calibration table
 * @return       0 on success, negative errno of the SPI transfer otherwise
 */
int ade9000_multipoint_get(const struct device *dev, struct ade9000_multipoint *mp);
#endif /* CONFIG_ADE9000_MULTIPOINT */
