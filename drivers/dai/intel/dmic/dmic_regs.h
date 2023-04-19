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

/* Time Stamp Control Register
 *
 * These registers control the behavior of the timestamping logic and allow requesting-on-demand
 * timestamp.
 */
#define TS_DMIC_LOCAL_TSCTRL		(TIMESTAMP_BASE + TS_DMIC_LOCAL_TSCTRL_OFFSET)

/* Intersample offset Register */
#define TS_DMIC_LOCAL_OFFS		(TIMESTAMP_BASE + TS_DMIC_LOCAL_OFFS_OFFSET)

#define TS_DMIC_LOCAL_SAMPLE		(TIMESTAMP_BASE + TS_DMIC_LOCAL_SAMPLE_OFFSET)
#define TS_DMIC_LOCAL_WALCLK		(TIMESTAMP_BASE + TS_DMIC_LOCAL_WALCLK_OFFSET)

/* Time Stamp Counter Captured 64 bits */
#define TS_DMIC_TSCC			(TIMESTAMP_BASE + TS_DMIC_TSCC_OFFSET)

/* New Timestamp Taken
 *
 * This bit is set when a new timestamp was taken either as a result of HH event or on-demand local
 * timestamp. SW needs to clear this bit to 0 by writing a 1 before starting the next time stamp
 * capture.
 */
#define TS_LOCAL_TSCTRL_NTK		BIT(31)

/* Interrupt on New Timestamp Enable
 *
 * This bit controls whether an interrupt request to DSP will be sent on the timestamping event when
 * the new timestamp is ready. When set to 1, it allows NTK bit to propagate for causing DSP
 * interrupt (if the owner is DSP FW).
 */
#define TS_LOCAL_TSCTRL_IONTE		BIT(30)

#ifdef CONFIG_SOC_SERIES_INTEL_ACE
/* DMA Type Select
 *
 * Used to select which DMA type to time stamp, or to skip DMA time stamp if none of them are active
 * 00: GPDMA
 * 01: HD-A DMA
 * 1x: no DMA active for time stamp
 * Must be programmed before ODTS or HHTSE is set.
 */
#define TS_LOCAL_TSCTRL_DMATS		GENMASK(13, 12)

/* Capture Link Select
 *
 * Used to select which link wall clock to time stamp.
 * 00: USB Audio Offload Link
 * 01: Reserved
 * 10: HD-A Link
 * 11: iDisp-A link
 * Must be programmed before ODTS or HHTSE is set.
 * Locked to appear as RO per allocation programmed in HfUAOLA.LC + HfHDALA.LC register fields.
 */
#define TS_LOCAL_TSCTRL_CLNKS		GENMASK(11, 10)

#else	/* CONFIG_SOC_SERIES_INTEL_ACE */

/* When set, this bit will let the timestamer automatically capture the local timestamp when the
 * stream is started. Must be mutually exclusive with ODTS and HHTSE.
 */
#define TS_LOCAL_TSCTRL_SIP		BIT(8)

#endif	/* CONFIG_SOC_SERIES_INTEL_ACE */

/* Hammock Harbor Time Stamp Enable
 *
 * When set, it initiates a system-wide (HH) timestamping events. The bit is cleared when the HH
 * timestamping process is complete and ART is delivered as the global time stamp counter.
 * Do not set this bit when Vnn is off.
 * Must be mutually exclusive with ODTS.
 */
#define TS_LOCAL_TSCTRL_HHTSE		BIT(7)

/* Link Wall Clock Select
 *
 * When set, it selects link wall clock value is to be captured in addition to the DSP wall clock.
 * Note that the bit only functional for USB Audio Offload Link, HD-A Link, or iDisp-A Link.
 * Must be programmed before ODTS or HHTSE is set.
 * Locked to appear as RO per allocation programmed in HfUAOLA.LC + HfHDALA.LC register fields.
 */
#define TS_LOCAL_TSCTRL_LWCS		BIT(6)

/* On Demand Time Stamp
 *
 * Request of the audio timestamping event on demand. When the bit is written, it toggles local on
 * demand timestamp wire which causes coordinated timestamp capture within the audio cluster. The
 * bit is cleared when the new local timestamp is available.
 * Must be mutually exclusive with HHTSE
 */
#define TS_LOCAL_TSCTRL_ODTS		BIT(5)

