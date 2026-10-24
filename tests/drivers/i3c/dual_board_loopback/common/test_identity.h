/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared I3C identity for the dual_board_loopback test pair.
 *
 * The target board (B) configures its DW I3C SLV_MIPI_ID_VALUE /
 * SLV_PID_VALUE registers via i3c_configure(I3C_CONFIG_TARGET, ...).
 * The controller board (A) attaches a static i3c_device_desc with the
 * same .pid so ENTDAA matches and the descriptor receives a dynamic
 * address.
 *
 * The PID below is deliberately asymmetric: bits 47:32 differ from bits
 * 31:16.  A PID where those halves are equal survives a driver that
 * programs SLV_MIPI_ID_VALUE from the wrong half of the PID, so it cannot
 * tell a correct driver from a broken one.  Keep them different.
 */

#ifndef I3C_DUAL_BOARD_LOOPBACK_COMMON_TEST_IDENTITY_H_
#define I3C_DUAL_BOARD_LOOPBACK_COMMON_TEST_IDENTITY_H_

#include <stdint.h>

/*
 * Provisioned ID fields, per MIPI I3C Basic v1.0 section 5.1.4.1:
 *   [47:33] MIPI manufacturer ID   [32] provisioned ID type selector
 *   [31:16] part ID                [15:12] instance ID   [11:0] vendor defined
 *
 * The manufacturer ID is a test-only value, not a registered MIPI vendor ID.
 */
#define TARGET_MFG_ID     0x02a5U
#define TARGET_PART_ID    0x1234U
#define TARGET_INSTANCE   0x5U
#define TARGET_VENDOR_DEF 0x006U

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
 * BCR layout (per MIPI I3C, see I3C_BCR_* in <zephyr/drivers/i3c.h>):
 *   bit 0    = Max Data Speed Limit (advertises GETMXDS support)
 *   bit 1    = IBI Request Capable
 *   bit 2    = IBI Payload
 *   bit 5    = Advanced Capabilities
 *   bits 7:6 = Device Role, 0b00 for a target
 * 0x27 sets bits 0, 1, 2 and 5 and leaves Device Role at 0.
 */
#define TARGET_BCR 0x27U
#define TARGET_DCR 0xCBU /* generic device class */

/* 48-bit PID, type selector left 0 (vendor-fixed value). */
#define TARGET_PID                                                                                 \
	((((uint64_t)(TARGET_MFG_ID)) << 33) |                                                     \
	 (((uint64_t)(TARGET_PART_ID)) << 16) |                                                    \
	 (((uint64_t)(TARGET_INSTANCE)) << 12) | (uint64_t)(TARGET_VENDOR_DEF))

#endif /* I3C_DUAL_BOARD_LOOPBACK_COMMON_TEST_IDENTITY_H_ */
