/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS.
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_

#include <zephyr/drivers/flash.h>

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_flash_controller), clocks) || \
	DT_NODE_HAS_PROP(DT_INST(0, st_stm32h7_flash_controller), clocks)
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#endif

/* Get the base address of the flash from the DTS node */
#define FLASH_STM32_BASE_ADDRESS DT_REG_ADDR(DT_INST(0, st_stm32_nv_flash))

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

#if defined(CONFIG_SOC_SERIES_STM32H5X)
/* FLASH register names differ for this serie */
#define FLASH_NSSR_BSY FLASH_SR_BSY
#define OPTR OPTCR
#endif /* CONFIG_SOC_SERIES_STM32H5X */

/* Differentiate between arm trust-zone non-secure/secure, and others. */
#if defined(FLASH_NSSR_NSBSY) || defined(FLASH_NSSR_BSY) /* For mcu w. TZ in non-secure mode */
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


/* Redefinitions of flags and masks to harmonize stm32 series: */
#if defined(CONFIG_SOC_SERIES_STM32U5X)
#define FLASH_STM32_NSLOCK FLASH_NSCR_LOCK
#define FLASH_STM32_DBANK FLASH_OPTR_DUALBANK
#define FLASH_STM32_NSPG FLASH_NSCR_PG
#define FLASH_STM32_NSBKER_MSK FLASH_NSCR_BKER_Msk
#define FLASH_STM32_NSBKER FLASH_NSCR_BKER
#define FLASH_STM32_NSPER FLASH_NSCR_PER
#define FLASH_STM32_NSPNB_MSK FLASH_NSCR_PNB_Msk
#define FLASH_STM32_NSPNB_POS FLASH_NSCR_PNB_Pos
#define FLASH_STM32_NSPNB FLASH_NSCR_PNB
#define FLASH_STM32_NSSTRT FLASH_NSCR_STRT
#define FLASH_PAGE_SIZE_128_BITS FLASH_PAGE_SIZE
#elif defined(CONFIG_SOC_SERIES_STM32H5X)
#define FLASH_OPTR_SWAP_BANK FLASH_OPTCR_SWAP_BANK
#define FLASH_STM32_NSLOCK FLASH_CR_LOCK
#define FLASH_STM32_DBANK 1
#define FLASH_STM32_NSPG FLASH_CR_PG
#define FLASH_STM32_NSBKER_MSK FLASH_CR_BKSEL_Msk
#define FLASH_STM32_NSBKER FLASH_CR_BKSEL
#define FLASH_STM32_NSPER FLASH_CR_SER
#define FLASH_STM32_NSPNB_MSK FLASH_CR_SNB_Msk
#define FLASH_STM32_NSPNB_POS FLASH_CR_SNB_Pos
#define FLASH_STM32_NSPNB FLASH_CR_PNB
#define FLASH_STM32_NSSTRT FLASH_CR_START
/* TODO: get values from the cmsis and stm32h5_hal_flash.h */
#undef FLASH_SIZE
/* Retrieve the FLASH SIZE from the DTS instead of cmsis as it seems erroneous */
#define FLASH_SIZE (CONFIG_FLASH_SIZE * 1024)
/* Values are redefined below from the stm32h5_hal_flash.h */
#define FLASH_PAGE_SIZE          (FLASH_SECTOR_SIZE)
#define FLASH_PAGE_NB            (FLASH_SECTOR_NB)
#define FLASH_PAGE_NB_PER_BANK   (FLASH_BANK_SIZE / FLASH_PAGE_SIZE)
#define FLASH_PAGE_SIZE_128_BITS FLASH_PAGE_SIZE
#elif defined(CONFIG_SOC_SERIES_STM32L5X)
#define FLASH_STM32_NSLOCK FLASH_NSCR_NSLOCK
#define FLASH_STM32_NSPG FLASH_NSCR_NSPG
#define FLASH_STM32_NSBKER_MSK FLASH_NSCR_NSBKER_Pos
#define FLASH_STM32_NSBKER FLASH_NSCR_NSBKER
#define FLASH_STM32_NSPER FLASH_NSCR_NSPER
#define FLASH_STM32_NSPNB_MSK FLASH_NSCR_NSPNB_Msk
#define FLASH_STM32_NSPNB_POS FLASH_NSCR_NSPNB_Pos
#define FLASH_STM32_NSPNB FLASH_NSCR_NSPNB
#define FLASH_STM32_NSSTRT FLASH_NSCR_NSSTRT
#elif defined(CONFIG_SOC_SERIES_STM32WBAX)
#define NSCR NSCR1
#define FLASH_STM32_NSLOCK FLASH_NSCR1_LOCK
#define FLASH_STM32_NSPG FLASH_NSCR1_PG
#define FLASH_STM32_NSBKER_MSK FLASH_NSCR1_BKER_Msk
#define FLASH_STM32_NSBKER FLASH_NSCR1_BKER
#define FLASH_STM32_NSPER FLASH_NSCR1_PER
#define FLASH_STM32_NSPNB_MSK FLASH_NSCR1_PNB_Msk
#define FLASH_STM32_NSPNB_POS FLASH_NSCR1_PNB_Pos
#define FLASH_STM32_NSPNB FLASH_NSCR1_PNB
#define FLASH_STM32_NSSTRT FLASH_NSCR1_STRT
#endif /* CONFIG_SOC_SERIES_STM32U5X */
#if defined(FLASH_OPTR_DBANK)
#define FLASH_STM32_DBANK FLASH_OPTR_DBANK
#endif /* FLASH_OPTR_DBANK */

