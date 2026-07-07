/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Broadcom BCM43xxx SDIO Wi-Fi driver (brcmfmac protocol).
 *
 * WPA2-PSK association + event-driven link state + DHCP
 * autostart on top of 4.5a's net_if + scan. Connect IOCTL sequence
 * (mpc/auth/wsec/wpa_auth/wsec_pmk/WLC_SET_SSID) and chan=1 event
 * parsing (WLC_E_AUTH / ASSOC / LINK / DISASSOC_IND -> wifi_mgmt
 * raise calls) live in brcmfmac_net.c. On link-up: net_if_dormant_off
 * + net_dhcpv4_restart drive the iface to UP and acquire an IP.
 */

#define DT_DRV_COMPAT brcm_bcm43xxx_sdio

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdio.h>

#include "brcmfmac_priv.h"

LOG_MODULE_REGISTER(brcmfmac, CONFIG_WIFI_LOG_LEVEL);

/* WL_REG_ON pulse timing. The chip's CBUCK regulator needs a brief window
 * to fully discharge before re-power, then ~250 ms for the chip's clocks
 * to come up and the SDIO bootloader to reach a state where F0/F1
 * enumerate. Values match the BCM43xxx/CYW43xxx Cypress BSP board
 * bring-up sequences.
 */
#define BRCMFMAC_REG_ON_LOW_DELAY_MS     10
#define BRCMFMAC_REG_ON_HIGH_DELAY_MS    250

/* CLM blob upload chunking. Linux uses 1400 bytes/chunk; we cap at 512
 * because brcmfmac_bcdc_iovar_set caps the payload at 1024 bytes total
 * and the dload header eats 12 of those.
 */
#define BRCMFMAC_CLM_CHUNK_SIZE          512

static int brcmfmac_initialization(struct brcmfmac_data *data)
{
	int ret = brcmfmac_chip_read_id(data);

	if (ret != 0) {
		return ret;
	}
	if (data->chip_id == BRCM_CC_43430_CHIP_ID && data->chip_rev == 1) {
		LOG_INF("  -> matches BCM43430A1");
	}

	ret = brcmfmac_chip_ram_data(data);
	if (ret != 0) {
		return ret;
	}

	ret = brcmfmac_chip_pmu_setup(data);
	if (ret != 0) {
		return ret;
	}

	ret = brcmfmac_chip_erom_scan(data);
	if (ret != 0) {
		return ret;
	}

	ret = brcmfmac_chip_set_passive(data);
	if (ret != 0) {
		return ret;
	}

	ret = brcmfmac_sdio_fw_upload(data);
	if (ret != 0) {
		return ret;
	}

	ret = brcmfmac_sdio_nvram_upload(data);
	if (ret != 0) {
		return ret;
	}

	return brcmfmac_chip_set_active(data);
}

static int brcmfmac_power_on(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, wifi_reg_on_gpios)
	int ret;
	const struct brcmfmac_config *cfg = dev->config;

	/* Check WIFI REG_ON gpio instance */
	if (!device_is_ready(cfg->reg_on.port)) {
		LOG_ERR("power_on: failed to configure reg_on %s pin %d",
			cfg->reg_on.port->name, cfg->reg_on.pin);
		return -EIO;
	}

	/* Configure wifi_reg_on as output  */
	ret = gpio_pin_configure_dt(&cfg->reg_on, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("power_on: failed to configure reg_on %s pin %d, ret=%d",
			cfg->reg_on.port->name, cfg->reg_on.pin, ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&cfg->reg_on, 0);
	if (ret) {
		return ret;
	}

	/* Allow CBUCK regulator to discharge */
	k_msleep(BRCMFMAC_REG_ON_LOW_DELAY_MS);

	/* WIFI power on */
	ret = gpio_pin_set_dt(&cfg->reg_on, 1);
	if (ret) {
		return ret;
	}

	k_msleep(BRCMFMAC_REG_ON_HIGH_DELAY_MS);
#else
	(void)dev;
#endif /* DT_INST_NODE_HAS_PROP(0, reg_on_gpios) */

	return 0;
}

