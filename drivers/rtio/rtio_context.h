/*
 * Copyright (c) 2019 Tom Burdick <tom.burdick@electromatic.us>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for RTIO drivers
 */

#ifndef ZEPHYR_DRIVERS_RTIO_RTIO_CONTEXT_H_
#define ZEPHYR_DRIVERS_RTIO_RTIO_CONTEXT_H_

#include <drivers/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Private RTIO driver context
 *
 * This is to be used by drivers in their data struct if they wish to
 * take advantage of the common RTIO functionality for configuration
 * of triggers.
 */
struct rtio_context {
	/**
	 * @brief Semaphore to be used by configure and trigger
	 *
	 * This is used to avoid interrupts causing a rtio_trigger
	 * manipulating data while the device is reconfiguring. This provides
	 * the atomicity of configure for the device from the applications
	 * perspective while allowing configure to be pre-empted.
	 *
	 * This is taken without blocking when rtio_begin_trigger()
	 * is called, and taken waiting forever when rtio_begin_configuration()
	 * is called.
	 */
	struct k_sem sem;

	/**
	 * @brief The current RTIO configuration for the device
	 *
	 * This is copied in whenever rtio_begin_configuration() is called
	 */
	struct rtio_config config;

	/**
	 * @brief The current block being filled
	 */
	struct rtio_block *block;

	/**
	 * @brief Timer if needed by the output policy
	 */
	struct k_timer output_timer;
};

/**
 * @brief Initialize the common driver context struct
 *
 * @param ctx Pointer to common rtio driver context
 */
void rtio_context_init(struct rtio_context *ctx);

/**
 * @brief Begin configuring the device
 *
 * This must *not* be called in an ISR as it waits until any currently
 * executing trigger functions complete.
 *
 * rtio_context_configure_end *must* be called afterwards when
 * driver-specific configuration is done.
 *
 * @param ctx Common driver context
 *
 * @retval 0 No FIFO put was done.
 * @retval 1 Current block was put into fifo and ctx->block is no longer valid.
 */
int rtio_context_configure_begin(struct rtio_context *ctx);

/**
 * @brief End configuring the device
 *
 * @param ctx Common driver context
 * @param config New configuration to use (will be copied).
 *
 * This will give back the semaphore allowing trigger to execute.
 */
void rtio_context_configure_end(struct rtio_context *ctx,
				struct rtio_config *config);

/**
 * @brief Begin trigger read call
 *
 * This may be called in any context that can take a semaphore without
 * blocking, including an ISR.
 *
 * If the current block is NULL it will attempt to allocate a new one.
 * The size of the block is given by rtio_context block_alloc_size;
 * Not all allocators take size into account.
 *
 * Returns the current block to be filled on success.
 *
 * On success rtio_end_trigger *must* be called afterwards when done.
 *
 * @param ctx Common driver context
 * @param block A pointer to a rtio_block that will be set on success of this
 *              function. The block is the one to use for reading/writing.
 * @param timeout Timeout to wait for allocation.
 *
 * @retval 0 Trigger began
 * -EBUSY if not possible to do without blocking
 * -ENOMEM if not possible because allocation isn't possible.
 */
int rtio_context_trigger_read_begin(struct rtio_context *ctx,
				    struct rtio_block **block,
				    s32_t timeout);

/**
 * @brief End trigger read call
 *
 * This will give back the semaphore allowing configuration or the next trigger
 * to execute.
 *
 * It will also put the current block into the output fifo if the output policy
 * has been met.
 *
 * The current block in rtio_context may be NULL after this call. This is
 * to avoid attempting allocation in this call, which would make it fallible.
 *
 * @param ctx Common driver context
 *
 * @retval 0 No FIFO put was done.
 * @retval 1 Current block was put into fifo and ctx->block is no longer valid.
 */
int rtio_context_trigger_read_end(struct rtio_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
