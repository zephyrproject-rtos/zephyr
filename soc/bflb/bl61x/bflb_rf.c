/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/otp.h>

#include <bflb_soc.h>
#include <wl_api.h>
#include <rfparam/rfparam_adapter.h>

LOG_MODULE_REGISTER(bflb_rf, LOG_LEVEL_ERR);

#define XTAL_FREQ DT_PROP(DT_NODELABEL(clk_crystal), clock_frequency)

#define AON_BASE_ADDR       0x2000f000U
#define AON_XTAL_CFG_OFFSET 0x884U
#define AON_XTAL_CFG_ADDR   (AON_BASE_ADDR + AON_XTAL_CFG_OFFSET)
#define CAPCODE_IN_POS      22U
#define CAPCODE_OUT_POS     16U
#define CAPCODE_MSK         0x3fU

#define BFLB_EFUSE_DEFAULT_TRIM 0x80U
#define TEMPERATURE_MP_DEFAULT  35

struct efuse_trim_desc {
	uint16_t en_addr;
	uint16_t parity_addr;
	uint16_t value_addr;
	uint8_t value_len;
};

static const struct efuse_trim_desc trim_dcdc = {
	.en_addr = 0x78 * 8 + 31,
	.parity_addr = 0x78 * 8 + 30,
	.value_addr = 0x78 * 8 + 26,
	.value_len = 4,
};
static const struct efuse_trim_desc trim_icx = {
	.en_addr = 0x74 * 8 + 29,
	.parity_addr = 0x74 * 8 + 28,
	.value_addr = 0x74 * 8 + 22,
	.value_len = 6,
};
static const struct efuse_trim_desc trim_iptat = {
	.en_addr = 0x74 * 8 + 31,
	.parity_addr = 0x74 * 8 + 30,
	.value_addr = 0x68 * 8 + 22,
	.value_len = 5,
};
static const struct efuse_trim_desc trim_tmp_mp0 = {
	.en_addr = 0xD8 * 8 + 9,
	.parity_addr = 0xD8 * 8 + 8,
	.value_addr = 0xD8 * 8 + 0,
	.value_len = 8,
};
static const struct efuse_trim_desc trim_tmp_mp1 = {
	.en_addr = 0xD8 * 8 + 19,
	.parity_addr = 0xD8 * 8 + 18,
	.value_addr = 0xD8 * 8 + 10,
	.value_len = 8,
};
static const struct efuse_trim_desc trim_tmp_mp2 = {
	.en_addr = 0xD8 * 8 + 29,
	.parity_addr = 0xD8 * 8 + 28,
	.value_addr = 0xD8 * 8 + 20,
	.value_len = 8,
};

static int efuse_read_trim(const struct device *efuse, const struct efuse_trim_desc *td,
			   uint32_t *value)
{
	uint32_t reg;
	uint32_t en_byte = td->en_addr / 32 * 4;
	uint32_t en_bit = td->en_addr % 32;
	uint32_t par_byte = td->parity_addr / 32 * 4;
	uint32_t par_bit = td->parity_addr % 32;
	uint32_t val_start = td->value_addr;
	uint32_t val = 0;
	uint8_t en, parity;

	if (otp_read(efuse, en_byte, &reg, sizeof(uint32_t)) < 0) {
		return -EIO;
	}
	en = (reg >> en_bit) & 1U;
	if (en == 0) {
		return -ENODATA;
	}

	if (otp_read(efuse, par_byte, &reg, sizeof(uint32_t)) < 0) {
		return -EIO;
	}
	parity = (reg >> par_bit) & 1U;

	for (int i = 0; i < td->value_len; i++) {
		uint32_t bit_addr = val_start + i;
		uint32_t byte_off = bit_addr / 32 * 4;
		uint32_t bit_pos = bit_addr % 32;

		if (otp_read(efuse, byte_off, &reg, sizeof(uint32_t)) < 0) {
			return -EIO;
		}
		val |= ((reg >> bit_pos) & 1U) << i;
	}

	if ((__builtin_popcount(val) & 1U) != parity) {
		return -EINVAL;
	}

	*value = val;
	return 0;
}

