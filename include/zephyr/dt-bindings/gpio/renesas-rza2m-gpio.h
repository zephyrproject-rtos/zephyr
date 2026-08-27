/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZA2M_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZA2M_GPIO_H_

/**
 * @brief RZ/A2M specific GPIO Flags
 *
 * The drive flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: Drive strength
 */

/** Normal drive */
#define RZA2M_GPIO_DRIVE_NORMAL (0U << 8U)
/** High drive */
#define RZA2M_GPIO_DRIVE_HIGH   (1U << 8U)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZA2M_GPIO_H_ */