/* Capture DMA Select
 *
 * Used to select which DMA?s link position value is to be captured for the timestamping event.
 * For HD-A DMA:
 * Bit 4 = 1 for ODMA, 0 for IDMA
 * Bit 3:0 indicates the respective DMA engine index.
 * For GPDMA:
 * Bit 4:0 indicates the respective DMA channel index, starting from value 0 to n*m-1, selecting
 * GPDMAC0 ch 0 to ch n, followed by next GPDMA ch 0 to ch n, until GPDMACm ch 0 to ch n.
 * Must be programmed before ODTS or HHTSE is set.
 */
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
#define OUTCONTROL0		0x0000

/* Status Register for FIFO interface */
#define OUTSTAT0		0x0004

/* Data read/Write port for FIFO */
#define OUTDATA0		0x0008

/* (crossed out)	000Ch		LOCAL_OFFS		Offset Counter
 * (crossed out)	0010h		LOCAL_TSC0		64-bit Wall Clock timestamp
 * (crossed out)	0018h		LOCAL_SAMPLE0		64-bit Sample Count
 *			001Ch - 00FFh				Reserved space for extensions
 */


/* PDM Secondary Channel */

/* Control registers for packers */
#define OUTCONTROL1		0x0100

/* Status Register for FIFO interface */
#define OUTSTAT1		0x0104

/* Data read/Write port for FIFO */
#define OUTDATA1		0x0108

/* (crossed out)	010Ch		LOCAL_OFFS		Offset Counter
 * (crossed out)	0110h		LOCAL_TSC0		64-bit Wall Clock timestamp
 * (crossed out)	0118h		LOCAL_SAMPLE0		64-bit Sample Count
 *			011Ch - 0FFFh				Reserved space for extensions
 */

#define GLOBAL_CAPABILITIES	0x200

#define PDM0			0x1000
#define PDM0_COEFFICIENT_A	0x1400
#define PDM0_COEFFICIENT_B	0x1800

#define PDM1			0x2000
#define PDM1_COEFFICIENT_A	0x2400
#define PDM1_COEFFICIENT_B	0x2800

#define PDM2			0x3000
#define PDM2_COEFFICIENT_A	0x3400
#define PDM2_COEFFICIENT_B	0x3800

#define PDM3			0x4000
#define PDM3_COEFFICIENT_A	0x4400
#define PDM3_COEFFICIENT_B	0x4800

#define PDM_COEF_RAM_A_LENGTH	0x0400
#define PDM_COEF_RAM_B_LENGTH	0x0400

/* Local registers in each PDMx */

/* Control register for CIC configuration and decimator setting */
#define CIC_CONTROL		0x000

/* Control of the CIC filter plus voice channel (B) FIR decimation factor */
#define CIC_CONFIG		0x004

/* Microphone interface control register */
#define MIC_CONTROL		0x00c

/* FIR config */

/* Control for the FIR decimator (channel A) */
#define FIR_CONTROL_A		0x020

/* Configuration of FIR decimator parameters (channel A) */
#define FIR_CONFIG_A		0x024

/* DC offset for left channel */
#define DC_OFFSET_LEFT_A	0x028

/* DC offset for right channel */
#define DC_OFFSET_RIGHT_A	0x02c

/* Gain for left channel */
#define OUT_GAIN_LEFT_A		0x030

/* Gain for right channel */
#define OUT_GAIN_RIGHT_A	0x034

/* Control for the FIR decimator (channel B) */
#define FIR_CONTROL_B		0x040

/* Configuration of FIR decimator parameters (channel B) */
#define FIR_CONFIG_B		0x044

/* DC offset for left channel */
#define DC_OFFSET_LEFT_B	0x048

/* DC offset for right channel */
#define DC_OFFSET_RIGHT_B	0x04c

/* Gain for left channel */
#define OUT_GAIN_LEFT_B		0x050

/* Gain for right channel */
#define OUT_GAIN_RIGHT_B	0x054

#define PDM_COEFFICIENT_A	0x400
#define PDM_COEFFICIENT_B	0x800


/* Digital Mic Shim Registers */

/* Digital Microphone Link Control
 *
 * This register controls the direct attached digital microphone interface.
 * Note: The number of DMIC stereo port implementation is wrapped inside a single DMIC IP. Therefore
 * only a single SPA and CPA bit even through parameter DMICPC may be > 1.
 */
#define DMICLCTL		0x04