static int brcmfmac_probe_sdio(const struct device *dev)
{
	const struct brcmfmac_config *cfg = dev->config;
	struct brcmfmac_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->sdhc)) {
		LOG_ERR("SDHC parent %s not ready", cfg->sdhc->name);
		return -ENODEV;
	}

	ret = sd_init(cfg->sdhc, &data->card);
	if (ret != 0) {
		LOG_ERR("sd_init failed: %d", ret);
		return ret;
	}
	LOG_INF("sd_init ok: num_io=%u rca=0x%04x bus_width=%u",
		(unsigned int)data->card.num_io,
		data->card.relative_addr,
		data->card.bus_io.bus_width);

	ret = sdio_init_func(&data->card, &data->backplane, SDIO_FUNC_NUM_1);
	if (ret != 0) {
		LOG_ERR("sdio_init_func(F1) failed: %d", ret);
		return ret;
	}
	ret = sdio_enable_func(&data->backplane);
	if (ret != 0) {
		LOG_ERR("sdio_enable_func(F1) failed: %d", ret);
		return ret;
	}
	/* sdio_init_func leaves block_size=0; subsys helper then divides by
	 * zero on the block-mode branch. brcmfmac canonical for F1 is 64.
	 */
	ret = sdio_set_block_size(&data->backplane, 64);
	if (ret != 0) {
		LOG_ERR("sdio_set_block_size(F1, 64) failed: %d", ret);
		return ret;
	}
	LOG_DBG("F1 claimed (max_blk=%u)", data->backplane.cis.max_blk_size);
	return 0;
}

static int brcmfmac_process_clm_blob(struct brcmfmac_data *data)
{
	struct brcmf_dload_data_le *hdr;
	uint8_t buf[sizeof(*hdr) + BRCMFMAC_CLM_CHUNK_SIZE];
	const uint8_t *current = brcmfmac_clm_blob;
	size_t remaining = brcmfmac_clm_blob_len;

	hdr = (struct brcmf_dload_data_le *)buf;
	hdr->flag = BRCMF_DL_CRC_IN_HDR | BRCMF_DL_BEGIN;
	hdr->dload_type = BRCMF_DL_TYPE_CLM;
	hdr->crc = 0;

	while (remaining > 0) {
		size_t transfer = BRCMFMAC_CLM_CHUNK_SIZE;
		int ret;

		if (remaining <= transfer) {
			transfer = remaining;
			hdr->flag |= BRCMF_DL_END;
		}

		memcpy(hdr->data, current, transfer);
		hdr->len = transfer;

		LOG_DBG("sending clm chunk, len=%u", transfer);
		ret = brcmfmac_bcdc_iovar_set(data, "clmload", buf, sizeof(*hdr) + transfer);
		if (ret != 0) {
			LOG_ERR("clm chunk download failed: %d", ret);
			return ret;
		}

		current += transfer;
		remaining -= transfer;
		hdr->flag &= ~BRCMF_DL_BEGIN;
	}

	/* The chip acks each chunk even when it rejects the blob; only
	 * "clmload_status" reports the outcome (mirrors Linux
	 * brcmf_c_process_clm_blob()).
	 */
	uint32_t status = 0;
	int ret = brcmfmac_bcdc_iovar_get(data, "clmload_status",
					  (uint8_t *)&status, sizeof(status));

	if (ret < 0 || status != 0U) {
		LOG_ERR("clmload_status=%u (ret=%d)", status, ret);
		return -EIO;
	}
	LOG_INF("CLM blob loaded (%zu bytes)", brcmfmac_clm_blob_len);

	return 0;
}

