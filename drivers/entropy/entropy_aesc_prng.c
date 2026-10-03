/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_prng

#include <errno.h>
#include <ip_identification.h>
#include <soc.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_prng, CONFIG_ENTROPY_LOG_LEVEL);

#define PRNG_AESC_CONTROL	0x00
#define PRNG_AESC_ERROR_PENDING	0x04
#define PRNG_AESC_ERROR_MASK	0x08
#define PRNG_AESC_SEED		0x0c
#define PRNG_AESC_OUTPUT	0x10

#define PRNG_AESC_CONTROL_ENABLE	BIT(0)

struct prng_aesc_config {
	DEVICE_MMIO_NAMED_ROM(mmio);
};

struct prng_aesc_data {
	DEVICE_MMIO_NAMED_RAM(mmio);

	uintptr_t reg_base;
};

#define DEV_CFG(dev) ((const struct prng_aesc_config * const)(dev)->config)
#define DEV_DATA(dev) ((struct prng_aesc_data *)(dev)->data)

static void prng_aesc_fill(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	struct prng_aesc_data *data = DEV_DATA(dev);
	uint32_t value;
	size_t to_copy;

	while (length > 0) {
		value = sys_read32(data->reg_base + PRNG_AESC_OUTPUT);
		to_copy = MIN(length, sizeof(value));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}
}

static int prng_aesc_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	prng_aesc_fill(dev, buffer, length);

	return 0;
}

static int prng_aesc_get_entropy_isr(const struct device *dev, uint8_t *buffer,
				     uint16_t length, uint32_t flags)
{
	ARG_UNUSED(flags);

	prng_aesc_fill(dev, buffer, length);

	return length;
}

static int prng_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, mmio, K_MEM_CACHE_NONE);
	volatile uintptr_t *base_addr =
		(volatile uintptr_t *)DEVICE_MMIO_NAMED_GET(dev, mmio);
	struct prng_aesc_data *data = DEV_DATA(dev);

	if (ip_id_get_id(base_addr) != IP_ID_PRNG) {
		LOG_ERR("Unexpected IP core ID %u.", ip_id_get_id(base_addr));
		return -ENODEV;
	}

	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr)
	);
	data->reg_base = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate driver to address 0x%lx.", data->reg_base);

	sys_write32(PRNG_AESC_CONTROL_ENABLE, data->reg_base + PRNG_AESC_CONTROL);

	return 0;
}

static DEVICE_API(entropy, prng_aesc_driver_api) = {
	.get_entropy = prng_aesc_get_entropy,
	.get_entropy_isr = prng_aesc_get_entropy_isr,
};

#define AESC_PRNG_INIT(no)						      \
	static struct prng_aesc_data prng_aesc_dev_data_##no;		      \
	static const struct prng_aesc_config prng_aesc_dev_cfg_##no = {	      \
		DEVICE_MMIO_NAMED_ROM_INIT(mmio, DT_DRV_INST(no)),	      \
	};								      \
	DEVICE_DT_INST_DEFINE(no,					      \
			      prng_aesc_init,				      \
			      NULL,					      \
			      &prng_aesc_dev_data_##no,			      \
			      &prng_aesc_dev_cfg_##no,			      \
			      PRE_KERNEL_1,				      \
			      CONFIG_ENTROPY_INIT_PRIORITY,		      \
			      &prng_aesc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AESC_PRNG_INIT)
