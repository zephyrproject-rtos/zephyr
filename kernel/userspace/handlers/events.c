/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/events.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

void z_vrfy_k_event_init(struct k_event *event)
{
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(event, K_OBJ_EVENT));
	z_impl_k_event_init(event);
}
#include <zephyr/syscalls/k_event_init_mrsh.c>

uint32_t z_vrfy_k_event_post(struct k_event *event, uint32_t events)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_post(event, events);
}
#include <zephyr/syscalls/k_event_post_mrsh.c>

uint32_t z_vrfy_k_event_set(struct k_event *event, uint32_t events)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_set(event, events);
}
#include <zephyr/syscalls/k_event_set_mrsh.c>

uint32_t z_vrfy_k_event_set_masked(struct k_event *event, uint32_t events,
			       uint32_t events_mask)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_set_masked(event, events, events_mask);
}
#include <zephyr/syscalls/k_event_set_masked_mrsh.c>

uint32_t z_vrfy_k_event_clear(struct k_event *event, uint32_t events)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_clear(event, events);
}
#include <zephyr/syscalls/k_event_clear_mrsh.c>

uint32_t z_vrfy_k_event_wait(struct k_event *event, uint32_t events,
				    bool reset, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait(event, events, reset, timeout);
}
#include <zephyr/syscalls/k_event_wait_mrsh.c>

uint32_t z_vrfy_k_event_wait_all(struct k_event *event, uint32_t events,
					bool reset, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait_all(event, events, reset, timeout);
}
#include <zephyr/syscalls/k_event_wait_all_mrsh.c>

uint32_t z_vrfy_k_event_wait_safe(struct k_event *event, uint32_t events,
				  bool reset, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait_safe(event, events, reset, timeout);
}
#include <zephyr/syscalls/k_event_wait_safe_mrsh.c>

uint32_t z_vrfy_k_event_wait_all_safe(struct k_event *event, uint32_t events,
				      bool reset, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait_all_safe(event, events, reset, timeout);
}
#include <zephyr/syscalls/k_event_wait_all_safe_mrsh.c>