static uint8_t wl_rmem_buf[CONFIG_BFLB_BL61X_WL_RMEM_SIZE] __aligned(4);

static void capcode_get(uint8_t *capcode_in, uint8_t *capcode_out)
{
	uint32_t val = sys_read32(AON_XTAL_CFG_ADDR);

	*capcode_in = (val >> CAPCODE_IN_POS) & CAPCODE_MSK;
	*capcode_out = (val >> CAPCODE_OUT_POS) & CAPCODE_MSK;
}

static void capcode_set(uint8_t capcode_in, uint8_t capcode_out)
{
	uint32_t val = sys_read32(AON_XTAL_CFG_ADDR);

	val &= ~((CAPCODE_MSK << CAPCODE_IN_POS) | (CAPCODE_MSK << CAPCODE_OUT_POS));
	val |= ((uint32_t)(capcode_in & CAPCODE_MSK) << CAPCODE_IN_POS) |
	       ((uint32_t)(capcode_out & CAPCODE_MSK) << CAPCODE_OUT_POS);
	sys_write32(val, AON_XTAL_CFG_ADDR);
}

#define WIFI_NODE DT_INST(0, bflb_bl61x_wifi)
#define BT_NODE   DT_INST(0, bflb_bt_hci)

#define DT_COPY_I8(node, prop, dst, n)                                                             \
	do {                                                                                       \
		static const int32_t _v[] = {                                                      \
			DT_FOREACH_PROP_ELEM_SEP(node, prop,              \
				DT_PROP_BY_IDX, (,))};                   \
		BUILD_ASSERT(ARRAY_SIZE(_v) == (n), #prop " must have " #n " entries");            \
		for (int _i = 0; _i < (n); _i++) {                                                 \
			(dst)[_i] = (int8_t)_v[_i];                                                \
		}                                                                                  \
	} while (0)

#define DT_COPY_I16(node, prop, dst, n)                                                            \
	do {                                                                                       \
		static const int32_t _v[] = {                                                      \
			DT_FOREACH_PROP_ELEM_SEP(node, prop,              \
				DT_PROP_BY_IDX, (,))};                   \
		BUILD_ASSERT(ARRAY_SIZE(_v) == (n), #prop " must have " #n " entries");            \
		for (int _i = 0; _i < (n); _i++) {                                                 \
			(dst)[_i] = (int16_t)_v[_i];                                               \
		}                                                                                  \
	} while (0)

/* pwr-offset DTS encoding: 16 = 0 dBm, step 0.25 dBm */
#define DT_COPY_PWR_OFFSET(node, prop, dst, n)                                                     \
	do {                                                                                       \
		static const int32_t _v[] = {                                                      \
			DT_FOREACH_PROP_ELEM_SEP(node, prop,              \
				DT_PROP_BY_IDX, (,))};                   \
		BUILD_ASSERT(ARRAY_SIZE(_v) == (n), #prop " must have " #n " entries");            \
		for (int _i = 0; _i < (n); _i++) {                                                 \
			(dst)[_i] = (int8_t)(_v[_i] - 16);                                         \
		}                                                                                  \
	} while (0)

int8_t rfparam_load(struct wl_param_t *param)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	static const struct efuse_trim_desc *const tmp_mp_descs[] = {&trim_tmp_mp2, &trim_tmp_mp1,
								     &trim_tmp_mp0};
	uint32_t val;
	int i;

	if (param == NULL) {
		return RFPARAM_ERR_PARAM_CHECK;
	}

#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11b)
	DT_COPY_I8(WIFI_NODE, pwr_table_11b, param->pwrtarget.pwr_11b, 4);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11g)
	DT_COPY_I8(WIFI_NODE, pwr_table_11g, param->pwrtarget.pwr_11g, 8);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11n_ht20)
	DT_COPY_I8(WIFI_NODE, pwr_table_11n_ht20, param->pwrtarget.pwr_11n_ht20, 8);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11n_ht40)
	DT_COPY_I8(WIFI_NODE, pwr_table_11n_ht40, param->pwrtarget.pwr_11n_ht40, 8);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ac_vht20)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ac_vht20, param->pwrtarget.pwr_11ac_vht20, 10);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ac_vht40)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ac_vht40, param->pwrtarget.pwr_11ac_vht40, 10);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ax_he20)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ax_he20, param->pwrtarget.pwr_11ax_he20, 12);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ax_he40)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ax_he40, param->pwrtarget.pwr_11ax_he40, 12);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ac_vht80)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ac_vht80, param->pwrtarget.pwr_11ac_vht80, 10);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ax_he80)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ax_he80, param->pwrtarget.pwr_11ax_he80, 12);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_table_11ax_he160)
	DT_COPY_I8(WIFI_NODE, pwr_table_11ax_he160, param->pwrtarget.pwr_11ax_he160, 12);
