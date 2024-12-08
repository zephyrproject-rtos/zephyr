/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing FW load functions for Zephyr.
 */
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

#include <fmac_main.h>
static const char fw_patch[] = {
	#include <nrf70_fw_patch/nrf70.bin.inc>
};

enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_fw_info fw_info = { 0 };

	status = nrf_wifi_fmac_fw_parse(rpu_ctx, fw_patch, sizeof(fw_patch), &fw_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_parse failed", __func__);
		return status;
	}
	/* Load the FW patches to the RPU */
	status = nrf_wifi_fmac_fw_load(rpu_ctx, &fw_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_load failed", __func__);
	}

	return status;
}
