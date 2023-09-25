/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_COMPARATOR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_COMPARATOR_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name Comparator configuration flags
 * @ingroup comparator_interface
 * @{
 */

/** Signal with the callback when the comparator result changes to BELOW. */
#define COMPARATOR_FLAG_SIGNAL_BELOW BIT(0)
/** Signal with the callback when the comparator result changes to ABOVE. */
#define COMPARATOR_FLAG_SIGNAL_ABOVE BIT(1)

/** @cond INTERNAL_HIDDEN */
#define COMPARATOR_FLAG_SIGNAL_MASK  0x3UL
/* Position in the flags bitfield from which vendor-specific flags can be defined. */
#define COMPARATOR_FLAG_VENDOR_POS  8
/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_COMPARATOR_H_ */
