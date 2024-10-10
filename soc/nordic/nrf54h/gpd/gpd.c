/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <nrf/gpd.h>
#include <nrfs_gdpwr.h>
#include <nrfs_backend_ipc_service.h>

LOG_MODULE_REGISTER(gpd, CONFIG_SOC_LOG_LEVEL);

/* enforce alignment between DT<->nrfs */
BUILD_ASSERT(GDPWR_POWER_DOMAIN_ACTIVE_FAST == NRF_GPD_FAST_ACTIVE1);
BUILD_ASSERT(GDPWR_POWER_DOMAIN_ACTIVE_SLOW == NRF_GPD_SLOW_ACTIVE);
BUILD_ASSERT(GDPWR_POWER_DOMAIN_MAIN_SLOW == NRF_GPD_SLOW_MAIN);

struct gpd_onoff_manager {
	struct onoff_manager mgr;
	onoff_notify_fn notify;
	uint8_t id;
};

static void start(struct onoff_manager *mgr, onoff_notify_fn notify);
static void stop(struct onoff_manager *mgr, onoff_notify_fn notify);

#define GPD_READY_TIMEOUT_MS 1000

#define GPD_SERVICE_READY   BIT(0)
#define GPD_SERVICE_ERROR   BIT(1)
#define GPD_SERVICE_REQ_OK  BIT(2)
#define GPD_SERVICE_REQ_ERR BIT(3)
static atomic_t gpd_service_status = ATOMIC_INIT(0);

static struct gpd_onoff_manager fast_active1 = {.id = NRF_GPD_FAST_ACTIVE1};
static struct gpd_onoff_manager slow_active = {.id = NRF_GPD_SLOW_ACTIVE};
static struct gpd_onoff_manager slow_main = {.id = NRF_GPD_SLOW_MAIN};

static const struct onoff_transitions transitions =
	ONOFF_TRANSITIONS_INITIALIZER(start, stop, NULL);

static struct gpd_onoff_manager *get_mgr(uint8_t id)
{
	switch (id) {
	case NRF_GPD_FAST_ACTIVE1:
		return &fast_active1;
	case NRF_GPD_SLOW_ACTIVE:
		return &slow_active;
	case NRF_GPD_SLOW_MAIN:
		return &slow_main;
	default:
		return NULL;
	}
}

static int nrf_gpd_sync(struct gpd_onoff_manager *gpd_mgr)
{
	int64_t start;
	nrfs_err_t err;
	gdpwr_request_type_t request;

	K_SPINLOCK(&gpd_mgr->mgr.lock) {
		if (gpd_mgr->mgr.refs == 0) {
			request = GDPWR_POWER_REQUEST_CLEAR;
		} else {
			request = GDPWR_POWER_REQUEST_SET;
		}
	}

	atomic_clear_bit(&gpd_service_status, GPD_SERVICE_REQ_ERR);
	atomic_clear_bit(&gpd_service_status, GPD_SERVICE_REQ_OK);

	err = nrfs_gdpwr_power_request(gpd_mgr->id, request, gpd_mgr);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	start = k_uptime_get();
	while (k_uptime_get() - start < GPD_READY_TIMEOUT_MS) {
		if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_REQ_ERR)) {
			return -EIO;
		}

		if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_REQ_OK)) {
			return 0;
		}
	}

	LOG_ERR("nRFs GDPWR request timed out");

	return -ETIMEDOUT;
}

static void evt_handler(nrfs_gdpwr_evt_t const *p_evt, void *context)
{
	if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_READY)) {
		struct gpd_onoff_manager *gpd_mgr = context;

		switch (p_evt->type) {
		case NRFS_GDPWR_REQ_APPLIED:
			gpd_mgr->notify(&gpd_mgr->mgr, 0);
			break;
		default:
			LOG_ERR("nRFs GDPWR request not applied");
			gpd_mgr->notify(&gpd_mgr->mgr, -EIO);
			break;
		}
	} else {
		switch (p_evt->type) {
		case NRFS_GDPWR_REQ_APPLIED:
			atomic_set_bit(&gpd_service_status, GPD_SERVICE_REQ_OK);
			break;
		default:
			LOG_ERR("nRFs GDPWR request not applied");
			atomic_set_bit(&gpd_service_status, GPD_SERVICE_REQ_ERR);
			break;
		}
	}
}