static int brcmfmac_init(const struct device *dev)
{
	struct brcmfmac_data *data = dev->data;
	int ret;

	ret = brcmfmac_power_on(dev);
	if (ret != 0) {
		return ret;
	}

	ret = brcmfmac_probe_sdio(dev);
	if (ret != 0) {
		return ret;
	}

	int64_t t0 = k_uptime_get();

	ret = brcmfmac_initialization(data);
	if (ret != 0) {
		LOG_ERR("initialization failed: %d", ret);
		return ret;
	}

	ret = brcmfmac_bcdc_init(data);
	if (ret != 0) {
		LOG_ERR("BCDC init failed: %d", ret);
		return ret;
	}

	/*
	 * CLM blob upload. Trim-on-build (TOB) firmware images ship with the
	 * CLM regulatory database stripped out (e.g. BCM43458F, or the
	 * RPi-Distro 43430A1 image) and keep the radio country-disabled
	 * until one is downloaded; images with a built-in CLM (e.g. 43436s)
	 * don't need it. We upload when a blob is configured in the build
	 * via CONFIG_WIFI_BRCMFMAC_CLM_FILE; an unconfigured build links an
	 * empty array (len==0) and we skip.
	 */
	if (brcmfmac_clm_blob_len > 0) {
		ret = brcmfmac_process_clm_blob(data);
		if (ret != 0) {
			LOG_ERR("CLM upload failed: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("CLM blob upload skipped (no blob configured)");
	}

	int got = brcmfmac_bcdc_iovar_get(data, "cur_etheraddr",
					  data->chip_mac, sizeof(data->chip_mac));
	if (got < (int)sizeof(data->chip_mac)) {
		LOG_ERR("cur_etheraddr read returned %d (want >=6)", got);
		return (got < 0) ? got : -EIO;
	}
	LOG_INF("chip MAC = %02x:%02x:%02x:%02x:%02x:%02x",
		data->chip_mac[0], data->chip_mac[1], data->chip_mac[2],
		data->chip_mac[3], data->chip_mac[4], data->chip_mac[5]);

	/* WLC_UP: bring the MAC layer up. Most write IOCTLs (notably the
	 * "escan" IOVAR) return BCME_NOTUP (-4) until this fires. The
	 * command must carry a 4-byte zero payload, like Linux's
	 * brcmf_fil_cmd_int_set(BRCMF_C_UP, 0): TOB firmware acks a
	 * zero-length WLC_UP but leaves the radio down.
	 */
	const uint32_t up_arg = 0;

	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_UP,
				     (const uint8_t *)&up_arg, sizeof(up_arg));
	if (ret != 0) {
		LOG_ERR("WLC_UP failed: %d", ret);
		return ret;
	}

	/* Read the up state back: a radio without a valid CLM/country acks
	 * WLC_UP but stays down, and every later radio op fails BCME_NOTUP
	 * with no other hint.
	 */
	uint32_t isup = 0;

	ret = brcmfmac_bcdc_query_dcmd(data, BRCMFMAC_WLC_GET_UP, NULL, 0,
				       (uint8_t *)&isup, sizeof(isup));
	if (ret >= 0 && isup == 1U) {
		LOG_INF("WLC_UP ok (isup=1)");
	} else {
		LOG_WRN("WLC_UP acked but radio not up (isup=%u) -- "
			"missing CLM blob or no valid country?", isup);
	}

	/* Optional regulatory override. The firmware boots on the CLM's
	 * default country; boards that need a specific domain set it via
	 * Kconfig. ISO3166 alpha2 with rev=0, matching Linux's
	 * brcmf_use_iso3166_ccode_fallback() handling for this chip
	 * family.
	 */
	if (sizeof(CONFIG_WIFI_BRCMFMAC_COUNTRY) > 1) {
		struct {
			char country_abbrev[4];
			int32_t rev;
			char ccode[4];
		} __packed cspec;

		memset(&cspec, 0, sizeof(cspec));
		strncpy(cspec.country_abbrev, CONFIG_WIFI_BRCMFMAC_COUNTRY, 3);
		strncpy(cspec.ccode, CONFIG_WIFI_BRCMFMAC_COUNTRY, 3);
		cspec.rev = 0;

		ret = brcmfmac_bcdc_iovar_set(data, "country",
					      (const uint8_t *)&cspec,
					      sizeof(cspec));
		if (ret != 0) {
			LOG_WRN("country=%s rejected (%d), staying on CLM "
				"default", CONFIG_WIFI_BRCMFMAC_COUNTRY, ret);
		} else {
			LOG_INF("country set to %s",
				CONFIG_WIFI_BRCMFMAC_COUNTRY);
		}
	}

	/* Enable the events we care about in the chip's event mask. Read
	 * current mask first so we don't clobber chip defaults. Events the
	 * chip leaves disabled by default but we need:
	 *   - WLC_E_ESCAN_RESULT  (scan results stream)
	 *   - WLC_E_AUTH          (auth attempt outcome)
	 *   - WLC_E_ASSOC         (assoc attempt outcome)
	 *   - WLC_E_LINK          (link up/down -- triggers dormant_off + DHCP)
	 *   - WLC_E_DISASSOC_IND  (disconnect notice)
	 *   - WLC_E_SET_SSID      (echo of WLC_SET_SSID outcome)
	 */
	uint8_t event_mask[BRCMFMAC_EVENTING_MASK_LEN] = {0};
	int em_got = brcmfmac_bcdc_iovar_get(data, "event_msgs",
					     event_mask, sizeof(event_mask));
	if (em_got < (int)sizeof(event_mask)) {
		LOG_WRN("event_msgs get returned %d; starting from zero", em_got);
		memset(event_mask, 0, sizeof(event_mask));
	}
#define ENABLE_EVENT(ev) \
	(event_mask[(ev) / 8] |= (uint8_t)(1u << ((ev) % 8)))
	ENABLE_EVENT(WLC_E_ESCAN_RESULT);
	ENABLE_EVENT(WLC_E_AUTH);
	ENABLE_EVENT(WLC_E_ASSOC);
	ENABLE_EVENT(WLC_E_LINK);
	ENABLE_EVENT(WLC_E_DISASSOC_IND);
	ENABLE_EVENT(WLC_E_DEAUTH);
	ENABLE_EVENT(WLC_E_DEAUTH_IND);
	ENABLE_EVENT(WLC_E_AUTH_FAIL);
	ENABLE_EVENT(WLC_E_PSK_SUP);
	ENABLE_EVENT(WLC_E_SET_SSID);
#undef ENABLE_EVENT
	ret = brcmfmac_bcdc_iovar_set(data, "event_msgs",
				      event_mask, sizeof(event_mask));
	if (ret != 0) {
		LOG_ERR("event_msgs set failed: %d", ret);
		return ret;
	}
	LOG_DBG("event_msgs set (escan + auth/assoc/link/disassoc/set_ssid)");

	/* Mirror Linux brcmf_dongle_roam + brcmf_cfg80211_set_power_mgmt
	 * setup. Without these the firmware's defaults left us self-deauthing
	 * after ~10 Mbit/s UDP bursts (chip emitted WLC_E_LINK reason=2
	 * BRCMF_E_REASON_DEAUTH without an inbound deauth frame -- i.e. the
	 * firmware itself decided the AP was unreachable).
	 *
	 * Specifically:
	 *   - roam_off=1 disables firmware-internal roaming (we have one AP)
	 *   - bcn_timeout=4 matches Linux's roam-off default
	 *   - pm=PM_FAST + pm2_sleep_ret=2000 matches Linux's default
	 *     "power save on, but wake immediately on TX activity"
	 *
	 * Best-effort: errors are logged but non-fatal -- the chip still
	 * brings up, just with looser defaults.
	 */
	{
		const uint32_t roam_off    = 1;     /* no firmware-side roaming */
		const uint32_t bcn_timeout = 4;     /* seconds without beacons */
		const uint32_t pm_mode     = 2;     /* PM_FAST */
		const uint32_t pm2_sleep   = 2000;  /* ms */
		const uint32_t allmulti    = 1;     /* pass multicast to host */
		int rret;

		rret = brcmfmac_bcdc_iovar_set(data, "roam_off",
					       (const uint8_t *)&roam_off, 4);
		if (rret != 0) {
			LOG_WRN("roam_off=1 set failed: %d (best-effort)", rret);
		}
		rret = brcmfmac_bcdc_iovar_set(data, "bcn_timeout",
					       (const uint8_t *)&bcn_timeout, 4);
		if (rret != 0) {
			LOG_WRN("bcn_timeout=4 set failed: %d (best-effort)", rret);
		}
		rret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_PM,
					      (const uint8_t *)&pm_mode, 4);
		if (rret != 0) {
			LOG_WRN("WLC_SET_PM=FAST set failed: %d (best-effort)", rret);
		}
		rret = brcmfmac_bcdc_iovar_set(data, "pm2_sleep_ret",
					       (const uint8_t *)&pm2_sleep, 4);
		if (rret != 0) {
			LOG_WRN("pm2_sleep_ret=2000 set failed: %d (best-effort)", rret);
		}
		/* allmulti=1: pass all multicast frames up to the host. The
		 * chip otherwise hardware-filters multicast, so 224.0.0.251
		 * (mDNS) never reaches the Zephyr net stack -- DHCP works only
		 * because it is broadcast. Zephyr's IGMP layer still filters
		 * to the joined groups; this just opens the chip-side gate.
		 */
		rret = brcmfmac_bcdc_iovar_set(data, "allmulti",
					       (const uint8_t *)&allmulti, 4);
		if (rret != 0) {
			LOG_WRN("allmulti=1 set failed: %d (best-effort)", rret);
		}
		LOG_DBG("post-up tuning: roam_off=1 bcn=4s pm=FAST sleep_ret=2000ms allmulti=1");
	}

	data->probed = true;
	LOG_INF("initialization complete in %lld ms; awaiting iface_init",
		(long long)(k_uptime_get() - t0));
	return 0;
}

