/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_control_nrf2_common.h"
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define FLAG_UPDATE_IN_PROGRESS BIT(FLAGS_COMMON_BITS - 1)
#define FLAG_UPDATE_NEEDED      BIT(FLAGS_COMMON_BITS - 2)

#define ONOFF_CNT_MAX (FLAGS_COMMON_BITS - 2)

#define CONTAINER_OF_ITEM(ptr, idx, type, array) \
	(type *)((char *)ptr - \
		 (idx * sizeof(array[0])) - \
		 offsetof(type, array[0]))

/*
 * Definition of `struct clock_config_generic`.
 * Used to access `clock_config_*` structures in a common way.
 */
STRUCT_CLOCK_CONFIG(generic, ONOFF_CNT_MAX);

/* Structure used for synchronous clock request. */
struct sync_req {
	struct onoff_client cli;
	struct k_sem sem;
	int res;
};

static void update_config(struct clock_config_generic *cfg)
{
	atomic_val_t prev_flags = atomic_or(&cfg->flags, FLAG_UPDATE_NEEDED);

	/* If the update work is already scheduled (FLAG_UPDATE_NEEDED was
	 * set before the above OR operation) or is currently being executed,
	 * it is not to be submitted again. In the latter case, it will be
	 * submitted by clock_config_update_end().
	 */
	if (prev_flags & (FLAG_UPDATE_NEEDED | FLAG_UPDATE_IN_PROGRESS)) {
		return;
	}

	k_work_submit(&cfg->work);
}

static void onoff_start_option(struct onoff_manager *mgr,
			       onoff_notify_fn notify)
{
	struct clock_onoff *onoff =
		CONTAINER_OF(mgr, struct clock_onoff, mgr);
	struct clock_config_generic *cfg =
		CONTAINER_OF_ITEM(onoff, onoff->idx,
				  struct clock_config_generic, onoff);

	onoff->notify = notify;

	(void)atomic_or(&cfg->flags, BIT(onoff->idx));
	update_config(cfg);
}

static void onoff_stop_option(struct onoff_manager *mgr,
			      onoff_notify_fn notify)
{
	struct clock_onoff *onoff =
		CONTAINER_OF(mgr, struct clock_onoff, mgr);
	struct clock_config_generic *cfg =
		CONTAINER_OF_ITEM(onoff, onoff->idx,
				  struct clock_config_generic, onoff);

	(void)atomic_and(&cfg->flags, ~BIT(onoff->idx));
	update_config(cfg);

	notify(mgr, 0);
}

static inline uint8_t get_index_of_highest_bit(uint32_t value)
{
	return value ? (uint8_t)(31 - __builtin_clz(value)) : 0;
}

int clock_config_init(void *clk_cfg, uint8_t onoff_cnt, k_work_handler_t update_work_handler)
{
	struct clock_config_generic *cfg = clk_cfg;

	__ASSERT_NO_MSG(onoff_cnt <= ONOFF_CNT_MAX);

	for (int i = 0; i < onoff_cnt; ++i) {
		static const struct onoff_transitions transitions = {
			.start = onoff_start_option,
			.stop  = onoff_stop_option
		};
		int rc;

		rc = onoff_manager_init(&cfg->onoff[i].mgr, &transitions);
		if (rc < 0) {
			return rc;
		}

		cfg->onoff[i].idx = (uint8_t)i;
	}

	cfg->onoff_cnt = onoff_cnt;

	k_work_init(&cfg->work, update_work_handler);

	return 0;
}

uint8_t clock_config_update_begin(struct k_work *work)
{
	struct clock_config_generic *cfg =
		CONTAINER_OF(work, struct clock_config_generic, work);
	uint32_t active_options;

	(void)atomic_or(&cfg->flags, FLAG_UPDATE_IN_PROGRESS);
	cfg->flags_snapshot = atomic_and(&cfg->flags, ~FLAG_UPDATE_NEEDED);

	active_options = cfg->flags_snapshot & BIT_MASK(ONOFF_CNT_MAX);
	return get_index_of_highest_bit(active_options);
}

void clock_config_update_end(void *clk_cfg, int status)
{
	struct clock_config_generic *cfg = clk_cfg;
	atomic_val_t prev_flags;

	prev_flags = atomic_and(&cfg->flags, ~FLAG_UPDATE_IN_PROGRESS);
	if (!(prev_flags & FLAG_UPDATE_IN_PROGRESS)) {
		return;
	}

	for (int i = 0; i < cfg->onoff_cnt; ++i) {
		if (cfg->flags_snapshot & BIT(i)) {
			onoff_notify_fn notify = cfg->onoff[i].notify;

			if (notify) {
				/* If an option was to be activated now
				 * (it is waiting for a notification) and
				 * the activation failed, this option's flag
				 * must be cleared (the option can no longer
				 * be considered active).
				 */
				if (status < 0) {
					(void)atomic_and(&cfg->flags, ~BIT(i));
				}

				cfg->onoff[i].notify = NULL;
				notify(&cfg->onoff[i].mgr, status);
			}
		}
	}

	if (prev_flags & FLAG_UPDATE_NEEDED) {
		k_work_submit(&cfg->work);
	}
}

int api_nosys_on_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOSYS;
}

static void sync_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state, int res)
{
	struct sync_req *req = CONTAINER_OF(cli, struct sync_req, cli);

	req->res = res;
	k_sem_give(&req->sem);
}

int nrf_clock_control_request_sync(const struct device *dev,
				   const struct nrf_clock_spec *spec,
				   k_timeout_t timeout)
{
	struct sync_req req = {
		.sem = Z_SEM_INITIALIZER(req.sem, 0, 1)
	};
	int err;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	sys_notify_init_callback(&req.cli.notify, sync_cb);

	err = nrf_clock_control_request(dev, spec, &req.cli);
	if (err < 0) {
		return err;
	}

	err = k_sem_take(&req.sem, timeout);
	if (err < 0) {
		return err;
	}

	return req.res;
}
