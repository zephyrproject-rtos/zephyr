/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_H__

/* DMIC timestamping registers */
#define TS_DMIC_LOCAL_TSCTRL_OFFSET	0x000
#define TS_DMIC_LOCAL_OFFS_OFFSET	0x004
#define TS_DMIC_LOCAL_SAMPLE_OFFSET	0x008
#define TS_DMIC_LOCAL_WALCLK_OFFSET	0x010
#define TS_DMIC_TSCC_OFFSET		0x018

/* Timestamping */
#define TIMESTAMP_BASE			0x00071800

/* Time Stamp Control Register */
#define TS_DMIC_LOCAL_TSCTRL		(TIMESTAMP_BASE + TS_DMIC_LOCAL_TSCTRL_OFFSET)

/* Intersample offset Register */
#define TS_DMIC_LOCAL_OFFS		(TIMESTAMP_BASE + TS_DMIC_LOCAL_OFFS_OFFSET)

#define TS_DMIC_LOCAL_SAMPLE		(TIMESTAMP_BASE + TS_DMIC_LOCAL_SAMPLE_OFFSET)
#define TS_DMIC_LOCAL_WALCLK		(TIMESTAMP_BASE + TS_DMIC_LOCAL_WALCLK_OFFSET)

/* Time Stamp Counter Captured 64 bits */
#define TS_DMIC_TSCC			(TIMESTAMP_BASE + TS_DMIC_TSCC_OFFSET)

/* New Timestamp Taken */
#define TS_LOCAL_TSCTRL_NTK		BIT(31)

/* Interrupt on New Timestamp Enable */
#define TS_LOCAL_TSCTRL_IONTE		BIT(30)

/* DMA Type Select */
#define TS_LOCAL_TSCTRL_DMATS		GENMASK(13, 12)

/* Capture Link Select - select which link wall clock to time stamp. */
#define TS_LOCAL_TSCTRL_CLNKS		GENMASK(11, 10)

/* Hammock Harbor Time Stamp Enable */
#define TS_LOCAL_TSCTRL_HHTSE		BIT(7)

/* Link Wall Clock Select */
#define TS_LOCAL_TSCTRL_LWCS		BIT(6)

/* On Demand Time Stamp */
#define TS_LOCAL_TSCTRL_ODTS		BIT(5)

/* Capture DMA Select */
#define TS_LOCAL_TSCTRL_CDMAS		GENMASK(4, 0)

/* Snapshot of Audio Wall Clock Offset counter (frame offset). */
#define TS_LOCAL_OFFS_FRM		GENMASK(15, 12)

/* Snapshot of Audio Wall Clock Offset counter (clock offset). */
#define TS_LOCAL_OFFS_CLK		GENMASK(11, 0)


/* DMIC register offsets */

/* Global registers */

/* Common FIFO channels register (primary & secondary) (0000 - 0FFF)
 * PDM Primary Channel
 */

/* Control registers for packers */
#define OUTCONTROL		0x0000

/* Status Register for FIFO interface */
#define OUTSTAT			0x0004

/* Data read/Write port for FIFO */
#define OUTDATA			0x0008

/* (crossed out)	000Ch		LOCAL_OFFS		Offset Counter
 * (crossed out)	0010h		LOCAL_TSC0		64-bit Wall Clock timestamp
 * (crossed out)	0018h		LOCAL_SAMPLE0		64-bit Sample Count
 *			001Ch - 00FFh				Reserved space for extensions
 */

/* Offset to PDM Secondary Channel */
#define PDM_CHANNEL_REGS_SIZE	0x100


#define GLOBAL_CAPABILITIES	0x200

#define PDM_COEFFICIENT_A	0x400
#define PDM_COEF_RAM_A_LENGTH	0x400

#define PDM_COEFFICIENT_B	0x800
#define PDM_COEF_RAM_B_LENGTH	0x400

#define PDM0			0x1000
#define PDM1			0x2000
#define PDM2			0x3000
#define PDM3			0x4000


/* Local registers in each PDMx */

/* Control register for CIC configuration and decimator setting */
#define CIC_CONTROL		0x000

/* Control of the CIC filter plus voice channel (B) FIR decimation factor */
#define CIC_CONFIG		0x004

/* Microphone interface control register */
#define MIC_CONTROL		0x00c

/* FIR config */

