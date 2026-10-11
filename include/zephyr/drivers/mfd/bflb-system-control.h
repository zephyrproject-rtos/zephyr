/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 MASSDRIVER EI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common header for Bouffalo Lab system controls complexes (GLB, AON, HBN, PDS)
 *
 * Provides shared flags, constants, functions... for low power modes and various subsystems
 * interacting with the system controls: WUC, Reset Controller, Reboot...
 */

/** Expected first power-up value for HBN scratch register 0 (HBN_RSV0)
 * The HBN scratch registers persist down to HBN0.
 * These registers are all used in some variable fashion by the bootrom, avoid using them.
 */
#define HBN_SCRATCH0_INIT	0x0U
/** Expected first power-up value for HBN scratch register 1 (HBN_RSV1) */
#define HBN_SCRATCH1_INIT	0xffffffffU
/** Expected first power-up value for HBN scratch register 2 (HBN_RSV2) */
#define HBN_SCRATCH2_INIT	0x0U
/** Expected first power-up value for HBN scratch register 3 (HBN_RSV3) */
#define HBN_SCRATCH3_INIT	0xffffffffU

/** Expected first power-up value for AON scratch register 0 (AON_HBNCORE_RESV0)
 * The AON scratch registers persist down to HBN2.
 * These registers are not used anywhere.
 * AON_SCRATCH0 is used to store persistent status flags for zephyr.
 * They must be written 4 bytes at once.
 */
#define AON_SCRATCH0_INIT	0x0U
/** Expected first power-up value for AON scratch register 1 (AON_HBNCORE_RESV1) */
#define AON_SCRATCH1_INIT	0xffffffffU

/** HBN_SCRATCH0 Flag value for bootrom to jump to the address at HBN_SCRATCH1,
 * or to HBN RAM word 3 in the case of BL616CL and QCC74x. For these the first 2 HBN RAM words
 * must be 0x4e42484d.
 */
#define HBN_SCRATCH0_ENTER_FLAG		0x4e424845
/** Flag value the bootrom sets when it has found the previous flag and done the jump */
#define HBN_SCRATCH0_WAKEUP_FLAG	0x4e424857

/** AON scratch0 flag value indicating not a Power On Reset */
#define AON_SCRATCH0_FLAG_NPOR		BIT(0)
/** AON scratch0 flag value indicating an intentional software reboot */
#define AON_SCRATCH0_FLAG_SW_REBOOT	BIT(1)
