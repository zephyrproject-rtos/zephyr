/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include "clock_control_nrf_common.h"
#include <nrfx_clock_hfclk192m.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>
#include <nrf_erratas.h>

LOG_MODULE_REGISTER(clock_control_hfclk192m, CONFIG_CLOCK_CONTROL_LOG_LEVEL);
//TODO check all defines if they are used
#define DT_DRV_COMPAT nordic_nrf_clock_hfclk192m

#define CLOCK_DEVICE_HFCLK192M DEVICE_DT_GET(DT_NODELABEL(hfclk192m))

#define CTX_ONOFF		BIT(6)
#define CTX_API			BIT(7)
#define CTX_MASK (CTX_ONOFF | CTX_API)

#define STATUS_MASK		0x7
#define GET_STATUS(flags)	(flags & STATUS_MASK)
#define GET_CTX(flags)		(flags & CTX_MASK)

/* Helper logging macros. */
#ifdef CONFIG_LOG
#define CLOCK_LOG(lvl, dev, ...) \
	LOG_##lvl("%s: " GET_ARG_N(1, __VA_ARGS__), \
		"hfclk192m" \
		COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),\
				(), (, GET_ARGS_LESS_N(1, __VA_ARGS__))))
#else
#define CLOCK_LOG(...)
#endif

#define ERR(dev, ...) CLOCK_LOG(ERR, dev, __VA_ARGS__)
#define WRN(dev, ...) CLOCK_LOG(WRN, dev, __VA_ARGS__)
#define INF(dev, ...) CLOCK_LOG(INF, dev, __VA_ARGS__)
#define DBG(dev, ...) CLOCK_LOG(DBG, dev, __VA_ARGS__)

typedef void (*clk_ctrl_func_t)(void);

typedef struct {
	struct onoff_manager mgr;
	clock_control_cb_t cb;
	void *user_data;
	uint32_t flags;
} hfclk192m_data_t;

typedef struct {
	clk_ctrl_func_t start;		/* Clock start function */
	clk_ctrl_func_t stop;		/* Clock stop function */
#ifdef CONFIG_LOG
	const char *name;
#endif
} hfclk192m_config_t;

static int set_off_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	unsigned int key = irq_lock();
	uint32_t current_ctx = GET_CTX(*flags);

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
	uint32_t current_ctx = GET_CTX(*flags);

	if ((*flags & (STATUS_MASK)) == CLOCK_CONTROL_STATUS_OFF) {
		*flags = CLOCK_CONTROL_STATUS_STARTING | ctx;
	} else if (current_ctx != ctx) {
		err = -EPERM;
	} else {
		err = -EALREADY;
	}

	irq_unlock(key);

	return err;
}

static void set_on_state(uint32_t *flags)
{
	unsigned int key = irq_lock();

	*flags = CLOCK_CONTROL_STATUS_ON | GET_CTX(*flags);
	irq_unlock(key);
}

static void clkstarted_handle(const struct device *dev)
{
	clock_control_cb_t callback = ((hfclk192m_data_t *)dev->data)->cb;

	((hfclk192m_data_t *)dev->data)->cb = NULL;
	set_on_state(&((hfclk192m_data_t *)dev->data)->flags);
	DBG(dev, "Clock started");

	if (callback) {
		callback(dev, NULL, (hfclk192m_data_t *)dev->data)->user_data);
	}
}

static void hfclk192m_start(void)
{
	nrfx_clock_hfclk192m_start();
}

static void hfclk192m_stop(void)
{
	nrfx_clock_hfclk192m_stop();
}

