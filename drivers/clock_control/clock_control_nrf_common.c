/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_control_nrf_common.h"
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <nrfx.h>
#include <zephyr/logging/log.h>

#if IS_ENABLED(CONFIG_NRFX_POWER)
#include <nrfx_power.h>
#endif

LOG_MODULE_REGISTER(clock_control_nrf_common, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) || IS_ENABLED(CONFIG_SOC_SERIES_NRF92))

#define FLAG_UPDATE_IN_PROGRESS BIT(FLAGS_COMMON_BITS - 1)
#define FLAG_UPDATE_NEEDED      BIT(FLAGS_COMMON_BITS - 2)

#define ONOFF_CNT_MAX (FLAGS_COMMON_BITS - 2)

#define CONTAINER_OF_ITEM(ptr, idx, type, array)                                                   \
	(type *)((char *)ptr - (idx * sizeof(array[0])) - offsetof(type, array[0]))

/*
 * Definition of `struct clock_config_generic`.
 * Used to access `clock_config_*` structures in a common way.
 */
STRUCT_CLOCK_CONFIG(generic, ONOFF_CNT_MAX);

#else

#define COMMON_CTX_MASK       (COMMON_CTX_ONOFF | COMMON_CTX_API)
#define COMMON_GET_CTX(flags) (flags & COMMON_CTX_MASK)

static bool irq_connected;

#endif /* (IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) || IS_ENABLED(CONFIG_SOC_SERIES_NRF92)) */

/* Structure used for synchronous clock request. */
struct sync_req {
	struct onoff_client cli;
	struct k_sem sem;
	int res;
};

#if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) || IS_ENABLED(CONFIG_SOC_SERIES_NRF92))

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

static void onoff_start_option(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct clock_onoff *onoff = CONTAINER_OF(mgr, struct clock_onoff, mgr);
	struct clock_config_generic *cfg =
		CONTAINER_OF_ITEM(onoff, onoff->idx, struct clock_config_generic, onoff);

	onoff->notify = notify;

	(void)atomic_or(&cfg->flags, BIT(onoff->idx));
	update_config(cfg);
}

static void onoff_stop_option(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct clock_onoff *onoff = CONTAINER_OF(mgr, struct clock_onoff, mgr);
	struct clock_config_generic *cfg =
		CONTAINER_OF_ITEM(onoff, onoff->idx, struct clock_config_generic, onoff);

	(void)atomic_and(&cfg->flags, ~BIT(onoff->idx));
	update_config(cfg);

	notify(mgr, 0);
}

static void onoff_reset_option(struct onoff_manager *mgr, onoff_notify_fn notify)
{
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
			.stop = onoff_stop_option,
			.reset = onoff_reset_option,
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

int clock_config_request(struct onoff_manager *mgr, struct onoff_client *cli)
{
	/* If the on-off service recorded earlier an error, its state must be
	 * reset before a new request is made, otherwise the request would fail
	 * immediately.
	 */
	if (onoff_has_error(mgr)) {
		struct onoff_client reset_cli;

		sys_notify_init_spinwait(&reset_cli.notify);
		onoff_reset(mgr, &reset_cli);
	}

	return onoff_request(mgr, cli);
}

uint8_t clock_config_update_begin(struct k_work *work)
{
	struct clock_config_generic *cfg = CONTAINER_OF(work, struct clock_config_generic, work);
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

#else /* IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) || IS_ENABLED(CONFIG_SOC_SERIES_NRF92)) */

/* This function should be treated as static.
 * static keyword is not used so that it can be accessed by interrupt oriented tests.
 */
void clock_control_nrf_common_irq_handler(void)
{
#if IS_ENABLED(CONFIG_NRFX_POWER)
	nrfx_power_irq_handler();
#endif

	STRUCT_SECTION_FOREACH(clock_control_nrf_irq_handler, irq) {
		irq->handler();
	}
}

void common_connect_irq(void)
{
	if (irq_connected) {
		return;
	}
	irq_connected = true;

#if NRF_LFRC_HAS_CALIBRATION
	IRQ_CONNECT(LFRC_IRQn, DT_IRQ(DT_INST(0, nordic_nrf_clock), priority), nrfx_isr,
		    clock_control_nrf_common_irq_handler, 0);
	irq_enable(LFRC_IRQn);
#endif

	IRQ_CONNECT(DT_IRQN(DT_INST(0, nordic_nrf_clock)),
		    DT_IRQ(DT_INST(0, nordic_nrf_clock), priority), nrfx_isr,
		    clock_control_nrf_common_irq_handler, 0);
	irq_enable(DT_IRQN(DT_INST(0, nordic_nrf_clock)));
}

static int set_off_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	unsigned int key = irq_lock();
	uint32_t current_ctx = COMMON_GET_CTX(*flags);

	if ((current_ctx != 0) && (current_ctx != ctx)) {
		err = -EPERM;
	} else {
		*flags = CLOCK_CONTROL_STATUS_OFF;
	}

	irq_unlock(key);

	return err;
}

static int set_starting_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	unsigned int key = irq_lock();
	uint32_t current_ctx = COMMON_GET_CTX(*flags);

	if ((*flags & (COMMON_STATUS_MASK)) == CLOCK_CONTROL_STATUS_OFF) {
		*flags = CLOCK_CONTROL_STATUS_STARTING | ctx;
	} else if (current_ctx != ctx) {
		err = -EPERM;
	} else {
		err = -EALREADY;
	}

	irq_unlock(key);

	return err;
}

