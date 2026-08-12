/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Link-time stubs for CONFIG_BUILD_ONLY_NO_BLOBS=y on BL60x.
 * These satisfy the linker but must never execute at runtime.
 * Without the blobs the --wrap link flags are absent too, so the
 * __real_* symbols the wraps reference are stubbed as well.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <zephyr/toolchain.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <utils_list.h>
#include <ipc_compat.h>
#include <ipc_shared.h>
#include <supplicant_api.h>

/* Place the stub shared env in the same WIFI_RAM section the blob would
 * use so it doesn't count against the main RAM region.
 */
struct ipc_shared_env_tag ipc_shared_env Z_GENERIC_SECTION(SHAREDRAMIPC);

uint32_t ke_env;
uint8_t rxl_cntrl_env[64];
uint8_t vif_info_tab[2 * 1512];

void mac_irq(void)
{
	/* Stub */
}

void bl_irq_handler(void)
{
	/* Stub */
}

void ipc_emb_tx_irq(void)
{
	/* Stub */
}

void ipc_emb_msg_irq(void)
{
	/* Stub */
}

void ipc_emb_cfmback_irq(void)
{
	/* Stub */
}

void txu_cntrl_push(void *pad_txdesc, uint32_t arg1)
{
	ARG_UNUSED(pad_txdesc);
	ARG_UNUSED(arg1);
}

void __real_txl_cntrl_push(void *pad_txdesc, uint32_t arg1)
{
	ARG_UNUSED(pad_txdesc);
	ARG_UNUSED(arg1);
}

int __real_rxu_cntrl_frame_handle(void *frame)
{
	ARG_UNUSED(frame);
	return 0;
}

void __real_hal_machw_gen_handler(void)
{
	/* Stub */
}

int __real_rfc_init(uint32_t xtal)
{
	ARG_UNUSED(xtal);
	return 0;
}

int __real_rf_pri_pm_pwr_avg(int arg)
{
	ARG_UNUSED(arg);
	return 0;
}

void wifi_main(void *arg)
{
	ARG_UNUSED(arg);
}

void ipc_emb_wait(void)
{
	/* Stub */
}

void ipc_emb_notify(void)
{
	/* Stub */
}

int rfc_init(uint32_t xtal)
{
	ARG_UNUSED(xtal);
	return 0;
}

void rf_pri_init_calib_mem(void)
{
	/* Stub */
}

void rfc_config_channel(uint32_t channel_freq)
{
	ARG_UNUSED(channel_freq);
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

void mpif_clk_init(void)
{
	/* Stub */
}

void sysctrl_init(void)
{
	/* Stub */
}

void intc_init(void)
{
	/* Stub */
}

void ipc_emb_init(void)
{
	/* Stub */
}

void bl_init(void)
{
	/* Stub */
}

void bl_pm_ops_register(void)
{
	/* Stub */
}

void bl_sleep_schedule(void)
{
	/* Stub */
}

void ke_evt_schedule(void)
{
	/* Stub */
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
