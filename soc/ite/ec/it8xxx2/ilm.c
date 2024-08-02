/*
 * Copyright 2022 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(soc_it8xxx2_ilm, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * Instruction Local Memory (ILM) support for IT8xxx2.
 *
 * IT8xxx2 allows 4-kilobyte blocks of RAM be configured individually as either Instruction- or
 * Data Local Memory (ILM or DLM). Addresses from which instructions will be fetched by the CPU
 * *must* be in the Flash memory space: it is not permitted to execute from RAM addresses, only
 * through ILM mappings into RAM.
 *
 * When a RAM block is configured as ILM, accesses to addresses matching the corresponding Scratch
 * SRAM address register (SCARn{H,M,L}) are redirected to the corresponding ILM block in RAM.
 * If SCAR0 (corresponding to ILM0) has the value 0x8021532 and ILM0 is enabled, then instruction
 * fetches from the memory range 0x8021532..0x8022532 will be redirected to physical addresses
 * 0x80100000..0x80101000 (the first 4k block of RAM).
 *
 * Instruction fetch from Flash is normally cacheable, but configuring ILM for a region makes that
 * address range non-cacheable (which is appropriate because Flash has high latency but RAM is
 * essentially the same speed as cache).
 */

extern const uint8_t __ilm_flash_start[];
extern const uint8_t __ilm_flash_end[];
extern uint8_t __ilm_ram_start[];
extern uint8_t __ilm_ram_end[];

#define ILM_BLOCK_SIZE 0x1000
BUILD_ASSERT((ILM_BLOCK_SIZE & (ILM_BLOCK_SIZE - 1)) == 0, "ILM_BLOCK_SIZE must be a power of two");

#define FLASH_BASE CONFIG_FLASH_BASE_ADDRESS
#define RAM_BASE   CONFIG_SRAM_BASE_ADDRESS

#define ILM_NODE DT_NODELABEL(ilm)

#define SCARH_ENABLE	 BIT(3)
#define SCARH_ADDR_BIT19 BIT(7)

/*
 * SCAR registers contain 20-bit addresses in three registers, with one set
 * of SCAR registers for each ILM block that may be configured.
 */
struct scar_reg {
	/* Bits 0..7 of address; SCARnL */
	uint8_t l;
	/* Bits 8..15 of address; SCARnM */
	uint8_t m;
	/* Bits 16..18 and 19 of address, plus the enable bit for the entire SCAR; SCARnH */
	uint8_t h;
};

struct ilm_config {
	volatile struct scar_reg *scar_regs[CONFIG_ILM_MAX_SIZE / 4];
};

bool it8xxx2_is_ilm_configured(void)
{
	return device_is_ready(DEVICE_DT_GET(ILM_NODE));
}

static bool __maybe_unused is_block_aligned(const void *const p)
{
	return ((uintptr_t)p & (ILM_BLOCK_SIZE - 1)) == 0;
}

static int it8xxx2_configure_ilm_block(const struct ilm_config *const config, void *ram_addr,
				       const void *flash_addr, const size_t copy_sz)
{
	if ((uintptr_t)ram_addr < RAM_BASE) {
		return -EFAULT; /* Not in RAM */
	}
	const int dirmap_index = ((uintptr_t)ram_addr - RAM_BASE) / ILM_BLOCK_SIZE;

	if (dirmap_index >= ARRAY_SIZE(config->scar_regs)) {
		return -EFAULT; /* Past the end of RAM */
	}
	BUILD_ASSERT((FLASH_BASE & GENMASK(19, 0)) == 0,
		     "Flash is assumed to be aligned to SCAR register width");
	if (((uintptr_t)flash_addr - FLASH_BASE) & ~GENMASK(19, 0)) {
		return -EFAULT; /* Address doesn't fit in the SCAR */
	}
	if (!is_block_aligned(flash_addr)) {
		/* Bits 0..11 of SCAR can be programmed but ILM only works if they're zero */
		return -EFAULT;
	}

	LOG_DBG("Enabling ILM%d %p -> %p, copy %d", dirmap_index, flash_addr, ram_addr, copy_sz);

	volatile struct scar_reg *const scar = config->scar_regs[dirmap_index];

	int irq_key = irq_lock();

	/* Ensure scratch RAM for block data access is enabled */
	scar->h = SCARH_ENABLE;
	/* Copy block contents from flash into RAM */
	memcpy(ram_addr, flash_addr, copy_sz);
	/* Program SCAR */
	scar->l = (uintptr_t)flash_addr & GENMASK(7, 0);
	scar->m = ((uintptr_t)flash_addr & GENMASK(15, 8)) >> 8;

	uint8_t scarh_value = ((uintptr_t)flash_addr & GENMASK(18, 16)) >> 16;

	if ((uintptr_t)flash_addr & BIT(19)) {
		scarh_value |= SCARH_ADDR_BIT19;
	}
	scar->h = scarh_value;

	irq_unlock(irq_key);
	return 0;
}