#endif

#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_offset)
	DT_COPY_PWR_OFFSET(WIFI_NODE, pwr_offset, param->pwrcal.channel_pwrcomp_wlan, 14);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, pwr_offset_lp)
	DT_COPY_PWR_OFFSET(WIFI_NODE, pwr_offset_lp, param->pwrcal.channel_lp_pwrcomp_wlan, 14);
#endif

#if DT_NODE_HAS_PROP(WIFI_NODE, xtal_capcode_in)
	param->xtalcapcode_in = DT_PROP(WIFI_NODE, xtal_capcode_in);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, xtal_capcode_out)
	param->xtalcapcode_out = DT_PROP(WIFI_NODE, xtal_capcode_out);
#endif

#if DT_NODE_HAS_PROP(WIFI_NODE, tcal_linear_or_follow)
	param->tcal.linear_or_follow = DT_PROP(WIFI_NODE, tcal_linear_or_follow);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, tcal_channels)
	DT_COPY_I16(WIFI_NODE, tcal_channels, param->tcal.Tchannels, 5);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, tcal_channel_os)
	DT_COPY_I16(WIFI_NODE, tcal_channel_os, param->tcal.Tchannel_os, 5);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, tcal_channel_os_low)
	DT_COPY_I16(WIFI_NODE, tcal_channel_os_low, param->tcal.Tchannel_os_low, 5);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, tcal_troom_os)
	param->tcal.Troom_os = (int16_t)(DT_PROP(WIFI_NODE, tcal_troom_os) - 256);
#endif
	if (DT_PROP(WIFI_NODE, tcal_enable)) {
		param->tcal.en_tcal = 1;
	}

	if (DT_PROP(WIFI_NODE, tcap_enable)) {
		param->tcap.en_tcap = 1;
	}
#if DT_NODE_HAS_PROP(WIFI_NODE, tcap_tsen)
	DT_COPY_I8(WIFI_NODE, tcap_tsen, param->tcap.tcap_tsen, 10);
#endif
#if DT_NODE_HAS_PROP(WIFI_NODE, tcap_capcode)
	DT_COPY_I8(WIFI_NODE, tcap_capcode, param->tcap.tcap_cap, 11);
#endif

#if DT_NODE_HAS_PROP(BT_NODE, pwr_table_ble)
	param->pwrtarget.pwr_ble = DT_PROP(BT_NODE, pwr_table_ble);
#endif
#if DT_NODE_HAS_PROP(BT_NODE, pwr_table_bt)
	DT_COPY_I8(BT_NODE, pwr_table_bt, param->pwrtarget.pwr_bt, 3);
#endif
#if DT_NODE_HAS_PROP(BT_NODE, pwr_offset_bz)
	DT_COPY_PWR_OFFSET(BT_NODE, pwr_offset_bz, param->pwrcal.channel_pwrcomp_bz, 5);
#endif

	param->pwrcal.Temperature_MP = TEMPERATURE_MP_DEFAULT;
