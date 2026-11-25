/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Smoke test for the TI K3 ECC aggregator EDAC driver: probe the device,
 * clear logs, and report correctable / uncorrectable counts (expected 0
 * without injection).
 */

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/edac.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#define EDAC_DEV DEVICE_DT_GET(DT_ALIAS(edac0))

static void deposit(uint32_t magic, uint32_t cor, uint32_t uc)
{
	if (!DT_NODE_EXISTS(DT_CHOSEN(zephyr_ipc_shm))) {
		return;
	}

	volatile uint32_t *slot =
		(volatile uint32_t *)DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm));

	slot[0] = magic;
	slot[1] = cor;
	slot[2] = uc;
	slot[3] = 0xECC0ECC0U;
	sys_cache_data_flush_range((void *)slot, 16U);
}

int main(void)
{
	const struct device *dev = EDAC_DEV;
	int cor;
	int uc;
	uint64_t logv;

	if (!device_is_ready(dev)) {
		LOG_ERR("%s not ready", dev->name);
		deposit(0xECC00001U, 0, 0);
		return -ENODEV;
	}

	(void)edac_ecc_error_log_clear(dev);
	(void)edac_parity_error_log_clear(dev);

	cor = edac_errors_cor_get(dev);
	uc = edac_errors_uc_get(dev);
	LOG_INF("ECC aggregator ready: cor=%d uc=%d", cor, uc);

	if (edac_ecc_error_log_get(dev, &logv) == -ENODATA) {
		LOG_INF("no retained SEC log");
	}
	if (edac_parity_error_log_get(dev, &logv) == -ENODATA) {
		LOG_INF("no retained DED log");
	}

	deposit(0xECC0D00DU, (uint32_t)cor, (uint32_t)uc);
	return (cor < 0 || uc < 0) ? -EIO : 0;
}
