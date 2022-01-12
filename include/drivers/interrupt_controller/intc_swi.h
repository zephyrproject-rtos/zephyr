/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief Public APIs for software interrupt driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_SWI_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_SWI_H_

#include <zephyr.h>
#include <sys/slist.h>
#include <sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct swi_channel;

/**
 * @typedef swi_channel_cb_t
 * @brief Define the application callback function signature for
 * @ref swi_channel_init function.
 *
 * @param swi  SWI channel number that triggered the callback.
 */
typedef void (*swi_channel_cb_t)(struct swi_channel *swi);

/**
 * @brief SWI channel structure
 */
struct swi_channel {
	/* SWI channel list node. */
	sys_snode_t node;
	/* SWI channel auxiliary list node. */
	sys_snode_t aux_node;
	/* SWI signaled flag. */
	atomic_t signaled;
	/* SWI callback function. */
	swi_channel_cb_t cb;
	/* SWI channel initialized. */
	bool initialized;
};

/**
 * @brief Initialize SWI channel.
 *
 * @param swi  SWI channel structure.
 * @param cb   Pointer to callback function.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int swi_channel_init(struct swi_channel *swi, swi_channel_cb_t cb);

/**
 * @brief Deinitialize SWI channel.
 *
 * @param swi  SWI channel structure.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int swi_channel_deinit(struct swi_channel *swi);

/**
 * @brief Trigger SWI channel.
 *
 * @param swi  SWI channel structure.
 *
 * @note This function is **isr-ok** and can be safely used in ZLI context.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int swi_channel_trigger(struct swi_channel *swi);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_SWI_H_ */
