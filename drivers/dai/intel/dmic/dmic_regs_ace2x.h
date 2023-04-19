/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_ACE2X_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_ACE2X_H__

/* DMIC Link Synchronization
 *
 * This register controls the synchronization of the multiple link segments. This register is
 * applicable for link that support host mode only.
 * If LCAP.LSS = 0, this register is Reserved.
 */
#define DMICSYNC_OFFSET		0x1C

/* Sync Period
 *
 * Indicates the number of host link clock the sync event should be generated periodically. 0-based
 * value. Minimum sync period should never be programmed to less than 125 Hz.
 * The sync event is 1 host link clock wide per sync period.
 * Value must be programmed before setting SYNCPU bit. Default value of zero (sync period = 1 clock)
 * means the sync event will be asserted every clock, i.e. the sync period counter is disabled. When
 * value is programmed to a non-zero value from a current zero value and set SYNCPU bit, HW enables
 * the sync period counter to operate.
 * E.g. for 19.2 MHz XTAL oscillator clock, 4 KHz sync period, the value to be programmed is 12BFh.
 * Value may be programmed dynamically when sync period counter already running, and request an
 * update by setting the SYNCPU bit.
 */
#define DMICSYNC_SYNCPRD	GENMASK(19, 0)

/* Sync Period Update
 *
 * When set to 1, an update to the sync event generation period is being requested per the value
 * programmed in SYNCPRD field. HW will clear this bit to 0 when it has successfully updated the new
 * period requested.
 * HW will always make the update on 2nd sync event assertion, i.e. at the end (sampling) of the 2nd
 * sync event of current sync event generation period, after all CMDSYNCs cleared to 0. This is to
 * ensure all command synchronizations (if any) are completed before the update (as the 1st sync
 * event is where the command is triggered for sending out on the bus by the controller, and the 2nd
 * sync event is where the command is synchronously executed by the bus devices). In this case
 * (if command synchronization is needed), SW is required to set CMDSYNC, write SYNCPRD, set SYNCPU,
 * write SYNCGO allowing CMDSYNC to clear.
 */
#define DMICSYNC_SYNCPU		BIT(20)

/* Sync Go
 *
 * This bit is used to synchronize the command execution of multiple link segments. Software sets
 * the CMDSYNC bit to hold off the command from being executed on the intended link segments, until
 * they have been setup with the same command, and lastly write to this bit to simultaneously clear
 * the CMDSYNC bit on all link segments.
 */
#define DMICSYNC_SYNCGO		BIT(23)

/* Command Sync
 *
 * This bit is used to synchronize the command execution of multiple link segments. Software sets
 * this bit to hold off the command from being executed on the intended link segments, until they
 * have been setup with the same command, and lastly write to the SYNCGO bit to simultaneously clear
 * this bit on all link segments.
 * Note that the SYNCGO bit may be located in this register (same link type) or located in CLTSYNC
 * register (cross link type) depending on parameter TTSCLTSE.
 */
#define DMICSYNC_CMDSYNC	BIT(24)


/* DMIC Link Control
 *
 * This register controls the specific link.
 */
#define DMICLCTL_OFFSET		0x04

/* Set Clock Frequency
 *
 * Indicates the frequency that software wishes the link to run at. Implemented only if LCAP.ALT = 0
 */
#define DMICLCTL_SCF		GENMASK(3, 0)

/* Offload Enable
 *
 * When set to 1, it allows the link control to be offloaded to the DSP FW.  The corresponding multi
 * link segment control / status registers (and extended link control registers if exist) will be
 * accessible by DSP FW (in addition to host SW).
 * Implemented only if LCAP.OFLS = 1.
 */
#define DMICLCTL_OFLEN		BIT(4)

/* Interrupt Enable
 *
 * When set to 1, it allows the link to generate interrupt through the controller interrupt tree.
 * Implemented only if LCAP.INTC = 1.
 */
#define DMICLCTL_INTEN		BIT(5)

/* Set Power Active
 *
 * Software sets this bit to 1 to turn the link on (provided CRSTB = 1), and clears it to 0 when
 * it wishes to turn the link off. When CPA matches the value of this bit, the achieved power state
 * has been reached. Software is expected to wait for CPA to match SPA before it can program SPA
 * again. Any deviation may result in undefined behavior.
 */
#define DMICLCTL_SPA		BIT(16)

/* Current Power Active
 *
 * This value changes to the value set by SPA when the power of the link has reached that state.
 * Software sets SPA, then monitors CPA to know when the link has changed state.
 */
#define DMICLCTL_CPA		BIT(23)

/* Interrupt Status
 *
 * When set to 1, it indicates a pending interrupt from the link segment.
 * Implemented only if LCAP.INTC = 1.
 */
#define DMICLCTL_INTSTS		BIT(31)


/* Digital Microphone x Link Vendor Specific Control
 *
 * This register controls the direct attached digital microphone interface vendor specific control /
 * optimization.
 */
#define DMICLVSCTL_OFFSET	0x04

/* Force Clock Gating
 *
 * When set to '1', it ignores all the HW detect idle conditions and force the clock to be gated
 * assuming it is idle.
 */
#define DMICLVSCTL_FCG		BIT(26)

/* Host Link Clock Select
 *
 * Select the host link clock source.
 * 000: XTAL Oscillator clock
 * 001: Audio Cardinal clock
 * 010: Audio PLL fixed clock
 * 011: MCLK input
 * 100: WoV RING Oscillator clock
 * 101 - 111: Reserved
 */
#define DMICLVSCTL_MLCS		GENMASK(29, 27)

/* Dynamic Clock Gating Disable
 *
 * When cleared to '0', it allows more aggressive dynamic clock gating during the idle windows of IP
 * operation (CPA = 1).  When set to '1', the clock gating will be disabled when CPA = 1.
 * HW should treat this bit as 1 if FNCFG.CGD = 1 or FUSVAL.CGD = 1.
 * Implementation Notes: the IP level clock gating disable does not prevent trunk clock gating, if
 * trunk clock gating policy remain enabled (*TCGE = 1).
 */
#define DMICLVSCTL_DCGD		BIT(30)

/* Idle Clock Gating Disable
 *
 * When cleared to '0', the clock sources into the IP will be gated when CPA = 0.  When set to '1',
 * the clock gating will be disabled even though CPA = 0.
 * Independent of this bit, the IP may allow dynamic clock gating when CPA = 1.
 * HW should treat this bit as 1 if FNCFG.CGD = 1 or FUSVAL.CGD = 1.
 * Implementation Notes: the IP level clock gating disable does not prevent trunk clock gating, if
 * trunk clock gating policy remain enabled (*TCGE = 1).
 */
#define DMICLVSCTL_ICGD		BIT(31)


/* Digital Microphone PCM Stream y Channel Map
 *
 * Offset: 12h + 02h * y
 * Block: DMICSx
 *
 * This register controls the direct attached digital microphone individual PCM FIFO port mapping to
 * the streams and channels associated with the DSP link DMA or GPDMA, if the FIFO port of the link
 * IP is connected to the Audio Link Hub (DMICLCAP.SCMS = 1).
 * When mapping to GPDMA, the STRMzRXDA should be used to read from the FIFO port for input stream;
 * with the STRM field indicating the offset z; and the CHAN field will indicate which channel of
 * the sample block the FIFO port should start producing.
 */
#define DMICXPCMSyCM_OFFSET	0x16
#define DMICXPCMSyCM_SIZE	0x02

#endif /* !__INTEL_DAI_DRIVER_DMIC_REGS_ACE2X_H__ */