static int stop(const struct device *dev, uint32_t ctx)
{
	int err;

	err = set_off_state(&((hfclk192m_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	((hfclk192m_config_t *)dev->config)->stop();

	return 0;
}

static int async_start(const struct device *dev, clock_control_cb_t cb, void *user_data,
			uint32_t ctx)
{
	int err;

	err = set_starting_state(&((hfclk192m_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	((hfclk192m_data_t *)dev->data)->cb = cb;
	((hfclk192m_data_t *)dev->data)->user_data = user_data;

	((hfclk192m_config_t *)dev->config)->start();

	return 0;
}

static void blocking_start_callback(const struct device *dev,
				    clock_control_subsys_t subsys,
				    void *user_data)
{
	ARG_UNUSED(subsys);

	struct k_sem *sem = user_data;

	k_sem_give(sem);
}

static void onoff_stop(struct onoff_manager *mgr,
			onoff_notify_fn notify)
{
	int res;

	res = stop(CLOCK_DEVICE_HFCLK192M, CTX_ONOFF);
	notify(mgr, res);
}

static void onoff_started_callback(const struct device *dev,
				   clock_control_subsys_t sys,
				   void *user_data)
{
	ARG_UNUSED(sys);

	onoff_notify_fn notify = user_data;

	notify(&((hfclk192m_data_t *)dev->data)->mgr, 0);
}

static void onoff_start(struct onoff_manager *mgr,
			onoff_notify_fn notify)
{
	int err;

	err = async_start(CLOCK_DEVICE_HFCLK192M, onoff_started_callback, notify, CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void clock_event_handler(void)
{
	const struct device *dev = CLOCK_DEVICE_HFCLK192M;

	clkstarted_handle(dev);
}

static int api_start(const struct device *dev, clock_control_subsys_t subsys,
		     clock_control_cb_t cb, void *user_data)
{
	ARG_UNUSED(subsys);

	return async_start(dev, cb, user_data, CTX_API);
}

static int api_blocking_start(const struct device *dev,
			      clock_control_subsys_t subsys)
{
	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);
	int err;

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return -ENOTSUP;
	}

	err = api_start(dev, subsys, blocking_start_callback, &sem);
	if (err < 0) {
		return err;
	}

	return k_sem_take(&sem, K_MSEC(500));
}

static int api_stop(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);

	return stop(dev, CTX_API);
}

static enum clock_control_status api_get_status(const struct device *dev,
					    clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);

	return GET_STATUS(((hfclk192m_data_t *)dev->data)->flags);
}

static int api_request(const struct device *dev,
		       const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	hfclk192m_data_t *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_request(&dev_data->mgr, cli);
}

static int api_release(const struct device *dev,
		       const struct nrf_clock_spec *spec)
{
	hfclk192m_data_t *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_release(&dev_data->mgr);
}

static int api_cancel_or_release(const struct device *dev,
				 const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	hfclk192m_data_t *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_cancel_or_release(&dev_data->mgr, cli);
}

static int clk_init(const struct device *dev)
{
	nrfx_err_t nrfx_err;
	int err;
	static const struct onoff_transitions transitions = {
		.start = onoff_start,
		.stop = onoff_stop
	};

	clock_control_nrf_common_connect_irq();

	nrfx_err = nrfx_clock_hfclk192m_init(clock_event_handler);
	if (nrfx_err != NRFX_SUCCESS) {
		return -EIO;
	}

	err = onoff_manager_init(&((hfclk192m_data_t *)dev->data)->mgr,
					&transitions);
	if (err < 0) {
		return err;
	}

	((hfclk192m_data_t *)dev->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(clock_control_nrf_hfclk192m,
					&nrfx_clock_hfclk192m_irq_handler);

static DEVICE_API(nrf_clock_control, clock_control_api) = {
	.std_api = {
		.on = api_blocking_start,
		.off = api_stop,
		.async_on = api_start,
		.get_status = api_get_status,
	},
	.request = api_request,
	.release = api_release,
	.cancel_or_release = api_cancel_or_release,
};

static hfclk192m_data_t data;

static const hfclk192m_config_t config = {
	.start = hfclk192m_start,
	.stop = hfclk192m_stop,
	IF_ENABLED(CONFIG_LOG, (.name = "hfclk192m",))
};

DEVICE_DT_DEFINE(DT_NODELABEL(hfclk192m), clk_init, NULL,
		 &data, &config,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_api);
