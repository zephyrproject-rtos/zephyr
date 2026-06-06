/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * WPA3-SAE glue: the firmware exchanges the SAE Authentication frames
 * itself and calls the wpa3_* hooks for the cryptographic payloads.
 * hostap's sae.c does the math; the resulting PMK is injected into the
 * supplicant so its 4-way handshake can run (the driver advertises
 * WPA_DRIVER_FLAGS2_SAE_OFFLOAD_STA, so hostap skips its own SAE).
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "common/ieee802_11_defs.h"
#include "common/sae.h"

#include "bflb_wifi.h"
#include "bflb_wifi_sae.h"
#include "bflb_wifi_wpa_supp.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define SAE_MSG_COMMIT  1U
#define SAE_MSG_CONFIRM 2U
#define SAE_GROUP_19    19
#define SAE_BUF_MAX     192U

/* From supplicant_api.h (kept out: its enum wpa_alg collides with hostap). */
#define BL_SAE_INVALID_PACKET -8

static struct sae_data sae_ctx;
static uint8_t sae_out[SAE_BUF_MAX];

uint8_t *bflb_wifi_sae_build(uint8_t *bssid, uint8_t *mac, uint8_t *passphrase, uint32_t type,
			     size_t *len)
{
	struct wpabuf *buf;
	int rc;

	if (len == NULL) {
		return NULL;
	}
	*len = 0;

	if (type == SAE_MSG_COMMIT) {
		if (bssid == NULL || mac == NULL || passphrase == NULL || passphrase[0] == '\0') {
			return NULL;
		}
		sae_clear_data(&sae_ctx);
		if (sae_set_group(&sae_ctx, SAE_GROUP_19) != 0) {
			LOG_ERR("SAE group init failed");
			return NULL;
		}
		rc = sae_prepare_commit(mac, bssid, passphrase, strlen((const char *)passphrase),
					&sae_ctx);
		if (rc != 0) {
			LOG_ERR("SAE prepare commit failed: %d", rc);
			return NULL;
		}
	} else if (type == SAE_MSG_CONFIRM) {
		/* Peer commit was handed to bflb_wifi_sae_parse first. */
		if (sae_process_commit(&sae_ctx) != 0) {
			LOG_ERR("SAE process commit failed");
			return NULL;
		}
	} else {
		return NULL;
	}

	buf = wpabuf_alloc(SAE_BUF_MAX);
	if (buf == NULL) {
		return NULL;
	}

	if (type == SAE_MSG_COMMIT) {
		rc = sae_write_commit(&sae_ctx, buf, NULL, NULL);
	} else {
		rc = sae_write_confirm(&sae_ctx, buf);
	}
	if (rc != 0 || wpabuf_len(buf) > sizeof(sae_out)) {
		LOG_ERR("SAE write msg %u failed: %d", type, rc);
		wpabuf_free(buf);
		return NULL;
	}

	*len = wpabuf_len(buf);
	memcpy(sae_out, wpabuf_head(buf), *len);
	wpabuf_free(buf);

	LOG_INF("SAE TX msg=%u len=%zu", type, *len);
	return sae_out;
}

int bflb_wifi_sae_parse(uint8_t *buf, size_t len, uint32_t type, uint16_t status)
{
	uint16_t res;

	LOG_INF("SAE RX msg=%u len=%zu status=%u", type, len, status);

	if (buf == NULL) {
		return BL_SAE_INVALID_PACKET;
	}

	if (type == SAE_MSG_COMMIT) {
		res = sae_parse_commit(&sae_ctx, buf, len, NULL, NULL, NULL, 0, NULL);
		if (res != WLAN_STATUS_SUCCESS) {
			LOG_ERR("SAE parse commit failed: %u", res);
			return BL_SAE_INVALID_PACKET;
		}
		return 0;
	}

	if (type == SAE_MSG_CONFIRM) {
		if (sae_check_confirm(&sae_ctx, buf, len, NULL) != 0) {
			LOG_ERR("SAE confirm check failed");
			return BL_SAE_INVALID_PACKET;
		}
		/* PMK established -- hand it to the supplicant before the
		 * AP's EAPOL msg 1 arrives.
		 */
		bflb_wpa_supp_set_pmk(sae_ctx.pmk, sae_ctx.pmk_len, sae_ctx.pmkid);
		sae_clear_temp_data(&sae_ctx);
		return 0;
	}

	return BL_SAE_INVALID_PACKET;
}

void bflb_wifi_sae_clear(void)
{
	sae_clear_data(&sae_ctx);
}
