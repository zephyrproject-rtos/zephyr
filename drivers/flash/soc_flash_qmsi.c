/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <init.h>

#include <flash.h>
#include <misc/util.h>
#include "flash_priv.h"

#include "qm_flash.h"
#include "qm_soc_regs.h"

struct soc_flash_data {
#ifdef CONFIG_SOC_FLASH_QMSI_API_REENTRANCY
	struct k_sem sem;
#endif
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_flash_context_t saved_ctx[QM_FLASH_NUM];
#endif
};

#define FLASH_HAS_CONTEXT_DATA \
	(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY || CONFIG_DEVICE_POWER_MANAGEMENT)

#if FLASH_HAS_CONTEXT_DATA
static struct soc_flash_data soc_flash_context;
#define FLASH_CONTEXT (&soc_flash_context)
#else
#define FLASH_CONTEXT (NULL)
#endif /* FLASH_HAS_CONTEXT_DATA */

#ifdef CONFIG_SOC_FLASH_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct soc_flash_data *)(dev->driver_data))->sem)
#else
#define RP_GET(dev) (NULL)
#endif

static inline bool is_aligned_32(u32_t data)
{
	return (data & 0x3) ? false : true;
}

static qm_flash_region_t flash_region(u32_t addr)
{
	if ((addr >= QM_FLASH_REGION_SYS_0_BASE) && (addr <
	    (QM_FLASH_REGION_SYS_0_BASE + CONFIG_SOC_FLASH_QMSI_SYS_SIZE))) {
		return QM_FLASH_REGION_SYS;
	}

#if defined(CONFIG_SOC_QUARK_D2000)
	if ((addr >= QM_FLASH_REGION_DATA_0_BASE) &&
	    (addr < (QM_FLASH_REGION_DATA_0_BASE +
	    QM_FLASH_REGION_DATA_0_SIZE))) {
		return QM_FLASH_REGION_DATA;
	}
#endif

	/* invalid address */
	return QM_FLASH_REGION_NUM;
}

static u32_t get_page_num(u32_t addr)
{
	switch (flash_region(addr)) {
	case QM_FLASH_REGION_SYS:
		return (addr - QM_FLASH_REGION_SYS_0_BASE) >>
		       QM_FLASH_PAGE_SIZE_BITS;
#if defined(CONFIG_SOC_QUARK_D2000)
	case QM_FLASH_REGION_DATA:
		return (addr - QM_FLASH_REGION_DATA_0_BASE) >>
		       QM_FLASH_PAGE_SIZE_BITS;
#endif
	default:
		/* invalid address */
		return 0xffffffff;
	}
}

static int flash_qmsi_read(struct device *dev, off_t addr,
			   void *data, size_t len)
{
	ARG_UNUSED(dev);

	if ((!is_aligned_32(len)) || (!is_aligned_32(addr))) {
		return -EINVAL;
	}

	if (flash_region(addr) == QM_FLASH_REGION_NUM) {
		/* starting address is not within flash */
		return -EIO;
	}

	if (flash_region(addr + len - 4) == QM_FLASH_REGION_NUM) {
		/* data area is not within flash */
		return -EIO;
	}

	for (u32_t i = 0; i < (len >> 2); i++) {
		UNALIGNED_PUT(sys_read32(addr + (i << 2)),
			      (u32_t *)data + i);
	}

	return 0;
}

static int flash_qmsi_write(struct device *dev, off_t addr,
			    const void *data, size_t len)
{
	qm_flash_t flash = QM_FLASH_0;
	qm_flash_region_t reg;
	u32_t data_word = 0, offset = 0, f_addr = 0;

	if ((!is_aligned_32(len)) || (!is_aligned_32(addr))) {
		return -EINVAL;
	}

	reg = flash_region(addr);
	if (reg == QM_FLASH_REGION_NUM) {
		return -EIO;
	}

	if (flash_region(addr + len - 4) == QM_FLASH_REGION_NUM) {
		return -EIO;
	}

	for (u32_t i = 0; i < (len >> 2); i++) {
		data_word = UNALIGNED_GET((u32_t *)data + i);
		reg = flash_region(addr + (i << 2));
		f_addr = addr + (i << 2);

		switch (reg) {
		case QM_FLASH_REGION_SYS:
			offset = f_addr - QM_FLASH_REGION_SYS_0_BASE;
			break;
#if defined(CONFIG_SOC_QUARK_D2000)
		case QM_FLASH_REGION_DATA:
			offset = f_addr - QM_FLASH_REGION_DATA_0_BASE;
			break;
#endif
		default:
			return -EIO;
		}

#if defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
		if (offset >= (CONFIG_SOC_FLASH_QMSI_SYS_SIZE >> 1)) {
			flash = QM_FLASH_1;
			offset -= CONFIG_SOC_FLASH_QMSI_SYS_SIZE >> 1;
		}
#endif

		if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
			k_sem_take(RP_GET(dev), K_FOREVER);
		}

		qm_flash_word_write(flash, reg, offset, data_word);

		if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
			k_sem_give(RP_GET(dev));
		}
	}

	return 0;
}

