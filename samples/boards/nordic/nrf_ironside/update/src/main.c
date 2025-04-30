/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/nrf_ironside/update.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
	int err;
	const struct ironside_update_blob *update = (void *)CONFIG_UPDATE_BLOB_ADDRESS;

	err = ironside_update(update);
	LOG_INF("IRONside update retval: 0x%x", err);

	if (err == 0) {
		LOG_HEXDUMP_INF(update->manifest, sizeof(update->manifest), "Update manifest:");
		LOG_HEXDUMP_INF(update->pubkey, sizeof(update->pubkey), "Public key:");
		LOG_HEXDUMP_INF(update->signature, sizeof(update->signature), "Signature:");
		LOG_HEXDUMP_INF(update->firmware, 8, "First 8 bytes of encrypted fw:");
		LOG_INF("Reboot the device to trigger the update");
	} else {
		LOG_ERR("The request to update failed");
	}

	return 0;
}
