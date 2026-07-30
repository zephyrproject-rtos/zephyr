/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_PPC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_PPC_H_

/*
 * "pc-mask" property of an "infineon,ppc" node: a bitmask of the
 * protection contexts (bus masters) permitted to access the opened regions.
 * Each hardware protection context maps to a bus master in the part's security
 * configuration; OR together the contexts a region should be reachable from.
 */
#define INFINEON_PPC_PC(n) (1U << (n))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_PPC_H_ */
