/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_gd32.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <gd32_fmc.h>

LOG_MODULE_DECLARE(flash_gd32);

#define GD32_NV_FLASH_F527_NODE      SOC_NV_FLASH_NODE
#define GD32_NV_FLASH_F527_TIMEOUT   DT_PROP(GD32_NV_FLASH_F527_NODE, max_erase_time_ms)
#define GD32_NV_FLASH_F527_PAGE_SIZE DT_PROP(GD32_NV_FLASH_F527_NODE, page_size)

#define GD32_FMC_F527_DWORD_SIZE  8U
#define GD32_FMC_F527_DWORD_WORDS (GD32_FMC_F527_DWORD_SIZE / sizeof(uint32_t))

/* Double-word programming keeps the per-64-bit ECC intact (UM section 3.3.8). */
BUILD_ASSERT(SOC_NV_FLASH_PRG_SIZE == GD32_FMC_F527_DWORD_SIZE,
	     "GD32 F527 FMC requires write-block-size = <8>");

/* Bank1_Ex is a separate flash node, the main flash spans at most Bank0 and Bank1. */
BUILD_ASSERT(SOC_NV_FLASH_SIZE <= KB(4096), "GD32 F527 main flash must not include Bank1_Ex");

#define GD32_FMC_F527_WRITE_ERR                                                                    \
	(FMC_STAT_OPERR | FMC_STAT_PGSERR | FMC_STAT_PGMERR | FMC_STAT_PGAERR | FMC_STAT_WPERR)
#define GD32_FMC_F527_ERASE_ERR (FMC_STAT_OPERR | FMC_STAT_WPERR)

/* The main flash and Bank1_Ex share one FMC and cannot be operated concurrently. */
static K_SEM_DEFINE(gd32_fmc_f527_sem, 1, 1);

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout gd32_fmc_f527_layout[] = {
	{.pages_size = GD32_NV_FLASH_F527_PAGE_SIZE,
	 .pages_count = SOC_NV_FLASH_SIZE / GD32_NV_FLASH_F527_PAGE_SIZE}};
#endif

static inline void gd32_fmc_f527_unlock(void)
{
	if (FMC_CTL & FMC_CTL_LK) {
		FMC_KEY = UNLOCK_KEY0;
		FMC_KEY = UNLOCK_KEY1;
	}
}

static inline void gd32_fmc_f527_lock(void)
{
	FMC_CTL |= FMC_CTL_LK;
}

