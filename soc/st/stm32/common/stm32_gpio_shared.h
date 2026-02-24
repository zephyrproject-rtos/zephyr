/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_
#define ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_

/*
 * Name of all GPIO ports that may exist (without the `gpio` prefix).
 *
 * This should be the only thing that needs updating when adding support
 * for new GPIO ports if need arises.
 *
 * WARNING: make sure both list are kept in sync when modifying!
 */
#define STM32_GPIO_PORTS_LIST_LWR \
	a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z

#define STM32_GPIO_PORTS_LIST_UPR \
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z

#endif /* ZEPHYR_SOC_ST_STM32_COMMON_STM32_GPIO_SHARED_H_ */
