/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing internal structures for the offloaded raw TX feature in the driver.
 */

#include "offload_raw_tx/fmac_structs.h"
#include "osal_api.h"

struct nrf_wifi_ctx_zep {
	void *drv_priv_zep;
	void *rpu_ctx;
	uint8_t mac_addr[6];
};


struct nrf_wifi_off_raw_tx_drv_priv {
	struct nrf_wifi_fmac_priv *fmac_priv;
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct nrf_wifi_ctx_zep rpu_ctx_zep;
	struct k_spinlock lock;
};

enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx);
