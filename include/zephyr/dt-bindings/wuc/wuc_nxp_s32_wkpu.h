/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WUC_WUC_NXP_S32_WKPU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WUC_WUC_NXP_S32_WKPU_H_

#define NXP_S32_WKPU_EDGE_RISING  1
#define NXP_S32_WKPU_EDGE_FALLING 2
#define NXP_S32_WKPU_EDGE_BOTH    3

/*
 * Bits [7:0] are the hardware source number; bits [9:8] select rising (1),
 * falling (2), or both (3) edges. Other bits must be zero. Sources 0-3 are
 * internal and support rising edges only. A source number is not a GPIO
 * pin index or CPU interrupt number.
 */
#define NXP_S32_WKPU_SOURCE(source, edge) (((edge) << 8) | (source))
#define NXP_S32_WKPU_SOURCE_INDEX(id) ((id) & 0xff)
#define NXP_S32_WKPU_SOURCE_EDGE(id) (((id) >> 8) & 0x3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WUC_WUC_NXP_S32_WKPU_H_ */
