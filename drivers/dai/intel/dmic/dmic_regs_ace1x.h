/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__

/* Digital Microphone Synchronization
 *
 * This register controls the synchronization of the multiple link segments (same link type).
 */
#define DMICSYNC_OFFSET				0x0C

/* DMIC Sync Period
 *
 * ESYNCPRD&SYNCPRD indicates the number of master link clock the sync event should be generated
 * periodically. 0-based value. Minimum sync period should never be programmed to less than 125 Hz.
 * The sync event is 1 master link clock wide per sync period.
 * Value must be programmed before setting SYNCPU bit. Default value of zero (sync period = 1 clock)
 * means the sync event will be asserted every clock, i.e. the sync period counter is disabled.
 * When value is programmed to a non-zero value from a current zero value and set SYNCPU bit, HW
 * enables the sync period counter to operate.
 * E.g. for 19.2 MHz XTAL oscillator clock, 4 KHz sync period, the value to be programmed is 12BFh.
 * Value may be programmed dynamically when sync period counter already running, and request an
 * update by setting the SYNCPU bit.
 */
#define DMICSYNC_SYNCPRD			GENMASK(14, 0)

/* Sync Period Update
 *
 * When set to 1, an update to the sync event generation period is being requested per the value
 * programmed in SYNCPRD field.  HW will clear this bit to 0 when it has successfully updated the
 * new period requested.
 */
#define DMICSYNC_SYNCPU				BIT(15)

/* Command Sync
 *
 * This bit is used to synchronize the command execution of multiple link segments. Software sets
 * this bit to hold off the command from being executed on the intended link segments, until they
 * have been setup with the same command, and lastly write to the SYNCGO bit to simultaneously clear
 * this bit on all link segments.
 * Note that the SYNCGO bit may be located in this register (same link type) or located in CLTSYNC
 * register (cross link type) depending on parameter TTSCLTSE.
 */
#define DMICSYNC_CMDSYNC			BIT(16)

/* Sync Go
 *
 * This bit is used to synchronize the command execution of multiple link segments. Software sets
 * the CMDSYNC bit to hold off the command from being executed on the intended link segments, until
 * they have been setup with the same command, and lastly write to this bit to simultaneously clear
 * the CMDSYNC bit on all link segments.
 */
#define DMICSYNC_SYNCGO				BIT(24)

/* Extended Sync Period
 *
 * This field is to be appended as MSBs for SYNCPRD, to indicates the number of master link clock
 * the sync event should be generated periodically. See SYNCPRD for more details.
 */
#define DMICSYNC_ESYNCPRD			BIT(25)

#endif /* ! __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__ */
