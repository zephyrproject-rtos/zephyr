/*
 * Copyright (c) 2020 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* CCS811 specific pins */
#ifdef CONFIG_CCS811
#define CCS811_PWR_ENABLE_GPIO_NAME	"GPIO_F"
#define CCS811_PWR_ENABLE_GPIO_PIN	14
#endif /* CONFIG_CCS811 */

/* SI7021 specific pins */
#ifdef CONFIG_SI7021
#define SI7021_PWR_ENABLE_GPIO_NAME "GPIO_F"
#define SI7021_PWR_ENABLE_GPIO_PIN 9
#endif /* CONFIG_SI7021 */

#endif /* __INC_BOARD_H */
