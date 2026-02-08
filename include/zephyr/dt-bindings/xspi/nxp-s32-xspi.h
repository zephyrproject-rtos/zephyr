/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_NXP_S32_XSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_NXP_S32_XSPI_H_

#include <zephyr/dt-bindings/dt-util.h>

/* The XSPI secure attribute and secure policy references */
#define NXP_S32_XSPI_NON_SECURE	BIT(0)
#define NXP_S32_XSPI_SECURE	BIT(1)
#define NXP_S32_XSPI_PRIVILEGE	BIT(2)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_NXP_S32_XSPI_H_ */
