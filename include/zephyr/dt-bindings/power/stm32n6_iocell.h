/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I/O cell definitions for STM32N6 devices
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32N6_IOCELL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32N6_IOCELL_H_

/**
 * @name I/O power domains used to configure STM32 iocell device
 * @{
 */

#define STM32_DT_IOCELL_VDDIO2 (0) /**< I/O domain powered by VDDIO2 supply */
#define STM32_DT_IOCELL_VDDIO3 (1) /**< I/O domain powered by VDDIO3 supply */
#define STM32_DT_IOCELL_VDDIO4 (2) /**< I/O domain powered by VDDIO4 supply */
#define STM32_DT_IOCELL_VDDIO5 (3) /**< I/O domain powered by VDDIO5 supply */
#define STM32_DT_IOCELL_VDD    (4) /**< I/O domain powered by VDD supply */

/** @brief Return true if @a domain is valid I/O domain */
#define STM32_DT_IOCELL_DOMAIN_VALID(domain) (((domain) >= 0) && ((domain) <= 4))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32N6_IOCELL_H_ */
