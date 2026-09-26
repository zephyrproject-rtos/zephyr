/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup rts5817_wuc
 * @brief RTS5817 WUC (Wake-Up Controller) driver callback registration API.
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WUC_RTS5817_WUC_H_
#define ZEPHYR_INCLUDE_DRIVERS_WUC_RTS5817_WUC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTS5817 WUC Driver Callback API
 * @defgroup rts5817_wuc RTS5817 WUC
 * @since 4.4
 * @version 0.1.0
 * @ingroup wuc_interface
 * @{
 */

/**
 * @brief Type definition for wakeup event callback functions.
 *
 * @param id   Wakeup source identifier that triggered the callback
 *             (one of the @c RTS5817_WKUP_SRC_* constants).
 * @param data User-supplied context pointer registered with the callback.
 */
typedef void (*rts5817_wakeup_event_cb_t)(int id, void *data);

/**
 * @brief Register a callback for a specific wakeup source.
 *
 * Stores the callback and its context in the WUC driver's callback table.
 * The callback is invoked during the WUC device PM resume sequence
 * if the corresponding wakeup source is found to have been triggered.
 *
 * If a callback is already registered for the same @p id, it is
 * silently overwritten.
 *
 * @param id   Wakeup source identifier (one of the @c RTS5817_WKUP_SRC_*
 *             constants defined in @ref wuc_rts5817.h).
 * @param cb   Callback function to invoke when the wakeup source triggers.
 * @param data User-supplied context pointer passed to the callback.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p id is out of range or equals @c RTS5817_WKUP_SRC_NONE.
 */
int rts_register_wakeup_event_callback(int id, rts5817_wakeup_event_cb_t cb, void *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_WUC_RTS5817_WUC_H_ */
