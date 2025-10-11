/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr shim for NXP S32K3x C40 internal flash
 */

#define DT_DRV_COMPAT nxp_s32k3x_c40_flash

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>
#include <string.h>
#if defined(CONFIG_ARM)
#include <cmsis_core.h>
#endif
LOG_MODULE_REGISTER(flash_mcux_c40, CONFIG_FLASH_LOG_LEVEL);

#include "fsl_c40_flash.h"

/* -------- Helpers -------- */

static inline int mcux_to_errno(status_t s)
{
	switch (s) {
	case kStatus_FLASH_Success:
		return 0;
	case kStatus_FLASH_InvalidArgument:
	case kStatus_FLASH_SizeError:
	case kStatus_FLASH_AlignmentError:
	case kStatus_FLASH_AddressError:
		return -EINVAL;
	case kStatus_FLASH_AccessError:
	case kStatus_FLASH_ProtectionViolation:
	case kStatus_FLASH_CommandFailure:
		return -EIO;
	case kStatus_FLASH_EraseKeyError:
		return -EPERM;
	default:
		return -EIO;
	}
}

struct prot_range {
	uint32_t off, len;
	const char *name;
};

struct mcux_c40_cfg {
	uint32_t base;        /* flash XIP base */
	uint32_t size;        /* total bytes covered by this instance */
	uint32_t erase_block; /* 8 KiB on C40 */
	uint32_t write_block; /* 8 bytes min program unit */
	const struct flash_parameters *params;
#if defined(CONFIG_FLASH_MCUX_C40_APPLY_PROTECTION)
	const struct prot_range *prot_tbl;
	size_t prot_cnt;
#endif
};

struct mcux_c40_data {
	flash_config_t cfg; /* MCUX HAL context */
};

static inline bool intersects(uint32_t a_off, uint32_t a_len, uint32_t b_off, uint32_t b_len)
{
	const uint32_t a_end = a_off + a_len;
	const uint32_t b_end = b_off + b_len;
	return (a_off < b_end) && (b_off < a_end);
}

/* -------- Flash API -------- */

static int mcux_c40_read(const struct device *dev, off_t off, void *buf, size_t len)
{
	const struct mcux_c40_cfg *cfg = dev->config;

	if ((off < 0) || ((size_t)off + len > cfg->size)) {
		return -EINVAL;
	}

	memcpy(buf, (const void *)(cfg->base + (uintptr_t)off), len);
	return 0;
}

static int mcux_c40_write(const struct device *dev, off_t off, const void *buf, size_t len)
{
	const struct mcux_c40_cfg *cfg = dev->config;
	struct mcux_c40_data *data = dev->data;

	/* Bounds check */
	if ((off < 0) || ((size_t)off + len > cfg->size)) {
		return -EINVAL;
	}

	/* The HAL enforces alignment to C40 constraints:
	 * minimum write chunk is C40_WRITE_SIZE_MIN (8 bytes)
	 * writes are most efficient in quad-page (128 bytes)
	 * We perform a light check: write must be multiple of write_block,
	 * and start aligned to write_block. For more flexibility, you could
	 * implement a read-modify-write cache; for now we map 1:1 to HAL.
	 */
	if (((uintptr_t)off % cfg->write_block) != 0 || (len % cfg->write_block) != 0) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (IS_ENABLED(CONFIG_ARM)) {
		__DSB();
		__ISB();
	}

	status_t st = FLASH_Program(&data->cfg, (uint32_t)(cfg->base + (uintptr_t)off),
				    (uint32_t *)buf, (uint32_t)len);

	if (IS_ENABLED(CONFIG_ARM)) {
		__DSB();
	}

	irq_unlock(key);

	/* The array changed behind the CPU; drop stale D-cache lines covering the range */
	sys_cache_data_invd_range((void *)(cfg->base + (uintptr_t)off), len);

	return mcux_to_errno(st);
}

static int mcux_c40_erase(const struct device *dev, off_t off, size_t len)
{
	const struct mcux_c40_cfg *cfg = dev->config;
	struct mcux_c40_data *data = dev->data;

	if ((off < 0) || ((size_t)off + len > cfg->size)) {
		return -EINVAL;
	}
	if (((uintptr_t)off % cfg->erase_block) != 0 || (len % cfg->erase_block) != 0) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (IS_ENABLED(CONFIG_ARM)) {
		__DSB();
		__ISB();
	}

	status_t st = FLASH_Erase(&data->cfg, (uint32_t)(cfg->base + (uintptr_t)off), (uint32_t)len,
				  kFLASH_ApiEraseKey);

	if (IS_ENABLED(CONFIG_ARM)) {
		__DSB();
	}

	irq_unlock(key);

	sys_cache_data_invd_range((void *)(cfg->base + (uintptr_t)off), len);
	return mcux_to_errno(st);
}