/* Digital Microphone IP Pointer
 *
 * This register provides the pointer to the direct attached digital microphone IP registers.
 */
#define DMICIPPTR		0x08


/* OUTCONTROL0 and OUTCONTROL1 */

/* OUTCONTROLx IPM bit fields style */
#define OUTCONTROL_BFTH_MAX	4 /* Max depth 16 */

/* Threshold Interrupt Enable (TIE)
 *
 * When set, this bit allows to assert the interrupt request to DSPs when the FIFO contains data
 * above the threshold
 */
#define OUTCONTROL_TIE				BIT(27)

/* Start Input Packer (SIP)
 *
 * When cleared, this bit will immediately stop the input packer. At the same time,
 * the Array_Start_Enable signal will be de-asserted on all interfaces. This effectively resets
 * the PDM logic and stops audio capture. The FIFOs still may be drained of DMA controller is
 * working. When the bit is set, the decimators will start and will propagate the data into FIFOs.
 * When the first sample reaches the FIFO, its Start timestamp will be taken.
 */
#define OUTCONTROL_SIP				BIT(26)

/* FIFO Initialize (FINIT): The software will set this bit to immediately clear FIFO pointers. */
#define OUTCONTROL_FINIT			BIT(25)

/* Input Format Change Indicator (FCI)
 *
 * The software will set this bit when the format change is requested on the fly. When this bit is
 * set, the new IPM settings will not change immediately. Instead, the FIFO will first drain in the
 * current mode, and the end of stream signal will be asserted to the DMA controller. After that,
 * the new format will be set and data flow to FIFO will resume. At that time, this bit will be
 * automatically cleared.
 */
#define OUTCONTROL_FCI				BIT(24)

/* Burst FIFO Threshold (BFTH)
 *
 * These bits set the threshold above which the Asynchronous FIFO will request a DMA transaction
 * (DRQ) in units of samples.
 * 0: - Immediately request DMA if FIFO is not empty {invalid}
 * 1: - 2 entries needed for request
 * 2: - 4 entries needed for request
 * 3: - 8 entries needed for request
 * 4: - 16 entries needed for request
 * 5: - 32 entries needed for request  {invalid}
 * 6: - 64 entries needed for request {invalid}
 * 7: - 128 entries needed for request {invalid}
 * Note: When IPM_NUMBER_OF_DECIMATORS = 3-4, the maximum value can be set for BFTH is 2 (4 entries)
 * For 2 decimators maximum BFTH is 3 (8 entries).
 */
#define OUTCONTROL_BFTH				GENMASK(23, 20)

/* Output Format (OF)
 *
 * 0: 16-bit sampling, two samples are packed in 32-bit word (left sample in LSB)
 * 1: 16-bit sampling, two samples are packed in 32-bit word, channel swapped (left sample in MSB)
 * 2: 24-bit sampling, samples are packed in 32-bit word and the least significant bits are padded
 *    with sign; bits [1:0] contain the sample channel number
 * 3: reserved
 */
#define OUTCONTROL_OF				GENMASK(19, 18)


/* IPM: This field decides the packer mode, depending on the number of channels configured for the
 * PDM DMIC IP. It indicates the number of stereo decimators to be handled by input packer mode.
 * E.g. for stereo capture use 1. For mono this is the number of mono mode decimators needed.
 * 0 = none, 1-4 = 1-4 decimators, 5-7 = Reserved.
 */
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define OUTCONTROL_IPM				GENMASK(17, 15)
#else
/* For 4 channels or less, bits [2:1] of the field selects the packer mode as decoding below, and
 * bit [0] is a read-only reserved bit.
 */
#define OUTCONTROL_IPM				GENMASK(17, 16)
#endif

/* Defines the source decimator for 1st stereo/mono data placeholder.
 * If the number of sources for IPM is lower this is don't care.
 * 00 = Decimator 0
 * 01 = Decimator 1
 * 10 = Decimator 2
 * 11 = Decimator 3
 */
#define OUTCONTROL_IPM_SOURCE_1			GENMASK(14, 13)

/* Defines the source decimator for 2nd stereo/mono data placeholder.
 * If the number of sources for IPM is lower this is don't care
 */
#define OUTCONTROL_IPM_SOURCE_2			GENMASK(12, 11)

/* Defines the source decimator for 3rd stereo/mono data placeholder.
 * If the number of sources for IPM is lower this is don't care
 */
