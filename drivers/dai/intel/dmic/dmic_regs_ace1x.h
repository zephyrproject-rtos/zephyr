/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__

/* Digital Microphone Synchronization */
#define DMICSYNC_OFFSET				0x0C

/* DMIC Sync Period */
#define DMICSYNC_SYNCPRD			GENMASK(14, 0)

/* Sync Period Update */
#define DMICSYNC_SYNCPU				BIT(15)

/* Command Sync */
#define DMICSYNC_CMDSYNC			BIT(16)

/* Sync Go */
#define DMICSYNC_SYNCGO				BIT(24)

/* Extended Sync Period */
#define DMICSYNC_ESYNCPRD			BIT(25)

#endif /* ! __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__ */