/* === wifi_mgmt + ethernet_api wiring ====================================== */

static const struct wifi_mgmt_ops brcmfmac_mgmt_ops = {
	.scan         = brcmfmac_mgmt_scan,
	.connect      = brcmfmac_mgmt_connect,
	.disconnect   = brcmfmac_mgmt_disconnect,
	.iface_status = brcmfmac_mgmt_iface_status,
};

static const struct net_wifi_mgmt_offload brcmfmac_api = {
	.wifi_iface.iface_api.init = brcmfmac_iface_init,
	.wifi_iface.send           = brcmfmac_iface_send,
	.wifi_mgmt_api             = &brcmfmac_mgmt_ops,
};

/* Single-instance: the brcmfmac DT binding describes one chip on the
 * SDHC parent. Multi-instance would need a per-inst api + data + config
 * (a la DT_INST_FOREACH_STATUS_OKAY); deferred until we actually need it.
 */
static const struct brcmfmac_config brcmfmac_config_0 = {
	.sdhc          = DEVICE_DT_GET(DT_INST_PARENT(0)),
	.reg_on        = GPIO_DT_SPEC_INST_GET_OR(0, wifi_reg_on_gpios, {0}),
	.firmware_name = DT_INST_PROP_OR(0, firmware_name, ""),
};