/* Control for the FIR decimator */
#define FIR_CONTROL		0x020

/* Configuration of FIR decimator parameters */
#define FIR_CONFIG		0x024

/* DC offset for left channel */
#define DC_OFFSET_LEFT		0x028

/* DC offset for right channel */
#define DC_OFFSET_RIGHT		0x02c

/* Gain for left channel */
#define OUT_GAIN_LEFT		0x030

/* Gain for right channel */
#define OUT_GAIN_RIGHT		0x034

/* Offset to secondary FIR */
#define FIR_CHANNEL_REGS_SIZE	0x20


/* Digital Mic Shim Registers */

/* Digital Microphone Link Control */
#define DMICLCTL		0x04

/* Digital Microphone IP Pointer */
#define DMICIPPTR		0x08


/* OUTCONTROL0 and OUTCONTROL1 */

/* OUTCONTROLx IPM bit fields style */
#define OUTCONTROL_BFTH_MAX	4 /* Max depth 16 */

/* Threshold Interrupt Enable */
#define OUTCONTROL_TIE				BIT(27)

/* Start Input Packer */
#define OUTCONTROL_SIP				BIT(26)

/* FIFO Initialize (FINIT): The software will set this bit to immediately clear FIFO pointers. */
#define OUTCONTROL_FINIT			BIT(25)

/* Input Format Change Indicator */
#define OUTCONTROL_FCI				BIT(24)

/* Burst FIFO Threshold */
#define OUTCONTROL_BFTH				GENMASK(23, 20)

/* Output Format */
#define OUTCONTROL_OF				GENMASK(19, 18)


/* This field decides the packer mode */
#define OUTCONTROL_IPM				GENMASK(17, 15)

/* Source decimator for 1st stereo/mono data placeholder. */
#define OUTCONTROL_IPM_SOURCE_1			GENMASK(14, 13)

/* Source decimator for 2nd stereo/mono data placeholder. */
#define OUTCONTROL_IPM_SOURCE_2			GENMASK(12, 11)

/* Source decimator for 3rd stereo/mono data placeholder. */
#define OUTCONTROL_IPM_SOURCE_3			GENMASK(10, 9)

/* Source decimator for 4th stereo/mono data placeholder. */
#define OUTCONTROL_IPM_SOURCE_4			GENMASK(8, 7)

/* Defines the mode of operation for all source decimator. */
#define OUTCONTROL_IPM_SOURCE_MODE		BIT(6)

/* FIFO Trigger Threshold */
#define OUTCONTROL_TH				GENMASK(5, 0)


/* OUTSTAT0 and OUTSTAT1 bits */

/* Asynchronous FIFO is empty */
#define OUTSTAT_AFE				BIT(31)

/* Asynchronous FIFO Not Empty */
#define OUTSTAT_ASNE				BIT(29)

/* FIFO Service Request */
#define OUTSTAT_RFS				BIT(28)

/* Overrun */
#define OUTSTAT_ROR				BIT(27)

 /* FIFO Level (FL): Current FIFO Level in the Asynchronous FIFO. */
#define OUTSTAT_FL_MASK				GENMASK(8, 0)

/* CIC_CONTROL bits */

/* Microphone interface reset. */
#define CIC_CONTROL_SOFT_RESET			BIT(16)

/* When set to 1, the CIC channel B (right) is started, otherwise it is muted and idle. */
#define CIC_CONTROL_CIC_START_B			BIT(15)

/* When set to 1, the CIC channel A (left) is started, otherwise it is muted and idle. */
#define CIC_CONTROL_CIC_START_A			BIT(14)

/* Polarity of the microphone output. */
#define CIC_CONTROL_MIC_B_POLARITY		BIT(3)

/* Polarity of the microphone output. */
#define CIC_CONTROL_MIC_A_POLARITY		BIT(2)

/* Mute currently active microphones */
#define CIC_CONTROL_MIC_MUTE			BIT(1)


/* CIC_CONFIG masks */

/* Number of bits for shift right in the output stage of the CIC filter to compensate the gain
 * accumulated due to decimation.
 */
#define CIC_CONFIG_CIC_SHIFT			GENMASK(27, 24)

/* Period of activation of comb section in the microphone clocks minus 1 */
#define CIC_CONFIG_COMB_COUNT			GENMASK(15, 8)


/* MIC_CONTROL */

