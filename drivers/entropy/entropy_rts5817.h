/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_ENTROPY_ENTROPY_RTS5817_H
#define DRIVERS_ENTROPY_ENTROPY_RTS5817_H

#define R_RNG_VERSION      0x00
#define R_RNG_FEATURE      0x08
#define R_RNG_STATUS       0x10
#define R_RNG_ENABLE       0x20
#define R_RNG_CONFIG       0x24
#define R_RNG_TEST_CFG0    0x30
#define R_RNG_TEST_CFG1    0x38
#define R_RNG_TEST_CFG2    0x3c
#define R_RNG_TEST_REPORT0 0x40
#define R_RNG_TEST_REPORT1 0x44
#define R_RNG_TEST_REPORT2 0x50
#define R_RNG_TEST_REPORT3 0x54
#define R_RNG_TEST_REPORT4 0x58
#define R_RNG_TEST_REPORT5 0x5c
#define R_RNG_TEST_REPORT6 0x60
#define R_RNG_TEST_REPORT7 0x64
#define R_RNG_FIFO_CLEAR   0x6c
#define R_RNG_DATA_OUT     0x70

/* Bits of R_RNG_STATUS (0x10) */
#define RNG_IDLE_OFFSET 0UL
#define RNG_IDLE        BIT(0)

#define FIFO_CLEARED_OFFSET 1UL
#define FIFO_CLEARED        BIT(1)

#define ENTROPY_AVAILABLE_OFFSET 2UL
#define ENTROPY_AVAILABLE        BIT(2)

#define B_FIFO_AVAILABLE_OFFSET 4UL
#define B_FIFO_AVAILABLE        BIT(4)

#define A_FIFO_AVAILABLE_OFFSET 5UL
#define A_FIFO_AVAILABLE        BIT(5)

#define A_FIFO_FULL_OFFSET 6UL
#define A_FIFO_FULL        BIT(6)

#define RNG_IS_ENABLED_OFFSET 8UL
#define RNG_IS_ENABLED        BIT(8)

#define RNG_HEALTH_TEST_ACTIVE_OFFSET 9UL
#define RNG_HEALTH_TEST_ACTIVE        BIT(9)

#define GENERATING_RANDOM_DATA_OFFSET 10UL
#define GENERATING_RANDOM_DATA        BIT(10)

#define RNG_HALTED_OFFSET 11UL
#define RNG_HALTED        BIT(11)

/* Bits of R_RNG_ENABLE (0x20) */
#define RNG_FUN_EN_OFFSET 0UL
#define RNG_FUN_EN        BIT(0)

#define RNG_CLK_EN_OFFSET 1UL
#define RNG_CLK_EN        BIT(1)

#define RNG_OUT_EN_OFFSET 2UL
#define RNG_OUT_EN        BIT(2)

/* Bits of R_RNG_FIFO_CLEAR (0x6c) */

#define RNG_HT_CLR_OFFSET 0UL
#define RNG_HT_CLR        BIT(0)

/* Value for R_RNG_CONFIG */
#define RNG_CONFIG_VAL 0x20000

/* Timeout for RNG init */
#define RNG_TIMEOUT_US 1000

#endif /* DRIVERS_ENTROPY_ENTROPY_RTS5817_H */