static const struct flash_parameters *mcux_c40_get_parameters(const struct device *dev)
{
	const struct mcux_c40_cfg *cfg = dev->config;
	return cfg->params;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void mcux_c40_pages_layout(const struct device *dev,
				  const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct mcux_c40_cfg *cfg = dev->config;
	static struct flash_pages_layout l;

	l.pages_count = cfg->size / cfg->erase_block;
	l.pages_size = cfg->erase_block;

	*layout = &l;
	*layout_size = 1;
}
#endif

/* -------- Optional “lock policy” executed at init (opt-in via Kconfig) -------- */
#if defined(CONFIG_FLASH_MCUX_C40_APPLY_PROTECTION)
/* We place this TU into .ram_text_reloc via CMake, so no special attribute is needed. */
static __attribute__((noinline)) status_t c40_apply_protection(flash_config_t *fcfg,
							       uint32_t flash_base,
							       uint32_t total_sz, uint32_t erase_sz,
							       const struct prot_range *pr,
							       size_t npr)
{
	for (uint32_t off = 0; off < total_sz; off += erase_sz) {
		bool lock_it = false;

		for (size_t i = 0; i < npr; i++) {
			if (intersects(off, erase_sz, pr[i].off, pr[i].len)) {
				lock_it = true;
				break;
			}
		}
		const uint32_t abs = flash_base + off;

		status_t ps = FLASH_GetSectorProtection(fcfg, abs);
		if (lock_it) {
			if (ps != kStatus_FLASH_SectorLocked) {
				unsigned int key = irq_lock();

				if (IS_ENABLED(CONFIG_ARM)) {
					/* Didn't use __ISB() here since
					 * there is no instruction side stream change */
					__DSB();
				}

				status_t s = FLASH_SetSectorProtection(fcfg, abs, true);

				if (IS_ENABLED(CONFIG_ARM)) {
					__DSB();
				}

				irq_unlock(key);
				if (s != kStatus_FLASH_Success) {
					return s;
				}
			}
		} else {
			if (ps != kStatus_FLASH_SectorUnlocked) {
				unsigned int key = irq_lock();

				if (IS_ENABLED(CONFIG_ARM)) {
					__DSB();
				}

				status_t s = FLASH_SetSectorProtection(fcfg, abs, false);

				if (IS_ENABLED(CONFIG_ARM)) {
					__DSB();
				}

				irq_unlock(key);
				if (s != kStatus_FLASH_Success) {
					return s;
				}
			}
		}
	}
	return kStatus_FLASH_Success;
}
#endif /* CONFIG_FLASH_MCUX_C40_APPLY_PROTECTION */

static int mcux_c40_init(const struct device *dev)
{
	const struct mcux_c40_cfg *cfg = dev->config;
	struct mcux_c40_data *data = dev->data;

	status_t st = FLASH_Init(&data->cfg);
	if (st != kStatus_FLASH_Success) {
		LOG_ERR("FLASH_Init failed: %d", (int)st);
		return mcux_to_errno(st);
	}

	LOG_INF("C40 flash: base=0x%lx size=0x%lx erase=0x%lx write=0x%lx",
		(unsigned long)cfg->base, (unsigned long)cfg->size, (unsigned long)cfg->erase_block,
		(unsigned long)cfg->write_block);

#if defined(CONFIG_FLASH_MCUX_C40_APPLY_PROTECTION)
	/* Align protected windows to sector boundaries */
	struct prot_range prot_aligned[8];
	size_t nprot_al = 0;

	for (size_t i = 0; i < cfg->prot_cnt && nprot_al < ARRAY_SIZE(prot_aligned); i++) {
		uint32_t s = cfg->prot_tbl[i].off;
		uint32_t e = s + cfg->prot_tbl[i].len;
		uint32_t sa = ROUND_DOWN(s, cfg->erase_block);
		uint32_t ea = ROUND_UP(e, cfg->erase_block);
		if (sa >= cfg->size) {
			continue;
		}
		if (ea > cfg->size) {
			ea = cfg->size;
		}

		prot_aligned[nprot_al] = (struct prot_range){
			.off = sa,
			.len = ea - sa,
			.name = cfg->prot_tbl[i].name,
		};
		nprot_al++;
	}

	status_t s = c40_apply_protection(&data->cfg, cfg->base, cfg->size, cfg->erase_block,
					  prot_aligned, nprot_al);
	if (s != kStatus_FLASH_Success) {
		LOG_ERR("Protection apply failed: %d", (int)s);
		return mcux_to_errno(s);
	}
	LOG_INF("Protection policy applied (%u window%s)", (unsigned)nprot_al,
		(nprot_al == 1 ? "" : "s"));
#endif
	return 0;
}

/* -------- Driver API table -------- */

static const struct flash_driver_api mcux_c40_api = {
	.read = mcux_c40_read,
	.write = mcux_c40_write,
	.erase = mcux_c40_erase,
	.get_parameters = mcux_c40_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = mcux_c40_pages_layout,
#endif
};

/* Per-instance params from DT */

#define C40_PARAMS(inst)                                                                           \
	static const struct flash_parameters mcux_c40_params_##inst = {                            \
		.write_block_size = DT_PROP(DT_INST(inst, DT_DRV_COMPAT), write_block_size),       \
		.erase_value = 0xFF,                                                               \
	}

