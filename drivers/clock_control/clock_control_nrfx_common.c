/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_control_nrfx_common.h"
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <nrfx.h>

#if defined(CONFIG_CLOCK_CONTROL_NRFX_COMMON) && !defined(CONFIG_CLOCK_CONTROL_NRF)

#define COMMON_CTX_MASK  (COMMON_CTX_ONOFF | COMMON_CTX_API)
#define COMMON_GET_CTX(flags)    (flags & COMMON_CTX_MASK)

#if IS_ENABLED(CONFIG_NRFX_POWER)
#include <nrfx_power.h>
#endif

#define DT_DRV_COMPAT nordic_nrf_clock

/* Structure used for synchronous clock request. */
struct sync_req {
	struct onoff_client cli;
	struct k_sem sem;
	int res;
};

static bool irq_connected;

/* This function should be treated as static.
 * static keyword is not used so that it can be accessed by interrupt oriented tests.
 */
void clock_control_nrfx_common_irq_handler(void)
{
#if IS_ENABLED(CONFIG_NRFX_POWER)
	nrfx_power_irq_handler();
#endif

	STRUCT_SECTION_FOREACH(clock_control_nrfx_irq_handler, irq) {
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
	IRQ_CONNECT(LFRC_IRQn, DT_INST_IRQ(0, priority), nrfx_isr, clock_control_nrfx_common_irq_handler, 0);
	irq_enable(LFRC_IRQn);
#endif

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), nrfx_isr, clock_control_nrfx_common_irq_handler, 0);
	irq_enable(DT_INST_IRQN(0));
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
	int err;

	err = set_starting_state(&((common_clock_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	((common_clock_data_t *)dev->data)->cb = cb;
	((common_clock_data_t *)dev->data)->user_data = user_data;

	((common_clock_config_t *)dev->config)->start();

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
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
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
		nrf_clock_control_cancel_or_release(dev, spec, &req.cli);
		return err;
	}

	return req.res;
}


#endif /* defined(CONFIG_CLOCK_CONTROL_NRFX_COMMON) && !defined(CONFIG_CLOCK_CONTROL_NRF) */