#if defined(CONFIG_SOC_SERIES_STM32G0X)
#if defined(FLASH_FLAG_BSY2)
#define FLASH_STM32_SR_BUSY	(FLASH_FLAG_BSY1 | FLASH_FLAG_BSY2);
#else
#define FLASH_STM32_SR_BUSY	(FLASH_SR_BSY1)
#endif /* defined(FLASH_FLAG_BSY2) */
#else
#define FLASH_STM32_SR_BUSY	(FLASH_FLAG_BSY)
#endif

#if defined(CONFIG_SOC_SERIES_STM32G0X)
#define FLASH_STM32_SR_CFGBSY	(FLASH_SR_CFGBSY)
#elif defined(FLASH_FLAG_CFGBSY)
#define FLASH_STM32_SR_CFGBSY	(FLASH_FLAG_CFGBSY)
#endif

#if defined(CONFIG_SOC_SERIES_STM32G0X)
/* STM32G0 HAL FLASH_FLAG_x don't represent bit-masks, need FLASH_SR_x instead */
#define FLASH_STM32_SR_OPERR	FLASH_SR_OPERR
#define FLASH_STM32_SR_PGERR	0
#define FLASH_STM32_SR_PROGERR	FLASH_SR_PROGERR
#define FLASH_STM32_SR_WRPERR	FLASH_SR_WRPERR
#define FLASH_STM32_SR_PGAERR	FLASH_SR_PGAERR
#define FLASH_STM32_SR_SIZERR	FLASH_SR_SIZERR
#define FLASH_STM32_SR_PGSERR	FLASH_SR_PGSERR
#define FLASH_STM32_SR_MISERR	FLASH_SR_MISERR
#define FLASH_STM32_SR_FASTERR	FLASH_SR_FASTERR
#if defined(FLASH_SR_RDERR)
#define FLASH_STM32_SR_RDERR	FLASH_SR_RDERR
#else
#define FLASH_STM32_SR_RDERR	0
#endif
#define FLASH_STM32_SR_PGPERR	0

#else /* !defined(CONFIG_SOC_SERIES_STM32G0X) */
#if defined(FLASH_FLAG_OPERR)
#define FLASH_STM32_SR_OPERR	FLASH_FLAG_OPERR
#else
#define FLASH_STM32_SR_OPERR	0
#endif

#if defined(FLASH_FLAG_PGERR)
#define FLASH_STM32_SR_PGERR	FLASH_FLAG_PGERR
#else
#define FLASH_STM32_SR_PGERR	0
#endif

#if defined(FLASH_FLAG_PROGERR)
#define FLASH_STM32_SR_PROGERR	FLASH_FLAG_PROGERR
#else
#define FLASH_STM32_SR_PROGERR	0
#endif

