/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_UBX_COMMON_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_UBX_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/modem/ubx.h>

struct gnss_ubx_common_data {
	const struct device *gnss;
	struct gnss_data data;
#if CONFIG_GNSS_SATELLITES
	struct {
		struct gnss_satellite *data;
		size_t size;
	} satellites;
#endif
};

struct gnss_ubx_common_config {
	const struct device *gnss;
	struct {
		struct gnss_satellite *buf;
		size_t size;
	} satellites;
};

void gnss_ubx_common_pvt_callback(struct modem_ubx *ubx, const struct ubx_frame *frame,
				 size_t len, void *user_data);

void gnss_ubx_common_satellite_callback(struct modem_ubx *ubx, const struct ubx_frame *frame,
				       size_t len, void *user_data);

void gnss_ubx_common_init(struct gnss_ubx_common_data *data,
			 const struct gnss_ubx_common_config *config);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_UBX_COMMON_H_ */