static struct brcmfmac_data brcmfmac_data_0;

/* Diagnostic: read the chip's "counters" iovar and log the words we care
 * about for the post-burst tx-credit investigation. Word indices identified
 * by counter-delta sweep (see project memory) and are stable
 * across reboots for the BCM43430A1 7.45.96.s1 firmware on the AP we use.
 *
 *   word 1  = txframe    (frames offered to PHY)
 *   word 2  = txbyte     (bytes offered to PHY)
 *   word 3  = txretrans  (per-frame retransmissions on air)
 *   word 4  = txerror    (frames that errored out -- no ack after retry)
 *   word 14 = (txphyerr / txnobuf -- chip-side TX queue / PHY failure)
 *   word 18 = rxerror
 *   word 20 = rxnobuf    (chip's host-RX FIFO overflow)
 *
 * Not safe from ISR (does a CMD53 round-trip). Call from a thread.
 */
void brcmfmac_counters_dump(const char *label)
{
	static uint8_t buf[1500];
	int got = brcmfmac_bcdc_iovar_get(&brcmfmac_data_0, "counters",
					  buf, sizeof(buf));
	if (got < 0) {
		LOG_ERR("counters[%s]: iovar_get failed: %d",
			label ? label : "?", got);
		return;
	}
	if (got < 84) {
		LOG_ERR("counters[%s]: short read: %d bytes",
			label ? label : "?", got);
		return;
	}

	const uint32_t *w = (const uint32_t *)buf;

	LOG_INF("counters[%s]: txframe=%u txbyte=%u txretrans=%u txerror=%u "
		"w14=%u rxerror=%u rxnobuf=%u (resp=%d B)",
		label ? label : "?",
		w[1], w[2], w[3], w[4], w[14], w[18], w[20], got);
}

/* Public iovar interface -- exposed for outside callers (MP user module,
 * test code) that want to introspect or tweak the chip without reaching
 * into driver-private state. `dev` is the brcmfmac net device, typically
 * obtained via `net_if_get_device(net_if_get_first_wifi())`. Both paths
 * do a CMD53 round-trip via BCDC -- not safe from ISR.
 */
int brcmfmac_iovar_get(const struct device *dev, const char *name,
		       uint8_t *buf, uint16_t len)
{
	return brcmfmac_bcdc_iovar_get(dev->data, name, buf, len);
}

int brcmfmac_iovar_set(const struct device *dev, const char *name,
		       const uint8_t *value, uint16_t value_len)
{
	return brcmfmac_bcdc_iovar_set(dev->data, name, value, value_len);
}

NET_DEVICE_DT_INST_DEFINE(0, brcmfmac_init, NULL,
			  &brcmfmac_data_0, &brcmfmac_config_0,
			  CONFIG_WIFI_INIT_PRIORITY, &brcmfmac_api,
			  ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			  NET_ETH_MTU);

CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