#ifdef CONFIG_BFLB_WIFI_BL61X_DEFAULT_COUNTRY
	param->country_code = CONFIG_BFLB_WIFI_BL61X_DEFAULT_COUNTRY[0] |
			      (CONFIG_BFLB_WIFI_BL61X_DEFAULT_COUNTRY[1] << 8);
#endif

	for (i = 0; i < ARRAY_SIZE(tmp_mp_descs); i++) {
		if (efuse_read_trim(efuse, tmp_mp_descs[i], &val) == 0) {
			param->pwrcal.Temperature_MP = val;
			break;
		}
	}

	return RFPARAM_SUSS;
}

static void efuse_load_trims(struct wl_cfg_t *cfg)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	uint32_t val;

	cfg->param.ef.dcdc_vout_trim_aon = BFLB_EFUSE_DEFAULT_TRIM;
	if (efuse_read_trim(efuse, &trim_dcdc, &val) == 0) {
		cfg->param.ef.dcdc_vout_trim_aon = val;
	}

	cfg->param.ef.icx_code = BFLB_EFUSE_DEFAULT_TRIM;
	if (efuse_read_trim(efuse, &trim_icx, &val) == 0) {
		cfg->param.ef.icx_code = val;
	}

	cfg->param.ef.iptat_code = BFLB_EFUSE_DEFAULT_TRIM;
	if (efuse_read_trim(efuse, &trim_iptat, &val) == 0) {
		cfg->param.ef.iptat_code = val;
	}
}

int bflb_rf_init(void)
{
	static atomic_t initialized;
	struct wl_cfg_t *cfg;
	int ret;

	if (!atomic_cas(&initialized, 0, 1)) {
		return 0;
	}

	__ASSERT(wl_rmem_size_get() <= sizeof(wl_rmem_buf),
		 "WL rmem buffer too small: need %u, have %u", wl_rmem_size_get(),
		 (unsigned int)sizeof(wl_rmem_buf));

	cfg = wl_cfg_get(wl_rmem_buf);
	if (cfg == NULL) {
		LOG_ERR("wl_cfg_get failed");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_WIFI_BFLB) && IS_ENABLED(CONFIG_BT)) {
		cfg->mode = WL_API_MODE_ALL;
	} else if (IS_ENABLED(CONFIG_WIFI_BFLB)) {
		cfg->mode = WL_API_MODE_WLAN;
	} else {
		cfg->mode = WL_API_MODE_BZ;
	}

	cfg->param.xtalfreq_hz = XTAL_FREQ;
	cfg->en_param_load = 1;
	cfg->en_full_cal = 1;
	cfg->en_capcode_set = 1;
	cfg->capcode_get = capcode_get;
	cfg->capcode_set = capcode_set;
	cfg->param_load = rfparam_load;
	cfg->log_level = WL_LOG_LEVEL_NONE;
	cfg->log_printf = NULL;

	efuse_load_trims(cfg);

	ret = wl_init();
	if (ret != 0) {
		LOG_ERR("wl_init failed: %d", ret);
		atomic_set(&initialized, 0);
		return -EIO;
	}

	cfg->en_param_load = 0;
	cfg->en_full_cal = 0;

	LOG_INF("RF calibration complete (mode=%s)", cfg->mode == WL_API_MODE_ALL    ? "WLAN+BZ"
						     : cfg->mode == WL_API_MODE_WLAN ? "WLAN"
										     : "BZ");
	return 0;
}

int mfg_media_read_macaddr_with_lock(uint8_t mac[6], uint8_t reload)
{
	ARG_UNUSED(reload);

	if (hwinfo_get_device_id(mac, 6) != 6) {
		memset(mac, 0, 6);
		return -1;
	}
	return 0;
}

void arch_delay_ms(uint32_t ms)
{
	if (!k_is_in_isr() && !k_is_pre_kernel()) {
		k_msleep(ms);
	} else {
		k_busy_wait((uint32_t)MIN((uint64_t)ms * 1000U, UINT32_MAX));
	}
}

void arch_delay_us(uint32_t us)
{
	k_busy_wait(us);
}
