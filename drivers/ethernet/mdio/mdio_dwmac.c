/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwmac_mdio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include "../eth_dwmac_priv.h"

LOG_MODULE_REGISTER(snps_dwmac_mdio, CONFIG_MDIO_LOG_LEVEL);

#define DWMAC_MDIO_OP_WRITE MAC_MDIO_ADDRESS_GOC_0
#define DWMAC_MDIO_OP_READ  (MAC_MDIO_ADDRESS_GOC_1 | MAC_MDIO_ADDRESS_GOC_0)

#define DWMAC_MDIO_PINCTRL_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(pinctrl_0)

struct dwmac_mdio_config {
	const struct device *mac_dev;
	uint32_t mdc_frequency;
	uint8_t csr_clk_indx;
	bool suppress_preamble;
#if DWMAC_MDIO_PINCTRL_ENABLED
	const struct pinctrl_dev_config *pincfg;
#endif
};

struct dwmac_mdio_data {
	struct k_mutex lock;
	uint8_t csr_clk_indx;
};

struct dwmac_mdio_clk_div {
	uint16_t divisor;
	uint8_t csr_clk_indx;
};

/* IEEE 802.3-compliant MDC range (up to 2.5 MHz). */
static const struct dwmac_mdio_clk_div dwmac_mdio_std_clk_divs[] = {
	{ 42U, 0U },
	{ 62U, 1U },
	{ 16U, 2U },
	{ 26U, 3U },
	{ 102U, 4U },
	{ 124U, 5U },
	{ 204U, 6U },
	{ 324U, 7U },
};

/* Fast MDC range (above 2.5 MHz) for PHYs that support it. */
static const struct dwmac_mdio_clk_div dwmac_mdio_fast_clk_divs[] = {
	{ 4U, 8U },
	{ 6U, 9U },
	{ 8U, 10U },
	{ 10U, 11U },
	{ 12U, 12U },
	{ 14U, 13U },
	{ 16U, 14U },
	{ 18U, 15U },
};

#define DWMAC_MDIO_MDC_IEEE_LIMIT_HZ 2500000U

static int dwmac_mdio_wait_idle(mm_reg_t base)
{
	bool ready;

	ready = WAIT_FOR(!(sys_read32(base + MAC_MDIO_ADDRESS) & MAC_MDIO_ADDRESS_GOC_GB),
			 CONFIG_MDIO_DWMAC_BUSY_CHECK_TIMEOUT,
			 k_busy_wait(1));
	if (!ready) {
		return -ETIMEDOUT;
	}

	return 0;
}

static bool dwmac_mdio_csr_clk_idx_from_rate(const struct dwmac_mdio_clk_div *divs,
					     size_t divs_count, uint32_t clock_rate,
					     uint32_t mdc_frequency, uint8_t *csr_clk_indx)
{
	uint32_t best_mdc = 0U;
	uint8_t best_idx = UINT8_MAX;

	if (mdc_frequency == 0U) {
		return false;
	}

	for (size_t i = 0; i < divs_count; i++) {
		uint32_t mdc = clock_rate / divs[i].divisor;

		if ((mdc <= mdc_frequency) && (mdc > best_mdc)) {
			best_mdc = mdc;
			best_idx = divs[i].csr_clk_indx;
		}
	}

	if (best_idx != UINT8_MAX) {
		*csr_clk_indx = best_idx;
		return true;
	}

	/* Fallback: use slowest divider when no exact match found */
	*csr_clk_indx = divs[divs_count - 1].csr_clk_indx;

	return false;
}

static int dwmac_mdio_resolve_csr_clk_idx(const struct dwmac_mdio_config *cfg,
					  uint8_t *csr_clk_indx)
{
	const struct dwmac_config *mac_cfg = cfg->mac_dev->config;
	uint32_t clock_rate;
	size_t divs_count;
	int ret;

	if (cfg->csr_clk_indx != UINT8_MAX) {
		*csr_clk_indx = cfg->csr_clk_indx;
		return 0;
	}

	if (mac_cfg->clock == NULL) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(mac_cfg->clock, mac_cfg->mac_clk, &clock_rate);
	if (ret < 0) {
		LOG_ERR("Failed to get MAC CSR clock rate (%d)", ret);
		return ret;
	}

	if ((cfg->mdc_frequency > DWMAC_MDIO_MDC_IEEE_LIMIT_HZ) &&
	    dwmac_mdio_csr_clk_idx_from_rate(dwmac_mdio_fast_clk_divs,
					     ARRAY_SIZE(dwmac_mdio_fast_clk_divs), clock_rate,
					     cfg->mdc_frequency, csr_clk_indx)) {
		return 0;
	}

	/* For v4.21+, include CR 6/7; otherwise exclude them */
	divs_count = (FIELD_GET(MAC_VERSION_SNPSVER,
				sys_read32(DEVICE_MMIO_GET(cfg->mac_dev) + MAC_VERSION)) > 0x42)
			     ? ARRAY_SIZE(dwmac_mdio_std_clk_divs)
			     : ARRAY_SIZE(dwmac_mdio_std_clk_divs) - 2;

	if (dwmac_mdio_csr_clk_idx_from_rate(dwmac_mdio_std_clk_divs, divs_count, clock_rate,
					     cfg->mdc_frequency, csr_clk_indx)) {
		return 0;
	}

	LOG_WRN("Failed to find exact CSR clock index match for requested MDC frequency %u Hz; "
		"using closest %u Hz, CSR clock index %u",
		cfg->mdc_frequency, clock_rate / dwmac_mdio_std_clk_divs[divs_count - 1].divisor,
		dwmac_mdio_std_clk_divs[divs_count - 1].csr_clk_indx);

	return 0;
}

