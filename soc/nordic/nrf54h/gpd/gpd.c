/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/sys/util.h>

#include <nrf/gpd.h>
#include <nrfs_gdpwr.h>
#include <nrfs_backend_ipc_service.h>

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

#define GPD_SERVICE_READY BIT(0)
static atomic_t gpd_service_status = ATOMIC_INIT(0);

static atomic_t fast_active1_cnt = ATOMIC_INIT(0);
static atomic_t slow_active_cnt = ATOMIC_INIT(0);
static atomic_t slow_main_cnt = ATOMIC_INIT(0);

static struct gpd_onoff_manager fast_active1 = {.id = NRF_GPD_FAST_ACTIVE1};
static struct gpd_onoff_manager slow_active = {.id = NRF_GPD_SLOW_ACTIVE};
static struct gpd_onoff_manager slow_main = {.id = NRF_GPD_SLOW_MAIN};

static const struct onoff_transitions transitions =
	ONOFF_TRANSITIONS_INITIALIZER(start, stop, NULL);

static atomic_t* get_cnt(uint8_t id) {
	switch (id) {
	case NRF_GPD_FAST_ACTIVE1:
		return &fast_active1_cnt;
	case NRF_GPD_SLOW_ACTIVE:
		return &slow_active_cnt;
	case NRF_GPD_SLOW_MAIN:
		return &slow_main_cnt;
	default:
		return NULL;
	}
}

static struct gpd_onoff_manager* get_mgr(uint8_t id) {
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

static void evt_handler(nrfs_gdpwr_evt_t const *p_evt, void *context)
{
	struct gpd_onoff_manager* gpd_mgr = context;

	switch (p_evt->type) {
	case NRFS_GDPWR_REQ_APPLIED:
		gpd_mgr->notify(&gpd_mgr->mgr, 0);
		break;
	default:
		gpd_mgr->notify(&gpd_mgr->mgr, -EIO);
		break;
	}
}

static void start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	nrfs_err_t err;
	struct gpd_onoff_manager* gpd_mgr =
		CONTAINER_OF(mgr, struct gpd_onoff_manager, mgr);

	gpd_mgr->notify = notify;

	err = nrfs_gdpwr_power_request(gpd_mgr->id, GDPWR_POWER_REQUEST_SET, gpd_mgr);
	if (err != NRFS_SUCCESS) {
		notify(mgr, -EIO);
	}
}

static void stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	nrfs_err_t err;
	struct gpd_onoff_manager* gpd_mgr =
		CONTAINER_OF(mgr, struct gpd_onoff_manager, mgr);

	gpd_mgr->notify = notify;

	err = nrfs_gdpwr_power_request(gpd_mgr->id, GDPWR_POWER_REQUEST_CLEAR, gpd_mgr);
	if (err != NRFS_SUCCESS) {
		notify(mgr, -EIO);
	}
}

int nrf_gpd_request(uint8_t id)
{
	if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_READY)) {
		int ret;
		struct onoff_client client;
		struct gpd_onoff_manager* gpd_mgr;

		gpd_mgr = get_mgr(id);
		if (gpd_mgr == NULL) {
			return -EINVAL;
		}

		sys_notify_init_spinwait(&client.notify);

		onoff_request(&gpd_mgr->mgr, &client);

		while (sys_notify_fetch_result(&client.notify, &ret) == -EAGAIN) {}

		return ret;
	} else {
		atomic_t *gpd_cnt;

		gpd_cnt = get_cnt(id);
		if (gpd_cnt == NULL) {
			return -EINVAL;
		}

		(void)atomic_inc(gpd_cnt);

		return 0;
	}
}

int nrf_gpd_release(uint8_t id)
{
	if (atomic_test_bit(&gpd_service_status, GPD_SERVICE_READY)) {
		struct gpd_onoff_manager* gpd_mgr;

		gpd_mgr = get_mgr(id);
		if (gpd_mgr == NULL) {
			return -EINVAL;
		}

		return onoff_release(&gpd_mgr->mgr);
	} else {
		atomic_t *gpd_cnt;

		gpd_cnt = get_cnt(id);
		if (gpd_cnt == NULL) {
			return -EINVAL;
		}

		(void)atomic_dec(gpd_cnt);

		return 0;
	}
}

static int nrf_gpd_init(void)
{
	nrfs_err_t err;
	int ret;

	ret = nrfs_backend_wait_for_connection(K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	err = nrfs_gdpwr_init(evt_handler);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

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

	/* flag GPD service as ready */
	atomic_set_bit(&gpd_service_status, GPD_SERVICE_READY);

	/* release any domain that was not left active during the boot process */
	if (atomic_get(&fast_active1_cnt) <= 0) {
		ret = nrf_gpd_release(NRF_GPD_FAST_ACTIVE1);
		if (ret < 0) {
			return ret;
		}
	}

	if (atomic_get(&slow_active_cnt) <= 0) {
		ret = nrf_gpd_release(NRF_GPD_SLOW_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if (atomic_get(&slow_main_cnt) <= 0) {
		ret = nrf_gpd_release(NRF_GPD_SLOW_MAIN);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

SYS_INIT(nrf_gpd_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
