/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_ironside/boot_report.h>
#include <nrf_ironside/update.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
	int err;
	const struct ironside_update_blob *update = (void *)CONFIG_UPDATE_BLOB_ADDRESS;
	const struct ironside_boot_report *report;

	err = ironside_boot_report_get(&report);
	LOG_INF("ironside_boot_report_get err:  %d", err);
	LOG_INF("version: %d.%d.%d-%s+%d", report->ironside_se_version.major,
		report->ironside_se_version.minor, report->ironside_se_version.patch,
		report->ironside_se_version.extraversion, report->ironside_se_version.seqnum);
	LOG_INF("recovery version: %d.%d.%d-%s+%d", report->ironside_se_version.major,
		report->ironside_se_version.minor, report->ironside_se_version.patch,
		report->ironside_se_version.extraversion, report->ironside_se_version.seqnum);
	LOG_INF("update status:  0x%x", report->ironside_update_status);
	LOG_HEXDUMP_INF((void *)report->random_data, sizeof(report->random_data), "random data");

	err = ironside_update(update);
	LOG_INF("IronSide update retval: 0x%x", err);

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
