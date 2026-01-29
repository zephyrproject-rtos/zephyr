/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32H7RS_IOCELL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32H7RS_IOCELL_H_

/** @brief I/O power domains that can be configured in iocell driver */
#define STM32_DT_IOCELL_VDDIO (0)
#define STM32_DT_IOCELL_XSPI1 (1)
#define STM32_DT_IOCELL_XSPI2 (2)

#define STM32_DT_IOCELL_DOMAIN_VALID(x) (((x) >= 0) && ((x) <= 2))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32H7RS_IOCELL_H_ */
