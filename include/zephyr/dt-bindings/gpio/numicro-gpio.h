/*
 * Copyright (c) 2022 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NUVOTON_NUMICRO_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NUVOTON_NUMICRO_GPIO_H_

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Nuvoton NuMicro SoCs.
 */
#define NUMICRO_GPIO_INPUT_DEBOUNCE	(1U << 8)

/**
 * @brief Enable Schmitt trigger on input.
 *
 * The Schmitt trigger flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Nuvoton NuMicro SoCs.
 */
#define NUMICRO_GPIO_INPUT_SCHMITT	(1U << 9)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NUVOTON_NUMICRO_GPIO_H_ */
