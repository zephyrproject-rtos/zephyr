/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_flash_controller), clocks) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32h7_flash_controller), clocks)
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#endif

struct flash_stm32_priv {
	FLASH_TypeDef *regs;
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_flash_controller), clocks) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32h7_flash_controller), clocks)
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
#endif
	struct k_sem sem;
};

#if DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#define FLASH_STM32_WRITE_BLOCK_SIZE \
	DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#else
#error Flash write block size not available
	/* Flash Write block size is extracted from device tree */
	/* as flash node property 'write-block-size' */
#endif

/* Differentiate between arm trust-zone non-secure/secure, and others. */
#if defined(FLASH_NSSR_NSBSY)		/* For mcu w. TZ in non-secure mode */
#define FLASH_SECURITY_NS
#define FLASH_STM32_SR		NSSR
#elif defined(FLASH_SECSR_SECBSY)	/* For mcu w. TZ  in secured mode */
#error Flash is not supported in secure mode
#define FLASH_SECURITY_SEC
#else
#define FLASH_SECURITY_NA		/* For series which does not have
					 *  secured or non-secured mode
					 */
#define FLASH_STM32_SR		SR
#endif


#define FLASH_STM32_PRIV(dev) ((struct flash_stm32_priv *)((dev)->data))
#define FLASH_STM32_REGS(dev) (FLASH_STM32_PRIV(dev)->regs)


/* Redefintions of flags and masks to harmonize stm32 series: */
#if defined(CONFIG_SOC_SERIES_STM32G0X)
#define FLASH_STM32_SR_BUSY	(FLASH_SR_BSY1)
#else
#define FLASH_STM32_SR_BUSY	(FLASH_FLAG_BSY)
#endif


#ifdef CONFIG_FLASH_PAGE_LAYOUT
static inline bool flash_stm32_range_exists(const struct device *dev,
					    off_t offset,
					    uint32_t len)
{
	struct flash_pages_info info;

	return !(flash_get_page_info_by_offs(dev, offset, &info) ||
		 flash_get_page_info_by_offs(dev, offset + len - 1, &info));
}
#endif	/* CONFIG_FLASH_PAGE_LAYOUT */

bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len, bool write);

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len);

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len);

int flash_stm32_wait_flash_idle(const struct device *dev);

#ifdef CONFIG_SOC_SERIES_STM32WBX
int flash_stm32_check_status(const struct device *dev);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size);
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_ */
