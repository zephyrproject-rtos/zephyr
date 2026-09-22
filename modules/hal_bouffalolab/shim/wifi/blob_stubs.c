/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Link-time stubs for CONFIG_BUILD_ONLY_NO_BLOBS=y.
 * These satisfy the linker but must never execute at runtime.
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <sys/queue.h>

#include <zephyr/toolchain.h>

#include <wl_api.h>
#include "macsw_config.h"
#include "macsw_plat.h"
#include "mac_types.h"
#include "mac_frame.h"
#include "ieee80211.h"
#include "phy.h"
#include "macsw.h"
#include "wl80211.h"
#include "wl80211_mac.h"

struct wl80211_global_state wl80211_glb;
struct _scan_result_tree wl80211_scan_result;
int8_t wl80211_mac_vif[WL80211_VIF_MAX];

struct wl80211_scan_result_item *_scan_result_tree_RB_MINMAX(struct _scan_result_tree *head,
							     int val)
{
	return NULL;
}

struct wl80211_scan_result_item *_scan_result_tree_RB_NEXT(struct wl80211_scan_result_item *elm)
{
	return NULL;
}

void wl80211_init(void)
{
	/* Stub */
}

int wl80211_cntrl(int cmd, ...)
{
	return 0;
}

int wl80211_mac_chan_config_update(uint8_t channel24G_num, const uint8_t *channel24G_chan,
				   uint8_t channel5G_num, const uint8_t *channel5G_chan)
{
	return 0;
}

int wl80211_mac_clr_gtk(uint8_t vif_idx, uint8_t sta_idx)
{
	return 0;
}

int wl80211_mac_clr_ptk(uint8_t vif_idx, uint8_t sta_idx)
{
	return 0;
}

int wl80211_mac_ctrl_port(uint8_t sta_id, int control_port_open)
{
	return 0;
}

int wl80211_mac_set_key(uint8_t vif_idx, uint8_t sta_idx, uint8_t key_idx, uint8_t *key,
			uint8_t key_len, uint8_t cipher_suite, uint8_t spp, bool pairwise)
{
	return -ENOSYS;
}

int wl80211_mac_set_ps_mode(int enable)
{
	return -ENOSYS;
}

int wl80211_mac_tx(uint8_t vif_type, struct wl80211_tx_header *desc, unsigned int flags,
		   struct iovec *seg, int seg_cnt, void *txdone_cb, void *opaque)
{
	return -ENOSYS;
}

void *ke_msg_alloc(ke_msg_id_t const id, ke_task_id_t const dest_id, ke_task_id_t const src_id,
		   uint16_t const param_len)
{
	return NULL;
}

void ke_msg_send(void const *param_ptr)
{
	/* Stub */
}

int mac_cipher_suite_value(uint32_t cipher_suite)
{
	return 0;
}

uint16_t mac_vif_get_bcn_int(void *macif)
{
	return 0;
}

uint8_t mac_vif_get_sta_ap_id(void *macif)
{
	return 0;
}

void mac_vif_get_sta_status(void *macif, struct mac_addr *bssid, uint16_t *aid, int8_t *rssi)
{
	/* Stub */
}

void *vif_info_get_vif(int index)
{
	return NULL;
}

uint8_t export_stats_get_tx_mcs(void)
{
	return 0;
}

char *export_stats_get_tx_format(void)
{
	return NULL;
}

void phy_get_version(uint32_t *version_1, uint32_t *version_2)
{
	/* Stub */
}

struct wl_cfg_t *wl_cfg_get(uint8_t *rmem)
{
	return NULL;
}

const char *wl_get_version(void)
{
	return "";
}

int8_t wl_init(void)
{
	return 0;
}

void wifi_main(void *param)
{
	/* Stub */
}

void interrupt0_handler(void)
{
	/* Stub */
}
