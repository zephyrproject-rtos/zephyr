/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 The Zephyr Project Contributors */

#ifndef ZEPHYR_TESTS_DRIVERS_GPIO_GPIO_PCA_SERIES_PCA9555_EMUL_H_
#define ZEPHYR_TESTS_DRIVERS_GPIO_GPIO_PCA_SERIES_PCA9555_EMUL_H_

#include <stdint.h>

struct emul;

#define PCA9555_EMUL_INITIAL_POLARITY 0x9669

uint16_t pca9555_emul_get_word(const struct emul *target, uint8_t reg);
void pca9555_emul_set_word(const struct emul *target, uint8_t reg, uint16_t value);

#endif /* ZEPHYR_TESTS_DRIVERS_GPIO_GPIO_PCA_SERIES_PCA9555_EMUL_H_ */
