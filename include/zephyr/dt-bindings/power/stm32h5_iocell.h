/*
 * Copyright (c) 2026 CodeWrights GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I/O cell definitions for STM32H5 devices
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32H5_IOCELL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32H5_IOCELL_H_

/**
 * @name I/O power domains used to configure STM32H5 iocell device
 * @{
 */

#define STM32_DT_IOCELL_VDDIO  (0) /**< I/O domain powered by VDD supply */
#define STM32_DT_IOCELL_VDDIO2 (1) /**< I/O domain powered by VDDIO2 supply */

/** @brief Return true if @a domain is valid I/O domain */
#define STM32_DT_IOCELL_DOMAIN_VALID(domain) (((domain) >= 0) && ((domain) <= 1))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32H5_IOCELL_H_ */