static uint32_t dwmac_mdio_make_addr(const struct dwmac_mdio_config *cfg,
				     const struct dwmac_mdio_data *data, uint8_t prtad,
				     uint8_t regad, uint32_t op, bool c45)
{
	uint32_t addr = FIELD_PREP(MAC_MDIO_ADDRESS_PA, prtad) |
			FIELD_PREP(MAC_MDIO_ADDRESS_RDA, regad) |
			FIELD_PREP(MAC_MDIO_ADDRESS_CR, data->csr_clk_indx) |
			op |
			MAC_MDIO_ADDRESS_GOC_GB;

	if (c45) {
		addr |= MAC_MDIO_ADDRESS_GOC_C45E;
	}

	if (cfg->suppress_preamble) {
		addr |= MAC_MDIO_ADDRESS_PSE;
	}

	return addr;
}

static int dwmac_mdio_transfer(const struct device *dev, uint8_t prtad, uint8_t regad, uint32_t op,
			       uint32_t data_in, uint16_t *data_out, bool c45)
{
	const struct dwmac_mdio_config *cfg = dev->config;
	struct dwmac_mdio_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(cfg->mac_dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = dwmac_mdio_wait_idle(base);
	if (ret < 0) {
		goto end;
	}

	sys_write32(data_in, base + MAC_MDIO_DATA);
	sys_write32(dwmac_mdio_make_addr(cfg, data, prtad, regad, op, c45),
		    base + MAC_MDIO_ADDRESS);

	ret = dwmac_mdio_wait_idle(base);
	if ((ret == 0) && (data_out != NULL)) {
		*data_out = FIELD_GET(MAC_MDIO_DATA_GD, sys_read32(base + MAC_MDIO_DATA));
	}

end:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int dwmac_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	return dwmac_mdio_transfer(dev, prtad, regad, DWMAC_MDIO_OP_READ, 0U, data, false);
}

static int dwmac_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	return dwmac_mdio_transfer(dev, prtad, regad, DWMAC_MDIO_OP_WRITE,
				   FIELD_PREP(MAC_MDIO_DATA_GD, data), NULL, false);
}

static int dwmac_mdio_read_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t *data)
{
	int ret;

	ret = dwmac_mdio_transfer(dev, prtad, devad, DWMAC_MDIO_OP_READ,
				  FIELD_PREP(MAC_MDIO_DATA_RA, regad), NULL, true);
	if (ret == 0) {
		ret = dwmac_mdio_transfer(dev, prtad, devad, DWMAC_MDIO_OP_READ, 0U, data, true);
	}

	return ret;
}

static int dwmac_mdio_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				uint16_t regad, uint16_t data)
{
	int ret;

	ret = dwmac_mdio_transfer(dev, prtad, devad, DWMAC_MDIO_OP_WRITE,
				  FIELD_PREP(MAC_MDIO_DATA_RA, regad), NULL, true);
	if (ret == 0) {
		ret = dwmac_mdio_transfer(dev, prtad, devad, DWMAC_MDIO_OP_WRITE,
					  FIELD_PREP(MAC_MDIO_DATA_GD, data), NULL, true);
	}

	return ret;
}

static int dwmac_mdio_init(const struct device *dev)
{
	const struct dwmac_mdio_config *cfg = dev->config;
	struct dwmac_mdio_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->mac_dev)) {
		return -ENODEV;
	}

#if DWMAC_MDIO_PINCTRL_ENABLED
	if (cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}
#endif /* DWMAC_MDIO_PINCTRL_ENABLED */

	ret = dwmac_mdio_resolve_csr_clk_idx(cfg, &data->csr_clk_indx);
	if (ret < 0) {
		return ret;
	}

	k_mutex_init(&data->lock);

	return 0;
}

static DEVICE_API(mdio, dwmac_mdio_api) = {
	.read = dwmac_mdio_read,
	.write = dwmac_mdio_write,
	.read_c45 = dwmac_mdio_read_c45,
	.write_c45 = dwmac_mdio_write_c45,
};

#if DWMAC_MDIO_PINCTRL_ENABLED
#define DWMAC_MDIO_PINCTRL_DEFINE(inst)                                                            \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(inst, default), (PINCTRL_DT_INST_DEFINE(inst);))

#define DWMAC_MDIO_PINCTRL_CONFIG(inst)                                                            \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(inst, default), \
		    (.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),))
#else
#define DWMAC_MDIO_PINCTRL_DEFINE(inst)
#define DWMAC_MDIO_PINCTRL_CONFIG(inst)
#endif

#define DWMAC_MDIO_INIT(inst)                                                                      \
	DWMAC_MDIO_PINCTRL_DEFINE(inst)                                                            \
                                                                                                   \
	static const struct dwmac_mdio_config dwmac_mdio_config_##inst = {                         \
		.mac_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.csr_clk_indx = DT_PROP_OR(DT_INST_PARENT(inst), snps_clk_csr, UINT8_MAX),         \
		.mdc_frequency = DT_INST_PROP(inst, clock_frequency),                              \
		.suppress_preamble = DT_INST_PROP(inst, suppress_preamble),                        \
		DWMAC_MDIO_PINCTRL_CONFIG(inst)                                                    \
	};                                                                                         \
                                                                                                   \
	static struct dwmac_mdio_data dwmac_mdio_data_##inst;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dwmac_mdio_init, NULL, &dwmac_mdio_data_##inst,                \
			      &dwmac_mdio_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &dwmac_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(DWMAC_MDIO_INIT)
