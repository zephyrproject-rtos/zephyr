/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>
#include <ipc/ipc_based_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blocking_core_w91);

#define MTIME_REG    DT_REG_ADDR(DT_INST(0, telink_machine_timer))
#define BLOCKING_THREAD_PRIORITY (CONFIG_NUM_PREEMPT_PRIORITIES - 1)

enum {
	IPC_DISPATCHER_BLOCKING_SET_STATE_ADDR = IPC_DISPATCHER_BLOCKING,
	IPC_DISPATCHER_BLOCKING_STOP_CORE_REQ,
};

enum {
	BLOCKING_INVALID_STATE,
	BLOCKING_INITIATED_STATE,
	BLOCKING_CORE_ACTIVE_STATE,
	BLOCKING_CORE_STOP_REQ_STATE,
	BLOCKING_CORE_STOPPED_STATE,
	BLOCKING_CORE_ACTIVE_REQ_STATE,
};

static atomic_t __GENERIC_SECTION(.ram_dlm) blocking_state = BLOCKING_INVALID_STATE;
static struct ipc_based_driver ipc_data;    /* ipc driver data part */

/* API implementation: get Machine Timer value */
static uint64_t __GENERIC_SECTION(.ram_code) get_mtime(void)
{
	const volatile uint32_t *const rl = (const volatile uint32_t *const)MTIME_REG;
	const volatile uint32_t *const rh =
		(const volatile uint32_t *const)(MTIME_REG + sizeof(uint32_t));
	uint32_t mtime_l, mtime_h;

	do {
		mtime_h = *rh;
		mtime_l = *rl;
	} while (mtime_h != *rh);

	return (((uint64_t)mtime_h) << 32) | mtime_l;
}

/* APIs implementation: set address of blocking state */
static size_t pack_blocking_w91_set_state_addr(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	uint32_t *p_set_state_addr_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(uint32_t);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLOCKING_SET_STATE_ADDR, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, *p_set_state_addr_req);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(blocking_w91_set_state_addr);

static int blocking_w91_set_state_addr(uint32_t addr)
{
	int err;

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0,
			blocking_w91_set_state_addr, &addr, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: core stop request event */
static void __GENERIC_SECTION(.ram_code) __attribute__((noinline)) blocking_w91_stop_core(void)
{
	unsigned int key;

	bool blocking_timeout = false;
	uint64_t start_ticks;
	uint64_t timeout_ticks = (uint64_t)CONFIG_BLOCKING_CORE_TELINK_W91_CORE_STOP_TIMEOUT_MS *
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / MSEC_PER_SEC;

	if (atomic_get(&blocking_state) != BLOCKING_CORE_STOP_REQ_STATE) {
		assert(0);
		return;
	}

	key = irq_lock();

	atomic_set(&blocking_state, BLOCKING_CORE_STOPPED_STATE);

	start_ticks = get_mtime();

	while (atomic_get(&blocking_state) != BLOCKING_CORE_ACTIVE_REQ_STATE) {
		if ((get_mtime() - start_ticks) >= timeout_ticks) {
			blocking_timeout = true;
			break;
		}
	}

	atomic_set(&blocking_state, BLOCKING_CORE_ACTIVE_STATE);

	irq_unlock(key);

	if (blocking_timeout) {
		LOG_ERR("Blocking core timeout");
	}
}

static void blocking_w91_stop_core_req(const void *data, size_t len, void *param)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(param);

	blocking_w91_stop_core();
}

static int blocking_w91_init(void)
{
	int err;

	ipc_based_driver_init(&ipc_data);

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLOCKING_STOP_CORE_REQ, 0),
		blocking_w91_stop_core_req, NULL);

	err = blocking_w91_set_state_addr((uint32_t)&blocking_state);
	if (err < 0) {
		return err;
	}

	if (atomic_get(&blocking_state) != BLOCKING_INITIATED_STATE) {
		LOG_ERR("Incorrect state of blocking_state");
		return -EINVAL;
	}

	atomic_set(&blocking_state, BLOCKING_CORE_ACTIVE_STATE);

	return 0;
}

SYS_INIT(blocking_w91_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_PRE_DRIVERS_INIT_PRIORITY);
