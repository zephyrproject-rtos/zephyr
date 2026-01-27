/*
 * Copyright (c) 2026 ITE Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WUC_WUC_ITE_H_
#define ZEPHYR_INCLUDE_DRIVERS_WUC_WUC_ITE_H_

#include <zephyr/sys/util.h>

/*
 * Wakeup source encoding for ITE WUC:
 * Bits [7:0]   - Mask
 * Bits [15:8]  - Flags (edge/level selection)
 * Bits [23:16] - Priority (optional, controller-specific)
 */

#define ITE_WUC_ID_MASK_POS     0U
#define ITE_WUC_ID_FLAGS_POS    8U
#define ITE_WUC_ID_PRIORITY_POS 16U

#define ITE_WUC_ID_MASK_MASK     0xFFU
#define ITE_WUC_ID_FLAGS_MASK    0xFFU
#define ITE_WUC_ID_PRIORITY_MASK 0xFFU

#define ITE_WUC_ID_ENCODE(mask, flags)                                                         \
	(((uint32_t)(mask) & ITE_WUC_ID_MASK_MASK) |                                             \
	 ((uint32_t)(flags) << ITE_WUC_ID_FLAGS_POS))

#define ITE_WUC_ID_ENCODE_WITH_PRIORITY(mask, flags, priority)                                 \
	(((uint32_t)(mask) & ITE_WUC_ID_MASK_MASK) |                                             \
	 ((uint32_t)(flags) << ITE_WUC_ID_FLAGS_POS) |                                          \
	 ((uint32_t)(priority) << ITE_WUC_ID_PRIORITY_POS))

#define ITE_WUC_ID_DECODE_MASK(id)                                                             \
	((uint8_t)(((id) >> ITE_WUC_ID_MASK_POS) & ITE_WUC_ID_MASK_MASK))

#define ITE_WUC_ID_DECODE_FLAGS(id)                                                            \
	(((id) >> ITE_WUC_ID_FLAGS_POS) & ITE_WUC_ID_FLAGS_MASK)

#define ITE_WUC_ID_DECODE_PRIORITY(id)                                                         \
	(((id) >> ITE_WUC_ID_PRIORITY_POS) & ITE_WUC_ID_PRIORITY_MASK)

/** ITE WUC flags */
#define ITE_WUC_FLAG_EDGE_RISING  BIT(0)
#define ITE_WUC_FLAG_EDGE_FALLING BIT(1)
#define ITE_WUC_FLAG_EDGE_BOTH    (ITE_WUC_FLAG_EDGE_RISING | ITE_WUC_FLAG_EDGE_FALLING)
#define ITE_WUC_FLAG_LEVEL_TRIG   BIT(2)
#define ITE_WUC_FLAG_LEVEL_HIGH   BIT(3)
#define ITE_WUC_FLAG_LEVEL_LOW    BIT(4)

#endif /* ZEPHYR_INCLUDE_DRIVERS_WUC_WUC_ITE_H_ */