static int gd32_fmc_f527_wait_idle(uint32_t timeout_ms)
{
	const int64_t expired_time = k_uptime_get() + timeout_ms;

	while (FMC_STAT & FMC_STAT_BUSY) {
		if (k_uptime_get() > expired_time) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static bool gd32_fmc_f527_range_ok(off_t offset, size_t len, uint32_t size, uint32_t align)
{
	if ((offset < 0) || ((uint32_t)offset > size) || (len > (size - (uint32_t)offset))) {
		return false;
	}

	return ((offset % align) == 0) && ((len % align) == 0U);
}

bool flash_gd32_valid_range(off_t offset, uint32_t len, bool write)
{
	return gd32_fmc_f527_range_ok(offset, len, SOC_NV_FLASH_SIZE,
				      write ? GD32_FMC_F527_DWORD_SIZE
					    : GD32_NV_FLASH_F527_PAGE_SIZE);
}

static int gd32_fmc_f527_dword_program(uint32_t addr, const uint8_t *data, uint32_t timeout_ms)
{
	volatile uint32_t *dst = (volatile uint32_t *)addr;
	uint32_t word[GD32_FMC_F527_DWORD_WORDS];
	int ret;

	/* The caller buffer carries no alignment guarantee. */
	memcpy(word, data, sizeof(word));

	/* With DWPGE set the FMC joins the low and high words into one double-word. */
	dst[0] = word[0];
	dst[1] = word[1];

	__DSB();

	ret = gd32_fmc_f527_wait_idle(timeout_ms);
	if (ret < 0) {
		return ret;
	}

	if (FMC_STAT & GD32_FMC_F527_WRITE_ERR) {
		FMC_STAT = GD32_FMC_F527_WRITE_ERR;
		LOG_ERR("FMC double-word program at 0x%08x failed", addr);
		return -EIO;
	}

	FMC_STAT = FMC_STAT_END;

	return 0;
}

static int gd32_fmc_f527_program(uint32_t addr, const uint8_t *src, size_t len, uint32_t timeout_ms)
{
	int ret;

	k_sem_take(&gd32_fmc_f527_sem, K_FOREVER);

	gd32_fmc_f527_unlock();

	ret = gd32_fmc_f527_wait_idle(timeout_ms);
	if (ret < 0) {
		goto lock_out;
	}

	FMC_CTL |= FMC_CTL_DWPGE;
	FMC_CTL |= FMC_CTL_PG;

	while (len > 0U) {
		ret = gd32_fmc_f527_dword_program(addr, src, timeout_ms);
		if (ret < 0) {
			break;
		}

		addr += GD32_FMC_F527_DWORD_SIZE;
		src += GD32_FMC_F527_DWORD_SIZE;
		len -= GD32_FMC_F527_DWORD_SIZE;
	}

	FMC_CTL &= ~(FMC_CTL_PG | FMC_CTL_DWPGE);

lock_out:
	gd32_fmc_f527_lock();

	k_sem_give(&gd32_fmc_f527_sem);

	return ret;
}

static int gd32_fmc_f527_page_erase(uint32_t page_addr, uint32_t timeout_ms)
{
	int ret;

	gd32_fmc_f527_unlock();

	ret = gd32_fmc_f527_wait_idle(timeout_ms);
	if (ret < 0) {
		goto lock_out;
	}

	/* The page erase function is unlocked by PEKEY and needs SN to be zero. */
	FMC_PEKEY = UNLOCK_PE_KEY;
	FMC_PECFG = FMC_PECFG_PE_EN | (page_addr & FMC_PECFG_PE_ADDR);
	FMC_CTL &= ~(FMC_CTL_SN | FMC_CTL_SN_5);
	FMC_CTL |= FMC_CTL_SER;
	FMC_CTL |= FMC_CTL_START;

	ret = gd32_fmc_f527_wait_idle(timeout_ms);
	if (ret < 0) {
		goto clear_out;
	}

	if (FMC_STAT & GD32_FMC_F527_ERASE_ERR) {
		ret = -EIO;
		FMC_STAT = GD32_FMC_F527_ERASE_ERR;
		LOG_ERR("FMC page 0x%08x erase failed", page_addr);
	} else {
		FMC_STAT = FMC_STAT_END;
	}

clear_out:
	FMC_PECFG &= ~FMC_PECFG_PE_EN;
	FMC_CTL &= ~FMC_CTL_SER;

lock_out:
	gd32_fmc_f527_lock();

	return ret;
}

static int gd32_fmc_f527_erase(uint32_t page_addr, size_t size, uint32_t page_size,
			       uint32_t timeout_ms)
{
	int ret = 0;

	k_sem_take(&gd32_fmc_f527_sem, K_FOREVER);

	while (size > 0U) {
		ret = gd32_fmc_f527_page_erase(page_addr, timeout_ms);
		if (ret < 0) {
			break;
		}

		size -= page_size;
		page_addr += page_size;
	}

	k_sem_give(&gd32_fmc_f527_sem);

	return ret;
}

int flash_gd32_write_range(off_t offset, const void *data, size_t len)
{
	return gd32_fmc_f527_program(SOC_NV_FLASH_ADDR + offset, data, len,
				     GD32_NV_FLASH_F527_TIMEOUT);
}

int flash_gd32_erase_block(off_t offset, size_t size)
{
	return gd32_fmc_f527_erase(SOC_NV_FLASH_ADDR + offset, size, GD32_NV_FLASH_F527_PAGE_SIZE,
				   GD32_NV_FLASH_F527_TIMEOUT);
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_gd32_pages_layout(const struct device *dev, const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = gd32_fmc_f527_layout;
	*layout_size = ARRAY_SIZE(gd32_fmc_f527_layout);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

/* Bank1_Ex at 0x08400000, registered as its own flash device. */
struct gd32_fmc_f527_ex_config {
	uint32_t addr;
	uint32_t size;
	uint32_t page_size;
	uint32_t timeout_ms;
	struct flash_parameters parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif
};

static int gd32_fmc_f527_ex_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct gd32_fmc_f527_ex_config *cfg = dev->config;

	if (!gd32_fmc_f527_range_ok(offset, len, cfg->size, 1U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	/* Reading a bank while it is being programmed or erased must be avoided. */
	k_sem_take(&gd32_fmc_f527_sem, K_FOREVER);
	memcpy(data, (uint8_t *)cfg->addr + offset, len);
	k_sem_give(&gd32_fmc_f527_sem);

	return 0;
}

static int gd32_fmc_f527_ex_write(const struct device *dev, off_t offset, const void *data,
				  size_t len)
{
	const struct gd32_fmc_f527_ex_config *cfg = dev->config;

	if (!gd32_fmc_f527_range_ok(offset, len, cfg->size, GD32_FMC_F527_DWORD_SIZE)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	return gd32_fmc_f527_program(cfg->addr + offset, data, len, cfg->timeout_ms);
}

static int gd32_fmc_f527_ex_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct gd32_fmc_f527_ex_config *cfg = dev->config;

	if (size == 0U) {
		return 0;
	}

	if (!gd32_fmc_f527_range_ok(offset, size, cfg->size, cfg->page_size)) {
		return -EINVAL;
	}

	return gd32_fmc_f527_erase(cfg->addr + offset, size, cfg->page_size, cfg->timeout_ms);
}

static const struct flash_parameters *gd32_fmc_f527_ex_get_parameters(const struct device *dev)
{
	const struct gd32_fmc_f527_ex_config *cfg = dev->config;

	return &cfg->parameters;
}

static int gd32_fmc_f527_ex_get_size(const struct device *dev, uint64_t *size)
{
	const struct gd32_fmc_f527_ex_config *cfg = dev->config;

	*size = cfg->size;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void gd32_fmc_f527_ex_pages_layout(const struct device *dev,
					  const struct flash_pages_layout **layout,
					  size_t *layout_size)
{
	const struct gd32_fmc_f527_ex_config *cfg = dev->config;

	*layout = &cfg->layout;
	*layout_size = 1U;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, gd32_fmc_f527_ex_api) = {
	.read = gd32_fmc_f527_ex_read,
	.write = gd32_fmc_f527_ex_write,
	.erase = gd32_fmc_f527_ex_erase,
	.get_parameters = gd32_fmc_f527_ex_get_parameters,
	.get_size = gd32_fmc_f527_ex_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = gd32_fmc_f527_ex_pages_layout,
#endif
};

#define GD32_FMC_F527_EX_DEFINE(node_id)                                                           \
	BUILD_ASSERT(DT_PROP(node_id, write_block_size) == GD32_FMC_F527_DWORD_SIZE,               \
		     "GD32 F527 Bank1_Ex requires write-block-size = <8>");                        \
	static const struct gd32_fmc_f527_ex_config gd32_fmc_f527_ex_config_##node_id = {          \
		.addr = DT_REG_ADDR(node_id),                                                      \
		.size = DT_REG_SIZE(node_id),                                                      \
		.page_size = DT_PROP(node_id, page_size),                                          \
		.timeout_ms = DT_PROP(node_id, max_erase_time_ms),                                 \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size = DT_PROP(node_id, write_block_size),            \
				.erase_value = 0xff,                                               \
			},                                                                         \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (					\
		.layout = {								\
			.pages_size = DT_PROP(node_id, page_size),			\
			.pages_count = DT_REG_SIZE(node_id) / DT_PROP(node_id, page_size), \
		},)) };   \
	DEVICE_DT_DEFINE(node_id, NULL, NULL, NULL, &gd32_fmc_f527_ex_config_##node_id,            \
			 POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &gd32_fmc_f527_ex_api);

/* Bank1_Ex is the flash child that does not carry "soc-nv-flash". */
#define GD32_FMC_F527_EX_DEFINE_IF_EX(node_id)                                                     \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (),			\
		    (GD32_FMC_F527_EX_DEFINE(node_id)))

DT_FOREACH_CHILD_STATUS_OKAY(DT_INST(0, gd_gd32f527_flash_controller),
			     GD32_FMC_F527_EX_DEFINE_IF_EX)
