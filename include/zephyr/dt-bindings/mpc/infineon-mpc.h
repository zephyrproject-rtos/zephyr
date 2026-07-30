/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MPC_INFINEON_MPC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MPC_INFINEON_MPC_H_

/*
 * Field values for the "pc-configs" property of an "infineon,mpc" region,
 * which is a flat list of <protection-context secure access> triples.
 */

/* "secure" field: the region's security attribute for the protection context. */
#define INFINEON_MPC_SECURE     0
#define INFINEON_MPC_NON_SECURE 1

/* "access" field: the region's access permission for the protection context. */
#define INFINEON_MPC_ACCESS_NONE 0
#define INFINEON_MPC_ACCESS_R    1
#define INFINEON_MPC_ACCESS_W    2
#define INFINEON_MPC_ACCESS_RW   3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MPC_INFINEON_MPC_H_ */