#if defined(FLASH_FLAG_WRPERR)
#define FLASH_STM32_SR_WRPERR	FLASH_FLAG_WRPERR
#else
#define FLASH_STM32_SR_WRPERR	0
#endif

#if defined(FLASH_FLAG_PGAERR)
#define FLASH_STM32_SR_PGAERR	FLASH_FLAG_PGAERR
#else
#define FLASH_STM32_SR_PGAERR	0
#endif

#if defined(FLASH_FLAG_SIZERR)
#define FLASH_STM32_SR_SIZERR	FLASH_FLAG_SIZERR
#else
#define FLASH_STM32_SR_SIZERR	0
#endif

#if defined(FLASH_FLAG_PGSERR)
#define FLASH_STM32_SR_PGSERR	FLASH_FLAG_PGSERR
#else
#define FLASH_STM32_SR_PGSERR	0
#endif

#if defined(FLASH_FLAG_MISERR)
#define FLASH_STM32_SR_MISERR	FLASH_FLAG_MISERR
#else
#define FLASH_STM32_SR_MISERR	0
#endif

#if defined(FLASH_FLAG_FASTERR)
#define FLASH_STM32_SR_FASTERR	FLASH_FLAG_FASTERR
#else
#define FLASH_STM32_SR_FASTERR	0
#endif

#if defined(FLASH_FLAG_RDERR)
#define FLASH_STM32_SR_RDERR	FLASH_FLAG_RDERR
#else
#define FLASH_STM32_SR_RDERR	0
#endif

#if defined(FLASH_FLAG_PGPERR)
#define FLASH_STM32_SR_PGPERR	FLASH_FLAG_PGPERR
#else
#define FLASH_STM32_SR_PGPERR	0
#endif

#endif /* !defined(CONFIG_SOC_SERIES_STM32G0X) */

#define FLASH_STM32_SR_ERRORS  (FLASH_STM32_SR_OPERR |			\
				FLASH_STM32_SR_PGERR |			\
				FLASH_STM32_SR_PROGERR |		\
				FLASH_STM32_SR_WRPERR |			\
				FLASH_STM32_SR_PGAERR |			\
				FLASH_STM32_SR_SIZERR |			\
				FLASH_STM32_SR_PGSERR |			\
				FLASH_STM32_SR_MISERR |			\
				FLASH_STM32_SR_FASTERR |		\
				FLASH_STM32_SR_RDERR |			\
				FLASH_STM32_SR_PGPERR)

#define FLASH_STM32_RDP0 0xAA
#define FLASH_STM32_RDP2 0xCC
#define FLASH_STM32_RDP1                                                       \
	DT_PROP(DT_INST(0, st_stm32_flash_controller), st_rdp1_enable_byte)

#if FLASH_STM32_RDP1 == FLASH_STM32_RDP0 || FLASH_STM32_RDP1 == FLASH_STM32_RDP2
#error RDP1 byte has to be different than RDP0 and RDP2 byte
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

static inline bool flash_stm32_valid_write(off_t offset, uint32_t len)
{
	return ((offset % FLASH_STM32_WRITE_BLOCK_SIZE == 0) &&
		(len % FLASH_STM32_WRITE_BLOCK_SIZE == 0U));
}

bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len, bool write);

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len);

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len);

int flash_stm32_wait_flash_idle(const struct device *dev);

int flash_stm32_option_bytes_lock(const struct device *dev, bool enable);

#ifdef CONFIG_SOC_SERIES_STM32WBX
int flash_stm32_check_status(const struct device *dev);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size);
#endif

#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)

int flash_stm32_update_wp_sectors(const struct device *dev,
				  uint32_t changed_sectors,
				  uint32_t protected_sectors);

int flash_stm32_get_wp_sectors(const struct device *dev,
			       uint32_t *protected_sectors);
#endif
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)

int flash_stm32_update_rdp(const struct device *dev, bool enable,
			   bool permanent);

int flash_stm32_get_rdp(const struct device *dev, bool *enabled,
			bool *permanent);
#endif

/* Flash extended operations */
#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)
int flash_stm32_ex_op_sector_wp(const struct device *dev, const uintptr_t in,
				void *out);
#endif
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)
int flash_stm32_ex_op_rdp(const struct device *dev, const uintptr_t in,
			  void *out);
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_STM32_H_ */