static int flash_qmsi_erase(struct device *dev, off_t addr, size_t size)
{
	qm_flash_t flash = QM_FLASH_0;
	qm_flash_region_t reg;
	u32_t page = 0;

	/* starting address needs to be a 2KB aligned address */
	if (addr & QM_FLASH_ADDRESS_MASK) {
		return -EINVAL;
	}

	/* size needs to be multiple of 2KB */
	if (size & QM_FLASH_ADDRESS_MASK) {
		return -EINVAL;
	}

	reg = flash_region(addr);
	if (reg == QM_FLASH_REGION_NUM) {
		return -EIO;
	}

	if (flash_region(addr + size - (QM_FLASH_PAGE_SIZE_DWORDS << 2)) ==
	    QM_FLASH_REGION_NUM) {
		return -EIO;
	}

	for (u32_t i = 0; i < (size >> QM_FLASH_PAGE_SIZE_BITS); i++) {
		page = get_page_num(addr) + i;
#if defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
		if (page >= (CONFIG_SOC_FLASH_QMSI_SYS_SIZE >>
				 (QM_FLASH_PAGE_SIZE_BITS + 1))) {
			flash = QM_FLASH_1;
			page -= (CONFIG_SOC_FLASH_QMSI_SYS_SIZE >>
				     (QM_FLASH_PAGE_SIZE_BITS + 1));
		}
#endif
		if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
			k_sem_take(RP_GET(dev), K_FOREVER);
		}

		qm_flash_page_erase(flash, reg, page);

		if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
			k_sem_give(RP_GET(dev));
		}
	}

	return 0;
}

static int flash_qmsi_write_protection(struct device *dev, bool enable)
{
	qm_flash_config_t qm_cfg;

	qm_cfg.us_count = CONFIG_SOC_FLASH_QMSI_CLK_COUNT_US;
	qm_cfg.wait_states = CONFIG_SOC_FLASH_QMSI_WAIT_STATES;

	if (enable) {
		qm_cfg.write_disable = QM_FLASH_WRITE_DISABLE;
	} else {
		qm_cfg.write_disable = QM_FLASH_WRITE_ENABLE;
	}

	if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

	qm_flash_set_config(QM_FLASH_0, &qm_cfg);

#if defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
	qm_flash_set_config(QM_FLASH_1, &qm_cfg);
#endif

	if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(dev));
	}

	return 0;
}

static const struct flash_driver_api flash_qmsi_api = {
	.read = flash_qmsi_read,
	.write = flash_qmsi_write,
	.erase = flash_qmsi_erase,
	.write_protection = flash_qmsi_write_protection,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = (flash_api_pages_layout)
		       flash_page_layout_not_implemented,
#endif
	.write_block_size = 4,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void flash_qmsi_set_power_state(struct device *dev, u32_t power_state)
{
	struct soc_flash_data *ctx = dev->driver_data;

	ctx->device_power_state = power_state;
}

static u32_t flash_qmsi_get_power_state(struct device *dev)
{
	struct soc_flash_data *ctx = dev->driver_data;

	return ctx->device_power_state;
}

static int flash_qmsi_suspend_device(struct device *dev)
{
	struct soc_flash_data *ctx = dev->driver_data;
	qm_flash_t i;

	for (i = QM_FLASH_0; i < QM_FLASH_NUM; i++) {
		qm_flash_save_context(i, &ctx->saved_ctx[i]);
	}
	flash_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int flash_qmsi_resume_device(struct device *dev)
{
	struct soc_flash_data *ctx = dev->driver_data;
	qm_flash_t i;

	for (i = QM_FLASH_0; i < QM_FLASH_NUM; i++) {
		qm_flash_restore_context(i, &ctx->saved_ctx[i]);
	}
	flash_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

static int flash_qmsi_device_ctrl(struct device *dev, u32_t ctrl_command,
				  void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return flash_qmsi_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return flash_qmsi_resume_device(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = flash_qmsi_get_power_state(dev);
	}

	return 0;
}
#else
#define flash_qmsi_set_power_state(...)
#endif

static int quark_flash_init(struct device *dev)
{
	qm_flash_config_t qm_cfg;

	qm_cfg.us_count = CONFIG_SOC_FLASH_QMSI_CLK_COUNT_US;
	qm_cfg.wait_states = CONFIG_SOC_FLASH_QMSI_WAIT_STATES;
	qm_cfg.write_disable = QM_FLASH_WRITE_ENABLE;

	qm_flash_set_config(QM_FLASH_0, &qm_cfg);

#if defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
	qm_flash_set_config(QM_FLASH_1, &qm_cfg);
#endif

	if (IS_ENABLED(CONFIG_SOC_FLASH_QMSI_API_REENTRANCY)) {
		k_sem_init(RP_GET(dev), 1, UINT_MAX);
	}

	flash_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

DEVICE_DEFINE(quark_flash, CONFIG_SOC_FLASH_QMSI_DEV_NAME, quark_flash_init,
	      flash_qmsi_device_ctrl, FLASH_CONTEXT, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, (void *)&flash_qmsi_api);
