/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__
#define __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__

/* Digital Mic Shim Registers
 *
 * These registers are added by Intel outside of the digital microphone IP for general operation
 * control, such as capability declaration, link control, and stream mapping.
 * Note: These registers are accessible through the host space or DSP space depending on ownership,
 * as governed by SAI and RS.
 */


/* Digital Microphone Link Capability
 *
 * This register identifies the direct attached digital microphone interface associated capabilities
 */
#define DMICLCAP_OFFSET				0x00

/* Link Count
 *
 * Indicates the number of DMIC IP instances managed by this shim.
 */
#define DMICLCAP_LCOUNT				GENMASK(2, 0)

/* Cross Link Type Sync Supported
 *
 * Indicates whether the cross link type synchronization is supported or not.
 */
#define DMICLCAP_CLTSS				BIT(5)

/* Link Synchronization Supported
 *
 * Indicates whether the multiple links synchronization is supported or not.
 */
#define DMICLCAP_LSS				BIT(6)

/* Stream Channel Mapping Supported
 *
 * Indicates whether the PCM stream channel mapping flexibility exist for the link or not.
 * 0: The FIFO ports are located as part of the link IP register block, and not connected to Audio
 *    Link Hub.
 * 1: The FIFO ports are separate from the link IP register block, and connected to Audio Link Hub.
 * For cAVS 1.8 and cAVS 2.0, only support FIFO ports as part of link IP register block just like
 * cAVS 1.5. It is a stretch goal to provide connection through the Audio Link Hub.
 */
#define DMICLCAP_SCMS				BIT(7)

/* Master Link Clock Select
 *
 * Indicates the IP capability to select clock source other than XTAL Oscillator clock.
 */
#define DMICLCAP_MLCS				BIT(8)

/* PREQ/WakeUp
 *
 * Indicates the IP capability to generate PREQ/WakeUp or not.
 */
#define DMICLCAP_PW				BIT(26)

/* Owner Select
 *
 * Indicates the ownershp concept is supported or not.
 */
#define DMICLCAP_OSEL				BIT(27)

/* Power Gating Domain
 *
 * Indicates the IP & shim are located in gated-HUB-ULP or gated-IOx domain.
 * 000: gated-HUB-ULP
 * 001 - 011: reserved
 * 100: gated-IO0
 * 101: gated-IO1
 * 110: gated-IO2
 * 111: gated-IO3
 */
#define DMICLCAP_PGD				GENMASK(30, 28)


/* Digital Microphone IP Pointer
 *
 * This register provides the pointer to the direct attached digital microphone IP registers.
 */
#define DMICIPPTR_OFFSET			0x08

/* IP Pointer
 *
 * This field contains the offset to the IP.
 */
#define DMICIPPTR_PTR				GENMASK(20, 0)

/* IP Version
 *
 * This field indicates the version of the IP.
 */
#define DMICIPPTR_VER				GENMASK(23, 21)


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


/* Digital Microphone PCM Stream Capabilities
 *
 * This register identifies the direct attached digital microphone PCM stream capabilities.
 */
#define DMICPCMSCAP_OFFSET			0x10

/* Number of Input Streams Supported
 *
 * Indicates the number of input PCM streams that the link supports.
 */
#define DMICPCMSCAP_ISS				GENMASK(3, 0)

/* Number of Output Streams Supported
 *
 * Indicates the number of output PCM streams that the link supports.
 * No support of output PCM streams for this IP.
 */
#define DMICPCMSCAP_OSS				GENMASK(7, 4)

/* Number of Bidirectional Streams Supported
 *
 * Indicates the number of bidirectional PCM streams that the link supports.
 * No support of bidirectional PCM streams for this IP.
 */
#define DMICPCMSCAP_BSS				GENMASK(12, 8)


/*
 * Digital Microphone PCM Stream y Channel Map
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
#define DMICPCMSyCM_OFFSET			0x12
#define DMICPCMSyCM_SIZE			0x02

/* Lowest Channel
 *
 * An integer representing the lowest channel used by the FIFO port. The FIFO port will use all
 * channels between LCHAN and HCHAN (inclusive), for its data input or output.  For mono channel
 * operation, program LCHAN = HCHAN.
 */
#define DMICPCMSyCM_LCHAN			GENMASK(3, 0)

/* Highest Channel
 *
 * An integer representing the highest channel used by the FIFO port.  The FIFO port will use all
 * channels between LCHAN and HCHAN (inclusive), for its data input or output.  For mono channel
 * operation, program LCHAN = HCHAN.
 */
#define DMICPCMSyCM_HCHAN			GENMASK(7, 4)

/* Stream
 *
 * An integer representing the link stream used by the FIFO port for data input or output. 00h is
 * stream 0, 01h is stream 1, etc. Although the link is capable of transmitting any stream number,
 * by convention stream 0 is reserved as unused so that FIFO port whose stream numbers have been
 * reset to 0 do not unintentionally decode data not intended for them.
 */
#define DMICPCMSyCM_STRM			GENMASK(13, 8)


/* Digital Microphone PCM Stream y Channel Count
 *
 * Offset: 18h + 02h * y
 *
 * This register identifies the direct attached digital microphone individual PCM FIFO channel count
 * supports.
 */
#define DMICPCMSyCHC_OFFSET			0x18
#define DMICPCMSyCHC_SIZE			0x02

/* Number of Channel Supported
 *
 * Indicates the number of channel supported per PCM stream on this link. 0-based value.
 */
#define DMICPCMSyCHC_CS				GENMASK(3, 0)


/* Digital Microphone Port y PDM SoundWire Map
 *
 * Offset: 20h + y * 02h
 *
 * This register controls the direct attached digital microphone individual PDM FIFO port mapping to
 * SoundWire.
 * When SNDWSEL = 0, the PDM stream is routed from the I/O pin directly. When SNDWSEL = 1, the PDM
 * stream is routed from the corresponding SoundWire IP bus segments as indicated by LCSBS and RCSBS
 * fields.
 */
#define DMICPyPDMSM_OFFSET			0x20
#define DMICPyPDMSM_SIZE			0x02

/* Left Channel SoundWire Bus Segment
 *
 * An integer representing the SoundWire bus segment to be connected to the left channel FIFO port.
 * Locked to appear as RO per allocation programmed in HfSNDWA.LC register fields.
 */
#define DMICPyPDMSM_LCSBS			GENMASK(1, 0)

/* Right Channel SoundWire Bus Segment
 *
 * An integer representing the SoundWire bus segment to be connected to the right channel FIFO port.
 * Locked to appear as RO per allocation programmed in HfSNDWA.LC register fields.
 */
#define DMICPyPDMSM_RCSBS			GENMASK(3, 2)

/* SoundWire Select
 *
 * When set to 1, it indicates that the connection of the PDM input should be routed from SoundWire.
 * When clear to 0, it indicates that the connection of the PDM input should be routed from I/O pad.
 * Locked to appear as RO per allocation programmed in HfSNDWA.LC register fields.
 */
#define DMICPyPDMSM_SNDWSEL			BIT(4)

/* Stereo
 *
 * When set to 1, it indicates both left and right channels are enabled for stereo operation. When
 * clear to 0, it indicates only left channel is enabled for mono operation.
 * Locked to appear as RO per allocation programmed in HfSNDWA.LC register fields.
 */
#define DMICPyPDMSM_STR				BIT(5)

#endif /* ! __INTEL_DAI_DRIVER_DMIC_REGS_ACE1X_H__ */
