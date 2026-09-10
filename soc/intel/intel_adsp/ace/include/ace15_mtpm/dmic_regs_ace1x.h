/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__

/* Digital Mic Shim Registers */

/* Digital Microphone Link Capability */
#define DMICLCAP_OFFSET				0x00

/* Link Count */
#define DMICLCAP_LCOUNT				GENMASK(2, 0)

/* Cross Link Type Sync Supported */
#define DMICLCAP_CLTSS				BIT(5)

/* Link Synchronization Supported */
#define DMICLCAP_LSS				BIT(6)

/* Stream Channel Mapping Supported */
#define DMICLCAP_SCMS				BIT(7)

/* Master Link Clock Select */
#define DMICLCAP_MLCS				BIT(8)

/* PREQ/WakeUp */
#define DMICLCAP_PW				BIT(26)

/* Owner Select */
#define DMICLCAP_OSEL				BIT(27)

/* Power Gating Domain */
#define DMICLCAP_PGD				GENMASK(30, 28)


/* Digital Microphone IP Pointer */
#define DMICIPPTR_OFFSET			0x08

/* IP Pointer */
#define DMICIPPTR_PTR				GENMASK(20, 0)

/* IP Version */
#define DMICIPPTR_VER				GENMASK(23, 21)


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


/* Digital Microphone PCM Stream Capabilities */
#define DMICPCMSCAP_OFFSET			0x10

/* Number of Input Streams Supported */
#define DMICPCMSCAP_ISS				GENMASK(3, 0)

/* Number of Output Streams Supported */
#define DMICPCMSCAP_OSS				GENMASK(7, 4)

/* Number of Bidirectional Streams Supported */
#define DMICPCMSCAP_BSS				GENMASK(12, 8)


/* Digital Microphone PCM Stream y Channel Map
 *
 * Offset: 12h + 02h * y
 */
#define DMICPCMSyCM_OFFSET			0x12
#define DMICPCMSyCM_SIZE			0x02

/* Lowest Channel */
#define DMICPCMSyCM_LCHAN			GENMASK(3, 0)

/* Highest Channel */
#define DMICPCMSyCM_HCHAN			GENMASK(7, 4)

/* Stream */
#define DMICPCMSyCM_STRM			GENMASK(13, 8)


/* Digital Microphone PCM Stream y Channel Count
 *
 * Offset: 18h + 02h * y
 */
#define DMICPCMSyCHC_OFFSET			0x18
#define DMICPCMSyCHC_SIZE			0x02

/* Number of Channel Supported */
#define DMICPCMSyCHC_CS				GENMASK(3, 0)


/* Digital Microphone Port y PDM SoundWire Map
 *
 * Offset: 20h + y * 02h
 */
#define DMICPyPDMSM_OFFSET			0x20
#define DMICPyPDMSM_SIZE			0x02

/* Left Channel SoundWire Bus Segment */
#define DMICPyPDMSM_LCSBS			GENMASK(1, 0)

/* Right Channel SoundWire Bus Segment */
#define DMICPyPDMSM_RCSBS			GENMASK(3, 2)

/* SoundWire Select */
#define DMICPyPDMSM_SNDWSEL			BIT(4)

/* Stereo */
#define DMICPyPDMSM_STR				BIT(5)

/* DMIC Link Control
 *
 * This register controls the specific link.
 */
#define DMICLCTL_OFFSET		0x04

/* Set Power Active */
#define DMICLCTL_SPA				BIT(0)

/* Current Power Active */
#define DMICLCTL_CPA				BIT(8)

/* Owner Select */
#define DMICLCTL_OSEL				GENMASK(25, 24)

/* Force Clock Gating */
#define DMICLCTL_FCG				BIT(26)

/* Master Link Clock Select */
#define DMICLCTL_MLCS				GENMASK(29, 27)

/* Dynamic Clock Gating Disable */
#define DMICLCTL_DCGD				BIT(30)

/* Idle Clock Gating Disable */
#define DMICLCTL_ICGD				BIT(31)

#endif /* ! __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__ */
