/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DRIVER_MIC_PRIVACY_REGISTERS_H__
#define __INTEL_DRIVER_MIC_PRIVACY_REGISTERS_H__

#include <stdint.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/sys_io.h>

/**
 * DFMICPVCP
 * Microphone Privacy Policy
 *
 * Offset: 00h
 * Block: DfMICPVC
 *
 * This register controls the microphone privacy DMA data zeroing feature HW policy.
 */
union DFMICPVCP {
	uint32_t full;
	struct {
	/**
	 * DMA Data Zeroing Wait Time
	 * type: RW/L, rst: 0b, rst domain: PLTRST
	 *
	 * Indicates the time-out duration to wait before forcing the actual
	 * microphone privacy DMA data zeroing. Unit in number of RTC clocks.
	 * Valid and static when DDZE = 10. For DDZE = 0x or 11 case, time-out
	 * is not necessary as it will not be enabled or force mic disable statically.
	 * Locked when DDZPL = 1.
	 */
	uint32_t ddzwt : 16;
	/**
	 * DMA Data Zeroing Enable
	 * type: RW/L, rst: 00b, rst domain: PLTRST
	 *
	 * Indicates the policy setting for HW to force the microphone privacy DMA data zeroing.
	 * 0x: Disabled
	 * 10: Enabled (mic disable dynamically depending on privacy signaling input)
	 * 11: Enabled (force mic disable statically)
	 * Locked when DDZPL = 1.
	 */
	uint32_t ddze : 2;
	/**
	 * De-glitcher Enable
	 * type: RW/L, rst: 0b, rst domain: PLTRST
	 *
	 * De-glitcher enable for privacy signaling GPIO input running on resume clock domain.
	 * Locked when DDZPL = 1.
	 */
	uint32_t dge : 1;
	/**
	 * DMA Data Zeroing Policy Lock
	 * type: RW/L, rst: 0b, rst domain: PLTRST
	 *
	 * When set to 1, it locks the privacy DMA data zeroing policy setting.
	 */
	uint32_t ddzpl : 1;

	/**
	 * DMA Data Zeroing Link Select
	 * type: RW/L, rst: 0b, rst domain: PLTRST
	 *
	 * Select 1 or more audio link to apply the microphone privacy DMA data zeroing.
	 * 1 bit per audio link.
	 * [6:0]: SoundWire link segment
	 * [7]: DMIC
	 * Valid and static when DDZE = 1.
	 * Locked when DDZPL = 1.
	 */
	uint32_t ddzls : 8;
	/**
	 * Reserved (Preserved)
	 * type: RO, rst: 000 0000h, rst domain: nan
	 *
	 * SW must preserve the original value when writing.
	 */
	uint32_t rsvd31 : 4;
	} part;
};

/**
 * DFMICPVCS
 * Microphone Privacy Status
 *
 * Offset: 04h
 * Block: DfMICPVC
 *
 * This register reports the microphone privacy DMA data zeroing status.
 */
union DFMICPVCS {
	uint16_t full;
	struct {
		/**
		 * Mic Disabled Indicator Output
		 * type: RO/V, rst: 0b, rst domain: nan
		 *
		 * Indicates the mic disabled status output to GPIO (i.e. the privacy
		 * indicator output).
		 */
		uint16_t mdio : 1;
		/**
		 * Reserved (Zero)
		 * type: RO, rst: 0000h, rst domain: nan
		 *
		 * SW must use zeros for writes.
		 */
		uint16_t rsvd15 : 15;
	} part;
};

/**
 * DFFWMICPVCCS
 * FW Microphone Privacy Control & Status
 *
 * Offset: 06h
 * Block: DfMICPVC
 *
 * This register allows DSP FW to manage the mic privacy operation (if not locked by trusted host).
 */
