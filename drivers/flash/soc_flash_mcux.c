/*
 * Copyright (c) 2016 Linaro Limited
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <errno.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/sys/barrier.h>
#include "flash_priv.h"

#include "fsl_common.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_mcux);


#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_kinetis_ftfa))
#define DT_DRV_COMPAT nxp_kinetis_ftfa
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_kinetis_ftfe))
#define DT_DRV_COMPAT nxp_kinetis_ftfe
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_kinetis_ftfl))
#define DT_DRV_COMPAT nxp_kinetis_ftfl
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_iap_fmc55))
#define DT_DRV_COMPAT nxp_iap_fmc55
#define SOC_HAS_IAP 1
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_iap_fmc553))
#define DT_DRV_COMPAT nxp_iap_fmc553
#define SOC_HAS_IAP 1
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_msf1))
#define DT_DRV_COMPAT nxp_msf1
#define SOC_HAS_MSF1 1
#else
#error No matching compatible for soc_flash_mcux.c
#endif

#if defined(SOC_HAS_IAP) && !defined(CONFIG_SOC_LPC55S36)
#include "fsl_iap.h"
#elif defined(CONFIG_SOC_MCXA156)
#include "fsl_romapi.h"
#define FLASH_Erase   FLASH_EraseSector
#define FLASH_Program FLASH_ProgramPhrase
#elif defined(CONFIG_MCUX_FLASH_K4_API)
#include "fsl_k4_flash.h"
#else
#include "fsl_flash.h"
#endif /* SOC_HAS_IAP && !CONFIG_SOC_LPC55S36*/

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#if defined(CONFIG_CHECK_BEFORE_READING)  && !defined(CONFIG_SOC_LPC55S36)
#define FMC_STATUS_FAIL	FLASH_INT_CLR_ENABLE_FAIL_MASK
#define FMC_STATUS_ERR	FLASH_INT_CLR_ENABLE_ERR_MASK
#define FMC_STATUS_DONE	FLASH_INT_CLR_ENABLE_DONE_MASK
#define FMC_STATUS_ECC	FLASH_INT_CLR_ENABLE_ECC_ERR_MASK

#define FMC_STATUS_FAILURES	\
	(FMC_STATUS_FAIL | FMC_STATUS_ERR | FMC_STATUS_ECC)

#define FMC_CMD_BLANK_CHECK		5
#define FMC_CMD_MARGIN_CHECK	6

/* Issue single command that uses an a start and stop address. */
static uint32_t get_cmd_status(uint32_t cmd, uint32_t addr, size_t len)
{
	FLASH_Type *p_fmc = (FLASH_Type *)DT_INST_REG_ADDR(0);
	uint32_t status;

	/* issue low level command */
	p_fmc->INT_CLR_STATUS = 0xF;
	p_fmc->STARTA = (addr>>4) & 0x3FFFF;
	p_fmc->STOPA = ((addr+len-1)>>4) & 0x3FFFF;
	p_fmc->CMD = cmd;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* wait for command to be done */
	while (!(p_fmc->INT_STATUS & FMC_STATUS_DONE))
		;

	/* get read status and then clear it */
	status = p_fmc->INT_STATUS;
	p_fmc->INT_CLR_STATUS = 0xF;

	return status;
}

/* This function prevents erroneous reading. Some ECC enabled devices will
 * crash when reading an erased or wrongly programmed area.
 */
