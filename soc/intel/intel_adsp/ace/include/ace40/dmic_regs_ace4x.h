/*
 * Copyright (c) 2025 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_ACE4X_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_ACE4X_H__

/* DMIC Link Synchronization */
#define DMICSYNC_OFFSET 0x1C

/* Sync Period */
#define DMICSYNC_SYNCPRD GENMASK(19, 0)

/* Sync Period Update */
#define DMICSYNC_SYNCPU BIT(20)

/* Sync Go */
#define DMICSYNC_SYNCGO BIT(23)

/* Command Sync */
#define DMICSYNC_CMDSYNC BIT(24)

/* DMIC Link Control */
#define DMICLCTL_OFFSET 0x04

/* Set Clock Frequency */
#define DMICLCTL_SCF GENMASK(3, 0)

/* Offload Enable */
#define DMICLCTL_OFLEN BIT(4)

/* Interrupt Enable */
#define DMICLCTL_INTEN BIT(5)

/* Set Power Active */
#define DMICLCTL_SPA BIT(16)

/* Current Power Active */
#define DMICLCTL_CPA BIT(23)

/* Interrupt Status */
#define DMICLCTL_INTSTS BIT(31)

/* Digital Microphone x Link Vendor Specific Control */
#define DMICLVSCTL_OFFSET 0x04

/* Force Clock Gating */
#define DMICLVSCTL_FCG BIT(26)

/* Host Link Clock Select */
#define DMICLVSCTL_MLCS GENMASK(29, 27)

/* Dynamic Clock Gating Disable */
#define DMICLVSCTL_DCGD BIT(30)

/* Idle Clock Gating Disable */
#define DMICLVSCTL_ICGD BIT(31)

/* Digital Microphone PCM Stream y Channel Map
 *
 * Offset: 12h + 02h * y
 */
#define DMICXPCMSyCM_OFFSET 0x16
#define DMICXPCMSyCM_SIZE   0x02

#endif /* !__INTEL_DAI_DRIVER_DMIC_REGS_ACE4X_H__ */