static int it8xxx2_ilm_init(const struct device *dev)
{
	/* Invariants enforced by the linker script */
	__ASSERT(is_block_aligned(__ilm_ram_start),
		 "ILM physical base address (%p) must be 4k-aligned", __ilm_ram_start);
	__ASSERT(is_block_aligned(__ilm_flash_start),
		 "ILM flash base address (%p) must be 4k-aligned", __ilm_flash_start);
	__ASSERT_NO_MSG((uintptr_t)__ilm_ram_end >= (uintptr_t)__ilm_ram_start &&
			(uintptr_t)__ilm_flash_end >= (uintptr_t)__ilm_flash_start);

	LOG_DBG("ILM init %p-%p -> %p-%p", __ilm_flash_start, __ilm_flash_end, __ilm_ram_start,
		__ilm_ram_end);
	for (uintptr_t block_base = (uintptr_t)__ilm_ram_start;
	     block_base < (uintptr_t)__ilm_ram_end; block_base += ILM_BLOCK_SIZE) {
		uintptr_t flash_base =
			(uintptr_t)__ilm_flash_start + (block_base - (uintptr_t)__ilm_ram_start);
		/*
		 * Part of the target RAM block might be used for non-code data; avoid overwriting
		 * it by only copying as much data as the ILM flash region contains.
		 */
		size_t used_size = MIN((uintptr_t)__ilm_flash_end - flash_base, ILM_BLOCK_SIZE);
		int rv = it8xxx2_configure_ilm_block(dev->config, (void *)block_base,
						     (const void *)flash_base, used_size);

		if (rv) {
			LOG_ERR("Unable to configure ILM block %p: %d", (void *)flash_base, rv);
			return rv;
		}
	}

	return 0;
}

#define SCAR_REG(n) (volatile struct scar_reg *)DT_REG_ADDR_BY_IDX(ILM_NODE, n)

static const struct ilm_config ilm_config = {
	.scar_regs = {
		/* SCAR0 SRAM 4KB */
		SCAR_REG(0),
		SCAR_REG(1),
		SCAR_REG(2),
		SCAR_REG(3),
		SCAR_REG(4),
		SCAR_REG(5),
		SCAR_REG(6),
		SCAR_REG(7),
		SCAR_REG(8),
		SCAR_REG(9),
		SCAR_REG(10),
		SCAR_REG(11),
		SCAR_REG(12),
		SCAR_REG(13),
		SCAR_REG(14),
		/*
		 * Except for CONFIG_SOC_IT81202CX and CONFIG_SOC_IT81302CX
		 * maximum ILM size are 60KB, the ILM size of other varients
		 * are equal to the SRAM size.
		 */
#if (CONFIG_ILM_MAX_SIZE == 256)
		/* SCAR15 SRAM 4KB */
		SCAR_REG(15),
		/* SCAR16 SRAM 16KB */
		SCAR_REG(16), SCAR_REG(16), SCAR_REG(16), SCAR_REG(16),
		/* SCAR17 SRAM 16KB */
		SCAR_REG(17), SCAR_REG(17), SCAR_REG(17), SCAR_REG(17),
		/* SCAR18 SRAM 16KB */
		SCAR_REG(18), SCAR_REG(18), SCAR_REG(18), SCAR_REG(18),
		/* SCAR19 SRAM 16KB */
		SCAR_REG(19), SCAR_REG(19), SCAR_REG(19), SCAR_REG(19),
		/* SCAR20 SRAM 32KB */
		SCAR_REG(20), SCAR_REG(20), SCAR_REG(20), SCAR_REG(20),
		SCAR_REG(20), SCAR_REG(20), SCAR_REG(20), SCAR_REG(20),
		/* SCAR21 SRAM 32KB */
		SCAR_REG(21), SCAR_REG(21), SCAR_REG(21), SCAR_REG(21),
		SCAR_REG(21), SCAR_REG(21), SCAR_REG(21), SCAR_REG(21),
		/* SCAR22 SRAM 32KB */
		SCAR_REG(22), SCAR_REG(22), SCAR_REG(22), SCAR_REG(22),
		SCAR_REG(22), SCAR_REG(22), SCAR_REG(22), SCAR_REG(22),
		/* SCAR23 SRAM 32KB */
		SCAR_REG(23), SCAR_REG(23), SCAR_REG(23), SCAR_REG(23),
		SCAR_REG(23), SCAR_REG(23), SCAR_REG(23), SCAR_REG(23)
#endif
	}};
BUILD_ASSERT(ARRAY_SIZE(ilm_config.scar_regs) * ILM_BLOCK_SIZE == KB(CONFIG_ILM_MAX_SIZE),
	     "Wrong number of SCAR registers defined for RAM size");

DEVICE_DT_DEFINE(ILM_NODE, &it8xxx2_ilm_init, NULL, NULL, &ilm_config, PRE_KERNEL_1, 0, NULL);
