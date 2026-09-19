/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CORDIC_ADOPTION_H_
#define ZEPHYR_INCLUDE_CORDIC_ADOPTION_H_


/**
 * @brief This headr shall include SoC vendor config adoptions guarded by Kconfig options.
 */
#ifdef CONFIG_CORDIC_STM32
#include <cordic/cordic_adoption_stm32.h>
#endif /* CONFIG_CORDIC_STM32 */

#endif /* ZEPHYR_INCLUDE_CORDIC_ADOPTION_H_ */
