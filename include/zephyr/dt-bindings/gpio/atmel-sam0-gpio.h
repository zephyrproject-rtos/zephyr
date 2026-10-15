/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ATMEL_SAM0_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ATMEL_SAM0_GPIO_H_

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Atmel SAM0 SoCs.
 */
#define SAM0_GPIO_DEBOUNCE (1U << 8)

/**
 * @brief Enable stronger output drive (PORT PINCFG.DRVSTR).
 *
 * Raises the pin's rated output current, e.g. from 2 mA to 8 mA at
 * VDD >= 3.0 V on SAM D5x/E5x. Only meaningful for pins configured as
 * outputs. Only applicable for Atmel SAM0 SoCs.
 */
#define SAM0_GPIO_DRIVE_STRONG (1U << 9)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ATMEL_SAM0_GPIO_H_ */
