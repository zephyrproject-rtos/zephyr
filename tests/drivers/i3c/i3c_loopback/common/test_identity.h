/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared I3C identity for the i3c_loopback test pair.
 *
 * The target board (B) configures its DW I3C SLV_MIPI_ID_VALUE /
 * SLV_PID_VALUE registers via i3c_configure(I3C_CONFIG_TARGET, ...).
 * The controller board (A) attaches a static i3c_device_desc with the
 * same .pid so ENTDAA matches and the descriptor receives a dynamic
 * address.
 *
 * NOTE — DW IP write quirk
 * ------------------------
 * `dw_i3c_configure(I3C_CONFIG_TARGET)` programs the on-bus PID as:
 *
 *   SLV_MIPI_ID_VALUE  = (uint32_t)(target_cfg->pid >> 16)  [masked]
 *   SLV_PID_VALUE      = (uint32_t)(target_cfg->pid & 0xFFFFFFFF)
 *
 * The hardware then transmits the standard 48-bit MIPI Provisioned ID
 * during ENTDAA, which the controller reconstructs as
 *
 *   pid_rx = ((u64)SLV_MIPI_ID_VALUE[15:0]) << 32 | SLV_PID_VALUE
 *
 * Substituting the write expressions gives
 *
 *   pid_rx = ((target_cfg->pid >> 16) & 0xFFFFu) << 32 |
 *            (target_cfg->pid & 0xFFFFFFFFu)
 *
 * For ``pid_rx == target_cfg->pid`` (so the descriptor PID on the
 * controller side matches what's actually on the bus) we need
 *
 *   bits 47:32 of pid  ==  bits 31:16 of pid
 *   bits 63:48 of pid  ==  0
 *
 * The construction below satisfies that constraint by deriving the
 * upper 16 bits of PID from the part-number's upper 16 bits.
 */

#ifndef I3C_LOOPBACK_COMMON_TEST_IDENTITY_H_
#define I3C_LOOPBACK_COMMON_TEST_IDENTITY_H_

#include <stdint.h>

/* Lower 32 bits - the SLV_PID_VALUE register contents. */
#define TARGET_PART_NUMBER 0x12345000U

/*
 * Static I3C address for Board B.  Both endpoints are known at build
 * time, so Board A enumerates Board B via SETDASA (the standard I3C
 * mechanism for targets with a known static address) rather than via
 * ENTDAA.
 *
 * Must not collide with the controller's own primary-controller-da
 * (set in the controller overlay) or with reserved I3C addresses.
 */
#define TARGET_STATIC_ADDR 0x63U

/* BCR / DCR constants used by the target's i3c_configure() call.
 * BCR layout (per MIPI I3C):
 *   bit 0 = Max Data Speed Limit (advertises GETMXDS support)
 *   bit 2 = IBI Payload (may be HW-forced to 1 on some IP variants)
 *   bit 5 = Advanced Capabilities
 *   bit 6 = Device Role (1 = target)
 * 0x27 = Max Data Speed Limit + IBI Payload + Advanced Cap + Target Role.
 */
#define TARGET_BCR 0x27U
#define TARGET_DCR 0xCBU /* generic device class */

/* Self-consistent 48-bit PID - see header comment. */
#define TARGET_PID                                                                                 \
	((((uint64_t)(((TARGET_PART_NUMBER) >> 16) & 0xFFFFu)) << 32) |                            \
	 (uint64_t)(TARGET_PART_NUMBER))

#endif /* I3C_LOOPBACK_COMMON_TEST_IDENTITY_H_ */
