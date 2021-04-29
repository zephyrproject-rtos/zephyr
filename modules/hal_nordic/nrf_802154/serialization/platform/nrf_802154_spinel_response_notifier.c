/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf_802154_spinel_response_notifier.h"

#include <assert.h>
#include <string.h>

#include <logging/log.h>
#include <zephyr.h>

#include "../spinel_base/spinel.h"
#include "nrf_802154_spinel_log.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME spinel_ipc_backend_rsp_ntf
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* TODO: Current implementation doesn't support reentrancy or multiple awaits. */

/**
 * Valid property IDs are from 0 to SPINEL_MAX_UINT_PACKED (2097151).
 * We use UINT32_MAX which is out of this range to indicate that we are
 * not waiting for any property.
 */
#define AWAITED_PROPERTY_NONE UINT32_MAX

struct spinel_notify_buff_internal {
	nrf_802154_spinel_notify_buff_t buff;
	bool                            free;
};

static K_SEM_DEFINE(notify_sem, 0, 1);
static struct k_mutex await_mutex;

static struct spinel_notify_buff_internal notify_buff;

static spinel_prop_key_t awaited_property = AWAITED_PROPERTY_NONE;

void nrf_802154_spinel_response_notifier_init(void)
{
	notify_buff.free = true;
	k_mutex_init(&await_mutex);
}

void nrf_802154_spinel_response_notifier_lock_before_request(spinel_prop_key_t property)
{
	/*
	 * Only one thread can await response.
	 * TODO: Implement matching responses to requests and allow multiple threads
	 *       to await simulatneously
	 */

	LOG_DBG("Locking response notifier");
	int ret = k_mutex_lock(&await_mutex, K_FOREVER);

	assert(ret == 0);
	(void)ret;

	assert(awaited_property == AWAITED_PROPERTY_NONE);
	awaited_property = property;
}

nrf_802154_spinel_notify_buff_t *nrf_802154_spinel_response_notifier_property_await(
	uint32_t timeout)
{
	nrf_802154_spinel_notify_buff_t *result = NULL;
	int ret;

	k_timeout_t k_timeout;

	if (timeout == 0) {
		k_timeout = K_NO_WAIT;
	} else if (timeout == SPINEL_RESPONSE_NOTIFIER_INF_TIMEOUT) {
		k_timeout = K_FOREVER;
	} else {
		k_timeout = K_MSEC(timeout);
	}

	if (k_sem_take(&notify_sem, k_timeout) == 0) {
		result = &notify_buff.buff;
	} else {
		LOG_ERR("No response within timeout %u", timeout);
	}

	ret = k_mutex_unlock(&await_mutex);
	assert(ret == 0);
	(void)ret;
	LOG_DBG("Unlockling response notifier");

	return result;
}

void nrf_802154_spinel_response_notifier_free(nrf_802154_spinel_notify_buff_t *p_notify)
{
	struct spinel_notify_buff_internal *p_notify_buff_free;

	p_notify_buff_free = CONTAINER_OF(p_notify,
					  struct spinel_notify_buff_internal,
					  buff);

	assert(p_notify_buff_free == &notify_buff);

	p_notify_buff_free->free = true;
}

void nrf_802154_spinel_response_notifier_property_notify(spinel_prop_key_t property,
					      const void       *p_data,
					      size_t            data_len)
{
	if (property == awaited_property) {
		assert(notify_buff.free);

		notify_buff.free = false;
		awaited_property = AWAITED_PROPERTY_NONE;

		assert(data_len <= sizeof(notify_buff.buff.data));

		memcpy(notify_buff.buff.data, p_data, data_len);
		notify_buff.buff.data_len = data_len;

		k_sem_give(&notify_sem);
	} else {
		/* TODO: Determine if this is an error condition. */
		NRF_802154_SPINEL_LOG_RAW("Received property that noone is waiting for\n");
	}
}
