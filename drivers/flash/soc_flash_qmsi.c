/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <nanokernel.h>
#include <device.h>
#include <init.h>

#include <flash.h>

#include "qm_flash.h"
#include "qm_soc_regs.h"

struct soc_flash_data {
	struct nano_sem sem;
};

#ifdef CONFIG_SOC_FLASH_QMSI_API_REENTRANCY
static struct soc_flash_data soc_flash_context;
#define FLASH_CONTEXT (&soc_flash_context)
static const int reentrancy_protection = 1;
#else
#define FLASH_CONTEXT (NULL)
static const int reentrancy_protection;
#endif /* CONFIG_SOC_FLASH_QMSI_API_REENTRANCY */


static void flash_reentrancy_init(struct device *dev)
{
	struct soc_flash_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_init(&context->sem);
	nano_sem_give(&context->sem);
}

static void flash_critical_region_start(struct device *dev)
{
	struct soc_flash_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_take(&context->sem, TICKS_UNLIMITED);
}

static void flash_critical_region_end(struct device *dev)
{
	struct soc_flash_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_give(&context->sem);
}

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

static qm_flash_region_t flash_region(uint32_t addr)
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

static uint32_t get_page_num(uint32_t addr)
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

	for (uint32_t i = 0; i < (len >> 2); i++) {
		UNALIGNED_PUT(sys_read32(addr + (i << 2)),
			      (uint32_t *)data + i);
	}

	return 0;
}

static int flash_qmsi_write(struct device *dev, off_t addr,
			    const void *data, size_t len)
{
	qm_flash_t flash = QM_FLASH_0;
	qm_flash_region_t reg;
	uint32_t data_word = 0, offset = 0, f_addr = 0;

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

	for (uint32_t i = 0; i < (len >> 2); i++) {
		data_word = UNALIGNED_GET((uint32_t *)data + i);
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

#if defined(CONFIG_SOC_QUARK_SE_C1000)
		if (offset >= (CONFIG_SOC_FLASH_QMSI_SYS_SIZE >> 1)) {
			flash = QM_FLASH_1;
			offset -= CONFIG_SOC_FLASH_QMSI_SYS_SIZE >> 1;
		}
#endif

		flash_critical_region_start(dev);
		qm_flash_word_write(flash, reg, offset, data_word);
		flash_critical_region_end(dev);
	}

	return 0;
}

static int flash_qmsi_erase(struct device *dev, off_t addr, size_t size)
{
	qm_flash_t flash = QM_FLASH_0;
	qm_flash_region_t reg;
	uint32_t page = 0;

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

	for (uint32_t i = 0; i < (size >> QM_FLASH_PAGE_SIZE_BITS); i++) {
		page = get_page_num(addr) + i;
#if defined(CONFIG_SOC_QUARK_SE_C1000)
		if (page >= (CONFIG_SOC_FLASH_QMSI_SYS_SIZE >>
				 (QM_FLASH_PAGE_SIZE_BITS + 1))) {
			flash = QM_FLASH_1;
			page -= (CONFIG_SOC_FLASH_QMSI_SYS_SIZE >>
				     (QM_FLASH_PAGE_SIZE_BITS + 1));
		}
#endif
		flash_critical_region_start(dev);
		qm_flash_page_erase(flash, reg, page);
		flash_critical_region_end(dev);
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

	flash_critical_region_start(dev);
	qm_flash_set_config(QM_FLASH_0, &qm_cfg);

#if defined(CONFIG_SOC_QUARK_SE_C1000)
	qm_flash_set_config(QM_FLASH_1, &qm_cfg);
#endif

	flash_critical_region_end(dev);

	return 0;
}

static const struct flash_driver_api flash_qmsi_api = {
	.read = flash_qmsi_read,
	.write = flash_qmsi_write,
	.erase = flash_qmsi_erase,
	.write_protection = flash_qmsi_write_protection,
};

static int quark_flash_init(struct device *dev)
{
	qm_flash_config_t qm_cfg;

	dev->driver_api = &flash_qmsi_api;

	qm_cfg.us_count = CONFIG_SOC_FLASH_QMSI_CLK_COUNT_US;
	qm_cfg.wait_states = CONFIG_SOC_FLASH_QMSI_WAIT_STATES;
	qm_cfg.write_disable = QM_FLASH_WRITE_ENABLE;

	qm_flash_set_config(QM_FLASH_0, &qm_cfg);

#if defined(CONFIG_SOC_QUARK_SE_C1000)
	qm_flash_set_config(QM_FLASH_1, &qm_cfg);
#endif

	flash_reentrancy_init(dev);

	return 0;
}

DEVICE_INIT(quark_flash, CONFIG_SOC_FLASH_QMSI_DEV_NAME,
	    quark_flash_init, FLASH_CONTEXT, NULL, SECONDARY,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
