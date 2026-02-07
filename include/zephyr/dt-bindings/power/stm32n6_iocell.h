/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32N6_IOCELL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32N6_IOCELL_H_

/** @brief I/O power domains used to configure STM32 iocell device */
#define STM32_DT_IOCELL_VDDIO2 (0)
#define STM32_DT_IOCELL_VDDIO3 (1)
#define STM32_DT_IOCELL_VDDIO4 (2)
#define STM32_DT_IOCELL_VDDIO5 (3)
#define STM32_DT_IOCELL_VDD    (4)

#define STM32_DT_IOCELL_DOMAIN_VALID(x) (((x) >= 0) && ((x) <= 4))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32N6_IOCELL_H_ */
