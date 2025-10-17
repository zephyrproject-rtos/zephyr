/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENT_H_

/**
 * @addtogroup sent_interface
 * @{
 */

/**
 * @name Fast Message CRC Configuration Flags
 * @{
 */

/** Disable CRC check for fast message */
#define FAST_CRC_DISABLE                    0
/** Use legacy CRC algorithm for fast message */
#define FAST_CRC_LEGACY_IMPLEMENTATION      1
/** Use the recommended CRC algorithm for fast message */
#define FAST_CRC_RECOMMENDED_IMPLEMENTATION 2
/** Include CRC status in fast message */
#define FAST_CRC_STATUS_INCLUDE             4

/** @} */

/**
 * @name Short Serial Message CRC Configuration Flags
 * @{
 */

/** Legacy CRC algorithm for short serial message */
#define SHORT_CRC_LEGACY_IMPLEMENTATION      0
/** Recommended CRC algorithm for short serial message */
#define SHORT_CRC_RECOMMENDED_IMPLEMENTATION 1

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENT_H_ */
