/*
 * Copyright (c) 2026 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_DRIVERS_LED_STRIP_OPTIONALS_H
#define ZEPHYR_SAMPLES_DRIVERS_LED_STRIP_OPTIONALS_H

/**
 * @brief Try to map logical colors to physical channel index, following the order of
 * LED_COLOR_ID_*.
 */
void try_map_channels(void);

/**
 * @brief Set channels according to pixel_channels from last to first pixel.
 */
void set_channels(void);

#endif /* ZEPHYR_SAMPLES_DRIVERS_LED_STRIP_OPTIONALS_H */
