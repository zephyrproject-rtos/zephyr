/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_ENTROPY_ENTROPY_RTS5817_H
#define DRIVERS_ENTROPY_ENTROPY_RTS5817_H

#include <zephyr/sys/util.h>

#define R_RNG_STATUS     0x10
#define R_RNG_ENABLE     0x20
#define R_RNG_CONFIG     0x24
#define R_RNG_FIFO_CLEAR 0x6c
#define R_RNG_DATA_OUT   0x70

/* R_RNG_STATUS bits */
#define FIFO_CLEARED           BIT(1)
#define RNG_IS_ENABLED         BIT(8)
#define RNG_HEALTH_TEST_ACTIVE BIT(9)
#define GENERATING_RANDOM_DATA BIT(10)
#define RNG_HALTED             BIT(11)

/* R_RNG_ENABLE bits */
#define RNG_FUN_EN BIT(0)
#define RNG_CLK_EN BIT(1)

/* R_RNG_FIFO_CLEAR bits */
#define RNG_HT_CLR BIT(0)

/* R_RNG_CONFIG data path: 0x20000 dumps RNG data, 0x30000 raw entropy */
#define RNG_CONFIG_PATH_RNG 0x20000

/* Timeout for RNG init */
#define RNG_TIMEOUT_US 1000

#endif /* DRIVERS_ENTROPY_ENTROPY_RTS5817_H */