union DFFWMICPVCCS {
	uint16_t full;
	struct {
	/**
	 * Mic Disable Status Changed Interrupt Enabled
	 * type: RW, rst: 0b, rst domain: DSPLRST
	 *
	 * When set to 1, it allows MDSTSCHG bit to be propagated as mic privacy interrupt
	 *  to the DSP Cores.
	 */
	uint16_t mdstschgie : 1;
	/**
	 * FW Managed Mic Disable
	 * type: RW/L, rst: 0b, rst domain: DSPLRST
	 *
	 * When set to 1, it indicates FW will manage its own time-out, decide which related link
	 * DMA should zero out the data (through DGLISxCS.DDZ), and update the privacy signaling
	 * output (through FMDSTS).
	 * HW will NOT control any of the DMIC / SoundWire link level DMA data zeroing or privacy
	 * signaling output in this case. Locked when DfMICPVCCGP. DDZPL = 1.
	 */
	uint16_t fmmd : 1;
	/**
	 * FW Mic Disable Status
	 * type: RW, rst: 0b, rst domain: DSPLRST
	 *
	 * When set to 1, it indicates FW has quiesced the mic input stream gracefully and
	 * instructs HW to set privacy indicator output (no dependency on privacy
	 * signaling input). Valid if FMMD = 1.
	 */
	uint16_t fmdsts : 1;
	/**
	 * Reserved (Preserved)
	 * type: RO, rst: 00h, rst domain: nan
	 *
	 * SW must preserve the original value when writing.
	 */
	uint16_t rsvd7 : 5;
	/**
	 * Mic Disable Status Changed
	 * type: RW/1C, rst: 0b, rst domain: DSPLRST
	 *
	 * Asserted when mic disable status has changed state (independent of MDSTSCHGIE setting),
	 * and trigger interrupt if enabled.
	 *
	 * Note: If MDSTS change again before the current MDSTSCHG is acknowledged by DSP FW,
	 * the bit will still remain set until cleared by DSP FW.
	 */
	uint16_t mdstschg : 1;
	/**
	 * Mic Disable Status
	 * type: RO/V, rst: 0b, rst domain: nan
	 *
	 * Indicates the live mic disable status input from GPIO (if FMMD = 1).  When asserted and
	 * the microphone privacy DMA data zeroing policy is enabled, FW will manage its
	 * own time-out and decide which related link DMA should zero out the data
	 * (DGLISxCS.DDZ = 1), followed by setting the mic privacy indicator output (FMDSTS = 1).
	 * When de-asserted, FW should remove the DMA data zeroing (DGLISxCS.DDZ = 0) and clear
	 * the privacy indicator output (FMDSTS = 0) as soon as possible.
	 */
	uint16_t mdsts : 1;
	/**
	 * Reserved (Preserved)
	 * type: RO, rst: 00h, rst domain: nan
	 *
	 * SW must preserve the original value when writing.
	 */
	uint16_t rsvd15 : 6;
	} part;
};

/**
 * DMICXPVCCS
 * Digital Microphone x Privacy Control & Status
 *
 * Offset: 10h
 * Block: DMICVSSX_AON
 *
 * This register controls the status reporting structure of the microphone privacy DMA data
 * zeroing feature.
 */
union DMICXPVCCS {
	uint16_t full;
	struct {
		/**
		 * Mic Disable Status Changed Interrupt Enabled
		 * type: RW, rst: 0h, rst domain: FLR
		 *
		 * When set to 1, it allows MDSTSCHG bit to be propagated as DMIC
		 * / SoundWire interrupt to the DSP Cores / host CPU.
		 */
		uint16_t    mdstschgie : 1;
		/**
		 * Reserved
		 * type: RO, rst: 00h, rst domain: N/A
		 */
		uint16_t rsvd7 : 7;
		/**
		 * Mic Disable Status Changed
		 * type: RW/1C, rst: 0h, rst domain: FLR
		 *
		 * Asserted when mic disable status has changed state (independent of MDSTSCHGIE
		 * setting), and trigger interrupt if enabled.
		 * Note: If MDSTS change again before the current MDSTSCHG is acknowledged by
		 * DSP FW / host SW, the bit will still remain set until cleared by
		 * DSP FW / host SW.
		 */
		uint16_t mdstschg: 1;
		/**
		 * Mic Disable Status
		 * type: RO/V, rst: 0h, rst domain: N/A
		 *
		 * Indicates the live mic disable status input from GPIO
		 * (for the selected mic audio link per DFMICPVCP.DDZLS).
		 * When asserted and the microphone privacy DMA data zeroing policy is enabled,
		 * the timer will start counting and force the selected mic data to zero
		 * (after time-out). When de-asserted,
		 * it remove the DMA data zeroing immediately (including stopping the timer
		 * if it has not expired).
		 */
		uint16_t mdsts: 1;
		/**
		 * Force Mic Disable
		 * type: RO/V, rst: 0h, rst domain: N/A
		 *
		 * Indicates the microphone endpoint
		 * (for the selected mic audio link per DFMICPVCP.DDZLS)
		 * is statically force mic disable by trusted agent and SW / FW should hide
		 * the endpoint from being exposed to OS.
		 */
		uint16_t fmdis: 1;
		/**
		 * Reserved
		 * type: RO, rst: 00h, rst domain: N/A
		 */
		uint16_t rsvd15: 5;
	} part;
};
#endif /* __INTEL_DAI_DRIVER_MIC_PRIVACY_REGISTERS_H__ */
