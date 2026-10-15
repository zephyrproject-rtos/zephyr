/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sff_sff8472

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sfp.h>
#include <zephyr/sff/sff8472.h>

/* Bytes read from each page, covering every field the library decodes. */
#define SFP_SFF8472_A0_READ_LEN 96U
#define SFP_SFF8472_A2_READ_LEN 118U

struct sfp_sff8472_config {
	struct i2c_dt_spec i2c; /* serial ID page (A0h), reg == 0x50 */
	uint16_t diag_addr;     /* diagnostics page (A2h), 0x51 by default */
};

static int sfp_sff8472_read_page(const struct device *dev, uint16_t i2c_addr, uint8_t offset,
				 uint8_t *buf, size_t len)
{
	const struct sfp_sff8472_config *cfg = dev->config;

	return i2c_write_read(cfg->i2c.bus, i2c_addr, &offset, sizeof(offset), buf, len);
}

static int sfp_sff8472_read_id(const struct device *dev, struct sff8472_id *id)
{
	const struct sfp_sff8472_config *cfg = dev->config;
	uint8_t page[SFP_SFF8472_A0_READ_LEN];
	int ret;

	ret = sfp_sff8472_read_page(dev, cfg->i2c.addr, 0U, page, sizeof(page));
	if (ret != 0) {
		return ret;
	}

	return sff8472_parse_id(page, id);
}

static int sfp_sff8472_read_diagnostics(const struct device *dev, struct sff8472_diagnostics *diag)
{
	const struct sfp_sff8472_config *cfg = dev->config;
	uint8_t page[SFP_SFF8472_A2_READ_LEN];
	int ret;

	ret = sfp_sff8472_read_page(dev, cfg->diag_addr, 0U, page, sizeof(page));
	if (ret != 0) {
		return ret;
	}

	return sff8472_parse_diagnostics(page, diag);
}

static int sfp_sff8472_init(const struct device *dev)
{
	const struct sfp_sff8472_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sfp, sfp_sff8472_api) = {
	.read_id = sfp_sff8472_read_id,
	.read_diagnostics = sfp_sff8472_read_diagnostics,
};

#define SFP_SFF8472_INIT(n)                                                                        \
	static const struct sfp_sff8472_config sfp_sff8472_config_##n = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.diag_addr = DT_INST_PROP_OR(n, diagnostics_address, 0x51),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, sfp_sff8472_init, NULL, NULL, &sfp_sff8472_config_##n,            \
			      POST_KERNEL, CONFIG_SFP_INIT_PRIORITY, &sfp_sff8472_api);

DT_INST_FOREACH_STATUS_OKAY(SFP_SFF8472_INIT)