/* Clock divider used for producing the microphone clock from audio IO clock with approximately 50%
 * duty cycle.
 */
#define MIC_CONTROL_PDM_CLKDIV			GENMASK(15, 8)

/* Inverts the clock edge that will be used to sample microphone data stream. */
#define MIC_CONTROL_CLK_EDGE			BIT(3)

/* Indicates the PDM DMIC clock for the decimator will be sourced from external component instead
 * of using the PDM DMIC clock generator output
 */
#define MIC_CONTROL_SLAVE_MODE			BIT(2)

/* Enable clock on microphone B (Right) */
#define MIC_CONTROL_PDM_EN_B			BIT(1)

/* Enable clock on microphone A (left) */
#define MIC_CONTROL_PDM_EN_A			BIT(0)


/* FIR_CONTROL_A bits */

/* Enable the power gating capability of the coefficient. */
#define FIR_CONTROL_CRFPGE			BIT(28)

/* Power gating capability of the left channel */
#define FIR_CONTROL_LDRFPGE			BIT(29)

/* Enable the power gating capability of the right channel */
#define FIR_CONTROL_RDRFPGE			BIT(30)

/* FIR decimation filter is started. */
#define FIR_CONTROL_START			BIT(7)

/* Array microphone control bit for synchronous start of multiple interfaces. */
#define FIR_CONTROL_ARRAY_START_EN		BIT(6)

/* Periodic synchronous start control of multiple PDM */
#define FIR_CONTROL_PERIODIC_START_EN		BIT(5)

/* Automatic DC compensation enable */
#define FIR_CONTROL_DCCOMP			BIT(4)

/* Write in the coefficient memory will mute the output for the N audio clocks */
#define FIR_CONTROL_AUTO_MUTE			BIT(2)

/* Mute outputs of this filter and set it to zero. */
#define FIR_CONTROL_MUTE			BIT(1)

/* Filter operates in stereo mode */
#define FIR_CONTROL_STEREO			BIT(0)


 /* FIR_CONFIG bits */

/* Decimation factor of the FIR filter minus 1. */
#define FIR_CONFIG_FIR_DECIMATION		GENMASK(20, 16)

/* Number of bits for shift right in the output stage of the CIC filter to compensate the gain
 * accumulated due to decimation.
 */
#define FIR_CONFIG_FIR_SHIFT			GENMASK(11, 8)

/* The number of active taps of the FIR filter minus 1. */
#define FIR_CONFIG_FIR_LENGTH			GENMASK(7, 0)


/* DC_OFFSET_LEFT and DC_OFFSET_RIGHT */

/* Value added to the output of the FIR filter. */
#define DC_OFFSET_DC_OFFS			GENMASK(21, 0)


/* OUT_GAIN_LEFT and OUT_GAIN_RIGHT */

/* Value added to the output of the FIR filter. */
#define OUT_GAIN				GENMASK(19, 0)

/* FIR coefficients */
#define FIR_COEF				GENMASK(19, 0)


/* GLOBAL_CAPABILITIES */

/* Nnumber of data entries supported in the PCM XCLK FIFO per FIR output. */
#define GLOBAL_CAP_PCM_XCLK_FIFO_DEPTH		GENMASK(5, 0)

/* Port Count */
#define GLOBAL_CAP_PORT_COUNT			GENMASK(7, 6)

/* FIR Count */
#define GLOBAL_CAP_FIR_COUNT			BIT(8)

/* FIR max gain configuration. */
#define GLOBAL_CAP_FIR_MAX_GAIN			BIT(9)

/* FIR A RF Depth */
#define GLOBAL_CAP_FIR_A_RF_DEPTH		GENMASK(23, 16)

/* FIR B RF Depth */
#define GLOBAL_CAP_FIR_B_RF_DEPTH		GENMASK(31, 24)


/* Digital Mic Shim Registers */
#ifdef CONFIG_SOC_INTEL_ACE20_LNL
#include <ace20_lnl/dmic_regs_ace2x.h>
#elif CONFIG_SOC_INTEL_ACE15_MTPM
#include <ace15_mtpm/dmic_regs_ace1x.h>
#elif CONFIG_SOC_INTEL_ACE30
#include <ace30/dmic_regs_ace3x.h>
#else
#error "Unknown SoC"
#endif

#endif /* !__INTEL_DAI_DRIVER_DMIC_REGS_H__ */
