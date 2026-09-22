/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA Clock Generator Circuit (CGC) definitions for Zephyr.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RA_CLOCK_H_

/**
 * @name Renesas RA Module Stop Control Register (MSTP) definitions
 * @{
 */
#define MSTPA 0 /**< Module stop control register A */
#define MSTPB 1 /**< Module stop control register B */
#define MSTPC 2 /**< Module stop control register C */
#define MSTPD 3 /**< Module stop control register D */
#define MSTPE 4 /**< Module stop control register E */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RA_CLOCK_H_ */
