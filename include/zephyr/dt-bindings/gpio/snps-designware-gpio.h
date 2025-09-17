/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SNPS_DESIGNWARE_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SNPS_DESIGNWARE_GPIO_H_

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for SNPS DesignWare GPIO
 * controllers.
 */
#define DW_GPIO_DEBOUNCE (1U << 8)

/**
 * @brief Enable hardware control/data source for a pin.
 *
 * Configures a pin to be controlled by a hardware data source (if
 * supported).
 */
#define DW_GPIO_HW_MODE (1U << 9)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SNPS_DESIGNWARE_GPIO_H_ */
