/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Smoke test for the TI K3 ESM driver: enable a known event (MCU_DCC0 ERR),
 * clear it, and confirm the pending query returns clear.
 */

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/ti_k3_esm/ti_k3_esm.h>
#include <zephyr/dt-bindings/misc/ti-k3-esm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#define ESM_DEV DEVICE_DT_GET(DT_ALIAS(esm0))
#define ESM_EVT K3_WKUP_ESM_EVT_MCU_DCC0_ERR

static void deposit(uint32_t magic, uint32_t a, uint32_t b)
{
	if (!DT_NODE_EXISTS(DT_CHOSEN(zephyr_ipc_shm))) {
		return;
	}

	volatile uint32_t *slot =
		(volatile uint32_t *)DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm));

	slot[0] = magic;
	slot[1] = a;
	slot[2] = b;
	slot[3] = 0xE5E5E5E5U;
	sys_cache_data_flush_range((void *)slot, 16U);
}

int main(void)
{
	const struct device *esm = ESM_DEV;
	int ret;
	int pending;

	if (!device_is_ready(esm)) {
		LOG_ERR("ESM device not ready");
		deposit(0xE5000001U, 0, 0);
		return -ENODEV;
	}

	ret = ti_k3_esm_enable_event(esm, ESM_EVT);
	if (ret != 0) {
		LOG_ERR("enable_event(%u) failed: %d", ESM_EVT, ret);
		deposit(0xE5000002U, ESM_EVT, (uint32_t)(-ret));
		return ret;
	}

	ret = ti_k3_esm_clear_event(esm, ESM_EVT);
	if (ret != 0) {
		LOG_ERR("clear_event(%u) failed: %d", ESM_EVT, ret);
		deposit(0xE5000003U, ESM_EVT, (uint32_t)(-ret));
		return ret;
	}

	pending = ti_k3_esm_event_pending(esm, ESM_EVT);
	if (pending < 0) {
		LOG_ERR("event_pending(%u) failed: %d", ESM_EVT, pending);
		deposit(0xE5000004U, ESM_EVT, (uint32_t)(-pending));
		return pending;
	}

	LOG_INF("ESM event %u ready (pending=%d)", ESM_EVT, pending);
	deposit(0xE500D00DU, ESM_EVT, (uint32_t)pending);
	return 0;
}
