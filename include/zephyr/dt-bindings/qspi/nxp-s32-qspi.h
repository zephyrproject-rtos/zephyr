/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_NXP_S32_QSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_NXP_S32_QSPI_H_

#include <zephyr/dt-bindings/dt-util.h>

/* The QSPI secure attribute and secure policy references */
#define NXP_S32_QSPI_NON_SECURE	BIT(0)
#define NXP_S32_QSPI_SECURE	BIT(1)
#define NXP_S32_QSPI_PRIVILEGE	BIT(2)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_NXP_S32_QSPI_H_ */
