/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sff_cmis

#include <errno.h>

#include <zephyr/cmis/cmis.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cmis.h>
#include <zephyr/drivers/i2c.h>

/* CMIS page-select register in Lower Memory; a write selects the upper page. */
#define CMIS_REG_PAGE_SELECT 0x7FU

/* Bytes read from Lower Memory to cover every module-level monitor field. */
#define CMIS_MONITOR_READ_LEN 18U

struct cmis_module_config {
	struct i2c_dt_spec i2c;
};

static int cmis_module_select_page(const struct device *dev, uint8_t page)
{
	const struct cmis_module_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, CMIS_REG_PAGE_SELECT, page);
}

static int cmis_module_read(const struct device *dev, uint8_t offset, uint8_t *buf, size_t len)
{
	const struct cmis_module_config *cfg = dev->config;

	return i2c_write_read_dt(&cfg->i2c, &offset, sizeof(offset), buf, len);
}

static int cmis_module_read_id(const struct device *dev, struct cmis_id *id)
{
	uint8_t page00[CMIS_PAGE_00H_SIZE];
	int ret;

	/* Page 00h is the reset default; select it so a stale page cannot leak. */
	ret = cmis_module_select_page(dev, 0U);
	if (ret != 0) {
		return ret;
	}

	ret = cmis_module_read(dev, 0U, &page00[0], CMIS_PAGE_SIZE);
	if (ret != 0) {
		return ret;
	}

	ret = cmis_module_read(dev, CMIS_PAGE_SIZE, &page00[CMIS_PAGE_SIZE], CMIS_PAGE_SIZE);
	if (ret != 0) {
		return ret;
	}

	return cmis_parse_id(page00, id);
}

static int cmis_module_read_monitors(const struct device *dev, struct cmis_monitors *mon)
{
	uint8_t lower[CMIS_MONITOR_READ_LEN];
	int ret;

	/* The module monitors live in Lower Memory, which is page-independent. */
	ret = cmis_module_read(dev, 0U, lower, sizeof(lower));
	if (ret != 0) {
		return ret;
	}

	return cmis_parse_monitors(lower, mon);
}

static int cmis_module_init(const struct device *dev)
{
	const struct cmis_module_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(cmis, cmis_module_api) = {
	.read_id = cmis_module_read_id,
	.read_monitors = cmis_module_read_monitors,
};

#define CMIS_MODULE_INIT(n)                                                                        \
	static const struct cmis_module_config cmis_module_config_##n = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, cmis_module_init, NULL, NULL, &cmis_module_config_##n,            \
			      POST_KERNEL, CONFIG_SFP_INIT_PRIORITY, &cmis_module_api);

DT_INST_FOREACH_STATUS_OKAY(CMIS_MODULE_INIT)