void common_set_on_state(uint32_t *flags)
{
	unsigned int key = irq_lock();

	*flags = CLOCK_CONTROL_STATUS_ON | COMMON_GET_CTX(*flags);
	irq_unlock(key);
}

void common_blocking_start_callback(const struct device *dev, clock_control_subsys_t subsys,
				    void *user_data)
{
	struct k_sem *sem = user_data;

	ARG_UNUSED(subsys);
	ARG_UNUSED(dev);

	k_sem_give(sem);
}

int common_async_start(const struct device *dev, clock_control_cb_t cb, void *user_data,
		       uint32_t ctx)
{
	common_clock_data_t *dev_data = dev->data;
	const common_clock_config_t *dev_config = dev->config;
	int err;

	err = set_starting_state(&((common_clock_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	dev_data->cb = cb;
	dev_data->user_data = user_data;

	dev_config->start();

	return 0;
}

int common_stop(const struct device *dev, uint32_t ctx)
{
	int err;

	err = set_off_state(&((common_clock_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	((common_clock_config_t *)dev->config)->stop();

	return 0;
}

void common_onoff_started_callback(const struct device *dev, clock_control_subsys_t sys,
				   void *user_data)
{
	ARG_UNUSED(sys);

	onoff_notify_fn notify = user_data;

	notify(&((common_clock_data_t *)dev->data)->mgr, 0);
}

void common_clkstarted_handle(const struct device *dev)
{
	clock_control_cb_t callback = ((common_clock_data_t *)dev->data)->cb;

	((common_clock_data_t *)dev->data)->cb = NULL;
	common_set_on_state(&((common_clock_data_t *)dev->data)->flags);

	if (callback) {
		callback(dev, NULL, ((common_clock_data_t *)dev->data)->user_data);
	}
}

void common_clear_pending_irq(void)
{
	NRFX_IRQ_PENDING_CLEAR(DT_IRQN(DT_INST(0, nordic_nrf_clock)));
}

#endif /* (IS_ENABLED(CONFIG_SOC_SERIES_NRF54H) || IS_ENABLED(CONFIG_SOC_SERIES_NRF92)) */

static void sync_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state, int res)
{
	struct sync_req *req = CONTAINER_OF(cli, struct sync_req, cli);

	req->res = res;
	k_sem_give(&req->sem);
}

int nrf_clock_control_request_sync(const struct device *dev, const struct nrf_clock_spec *spec,
				   k_timeout_t timeout)
{
	struct sync_req req = {.sem = Z_SEM_INITIALIZER(req.sem, 0, 1)};
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
		nrf_clock_control_cancel_or_release(dev, spec, &req.cli);
		return err;
	}

	return req.res;
}
