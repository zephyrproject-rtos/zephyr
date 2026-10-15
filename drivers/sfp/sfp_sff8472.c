/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sff_sfp

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sfp.h>
#include <zephyr/sff/sff8472.h>

/* SFF-8472 fixes the management addresses of every module. */
#define SFP_SFF8472_A0_ADDR 0x50U /* serial ID page */
#define SFP_SFF8472_A2_ADDR 0x51U /* diagnostics page */

/* Bytes read from each page, covering every field the library decodes. */
#define SFP_SFF8472_A0_READ_LEN 96U
#define SFP_SFF8472_A2_READ_LEN 118U

struct sfp_sff8472_config {
	const struct device *i2c;
	struct gpio_dt_spec mod_def0; /* optional module presence input */
};

static int sfp_sff8472_check_present(const struct device *dev)
{
	const struct sfp_sff8472_config *cfg = dev->config;
	int ret;

	if (cfg->mod_def0.port == NULL) {
		return 0;
	}

	ret = gpio_pin_get_dt(&cfg->mod_def0);
	if (ret < 0) {
		return ret;
	}

	return (ret == 1) ? 0 : -ENODEV;
}

static int sfp_sff8472_read(const struct device *dev, uint16_t i2c_addr, uint8_t offset,
			    uint8_t *buf, size_t len)
{
	const struct sfp_sff8472_config *cfg = dev->config;

	return i2c_write_read(cfg->i2c, i2c_addr, &offset, sizeof(offset), buf, len);
}

static int sfp_sff8472_read_id(const struct device *dev, struct sff8472_id *id)
{
	uint8_t page[SFP_SFF8472_A0_READ_LEN];
	int ret;

	ret = sfp_sff8472_check_present(dev);
	if (ret != 0) {
		return ret;
	}

	ret = sfp_sff8472_read(dev, SFP_SFF8472_A0_ADDR, 0U, page, sizeof(page));
	if (ret != 0) {
		return ret;
	}

	return sff8472_parse_id(page, id);
}

static int sfp_sff8472_read_diagnostics(const struct device *dev, struct sff8472_diagnostics *diag)
{
	uint8_t page[SFP_SFF8472_A2_READ_LEN];
	uint8_t diag_mon;
	int ret;

	ret = sfp_sff8472_check_present(dev);
	if (ret != 0) {
		return ret;
	}

	/* The module may have been exchanged since the last call: check the
	 * diagnostic monitoring type every time instead of caching it.
	 */
	ret = sfp_sff8472_read(dev, SFP_SFF8472_A0_ADDR, SFF8472_A0_DIAG_MON_TYPE, &diag_mon,
			       sizeof(diag_mon));
	if (ret != 0) {
		return ret;
	}

	if (((diag_mon & SFF8472_DIAG_MON_DDM_IMPLEMENTED) == 0U) ||
	    ((diag_mon & SFF8472_DIAG_MON_INT_CAL) == 0U) ||
	    ((diag_mon & SFF8472_DIAG_MON_ADDR_CHANGE) != 0U)) {
		return -ENOTSUP;
	}

	ret = sfp_sff8472_read(dev, SFP_SFF8472_A2_ADDR, 0U, page, sizeof(page));
	if (ret != 0) {
		return ret;
	}

	return sff8472_parse_diagnostics(page, diag);
}

static int sfp_sff8472_init(const struct device *dev)
{
	const struct sfp_sff8472_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c)) {
		return -ENODEV;
	}

	if (cfg->mod_def0.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->mod_def0)) {
			return -ENODEV;
		}

		return gpio_pin_configure_dt(&cfg->mod_def0, GPIO_INPUT);
	}

	return 0;
}

static DEVICE_API(sfp, sfp_sff8472_api) = {
	.read_id = sfp_sff8472_read_id,
	.read_diagnostics = sfp_sff8472_read_diagnostics,
};

#define SFP_SFF8472_INIT(n)                                                                        \
	static const struct sfp_sff8472_config sfp_sff8472_config_##n = {                          \
		.i2c = DEVICE_DT_GET(DT_INST_PHANDLE(n, i2c_bus)),                                 \
		.mod_def0 = GPIO_DT_SPEC_INST_GET_OR(n, mod_def0_gpios, {0}),                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, sfp_sff8472_init, NULL, NULL, &sfp_sff8472_config_##n,            \
			      POST_KERNEL, CONFIG_SFP_INIT_PRIORITY, &sfp_sff8472_api);

DT_INST_FOREACH_STATUS_OKAY(SFP_SFF8472_INIT)
