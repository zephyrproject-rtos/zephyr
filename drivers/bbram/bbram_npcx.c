/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_bbram

#include <zephyr/drivers/bbram.h>
#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npcx_bbram, CONFIG_BBRAM_LOG_LEVEL);

#include "npcx.h"

#define NPCX_STATUS_IBBR BIT(7)
#define NPCX_STATUS_VSBY BIT(1)
#define NPCX_STATUS_VCC1 BIT(0)

#define DRV_STATUS(dev)                                                        \
	(*((volatile uint8_t *)((const struct bbram_npcx_config *)(dev)->config)->status_reg_addr))

static int get_bit_and_reset(const struct device *dev, int mask)
{
	int result = DRV_STATUS(dev) & mask;

	/*
	 * Clear the bit(s):
	 *   For emulator, write 0 to clear status bit(s).
	 *   For real chip, write 1 to clear status bit(s).
	 */
#ifdef CONFIG_BBRAM_NPCX_EMUL
	DRV_STATUS(dev) &= ~mask;
#else
	DRV_STATUS(dev) = mask;
#endif

	return result;
}

static int bbram_npcx_check_invalid(const struct device *dev)
{
	return get_bit_and_reset(dev, NPCX_STATUS_IBBR);
}

static int bbram_npcx_check_standby_power(const struct device *dev)
{
	return get_bit_and_reset(dev, NPCX_STATUS_VSBY);
}

static int bbram_npcx_check_power(const struct device *dev)
{
	return get_bit_and_reset(dev, NPCX_STATUS_VCC1);
}

static int bbram_npcx_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_npcx_config *config = dev->config;

	*size = config->size;
	return 0;
}

static int bbram_npcx_read(const struct device *dev, size_t offset, size_t size,
			   uint8_t *data)
{
	const struct bbram_npcx_config *config = dev->config;

	if (size < 1 || offset + size > config->size || bbram_npcx_check_invalid(dev)) {
		return -EINVAL;
	}


	bytecpy(data, ((uint8_t *)config->base_addr + offset), size);
	return 0;
}

static int bbram_npcx_write(const struct device *dev, size_t offset, size_t size,
			    const uint8_t *data)
{
	const struct bbram_npcx_config *config = dev->config;

	if (size < 1 || offset + size > config->size || bbram_npcx_check_invalid(dev)) {
		return -EINVAL;
	}

	bytecpy(((uint8_t *)config->base_addr + offset), data, size);
	return 0;
}

static DEVICE_API(bbram, bbram_npcx_driver_api) = {
	.check_invalid = bbram_npcx_check_invalid,
	.check_standby_power = bbram_npcx_check_standby_power,
	.check_power = bbram_npcx_check_power,
	.get_size = bbram_npcx_get_size,
	.read = bbram_npcx_read,
	.write = bbram_npcx_write,
};

#define BBRAM_INIT(inst)                                                                           \
	BBRAM_NPCX_DECL_CONFIG(inst);                                                              \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &bbram_cfg_##inst, PRE_KERNEL_1,             \
			      CONFIG_BBRAM_INIT_PRIORITY, &bbram_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
