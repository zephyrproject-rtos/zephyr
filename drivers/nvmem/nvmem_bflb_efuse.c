/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_efuse

#include <zephyr/kernel.h>
#include <zephyr/drivers/nvmem.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <bflb_soc.h>
#include <hbn_reg.h>
#include <ef_ctrl_reg.h>
#include <extra_defines.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define LOG_LEVEL CONFIG_NVMEM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nvmem_bflb_efuse);

struct nvmem_bflb_efuse_data {
	uint8_t cache[DT_INST_PROP(0, size)];
	bool cached;
	struct nvmem_info live_info;
};

struct nvmem_bflb_efuse_config {
	uintptr_t addr;
	size_t size;
};

static void efuse_bflb_clock_delay_32M_ms(uint32_t ms)
{
	uint32_t count = 0;

	do {
		__asm__ volatile (".rept 32 ; nop ; .endr");
		count++;
	} while (count < ms);
}

static uint32_t efuse_bflb_is_pds_busy(const struct device *dev)
{
	uint32_t tmp;
	const struct nvmem_bflb_efuse_config *config = dev->config;

	tmp = sys_read32(config->addr + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	if (tmp & EF_CTRL_EF_IF_0_BUSY_MSK) {
		return 1;
	}
	return 0;
}

static void efuse_bflb_efuse_read(const struct device *dev)
{
	const struct nvmem_bflb_efuse_config *config = dev->config;
	uint32_t tmp;
	uint32_t *pefuse_start = (uint32_t *)(config->addr);
	uint32_t timeout = 0;

	do {
		efuse_bflb_clock_delay_32M_ms(1);
		timeout++;
	} while (timeout < EF_CTRL_DFT_TIMEOUT_VAL && efuse_bflb_is_pds_busy(dev) > 0);

	/* do a 'ahb clock' setup */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
		| (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
		| (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
		| (EF_CTRL_SAHB_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
#endif
		| (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
		| (0 << EF_CTRL_EF_IF_POR_DIG_POS)
		| (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
		| (0 << EF_CTRL_EF_IF_0_RW_POS)
		| (0 << EF_CTRL_EF_IF_0_TRIG_POS);

	sys_write32(tmp, config->addr + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	clock_bflb_settle();

	/* clear PDS cache registry */
	for (uint32_t i = 0; i < config->size / 4; i++) {
		pefuse_start[i] = 0;
	}

	/* Load efuse region0 */
	/* not ahb clock setup */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
		| (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
		| (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
		| (EF_CTRL_EF_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
#endif
		| (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
		| (0 << EF_CTRL_EF_IF_POR_DIG_POS)
		| (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
		| (0 << EF_CTRL_EF_IF_0_RW_POS)
		| (0 << EF_CTRL_EF_IF_0_TRIG_POS);
	sys_write32(tmp, config->addr + EF_CTRL_EF_IF_CTRL_0_OFFSET);

	/* trigger read */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
		| (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
		| (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
		| (EF_CTRL_EF_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
#endif
		| (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
		| (0 << EF_CTRL_EF_IF_POR_DIG_POS)
		| (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
		| (0 << EF_CTRL_EF_IF_0_RW_POS)
		| (1 << EF_CTRL_EF_IF_0_TRIG_POS);
	sys_write32(tmp, config->addr + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	efuse_bflb_clock_delay_32M_ms(5);

	/* wait for read to complete */
	do {
		efuse_bflb_clock_delay_32M_ms(1);
		tmp = sys_read32(config->addr + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	} while ((tmp & EF_CTRL_EF_IF_0_BUSY_MSK) ||
		!(tmp & EF_CTRL_EF_IF_0_AUTOLOAD_DONE_MSK));

	/* do a 'ahb clock' setup */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
		| (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
		| (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
		| (EF_CTRL_SAHB_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
#endif
		| (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
		| (0 << EF_CTRL_EF_IF_POR_DIG_POS)
		| (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
		| (0 << EF_CTRL_EF_IF_0_RW_POS)
		| (0 << EF_CTRL_EF_IF_0_TRIG_POS);

	sys_write32(tmp, config->addr + EF_CTRL_EF_IF_CTRL_0_OFFSET);
}

static void efuse_bflb_cache(const struct device *dev)
{
	struct nvmem_bflb_efuse_data *data = dev->data;
	const struct nvmem_bflb_efuse_config *config = dev->config;
	uint32_t tmp;
	uint8_t old_clock_root;
	uint32_t key;

	key = irq_lock();

	old_clock_root = clock_bflb_get_root_clock();

	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	clock_bflb_settle();

	efuse_bflb_efuse_read(dev);
	/* reads must be 32-bits aligned AND does not work with the method memcpy uses */
	for (int i = 0; i < config->size / sizeof(uint32_t); i++) {
		tmp = sys_read32(config->addr + i * 4);
		data->cache[i * sizeof(uint32_t) + 3] = (tmp & 0xFF000000U) >> 24;
		data->cache[i * sizeof(uint32_t) + 2] = (tmp & 0x00FF0000U) >> 16;
		data->cache[i * sizeof(uint32_t) + 1] = (tmp & 0x0000FF00U) >> 8;
		data->cache[i * sizeof(uint32_t) + 0] = (tmp & 0x000000FFU);
	}

	clock_bflb_set_root_clock(old_clock_root);
	clock_bflb_settle();
	data->cached = true;

	irq_unlock(key);
}

static int nvmem_bflb_efuse_read(const struct device *dev, off_t offset,
                                 void *buf, size_t len)
{
	struct nvmem_bflb_efuse_data *data = dev->data;
	const struct nvmem_bflb_efuse_config *config = dev->config;

	if (!buf) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	if ((offset + len) > (off_t)config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	if (!data->cached) {
		efuse_bflb_cache(dev);
	}

	memcpy(buf, &data->cache[offset], len);
	return 0;
}

static int nvmem_bflb_efuse_write(const struct device *dev, off_t offset,
				const void *buf, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	/* Efuse is read-only from nvmem API perspective */
	return -EROFS;
}

static size_t nvmem_bflb_efuse_get_size(const struct device *dev)
{
	const struct nvmem_bflb_efuse_config *config = dev->config;
	return config->size;
}

static const struct nvmem_info *nvmem_bflb_efuse_get_info(const struct device *dev)
{
	struct nvmem_bflb_efuse_data *data = dev->data;
	return &data->live_info;
}

static DEVICE_API(nvmem, nvmem_bflb_efuse_api) = {
	.read = nvmem_bflb_efuse_read,
	.write = nvmem_bflb_efuse_write,
	.get_size = nvmem_bflb_efuse_get_size,
	.get_info = nvmem_bflb_efuse_get_info,
};

#define NVMEM_BFLB_EFUSE_DEFINE(inst)								\
	static struct nvmem_bflb_efuse_data _CONCAT(nvmem_bflb_efuse_data, inst) = {		\
		.cached = false,								\
		.cache = {0},									\
		.live_info = {									\
			.type = NVMEM_TYPE_OTP,							\
			.read_only = true,							\
		},										\
	};											\
												\
	static const struct nvmem_bflb_efuse_config _CONCAT(nvmem_bflb_efuse_cfg, inst) = {	\
		.addr = DT_INST_REG_ADDR(inst),							\
		.size = DT_INST_PROP(inst, size),						\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &_CONCAT(nvmem_bflb_efuse_data, inst),		\
			&_CONCAT(nvmem_bflb_efuse_cfg, inst), POST_KERNEL,			\
			CONFIG_NVMEM_MODEL_INIT_PRIORITY, &nvmem_bflb_efuse_api)

DT_INST_FOREACH_STATUS_OKAY(NVMEM_BFLB_EFUSE_DEFINE);