static status_t is_area_readable(uint32_t addr, size_t len)
{
	uint32_t key;
	status_t status;

	key = irq_lock();

	/* Check if the are is correctly programmed and can be read. */
	status = get_cmd_status(FMC_CMD_MARGIN_CHECK, addr, len);
	if (status & FMC_STATUS_FAILURES) {
		/* If the area was erased, ECC errors are triggered on read. */
		status = get_cmd_status(FMC_CMD_BLANK_CHECK, addr, len);
		if (!(status & FMC_STATUS_FAIL)) {
			LOG_DBG("read request on erased addr:0x%08x size:%d",
				addr, len);
			irq_unlock(key);
			return -ENODATA;
		}
		LOG_DBG("read request error for addr:0x%08x size:%d",
			addr, len);
		irq_unlock(key);
		return -EIO;
	}

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_CHECK_BEFORE_READING && ! CONFIG_SOC_LPC55S36 */

#define SOC_FLASH_NEED_CLEAR_CACHES 1
#ifdef CONFIG_SOC_SERIES_MCXW
static void clear_flash_caches(void)
{
	volatile uint32_t *const smscm_ocmdr0 = (volatile uint32_t *)0x40015400;
	/* this bit clears the flash cache */
	*smscm_ocmdr0 |= BIT(8);
	volatile uint32_t *mcm_cpcr2 = (volatile uint32_t *)0xe0080034;
	/* this bit clears the code cache */
	*mcm_cpcr2 |= BIT(0);
}
#elif CONFIG_SOC_SERIES_MCXN
static void clear_flash_caches(void)
{
	volatile uint32_t *const nvm_ctrl = (volatile uint32_t *)0x40000400;
	/* this bit clears the flash cache */
	*nvm_ctrl |= BIT(5);
	volatile uint32_t *const lpcac_ctrl = (volatile uint32_t *)0x40000824;
	/* this bit clears the code cache */
	*lpcac_ctrl |= BIT(1);
}
#else
#undef SOC_FLASH_NEED_CLEAR_CACHES
#define clear_flash_caches(...)
#endif

struct flash_priv {
	flash_config_t config;
	/*
	 * HACK: flash write protection is managed in software.
	 */
	struct k_sem write_lock;
	uint32_t pflash_block_base;
};

static const struct flash_parameters flash_mcux_parameters = {
#if DT_NODE_HAS_PROP(SOC_NV_FLASH_NODE, write_block_size)
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
#else
	.write_block_size = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE,
#endif
	.erase_value = 0xff,
};

/*
 * Interrupt vectors could be executed from flash hence the need for locking.
 * The underlying MCUX driver takes care of copying the functions to SRAM.
 *
 * For more information, see the application note below on Read-While-Write
 * http://cache.freescale.com/files/32bit/doc/app_note/AN4695.pdf
 *
 */

static int flash_mcux_erase(const struct device *dev, off_t offset,
			    size_t len)
{
	struct flash_priv *priv = dev->data;
	uint32_t addr;
	status_t rc;
	unsigned int key;

	if (k_sem_take(&priv->write_lock, K_FOREVER)) {
		return -EACCES;
	}

	addr = offset + priv->pflash_block_base;

	key = irq_lock();
	rc = FLASH_Erase(&priv->config,
#if CONFIG_MCUX_FLASH_K4_API
			(FMU_Type *) DT_INST_REG_ADDR(0),
#endif
			addr, len, kFLASH_ApiEraseKey);

	if (IS_ENABLED(SOC_FLASH_NEED_CLEAR_CACHES)) {
		clear_flash_caches();
	}

	irq_unlock(key);

	k_sem_give(&priv->write_lock);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}


/*
 * @brief Read a flash memory area.
 *
 * @param dev Device struct
 * @param offset The address's offset
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 	0 on success,
 * 			-EIO for erroneous area
 */
static int flash_mcux_read(const struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct flash_priv *priv = dev->data;
	uint32_t addr;
	status_t rc = 0;

	/*
	 * The MCUX supports different flash chips whose valid ranges are
	 * hidden below the API: until the API export these ranges, we can not
	 * do any generic validation
	 */
	addr = offset + priv->pflash_block_base;

#ifdef CONFIG_CHECK_BEFORE_READING
  #ifdef CONFIG_SOC_LPC55S36
	/* Validates the given address range is loaded in the flash hiding region. */
	rc = FLASH_IsFlashAreaReadable(&priv->config, addr, len);
	if (rc != kStatus_FLASH_Success) {
		rc = -EIO;
	} else {
		/* Check whether the flash is erased ("len" and "addr" must be word-aligned). */
		rc = FLASH_VerifyErase(&priv->config, ((addr + 0x3) & ~0x3),  ((len + 0x3) & ~0x3));
		if (rc == kStatus_FLASH_Success) {
			rc = -ENODATA;
		} else {
			rc = 0;
		}
	}
  #else
	rc = is_area_readable(addr, len);
  #endif /* CONFIG_SOC_LPC55S36 */
#endif /* CONFIG_CHECK_BEFORE_READING */

	if (!rc) {
		memcpy(data, (void *) addr, len);
	}
#ifdef CONFIG_CHECK_BEFORE_READING
	else if (rc == -ENODATA) {
		/* Erased area, return dummy data as an erased page. */
		memset(data, 0xFF, len);
		rc = 0;
	}
#endif
	return rc;
}

static int flash_mcux_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_priv *priv = dev->data;
	uint32_t addr;
	status_t rc;
	unsigned int key;

	if (k_sem_take(&priv->write_lock, K_FOREVER)) {
		return -EACCES;
	}

	addr = offset + priv->pflash_block_base;

	key = irq_lock();
	rc = FLASH_Program(&priv->config,
#if CONFIG_MCUX_FLASH_K4_API
			(FMU_Type *) DT_INST_REG_ADDR(0),
#endif
			addr, (uint8_t *) data, len);

	if (IS_ENABLED(SOC_FLASH_NEED_CLEAR_CACHES)) {
		clear_flash_caches();
	}

	irq_unlock(key);

	k_sem_give(&priv->write_lock);

	return (rc == kStatus_Success) ? 0 : -EINVAL;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_mcux_pages_layout(const struct device *dev,
				    const struct flash_pages_layout **layout,
				    size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_mcux_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_mcux_parameters;
}

static struct flash_priv flash_data;

static const struct flash_driver_api flash_mcux_api = {
	.erase = flash_mcux_erase,
	.write = flash_mcux_write,
	.read = flash_mcux_read,
	.get_parameters = flash_mcux_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mcux_pages_layout,
#endif
};

#if (defined(SOC_HAS_IAP) || defined(SOC_HAS_MSF1)) && !defined(CONFIG_MCUX_FLASH_K4_API)
#define FLASH_PROP_BLOCK_BASE	kFLASH_PropertyPflashBlockBaseAddr
#else
#define FLASH_PROP_BLOCK_BASE kFLASH_PropertyPflash0BlockBaseAddr
#endif

static int flash_mcux_init(const struct device *dev)
{
	struct flash_priv *priv = dev->data;
	uint32_t pflash_block_base;
	status_t rc;

	k_sem_init(&priv->write_lock, 1, 1);

	rc = FLASH_Init(&priv->config);

	FLASH_GetProperty(&priv->config, FLASH_PROP_BLOCK_BASE, &pflash_block_base);

	priv->pflash_block_base = (uint32_t) pflash_block_base;

	return (rc == kStatus_Success) ? 0 : -EIO;
}

DEVICE_DT_INST_DEFINE(0, flash_mcux_init, NULL,
			&flash_data, NULL, POST_KERNEL,
			CONFIG_FLASH_INIT_PRIORITY, &flash_mcux_api);
