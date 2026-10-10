/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Link-time stubs for CONFIG_BUILD_ONLY_NO_BLOBS=y on BL808; never executed. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/slist.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <lmac_msg.h>
#include <ipc_shared.h>
#include <supplicant_api.h>

/* Same WIFI_RAM section as the blob's, so it stays out of main RAM. */
struct ipc_shared_env_tag ipc_shared_env Z_GENERIC_SECTION(.fw.SHRAMIPC);

struct sm_connect_req *sm_env[14];
uint8_t sta_info_tab[5 * 368];

void wifi_main(void *arg)
{
	ARG_UNUSED(arg);
}

void mac_irq(void)
{
	/* Stub */
}

void scanu_cached_scanresult_clear(void)
{
	/* Stub */
}

void bl_irq_handler(void)
{
	/* Stub */
}

void ipc_emb_notify(void)
{
	/* Stub */
}

void trpc_update_power_11b(int8_t *pwr)
{
	ARG_UNUSED(pwr);
}

void trpc_update_power_11g(int8_t *pwr)
{
	ARG_UNUSED(pwr);
}

void trpc_update_power_11n(int8_t *pwr)
{
	ARG_UNUSED(pwr);
}

int bl_wifi_register_wpa_cb_internal(const struct wpa_funcs *cb)
{
	ARG_UNUSED(cb);
	return 0;
}

int bl_wifi_set_appie_internal(uint8_t vif_idx, wifi_appie_t type, uint8_t *ie, uint16_t len,
			       bool sta)
{
	ARG_UNUSED(vif_idx);
	ARG_UNUSED(type);
	ARG_UNUSED(ie);
	ARG_UNUSED(len);
	ARG_UNUSED(sta);
	return 0;
}

bool bl_wifi_auth_done_internal(uint8_t sta_idx, uint16_t reason_code)
{
	ARG_UNUSED(sta_idx);
	ARG_UNUSED(reason_code);
	return true;
}

int bl_wifi_set_igtk_internal(uint8_t vif_idx, uint8_t sta_idx, uint16_t key_idx, const uint8_t *pn,
			      const uint8_t *key)
{
	ARG_UNUSED(vif_idx);
	ARG_UNUSED(sta_idx);
	ARG_UNUSED(key_idx);
	ARG_UNUSED(pn);
	ARG_UNUSED(key);
	return 0;
}

int bl_wifi_set_sta_key_internal(uint8_t vif_idx, uint8_t sta_idx, wpa_alg_t alg, int key_idx,
				 int set_tx, uint8_t *seq, size_t seq_len, uint8_t *key,
				 size_t key_len, bool pairwise)
{
	ARG_UNUSED(vif_idx);
	ARG_UNUSED(sta_idx);
	ARG_UNUSED(alg);
	ARG_UNUSED(key_idx);
	ARG_UNUSED(set_tx);
	ARG_UNUSED(seq);
	ARG_UNUSED(seq_len);
	ARG_UNUSED(key);
	ARG_UNUSED(key_len);
	ARG_UNUSED(pairwise);
	return 0;
}