static void start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct gpd_onoff_manager *gpd_mgr = CONTAINER_OF(mgr, struct gpd_onoff_manager, mgr);

	gpd_mgr->notify = notify;

	if (!atomic_test_bit(&gpd_service_status, GPD_SERVICE_READY)) {
		notify(mgr, 0);
	} else {
		nrfs_err_t err;

		err = nrfs_gdpwr_power_request(gpd_mgr->id, GDPWR_POWER_REQUEST_SET, gpd_mgr);
		if (err != NRFS_SUCCESS) {
			LOG_ERR("nRFs GDPWR request failed (%d)", err);
			notify(mgr, -EIO);
		}
	}
}

static void stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct gpd_onoff_manager *gpd_mgr = CONTAINER_OF(mgr, struct gpd_onoff_manager, mgr);

	gpd_mgr->notify = notify;

	if (!atomic_test_bit(&gpd_service_status, GPD_SERVICE_READY)) {
		notify(mgr, 0);
	} else {
		nrfs_err_t err;

		err = nrfs_gdpwr_power_request(gpd_mgr->id, GDPWR_POWER_REQUEST_CLEAR, gpd_mgr);
		if (err != NRFS_SUCCESS) {
			LOG_ERR("nRFs GDPWR request failed (%d)", err);
			notify(mgr, -EIO);
		}
	}
}

int nrf_gpd_request(uint8_t id)
{
	int ret;
	struct onoff_client client;
	struct gpd_onoff_manager *gpd_mgr;

	gpd_mgr = get_mgr(id);
	if (gpd_mgr == NULL) {
		return -EINVAL;
	}

	if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_ERROR)) {
		LOG_ERR("GPD service did not initialize properly");
		return -EIO;
	}

	sys_notify_init_spinwait(&client.notify);

	onoff_request(&gpd_mgr->mgr, &client);

	while (sys_notify_fetch_result(&client.notify, &ret) == -EAGAIN) {
	}

	return ret;
}

int nrf_gpd_release(uint8_t id)
{
	struct gpd_onoff_manager *gpd_mgr;

	gpd_mgr = get_mgr(id);
	if (gpd_mgr == NULL) {
		return -EINVAL;
	}

	if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_ERROR)) {
		LOG_ERR("GPD service did not initialize properly");
		return -EIO;
	}

	return onoff_release(&gpd_mgr->mgr);
}

static int nrf_gpd_pre_init(void)
{
	int ret;

	ret = onoff_manager_init(&fast_active1.mgr, &transitions);
	if (ret < 0) {
		return ret;
	}

	ret = onoff_manager_init(&slow_active.mgr, &transitions);
	if (ret < 0) {
		return ret;
	}

	ret = onoff_manager_init(&slow_main.mgr, &transitions);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int nrf_gpd_post_init(void)
{
	nrfs_err_t err;
	int ret;

	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		ret = -EIO;
		goto err;
	}

	err = nrfs_gdpwr_init(evt_handler);
	if (err != NRFS_SUCCESS) {
		ret = -EIO;
		goto err;
	}

	/* submit GD requests now to align collected statuses */
	ret = nrf_gpd_sync(&fast_active1);
	if (ret < 0) {
		goto err;
	}

	ret = nrf_gpd_sync(&slow_active);
	if (ret < 0) {
		goto err;
	}

	ret = nrf_gpd_sync(&slow_main);
	if (ret < 0) {
		goto err;
	}

	atomic_set_bit(&gpd_service_status, GPD_SERVICE_READY);

	return 0;

err:
	atomic_set_bit(&gpd_service_status, GPD_SERVICE_ERROR);

	return ret;
}

SYS_INIT(nrf_gpd_pre_init, PRE_KERNEL_1, 0);
SYS_INIT(nrf_gpd_post_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