#define OUTCONTROL_IPM_SOURCE_3			GENMASK(10, 9)

/* Defines the source decimator for 4th stereo/mono data placeholder.
 * If the number of sources for IPM is lower this is don't care
 */
#define OUTCONTROL_IPM_SOURCE_4			GENMASK(8, 7)

/* Defines the mode of operation for all source decimator. The number of channel of microphone
 * handled by packer can be derived from (IPM * (IPM_SOURCE_MODE+1)).
 * 0 = Mono mode
 * 1 = Stereo mode
 */
#define OUTCONTROL_IPM_SOURCE_MODE		BIT(6)

/* FIFO Trigger Threshold (TH)
 *
 * FIFO Trigger Threshold for Wake/Frame ready in sample units. When FIFO contains more than
 * TH samples, the trigger signal to local power sequencer is asserted.
 */
#define OUTCONTROL_TH				GENMASK(5, 0)


/* OUTSTAT0 and OUTSTAT1 bits */

/* Asynchronous FIFO is empty (AFE): This bit will be set when Asynchronous FIFOs have no data. */
#define OUTSTAT_AFE				BIT(31)

/* Asynchronous FIFO Not Empty (ASNE)
 *
 * 0: Asynchronous FIFO is empty.
 * 1: Asynchronous FIFO is not empty.
 */
#define OUTSTAT_ASNE				BIT(29)

/* FIFO Service Request (RFS):
 *
 * 0: FIFO level is at or below the FIFO threshold (TH+1), or the link disabled.
 * 1: FIFO level exceeds the FIFO threshold (TH+1), request interrupt.
 */
#define OUTSTAT_RFS				BIT(28)

/* Overrun (ROR):
 *
 * 0: PDM has not experienced an overrun.
 * 1: Attempted data write to full FIFO or PDM overrun occurred.
 */
#define OUTSTAT_ROR				BIT(27)

 /* FIFO Level (FL): Current FIFO Level in the Asynchronous FIFO. */
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define OUTSTAT_FL_MASK				GENMASK(8, 0)
#else
#define OUTSTAT_FL_MASK				GENMASK(6, 0)
#endif


/* CIC_CONTROL bits */

/* When set, this bit keeps the microphone interface in the reset state. Internally, it is combined
 * with the global hardware reset. The software needs to clear this bit to enable filters. The soft
 * reset does not clear coefficient tables.
 */
#define CIC_CONTROL_SOFT_RESET			BIT(16)

/* When set to 1, the CIC channel B (right) is started, otherwise it is muted and idle.
 * Note: When PERIODIC_START_EN is set, CIC channel B will be started at the global periodic tick
 * that is common to all PDM DMIC interface.
 */
#define CIC_CONTROL_CIC_START_B			BIT(15)

/* When set to 1, the CIC channel A (left) is started, otherwise it is muted and idle.
 * Note: When PERIODIC_START_EN is set, CIC channel A will be started at the global periodic tick
 * that is common to all PDM DMIC interface.
 */
#define CIC_CONTROL_CIC_START_A			BIT(14)

/* Polarity of the microphone output. When set, positive pressure change will cause positive PCM
 * output.
 */
#define CIC_CONTROL_MIC_B_POLARITY		BIT(3)

/* Polarity of the microphone output. When set, positive pressure change will cause positive PCM
 * output.
 */
#define CIC_CONTROL_MIC_A_POLARITY		BIT(2)

/* Mute currently active microphones; the interface will produce zero output when this bit is set.
 * The default on power up is mute.
 */
#define CIC_CONTROL_MIC_MUTE			BIT(1)

#ifndef CONFIG_SOC_SERIES_INTEL_ACE
/* When set, the microphone input operates in the stereo mode (two channels are sampled), when
 * cleared, only one (left) channel is sampled.
 */
#define CIC_CONTROL_STEREO_MODE			BIT(0)
#endif


/* CIC_CONFIG masks */

/* Number of bits for shift right in the output stage of the CIC filter to compensate the gain
 * accumulated due to decimation. Needs to be set to make overall gain close to 0dB.
 * 0 = Shift left 8 places
 * 1 = Shift left 7 places
 * 3 = Shift left 6 places
 * 4 = Shift left 5 places
 * 5 = Shift left 4 places
 * 6 = Shift left 3 places
 * 7 = Shift left 2 places
 * 8 = Shift left 1 places
 * 9 = No shift, direct input
 * A = Shift right 1 places
 * B = Shift right 2 places
 * C = Shift right 3 places
 * D = Shift right 4 places
 * E = Shift right 5 places
 * F = not allowed
 */