#if defined(CONFIG_FLASH_MCUX_C40_APPLY_PROTECTION)

/* Emit one initializer element if the label exists and belongs to this inst */
#define C40_PROT_ENTRY(lbl, inst)                                                   \
	COND_CODE_1(                                                                \
		DT_NODE_HAS_STATUS(DT_NODELABEL(lbl), okay),                        \
		( COND_CODE_1(                                                      \
			DT_SAME_NODE(                                               \
				DT_PARENT(DT_PARENT(DT_NODELABEL(lbl))),            \
				DT_INST(inst, DT_DRV_COMPAT)                       \
			),                                                         \
			( { .off  = (uint32_t)FIXED_PARTITION_OFFSET(lbl),        \
			    .len  = (uint32_t)FIXED_PARTITION_SIZE(lbl),          \
			    .name = #lbl }, ),                                     \
			() )                                                       \
		),                                                                \
		() )

#define C40_MAKE_PROT_TBL(inst)                                                    \
	static const struct prot_range mcux_c40_prot_##inst[] = {                   \
		C40_PROT_ENTRY(ivt_header, inst)                                   \
		C40_PROT_ENTRY(ivt_pad,    inst)                                   \
		C40_PROT_ENTRY(mcuboot,    inst) /* common MCUboot label */        \
		C40_PROT_ENTRY(boot_partition, inst) /* fallback */                  \
	};                                                                          \
	enum { mcux_c40_prot_cnt_##inst = ARRAY_SIZE(mcux_c40_prot_##inst) }

#else
#define C40_MAKE_PROT_TBL(inst)
#endif

#define C40_INIT(inst)                                                             \
	C40_MAKE_PROT_TBL(inst);                                                   \
	C40_PARAMS(inst);                                                          \
	static const struct mcux_c40_cfg mcux_c40_cfg_##inst = {                   \
		.base        = DT_REG_ADDR(DT_INST(inst, DT_DRV_COMPAT)),          \
		.size        = DT_REG_SIZE(DT_INST(inst, DT_DRV_COMPAT)),          \
		.erase_block = DT_PROP(DT_INST(inst, DT_DRV_COMPAT), erase_block_size), \
		.write_block = DT_PROP(DT_INST(inst, DT_DRV_COMPAT), write_block_size), \
		.params      = &mcux_c40_params_##inst,                            \
		IF_ENABLED(CONFIG_FLASH_MCUX_C40_APPLY_PROTECTION, (               \
			.prot_tbl = mcux_c40_prot_##inst,                          \
			.prot_cnt = mcux_c40_prot_cnt_##inst,                      \
		))                                                                \
	};                                                                        \
	static struct mcux_c40_data mcux_c40_data_##inst;                         \
	DEVICE_DT_INST_DEFINE(inst, mcux_c40_init, NULL,                          \
			      &mcux_c40_data_##inst, &mcux_c40_cfg_##inst,     \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			      &mcux_c40_api);

DT_INST_FOREACH_STATUS_OKAY(C40_INIT)
