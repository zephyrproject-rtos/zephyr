/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Disable syscall tracing for all calls from this compilation unit to avoid
 * undefined symbols as the macros are not expanded recursively
 */
#define DISABLE_SYSCALL_TRACING

#include <zephyr/init.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

#define TRACING_CMD_ENABLE  "enable"
#define TRACING_CMD_DISABLE "disable"

#ifdef CONFIG_TRACING_BACKEND_UART
#define TRACING_BACKEND_NAME "tracing_backend_uart"
#elif defined CONFIG_TRACING_BACKEND_USB
#define TRACING_BACKEND_NAME "tracing_backend_usb"
#elif defined CONFIG_TRACING_BACKEND_POSIX
#define TRACING_BACKEND_NAME "tracing_backend_posix"
#elif defined CONFIG_TRACING_BACKEND_RAM
#define TRACING_BACKEND_NAME "tracing_backend_ram"
#else
#define TRACING_BACKEND_NAME ""
#endif

enum tracing_state {
	TRACING_DISABLE = 0,
	TRACING_ENABLE
};

static atomic_t tracing_state;
static atomic_t tracing_packet_drop_num;
static struct tracing_backend *working_backend;

#ifdef CONFIG_TRACING_ASYNC
#define TRACING_THREAD_NAME "tracing_thread"

static k_tid_t tracing_thread_tid;
static struct k_thread tracing_thread;
static struct k_timer tracing_thread_timer;
static K_SEM_DEFINE(tracing_thread_sem, 0, 1);
static K_THREAD_STACK_DEFINE(tracing_thread_stack,
			CONFIG_TRACING_THREAD_STACK_SIZE);

static void tracing_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint8_t *transferring_buf;
	uint32_t transferring_length, tracing_buffer_max_length;

	tracing_thread_tid = k_current_get();

	tracing_buffer_max_length = tracing_buffer_capacity_get();

	while (true) {
		if (tracing_buffer_is_empty()) {
			k_sem_take(&tracing_thread_sem, K_FOREVER);
		} else {
			transferring_length =
				tracing_buffer_get_claim(
						&transferring_buf,
						tracing_buffer_max_length);
			tracing_buffer_handle(transferring_buf,
					      transferring_length);
			tracing_buffer_get_finish(transferring_length);
		}
	}
}

static void tracing_thread_timer_expiry_fn(struct k_timer *timer)
{
	k_sem_give(&tracing_thread_sem);
}
#endif

static void tracing_set_state(enum tracing_state state)
{
	atomic_set(&tracing_state, state);
}

static int tracing_init(void)
{

	tracing_buffer_init();

	working_backend = tracing_backend_get(TRACING_BACKEND_NAME);
	tracing_backend_init(working_backend);

	atomic_set(&tracing_packet_drop_num, 0);

	if (IS_ENABLED(CONFIG_TRACING_HANDLE_HOST_CMD)) {
		tracing_set_state(TRACING_DISABLE);
	} else {
		tracing_set_state(TRACING_ENABLE);
	}

#ifdef CONFIG_TRACING_ASYNC
	k_timer_init(&tracing_thread_timer,
		     tracing_thread_timer_expiry_fn, NULL);

	k_thread_create(&tracing_thread, tracing_thread_stack,
			K_THREAD_STACK_SIZEOF(tracing_thread_stack),
			tracing_thread_func, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&tracing_thread, TRACING_THREAD_NAME);
#endif

	return 0;
}

SYS_INIT(tracing_init, APPLICATION, 0);

#ifdef CONFIG_TRACING_ASYNC
void tracing_trigger_output(bool before_put_is_empty)
{
	if (before_put_is_empty) {
		k_timer_start(&tracing_thread_timer,
			      K_MSEC(CONFIG_TRACING_THREAD_WAIT_THRESHOLD),
			      K_NO_WAIT);
	}
}

bool is_tracing_thread(void)
{
	return (!k_is_in_isr() && (k_current_get() == tracing_thread_tid));
}
#endif

bool is_tracing_enabled(void)
{
	return atomic_get(&tracing_state) == TRACING_ENABLE;
}

void tracing_cmd_handle(uint8_t *buf, uint32_t length)
{
	if (strncmp(buf, TRACING_CMD_ENABLE, length) == 0) {
		tracing_set_state(TRACING_ENABLE);
	} else if (strncmp(buf, TRACING_CMD_DISABLE, length) == 0) {
		tracing_set_state(TRACING_DISABLE);
	}
}

void tracing_buffer_handle(uint8_t *data, uint32_t length)
{
	tracing_backend_output(working_backend, data, length);
}

void tracing_packet_drop_handle(void)
{
	atomic_inc(&tracing_packet_drop_num);
}