#define CIC_CONFIG_CIC_SHIFT			GENMASK(27, 24)

/* Period of activation of comb section in the microphone clocks minus 1 - i.e.,
 * CIC decimation rate.
 */
#define CIC_CONFIG_COMB_COUNT			GENMASK(15, 8)


/* MIC_CONTROL */

/* Clock divider used for producing the microphone clock from audio IO clock with approximately 50%
 * duty cycle.
 *
 * The microphone clock period is 2+PDM_CLKDIV, for example, frequency is:
 * 00 = 1/2 IOCLK
 * 01 = 1/3 IOCLK
 * 02 = 1/4 IOCLK
 * 03 = 1/5 IOCLK
 * 04 = 1/6 IOCLK
 * 05 = 1/7 IOCLK
 * 06 = 1/8 IOCLK
 * ...
 * 0xFE = 1/256 IOCLK
 * 0xFF = Invalid programming
 */
#define MIC_CONTROL_PDM_CLKDIV			GENMASK(15, 8)

#ifndef CONFIG_SOC_SERIES_INTEL_ACE
/* Selects the delay of the clocks output for microphones to align the sampling point of the data
 * and clock edge. The increments of the delay are in units of half-period of audio I/O clock
 * (19.2/24/24.576 MHz). This control is common for both microphones.
 */
#define MIC_CONTROL_PDM_SKEW			GENMASK(7, 4)
#endif
/* Inverts the clock edge that will be used to sample microphone data stream. Switching this bit
 * effectively reverses the left and right channels.
 */
#define MIC_CONTROL_CLK_EDGE			BIT(3)

#ifdef CONFIG_SOC_SERIES_INTEL_ACE
/* Indicates the PDM DMIC clock for the decimator will be sourced from external component instead
 * of using the PDM DMIC clock generator output.
 *
 * Note that the PDM DMIC clock output generator may be used in conjunction with the slave mode
 * operation, where the SoC may loopback one of the PDM DMIC IP clock output to become the slaves
 * of multiple PDM DMIC IP slave clocks (including the one that generate the PDM DMIC IP output).
 */
#define MIC_CONTROL_SLAVE_MODE			BIT(2)
#endif

/* Enable clock on microphone B (Right)
 *
 * Note: When PERIODIC_START_EN is set, clock to microphone B will be started at the global periodic
 * tick that is common to all PDM DMIC interface.
 */
#define MIC_CONTROL_PDM_EN_B			BIT(1)

/* Enable clock on microphone A (left)
 *
 * Note: When PERIODIC_START_EN is set, clock to microphone A will be started at the global periodic
 * tick that is common to all PDM DMIC interface.
 */
#define MIC_CONTROL_PDM_EN_A			BIT(0)


/* FIR_CONTROL_A bits */

#ifdef CONFIG_SOC_SERIES_INTEL_ACE
/* Register is used to enable the power gating capability of the coefficient RF EBB instance/s. */
#define FIR_CONTROL_CRFPGE			BIT(28)

/* Register is used to enable the power gating capability of the left channel Data RF EBB
 * instance/s.
 */
#define FIR_CONTROL_LDRFPGE			BIT(29)

/* Register is used to enable the power gating capability of the right channel Data RF EBB
 * instance/s. SW shall clear this bit if STEREO=1.
 */
#define FIR_CONTROL_RDRFPGE			BIT(30)
#endif

/* When set, the FIR decimation filter is started. */
#define FIR_CONTROL_START			BIT(7)

/* Array microphone control bit needed for synchronous start of multiple interfaces.
 * When set, this bit controls whether the filter start is driven by START bit directly, or by the
 * global master start signal. The global master start is obtained from the main (MIC0) microphone
 * interface. This bit is present in slave microphone interfaces only, and can be overridden by the
 * START bit in the same slave interface.
 */
#define FIR_CONTROL_ARRAY_START_EN		BIT(6)

