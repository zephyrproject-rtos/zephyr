/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_PCA_SERIES_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_PCA_SERIES_GPIO_H_

/**
 * @brief PCA-Series GPIO expander specific flags
 *
 * The driver flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8-9: Drive strength for output drive strength register.
 *
 * @ingroup gpio_interface
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_CONFIG_POS \
		8
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_CONFIG_MASK \
		(0x3U << PCA_SERIES_GPIO_DRIVE_STRENGTH_CONFIG_POS)

#define PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS \
		10
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_MASK \
		(0x1U << PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS)
/** @endcond */

/**
 * @name PCA-Series GPIO drive strength flags
 * @brief Macros for configuring output drive strength, aligned
 *        with enum gpio_pca_series_drive_strength in gpio_pca_series.c.
 *        PCA_SERIES_GPIO_DRIVE_STRENGTH_X1 is the lowest strength,
 *        PCA_SERIES_GPIO_DRIVE_STRENGTH_X4 is the highest strength.
 *        Upon reset, the default drive strength is PCA_SERIES_GPIO_DRIVE_STRENGTH_X4.
 * @note Only applies to part no with PCA_HAS_LATCH capability.
 * @{
 */

/** Interrupt routed to the WKPU controller */
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_X1 \
	((0x0U << PCA_SERIES_GPIO_DRIVE_STRENGTH_POS) | \
	 (0x1U << PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS))
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_X2 \
	((0x1U << PCA_SERIES_GPIO_DRIVE_STRENGTH_POS) | \
	 (0x1U << PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS))
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_X3 \
	((0x2U << PCA_SERIES_GPIO_DRIVE_STRENGTH_POS) | \
	 (0x1U << PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS))
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_X4 \
	((0x3U << PCA_SERIES_GPIO_DRIVE_STRENGTH_POS) | \
	 (0x1U << PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS))

/** Default drive strength on device reset */
#define PCA_SERIES_GPIO_DRIVE_STRENGTH_DEFAULT PCA_SERIES_GPIO_DRIVE_STRENGTH_X4

#define PCA_SERIES_GPIO_DRIVE_STRENGTH_CONFIG(x) \
	((x & PCA_SERIES_GPIO_DRIVE_STRENGTH_CONFIG_MASK) >> \
	PCA_SERIES_GPIO_DRIVE_STRENGTH_CONFIG_POS)

#define PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE(x) \
	((x & PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_MASK) >> \
	PCA_SERIES_GPIO_DRIVE_STRENGTH_ENABLE_POS)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_PCA_SERIES_GPIO_H_ */
