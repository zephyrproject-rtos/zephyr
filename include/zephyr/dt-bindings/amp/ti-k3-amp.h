/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_AMP_TI_K3_AMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_AMP_TI_K3_AMP_H_

#include <zephyr/dt-bindings/dt-util.h>

// TODO: "Unique" TI K3 Prefix or similar!

#define CORTEX_R_INVASIVE_DBG_BIT                BIT(0)
#define CORTEX_R_NON_INVASIVE_DBG_BIT            BIT(1)
#define CORTEX_R_THUMB_EXCEPTION_HANDLING_BIT    BIT(9)
#define CORTEX_R_NON_MASKABLE_FAST_INTERRUPT_BIT BIT(10)
#define CORTEX_R_ATCM_AT_ZERO_BIT                BIT(11)
#define CORTEX_R_BTCM_ENABLE_BIT                 BIT(12)
#define CORTEX_R_ATCM_ENABLE_BIT                 BIT(13)
#define CORTEX_R_DISABLE_MEMORY_INIT_BIT         BIT(14)
#define CORTEX_R_USE_SINGLECORE                  BIT(15)

#define CORTEX_X_IGNORE_BOOTVECTOR               BIT(28)

#define CORTEX_R_ALL_BITS \
	(BIT(0) | BIT(1) | GENMASK(15, 9) | BIT(28))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_AMP_TI_K3_AMP_H_ */