#ifdef CONFIG_SOC_SERIES_INTEL_ACE
/* This the array mode definition bit needed for periodic synchronous start control of multiple PDM
 * decimators. When set the START bit, CIC_CONTROL.CIC_START_x bits, and MIC_CONTROL.PDM_EN_x bits
 * will only take effect on the global periodic tick that is common to all PDM DMIC interfaces,
 * ensuring the alignment of the PDM clock and FIR PCM output cadence
 * w.r.t. other PDM DMIC interface.
 */
#define FIR_CONTROL_PERIODIC_START_EN		BIT(5)
#endif

/* Automatic DC compensation enable; when set, the high pass filter with cutoff of 6db/octave is
 * implemented to remove DC at the output.
 *
 * The cutoff frequency is controlled through DC offset register which is repurposed in this mode.
 */
#define FIR_CONTROL_DCCOMP			BIT(4)

/* When set, any write in the coefficient memory will mute the output for the N audio clocks,
 * where N is the selected length of the filter.
 *
 * The mode is useful when software changes decimation factor or filter settings on the fly.
 * { note: not implemented}
 */
#define FIR_CONTROL_AUTO_MUTE			BIT(2)

/* When set, the outputs of this filter are muted and set to zero. Write 0 to enable filter */
#define FIR_CONTROL_MUTE			BIT(1)

/* When set, this filter operates in stereo mode, otherwise only left channel is processed and the
 * mode is mono.
 *
 * Note that CLK_EDGE field in CIC_CONFIG allows selecting which microphone will be used in mono
 * mode.
 */
#define FIR_CONTROL_STEREO			BIT(0)


 /* FIR_CONFIG bits */

/* Decimation factor of the FIR filter minus 1.
 *
 * The decimation factor must be taken from the configuration table and depends on the desired
 * output sample rate.
 */
#define FIR_CONFIG_FIR_DECIMATION		GENMASK(20, 16)

/* Number of bits for shift right in the output stage of the CIC filter to compensate the gain
 * accumulated due to decimation. Needs to be set to make overall gain close to 0dB.
 * 0 = No Shift
 * 1 = Shift right 1 places
 * 2 = Shift right 2 places
 * 3 = Shift right 3 places
 * 4 = Shift right 4 places
 * 5 = Shift right 5 places
 * 6 = Shift right 6 places
 * 7 = Shift right 7 places
 * 8..12 = not allowed; no shift
 * 13 = Shift left 3 places
 * 14 = Shift left 2 places
 * 15 = Shift left 1 places
 */
#define FIR_CONFIG_FIR_SHIFT			GENMASK(11, 8)

/* The number of active taps of the FIR filter minus 1.
 * The number written here must match the number of coefficients written into the coefficient
 * memory.
 */
#define FIR_CONFIG_FIR_LENGTH			GENMASK(7, 0)


/* DC_OFFSET_LEFT and DC_OFFSET_RIGHT */

/* The value written into this register is treated as signed 2's complement binary value and added
 * to the output of the FIR filter.
 * Its width is FIR_DATA_PRECISION (currently 22 bit signed)
 */
#define DC_OFFSET_DC_OFFS			GENMASK(21, 0)


/* OUT_GAIN_LEFT and OUT_GAIN_RIGHT */

/* The value written into this register is treated as signed 2's complement binary value in Q1.19
 * format, and added to the output of the FIR filter.
 * Its width is FIR_COEFF_PRECISION  (currently 20 bit signed)
 */
#define OUT_GAIN				GENMASK(19, 0)

/* FIR coefficients */
#define FIR_COEF				GENMASK(19, 0)


/* GLOBAL_CAPABILITIES */

/* PCM XCLK FIFO Depth (PCMXCFFD): Indicates the number of data entries supported in the PCM XCLK
 * FIFO per FIR output. The DMA burst FIFO threshold cannot be more than the depth specified here.
 *
 * 0-based value.
 */
#define GLOBAL_CAP_PCM_XCLK_FIFO_DEPTH		GENMASK(5, 0)

/* Port Count (PC): Indicates the number of stereo DMIC pair supported by the IP. Each PDM pair
 * registers only exist per the stereo port count specified here.
 *
 * 0-based value. 0: single, 1: two, 2: three, 3: quad
 */
#define GLOBAL_CAP_PORT_COUNT			GENMASK(7, 6)

/* FIR Count (FIRC): Indicates the number of FIR supported in each stereo DMIC pair. Each FIR
 * registers only exist per the FIR count specified here.
 *
 * 0-based value. 0 - one FIR, 1 - two firs
 */
