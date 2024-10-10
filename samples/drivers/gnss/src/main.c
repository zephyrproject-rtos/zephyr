/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/logging/log.h>

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

LOG_MODULE_REGISTER(gnss_sample, CONFIG_GNSS_LOG_LEVEL);

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		printf("Got a fix!\n");
	}
}
GNSS_DATA_CALLBACK_DEFINE(GNSS_MODEM, gnss_data_cb);

#if CONFIG_GNSS_SATELLITES
static void gnss_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites,
			       uint16_t size)
{
	unsigned int tracked_count = 0;

	for (unsigned int i = 0; i != size; ++i) {
		tracked_count += satellites[i].is_tracked;
	}
	printf("%u satellite%s reported (of which %u tracked)!\n",
		size, size > 1 ? "s" : "", tracked_count);
}
#endif
GNSS_SATELLITES_CALLBACK_DEFINE(GNSS_MODEM, gnss_satellites_cb);

#define GNSS_SYSTEMS_PRINTF(define, supported, enabled)                                            \
	printf("\t%20s: Supported: %3s Enabled: %3s\n",                                            \
	       STRINGIFY(define), (supported & define) ? "Yes" : "No",                             \
			 (enabled & define) ? "Yes" : "No");

int main(void)
{
	gnss_systems_t supported, enabled;
	uint32_t fix_interval;
	int rc;

	rc = gnss_get_supported_systems(GNSS_MODEM, &supported);
	if (rc < 0) {
		printf("Failed to query supported systems (%d)\n", rc);
		return rc;
	}
	rc = gnss_get_enabled_systems(GNSS_MODEM, &enabled);
	if (rc < 0) {
		printf("Failed to query enabled systems (%d)\n", rc);
		return rc;
	}
	printf("GNSS Systems:\n");
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_GPS, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_GLONASS, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_GALILEO, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_BEIDOU, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_QZSS, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_IRNSS, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_SBAS, supported, enabled);
	GNSS_SYSTEMS_PRINTF(GNSS_SYSTEM_IMES, supported, enabled);

	rc = gnss_get_fix_rate(GNSS_MODEM, &fix_interval);
	if (rc < 0) {
		printf("Failed to query fix rate (%d)\n", rc);
		return rc;
	}
	printf("Fix rate = %d ms\n", fix_interval);

	return 0;
}
