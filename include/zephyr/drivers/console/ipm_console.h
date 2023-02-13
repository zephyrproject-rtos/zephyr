/* ipm_console.c - Console messages to/from another processor */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_IPM_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_IPM_CONSOLE_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPM_CONSOLE_STDOUT	(BIT(0))
#define IPM_CONSOLE_PRINTK	(BIT(1))

/*
 * Good way to determine these numbers other than trial-and-error?
 * using printf() in the thread seems to require a lot more stack space
 */
#define IPM_CONSOLE_STACK_SIZE		CONFIG_IPM_CONSOLE_STACK_SIZE
#define IPM_CONSOLE_PRI			2

struct ipm_console_receiver_config_info {
	/** Name of the low-level IPM driver to bind to */
	char *bind_to;

	/**
	 * Stack for the receiver's thread, which prints out messages as
	 * they come in. Should be sized CONFIG_IPM_CONSOLE_STACK_SIZE
	 */
	k_thread_stack_t *thread_stack;

	/**
	 * Ring buffer data area for stashing characters from the interrupt
	 * callback
	 */
	uint32_t *ring_buf_data;

	/** Size of ring_buf_data in 32-bit chunks */
	unsigned int rb_size32;

	/**
	 * Line buffer for incoming messages, characters accumulate here
	 * and then are sent to printk() once full (including a trailing NULL)
	 * or a carriage return seen
	 */
	char *line_buf;

	/** Size in bytes of the line buffer. Must be at least 2 */
	unsigned int lb_size;

	/**
	 * Destination for received console messages, one of
	 * IPM_CONSOLE_STDOUT or IPM_CONSOLE_PRINTK
	 */
	unsigned int flags;
};

struct ipm_console_receiver_runtime_data {
	/** Buffer for received bytes from the low-level IPM device */
	struct ring_buf rb;

	/** Semaphore to wake up the thread to print out messages */
	struct k_sem sem;

	/** pointer to the bound low-level IPM device */
	const struct device *ipm_device;

	/** Indicator that the channel is temporarily disabled due to
	 * full buffer
	 */
	int channel_disabled;

	/** Receiver worker thread */
	struct k_thread rx_thread;
};

struct ipm_console_sender_config_info {
	/** Name of the low-level driver to bind to */
	char *bind_to;

	/**
	 * Source of messages to forward, hooks will be installed.
	 * Can be IPM_CONSOLE_STDOUT, IPM_CONSOLE_PRINTK, or both
	 */
	int flags;
};

#if CONFIG_IPM_CONSOLE_RECEIVER
int ipm_console_receiver_init(const struct device *d);
#endif

#if CONFIG_IPM_CONSOLE_SENDER
int ipm_console_sender_init(const struct device *d);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_IPM_CONSOLE_H_ */
