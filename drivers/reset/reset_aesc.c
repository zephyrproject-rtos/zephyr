/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_reset_controller

#include <errno.h>
#include <ip_identification.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/reset/aesc.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_reset);

#define RESET_AESC_DOMAINS     0x00
#define RESET_AESC_ENABLE      0x04
#define RESET_AESC_TRIGGER     0x08
#define RESET_AESC_ACKNOWLEDGE 0x0c
#define RESET_AESC_STATUS      0x10

#define RESET_AESC_DOMAINS_MASK GENMASK(7, 0)
#define RESET_AESC_MAX_DOMAINS  32U

/* Upper bound for a toggled domain to leave reset */
#define RESET_AESC_TIMEOUT_US 1000

struct reset_aesc_config {
	DEVICE_MMIO_ROM;
};

struct reset_aesc_data {
	DEVICE_MMIO_RAM;

	uintptr_t reg_base;
	uint32_t valid_mask;
	/* Status register, available since IP version 1.1.0 */
	bool has_status;
	struct k_spinlock lock;
};

int reset_aesc_toggle_mask(const struct device *dev, uint32_t mask)
{
	struct reset_aesc_data *data = dev->data;
	k_spinlock_key_t key;

	if ((mask == 0U) || ((mask & ~data->valid_mask) != 0U)) {
		return -EINVAL;
	}

	/* Each domain is reset for its configured delay once the trigger is acknowledged */
	key = k_spin_lock(&data->lock);
	sys_write32(mask, data->reg_base + RESET_AESC_TRIGGER);
	sys_write32(1U, data->reg_base + RESET_AESC_ACKNOWLEDGE);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int reset_aesc_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	struct reset_aesc_data *data = dev->data;

	if (!data->has_status) {
		return -ENOSYS;
	}

	if ((id >= RESET_AESC_MAX_DOMAINS) || ((BIT(id) & data->valid_mask) == 0U)) {
		return -EINVAL;
	}

	*status = ((sys_read32(data->reg_base + RESET_AESC_STATUS) & BIT(id)) != 0U) ? 1U : 0U;

	return 0;
}

static int reset_aesc_line_toggle(const struct device *dev, uint32_t id)
{
	struct reset_aesc_data *data = dev->data;
	int ret;

	if (id >= RESET_AESC_MAX_DOMAINS) {
		return -EINVAL;
	}

	/* Without the status register, the end of the reset pulse is unknown */
	if (!data->has_status) {
		return -ENOTSUP;
	}

	ret = reset_aesc_toggle_mask(dev, BIT(id));
	if (ret != 0) {
		return ret;
	}

	/* The status bit is set from the request until the domain left reset */
	if (!WAIT_FOR((sys_read32(data->reg_base + RESET_AESC_STATUS) & BIT(id)) == 0U,
		      RESET_AESC_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int reset_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	volatile uintptr_t *base_addr = (volatile uintptr_t *)DEVICE_MMIO_GET(dev);
	struct reset_aesc_data *data = dev->data;
	uint32_t major = ip_id_get_major_version(base_addr);
	uint32_t minor = ip_id_get_minor_version(base_addr);
	uint32_t domains;

	LOG_DBG("IP core version: %u.%u.%u.", major, minor, ip_id_get_patchlevel(base_addr));
	data->has_status = (major > 1U) || ((major == 1U) && (minor >= 1U));
	data->reg_base = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate driver to address 0x%lx.", data->reg_base);

	domains = sys_read32(data->reg_base + RESET_AESC_DOMAINS) & RESET_AESC_DOMAINS_MASK;
	if ((domains == 0U) || (domains > RESET_AESC_MAX_DOMAINS)) {
		LOG_ERR("Unsupported number of reset domains: %u", domains);
		return -ENOTSUP;
	}
	data->valid_mask = (uint32_t)BIT64_MASK(domains);

	return 0;
}

static DEVICE_API(reset, reset_aesc_driver_api) = {
	.status = reset_aesc_status,
	.line_toggle = reset_aesc_line_toggle,
};

#define AESC_RESET_INIT(no)                                                                        \
	static struct reset_aesc_data reset_aesc_dev_data_##no;                                    \
	static const struct reset_aesc_config reset_aesc_dev_cfg_##no = {                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(no)),                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(no, reset_aesc_init, NULL, &reset_aesc_dev_data_##no,                \
			      &reset_aesc_dev_cfg_##no, PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,  \
			      &reset_aesc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AESC_RESET_INIT)