#define GLOBAL_CAP_FIR_COUNT			BIT(8)

/* FIR max gain configuration.
 *
 * 0: FIR gain defined in FIR_CONFIG_x.FIR_SHIFT is only able to shift left up to 3 places.
 * 1: FIR gain defined in FIR_CONFIG_x.FIR_SHIFT is able to shift left up to 7 places.
 */
#define GLOBAL_CAP_FIR_MAX_GAIN			BIT(9)

/* FIR A RF Depth (FIRARFD): FIR A coefficient / data RF depth.
 *
 * 0-based value.  The number of FIR tap configured cannot be more than the depth specified here.
 */
#define GLOBAL_CAP_FIR_A_RF_DEPTH		GENMASK(23, 16)

/* FIR B RF Depth (FIRBRFD): FIR B coefficient / data RF depth.
 *
 * 0-based value. Valid only if FIRC > 0. The number of FIR tap configured cannot be more than the
 * depth specified here.
 */
#define GLOBAL_CAP_FIR_B_RF_DEPTH		GENMASK(31, 24)


/* Digital Mic Shim Registers */
#ifdef CONFIG_SOC_INTEL_ACE20_LNL
#include "dmic_regs_ace2x.h"
#else /* All other CAVS and ACE platforms */
/* DMIC Link Control
 *
 * This register controls the specific link.
 */
#define DMICLCTL_OFFSET		0x04

/* Set Power Active (SPA)
 * Software sets this bit to '1' to turn the link on (provided CRSTB = 1), and clears it to '0' when
 * it wishes to turn the link off. When CPA matches the value of this bit, the achieved power state
 * has been reached. Software is expected to wait for CPA to match SPA before it can program SPA
 * again. Any deviation may result in undefined behavior.
 */
#define DMICLCTL_SPA				BIT(0)

/*
 * Current Power Active (CPA)
 * This value changes to the value set by SPA when the power of the link has reached that state.
 * Software sets SPA, then monitors CPA to know when the link has changed state.
 */
#define DMICLCTL_CPA				BIT(8)

#ifdef CONFIG_SOC_SERIES_INTEL_ACE
/* Owner Select
 *
 * Programs the owner of the IP & shim registers.
 * This determines the SAI policy to be compared for accepting transactions to the Tensilica Core
 * shim registers.
 * 0x: Host CPU + DSP
 * 10: Host CPU
 * 11: DSP
 */
#define DMICLCTL_OSEL				GENMASK(25, 24)

/* Force Clock Gating
 *
 * When set to '1', it ignores all the HW detect idle conditions and force the clock to be gated
 * assuming it is idle.
 */
#define DMICLCTL_FCG				BIT(26)

/* Master Link Clock Select
 *
 * Select the link master clock source.
 * 000: XTAL Oscillator clock
 * 001: Audio Cardinal clock
 * 010: Audio PLL fixed clock
 * 011: MCLK input
 * 100: WoV RING Oscillator clock
 * 101 - 111: Reserved
 */
#define DMICLCTL_MLCS				GENMASK(29, 27)
#endif	/* CONFIG_SOC_SERIES_INTEL_ACE */

/*
 * Dynamic Clock Gating Disable (DCGD)
 * When cleared to '0', it allows more aggressive dynamic clock gating during the idle windows of IP
 * operation (CPA = 1).
 * When set to '1', the clock gating will be disabled when CPA = 1.
 * HW should treat this bit as 1 if FNCFG.CGD = 1 or FUSVAL.CGD = 1.
 */
#define DMICLCTL_DCGD				BIT(30)

/*
 * Idle Clock Gating Disable (ICGD)
 * When cleared to '0', the clock sources into the IP will be gated when CPA = 0. When set to '1',
 * the clock gating will be disabled even though CPA = 0. Independent of this bit, the link may
 * allow dynamic clock gating when CPA = 1. HW should treat this bit as 1 if FNCFG.CGD = 1 or
 * FUSVAL.CGD = 1.
 */
#define DMICLCTL_ICGD				BIT(31)


#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#include "dmic_regs_ace1x.h"
#endif	/* CONFIG_SOC_SERIES_INTEL_ACE */

#endif	/* !CONFIG_SOC_INTEL_ACE20_LNL */

#endif /* !__INTEL_DAI_DRIVER_DMIC_REGS_H__ */
