/**
 *******************************************************************************
 *
 * @file flash_atm.c
 *
 * @brief Atmosic Flash Driver
 *
 * Copyright (C) Atmosic 2018-2025
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *******************************************************************************
 */

#define DT_DRV_COMPAT atmosic_external_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/math_extras.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_atm, CONFIG_FLASH_LOG_LEVEL);

#include "arch.h"
#include "at_wrpr.h"
#include "at_apb_qspi_regs_core_macro.h"

#define EXECUTING_IN_PLACE DT_NODE_HAS_COMPAT( \
	DT_MTD_FROM_FIXED_PARTITION(DT_CHOSEN(zephyr_code_partition)), DT_DRV_COMPAT)

#if defined(QSPI_REMOTE_AHB_SETUP_9__ESL__WRITE) && defined(CONFIG_SOC_FLASH_ATM_USE_BREAK_IN)
// enable break in for SOCs that support erase/write breakin
// this allows the CPU to break in during a long running operation
#define FLASH_BREAK_IN
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include "atm_bp_clock.h"
#endif

#if !EXECUTING_IN_PLACE
#include "at_pinmux.h"
#include "at_apb_spi_regs_core_macro.h"
#include "spi.h"
#define __PP_FAST __STATIC_INLINE
#else
// Need to make sure page programming (PP) routines are in RAM
#define __PP_FAST __ramfunc __STATIC
#endif

#ifdef CMSDK_QSPI_NONSECURE
#define CMSDK_QSPI CMSDK_QSPI_NONSECURE
#define CMSDK_SPI2 CMSDK_SPI2_NONSECURE
#include "atm_bp_clock.h"
#include "sec_cache.h"
#define EXT_FLASH_CPU_CACHE_SYNC() ICACHE_FLUSH()
#else
#define EXT_FLASH_CPU_CACHE_SYNC() do {} while(0)
#endif

#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include "at_apb_pseq_regs_core_macro.h"
#ifdef __PSEQ_FLASH_CONTROL2_MACRO__
#include "pseq_states.h"
#define FLASH_PD
#endif
#endif // CONFIG_PM

#ifdef QSPI_FLASH_DBG
#define QSPI_FL_DBG 1
#else
#define QSPI_FL_DBG 0
#endif

#define MAGIC_SECTOR_ERASE_ADDR 0xfffffc

/// SPI flash manufacturer IDs
#define FLASH_MAN_ID_MICRON 0x20
#define FLASH_MAN_ID_PUYA 0x85
#define FLASH_MAN_ID_MACRONIX 0xc2
#define FLASH_MAN_ID_GIANTEC 0xc4
#define FLASH_MAN_ID_GIGA 0xc8
#define FLASH_MAN_ID_FUDAN 0xa1
#define FLASH_MAN_ID_WINBOND 0xef

typedef enum {
	SPI_FLASH_FREAD = 0x0b, // Read Array (fast)
	SPI_FLASH_READ = 0x03, // Read Array (low power)
	SPI_FLASH_DREAD = 0x3b, // Read Dual Output
	SPI_FLASH_2READ = 0xbb, // Read 2x I/O
	SPI_FLASH_QREAD = 0x6b, // Read Quad Output
	SPI_FLASH_4READ = 0xeb, // Read 4x I/O

	SPI_FLASH_PE = 0x81, // Page Erase
	SPI_FLASH_SE = 0x20, // Sector Erase (4K bytes)
	SPI_FLASH_BE32 = 0x52, // Block Erase (32K bytes)
	SPI_FLASH_BE64 = 0xd8, // Block Erase (64K bytes)
	SPI_FLASH_CE = 0x60, // Chip Erase
	SPI_FLASH_CE_ALT = 0xc7, // Chip Erase
	SPI_FLASH_PP = 0x02, // Page Program
	SPI_FLASH_2PP = 0xa2, // Dual-IN Page Program
	SPI_FLASH_QPP = 0x32, // Quad Page Program
	SPI_FLASH_4PP = 0x38, // 4X IO page program (Macronix)
	SPI_FLASH_PES = 0x75, // Program/Erase Suspend
	SPI_FLASH_PES_ALT = 0xb0, // Program/Erase Suspend
	SPI_FLASH_PER = 0x7a, // Program/Erase Resume
	SPI_FLASH_PER_ALT = 0x30, // Program/Erase Resume

	SPI_FLASH_WREN = 0x06, // Write Enable
	SPI_FLASH_WRDI = 0x04, // Write Disable
	SPI_FLASH_VWREN = 0x50, // Volatile SR Write Enable

	SPI_FLASH_ERSCUR = 0x44, // Erase Security Registers
	SPI_FLASH_PRSCUR = 0x42, // Program Security Registers
	SPI_FLASH_RDSCUR = 0x48, // Read Security Registers

	SPI_FLASH_RDSR = 0x05, // Read Status Register
	SPI_FLASH_RDSR2 = 0x35, // Read Status Register
	SPI_FLASH_ASI = 0x25, // Active Status Interrupt
	SPI_FLASH_WRSR = 0x01, // Write Status Register

	SPI_FLASH_RSTEN = 0x66, // Reset Enable
	SPI_FLASH_RST = 0x99, // Reset
	SPI_FLASH_RDID = 0x9f, // Read Manufacturer/Device ID
	SPI_FLASH_REMS = 0x90, // Read Manufacturer ID
	SPI_FLASH_DREMS = 0x92, // Dual Read Manufacturer ID
	SPI_FLASH_QREMS = 0x94, // Quad Read Manufacturer ID
	SPI_FLASH_DP = 0xb9, // Deep Power-down
	SPI_FLASH_RDP = 0xab, // Release Deep Power-down
	SPI_FLASH_SBL = 0x77, // Set Burst Length
	SPI_FLASH_RDSFDP = 0x5a, // Read SFDP
	SPI_FLASH_RRE = 0xff, // Release Read Enhanced
	SPI_FLASH_RUID = 0x4b, // Read Unique ID
} spi_flash_cmd_t;

// performance mode opcode patterns
// GIGA needs M[7:4] bits to be 1010b
#define GIGA_PERF_MODE_OP 0xa0
// PUYA needs M[5:4] bits to be 10b, use this to differentiate PUYA
#define PUYA_PERF_MODE_OP 0x20
// Performance enhance indicator compatible with Macronix, GIGA, Puya
#define COMPAT_PERF_MODE_IND 0xa5

static uint8_t man_id;
static uint32_t flash_size;

#ifdef FLASH_BREAK_IN

#define FLASH_BREAKIN_TIMEOUT_MS 500
#define QSPI_SUSPEND_US          CONFIG_SOC_FLASH_ATM_BREAK_IN_SUSPEND_US

#define QSPI_CLEAR_INTERRUPT()                                                                     \
	do {                                                                                       \
		QSPI_REMOTE_AHB_SETUP_4__CLEAR_INTRP__SET(                                         \
			CMSDK_QSPI_NONSECURE->REMOTE_AHB_SETUP_4);                                 \
		QSPI_REMOTE_AHB_SETUP_4__CLEAR_INTRP__CLR(                                         \
			CMSDK_QSPI_NONSECURE->REMOTE_AHB_SETUP_4);                                 \
	} while (0)

static K_SEM_DEFINE(flash_atm_break_sem, 0, 1);

// wait for the outstanding operation allowing break in to complete
static int flash_atm_wait_break_in_op_done(const char *desc, off_t offset)
{
	LOG_DBG("Wait op: %s offset:0x%08lx", desc, (unsigned long)offset);
	int err = k_sem_take(&flash_atm_break_sem, K_MSEC(FLASH_BREAKIN_TIMEOUT_MS));
	__ASSERT(!err, "breakin wait failed (%d)", err);
	if (err) {
		LOG_ERR("  op wait error: %d", err);
		return err;
	}
	LOG_DBG("  op wait done");
	return 0;
}

void flash_atm_breakin_isr_handler(void const *arg)
{
	QSPI_CLEAR_INTERRUPT();
	k_sem_give(&flash_atm_break_sem);
}

static void external_flash_enable_breakin(void)
{
#ifdef CONFIG_PM
	// breakin is interrupt driven and erase can take 100s of ms to finish
	// hold off low power states to prevent disruption
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
#endif
	// NOTE: any previous mem-mapped write will set the interrupt bit
	// clear this to prevent handling a stale interrupt when unmasking
	QSPI_CLEAR_INTERRUPT();
	QSPI_REMOTE_AHB_SETUP_4__ALLOW_READS_DURING_WRITE__SET(CMSDK_QSPI->REMOTE_AHB_SETUP_4);
	QSPI_REMOTE_AHB_SETUP_4__MASK_INTRP__SET(CMSDK_QSPI->REMOTE_AHB_SETUP_4);
	uint32_t suspend_time = QSPI_SUSPEND_US * atm_bp_clock_get() / 1000000;
	QSPI_REMOTE_AHB_SETUP_9__ESL__MODIFY(CMSDK_QSPI_NONSECURE->REMOTE_AHB_SETUP_9,
					     suspend_time);
	QSPI_REMOTE_AHB_SETUP_9__PSL__MODIFY(CMSDK_QSPI_NONSECURE->REMOTE_AHB_SETUP_9,
					     suspend_time);
}

static void external_flash_disable_breakin(void)
{
	QSPI_REMOTE_AHB_SETUP_4__ALLOW_READS_DURING_WRITE__CLR(CMSDK_QSPI->REMOTE_AHB_SETUP_4);
	QSPI_REMOTE_AHB_SETUP_4__MASK_INTRP__CLR(CMSDK_QSPI->REMOTE_AHB_SETUP_4);
	QSPI_REMOTE_AHB_SETUP_9__ESL__MODIFY(CMSDK_QSPI_NONSECURE->REMOTE_AHB_SETUP_9, 0);
	QSPI_REMOTE_AHB_SETUP_9__PSL__MODIFY(CMSDK_QSPI_NONSECURE->REMOTE_AHB_SETUP_9, 0);
#ifdef CONFIG_PM
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
#endif
}

static void init_flash_breakin(void)
{
	IRQ_CONNECT(QSPI_BREAKIN_IRQn, IRQ_PRI_MID, flash_atm_breakin_isr_handler, NULL, 0);
	QSPI_CLEAR_INTERRUPT();
	irq_enable(QSPI_BREAKIN_IRQn);
	LOG_INF("Flash breakin enabled");
}

#define INIT_FLASH_BREAKIN       init_flash_breakin
#define ENABLE_FLASH_BREAKIN     external_flash_enable_breakin
#define DISABLE_FLASH_BREAKIN    external_flash_disable_breakin
#define SYNC_FLASH_BREAKIN(d, o) flash_atm_wait_break_in_op_done((d), (o))
#else
#define INIT_FLASH_BREAKIN()                                                                       \
	do {                                                                                       \
	} while (false)
#define ENABLE_FLASH_BREAKIN()                                                                     \
	do {                                                                                       \
	} while (false)
#define DISABLE_FLASH_BREAKIN()                                                                    \
	do {                                                                                       \
	} while (false)
#define SYNC_FLASH_BREAKIN(d, o) ({ 0; })
#endif

static void ext_flash_enable_AHB_writes(void)
{
	// PAGE PROGRAM : QIPP : 4PP
	uint8_t pp = (man_id == FLASH_MAN_ID_MICRON) ? SPI_FLASH_PP :
		     ((man_id == FLASH_MAN_ID_WINBOND) || (man_id == FLASH_MAN_ID_GIGA) ||
		      (man_id == FLASH_MAN_ID_FUDAN) || (man_id == FLASH_MAN_ID_PUYA) ||
		      (man_id == FLASH_MAN_ID_GIANTEC)) ?
							       SPI_FLASH_QPP :
							       SPI_FLASH_4PP;

	// Enable writes via AHB
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(pp) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WREN);
}

static void ext_flash_enable_AHB_erases(void)
{
	// Enable erase via AHB
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(SPI_FLASH_SE) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WREN);
}

static void ext_flash_disable_AHB_writes(void)
{
	// Restore REMOTE_AHB_SETUP_2
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WRDI);
}

static void ext_flash_inval_cache(void)
{
	// invalidate the cache if the state of flash has changed (erase, writes)
	uint32_t ras_save = CMSDK_QSPI->REMOTE_AHB_SETUP;
	uint32_t ras = ras_save;
	QSPI_REMOTE_AHB_SETUP__INVALIDATE_ENTIRE_CACHE__SET(ras);
	CMSDK_QSPI->REMOTE_AHB_SETUP = ras;
	CMSDK_QSPI->REMOTE_AHB_SETUP = ras_save;
	EXT_FLASH_CPU_CACHE_SYNC();
}

static int flash_atm_read(struct device const *dev, off_t addr, void *data, size_t len)
{
	LOG_DBG("flash_atm_read(0x%08lx, %zu)", (unsigned long)addr, len);

	if (!man_id) {
		return -ENODEV;
	}

	if (!len) {
		return 0;
	}

	addr += DT_REG_ADDR(SOC_NV_FLASH_NODE);
	memcpy(data, (void *)addr, len);

	return 0;
}

typedef struct {
	uint16_t d;
} __PACKED misaligned_uint16_t;

typedef struct {
	uint32_t d;
} __PACKED misaligned_uint32_t;

static void flash_write_mapped(struct device const *dev, off_t addr, size_t len, void const *data)
{
	// convert to memory mapped address
	addr += DT_REG_ADDR(SOC_NV_FLASH_NODE);
	// Flash writes are WAY more expensive than misaligned RAM reads,
	// so optimize the write transactions more than memcpy() would
	uint32_t end = addr + len;
	uint8_t const *buffer = data;
	if (addr & 0x1) {
		*(volatile uint8_t *)addr++ = *buffer++;
	}

	uint32_t final_16 = end - sizeof(uint16_t);
	if (addr <= final_16) {
		if (addr & 0x2) {
			*(volatile uint16_t *)addr = ((misaligned_uint16_t const *)buffer)->d;
			addr += sizeof(uint16_t);
			buffer += sizeof(uint16_t);
		}

		uint32_t final_32 = end - sizeof(uint32_t);
		while (addr <= final_32) {
			*(volatile uint32_t *)addr = ((misaligned_uint32_t const *)buffer)->d;
			addr += sizeof(uint32_t);
			buffer += sizeof(uint32_t);
		}

		if (addr <= final_16) {
			*(volatile uint16_t *)addr = ((misaligned_uint16_t const *)buffer)->d;
			addr += sizeof(uint16_t);
			buffer += sizeof(uint16_t);
		}
	}

	while (addr < end) {
		*(volatile uint8_t *)addr++ = *buffer++;
	}
}

#define PAGE_SIZE 256
#define PAGE_MASK (PAGE_SIZE - 1)

static void flash_write_pages(struct device const *dev, off_t addr, size_t length,
			      uint8_t const *buffer);

static int flash_atm_write(struct device const *dev, off_t addr, void const *data, size_t len)
{
	LOG_DBG("flash_atm_write(0x%08lx, %zu)", (unsigned long)addr, len);

	if (!man_id) {
		return -ENODEV;
	}

	if (!len) {
		return 0;
	}

	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);
	ext_flash_enable_AHB_writes();

	switch (man_id) {
	case FLASH_MAN_ID_FUDAN:
	case FLASH_MAN_ID_GIANTEC:
	case FLASH_MAN_ID_GIGA:
	case FLASH_MAN_ID_MACRONIX:
	case FLASH_MAN_ID_WINBOND:
		if ((addr % PAGE_SIZE) || (len % PAGE_SIZE)) {
			flash_write_mapped(dev, addr, len, data);
			break;
		}
	// fall through
	case FLASH_MAN_ID_PUYA:
		// always page write with PUYA
		flash_write_pages(dev, addr, len, data);
		break;
	default:
		flash_write_mapped(dev, addr, len, data);
		break;
	}

	ext_flash_disable_AHB_writes();
	// flash state is now changed, invalidate cache
	ext_flash_inval_cache();
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);

	return 0;
}

static int flash_atm_erase(struct device const *dev, off_t addr, size_t size)
{
	LOG_DBG("flash_atm_erase(0x%08lx, %zu)", (unsigned long)addr, size);

	if (!man_id) {
		return -ENODEV;
	}

	if (addr % FLASH_ERASE_BLK_SZ) {
		LOG_ERR("misaligned address: 0x%08lx", (unsigned long)addr);
		return -EINVAL;
	}
	if (size % FLASH_ERASE_BLK_SZ) {
		LOG_ERR("misaligned size: %zu", size);
		return -EINVAL;
	}

	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);
	ext_flash_enable_AHB_erases();
	int err = 0;
	ENABLE_FLASH_BREAKIN();
	for (off_t end = addr + size; addr < end; addr += FLASH_ERASE_BLK_SZ) {
		*(volatile uint32_t *)(DT_REG_ADDR(SOC_NV_FLASH_NODE) + MAGIC_SECTOR_ERASE_ADDR) =
			addr;
		err = SYNC_FLASH_BREAKIN("Erase 1 sector", addr);
		if (err) {
			break;
		}
	}

	ext_flash_disable_AHB_writes();
	DISABLE_FLASH_BREAKIN();
	// flash state is now changed, invalidate cache
	ext_flash_inval_cache();
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);

	return err;
}

static struct flash_parameters const *flash_atm_get_parameters(struct device const *dev)
{
	ARG_UNUSED(dev);

	static struct flash_parameters const flash_atm_parameters = {
		.write_block_size = FLASH_WRITE_BLK_SZ,
		.erase_value = 0xff,
	};

	return &flash_atm_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_atm_pages_layout(struct device const *dev,
				   struct flash_pages_layout const **layout, size_t *layout_size)
{
	static struct flash_pages_layout const flash_atm_pages_layout = {
		.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / FLASH_ERASE_BLK_SZ,
		.pages_size = FLASH_ERASE_BLK_SZ,
	};

	*layout = &flash_atm_pages_layout;
	*layout_size = 1;
}
#endif // CONFIG_FLASH_PAGE_LAYOUT

static struct flash_driver_api const flash_atm_api = {
	.read = flash_atm_read,
	.write = flash_atm_write,
	.erase = flash_atm_erase,
	.get_parameters = flash_atm_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_atm_pages_layout,
#endif
};

#ifdef FLASH_PD
static void notify_pm_state_entry(enum pm_state state)
{
	if (state != PM_STATE_SOFT_OFF) {
		return;
	}

	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE) {
		PSEQ_FLASH_CONTROL2__PSEQ_STATE_MATCH__MODIFY(
			CMSDK_PSEQ->FLASH_CONTROL2, PSEQ_STATE_HIB_START);
	} WRPR_CTRL_POP();
}

static struct pm_notifier notifier = {
	.state_entry = notify_pm_state_entry,
};

static void macronix_flash_enable_pm(void)
{
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE) {
		// Use PSEQ to control flash power across retention
		CMSDK_PSEQ->FLASH_CONTROL =
			PSEQ_FLASH_CONTROL__PD_B4_SLEEP__MASK |
			PSEQ_FLASH_CONTROL__EXPM_EN__MASK |
			PSEQ_FLASH_CONTROL__EXPM_OPCODE__WRITE(SPI_FLASH_RRE) |
			PSEQ_FLASH_CONTROL__PD_OPCODE__WRITE(SPI_FLASH_DP) |
			PSEQ_FLASH_CONTROL__POWER_CYCLE_EN__MASK;
		CMSDK_PSEQ->FLASH_CONTROL2 =
			PSEQ_FLASH_CONTROL2__EXPM_MODE__WRITE(2) |
			PSEQ_FLASH_CONTROL2__PSEQ_STATE_MATCH__WRITE(PSEQ_STATE_RET_ALL_START);
	} WRPR_CTRL_POP();
	pm_notifier_register(&notifier);
}

static void giga_flash_enable_pm(void)
{
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE) {
		// Use PSEQ to control flash power across retention
		CMSDK_PSEQ->FLASH_CONTROL =
			PSEQ_FLASH_CONTROL__PD_B4_SLEEP__MASK |
			PSEQ_FLASH_CONTROL__EXPM_EN__MASK |
			PSEQ_FLASH_CONTROL__EXPM_OPCODE__WRITE(SPI_FLASH_RRE) |
			PSEQ_FLASH_CONTROL__PD_OPCODE__WRITE(SPI_FLASH_DP) |
			PSEQ_FLASH_CONTROL__OPCODE__WRITE(SPI_FLASH_RDP) |
			PSEQ_FLASH_CONTROL__RPD_HAS_CLOCK__MASK |
			PSEQ_FLASH_CONTROL__POWER_CYCLE_EN__MASK;
		CMSDK_PSEQ->FLASH_CONTROL2 =
			PSEQ_FLASH_CONTROL2__EXPM_MODE__WRITE(2) |
			PSEQ_FLASH_CONTROL2__PSEQ_STATE_MATCH__WRITE(PSEQ_STATE_RET_ALL_START);
	} WRPR_CTRL_POP();
	pm_notifier_register(&notifier);
}

static void winbond_flash_enable_pm(void)
{
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE) {
		// Use PSEQ to control flash power across retention
		CMSDK_PSEQ->FLASH_CONTROL =
			PSEQ_FLASH_CONTROL__PD_B4_SLEEP__MASK |
			PSEQ_FLASH_CONTROL__PD_OPCODE__WRITE(SPI_FLASH_DP) |
			PSEQ_FLASH_CONTROL__OPCODE__WRITE(SPI_FLASH_RDP) |
			PSEQ_FLASH_CONTROL__RPD_HAS_CLOCK__MASK |
			PSEQ_FLASH_CONTROL__POWER_CYCLE_EN__MASK;
		CMSDK_PSEQ->FLASH_CONTROL2 =
			PSEQ_FLASH_CONTROL2__EXPM_MODE__WRITE(2) |
			PSEQ_FLASH_CONTROL2__PSEQ_STATE_MATCH__WRITE(PSEQ_STATE_RET_ALL_START);
	} WRPR_CTRL_POP();
	pm_notifier_register(&notifier);
}
#endif

/**
 * @brief Convert nibble to OE/val format for TRANSACTION_SETUP register
 * @param[in] nibble 4-bit value to convert
 * @return OE/val format for TRANSACTION_SETUP register
 */
__PP_FAST uint32_t to_oe_format_quad(uint8_t nibble)
{
	if (nibble > 0xf) {
		return 0;
	}
	return (0x2222 | (nibble & 0x1) | ((nibble & 0x2) << 3) | ((nibble & 0x4) << 6) |
		((nibble & 0x8) << 9));
}

/**
 * @brief Begin QSPI transaction
 */
__PP_FAST void qspi_drive_start(void)
{
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;
}

/**
 * @brief Drive all QSPI outputs for a single cycle
 * @param[in] nibble 4-bit value to drive
 */
__PP_FAST void qspi_drive_nibble(uint8_t nibble)
{
	uint32_t oe = to_oe_format_quad(nibble) << QSPI_TRANSACTION_SETUP__DOUT_0_CTRL__SHIFT;
	CMSDK_QSPI->TRANSACTION_SETUP = oe;
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK | oe;
}

/**
 * @brief Drive all QSPI outputs for two cycles
 * @param[in] byte 8-bit value to drive
 */
__PP_FAST void qspi_drive_byte(uint8_t byte)
{
	qspi_drive_nibble((byte & 0xf0) >> 4);
	qspi_drive_nibble(byte & 0x0f);
}

/**
 * @brief Read all QSPI inputs for two cycles
 */
__PP_FAST void qspi_capture_byte(void)
{
	CMSDK_QSPI->TRANSACTION_SETUP = 0;
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK;
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK |
					QSPI_TRANSACTION_SETUP__SAMPLE_DIN__WRITE(0xf0);

	CMSDK_QSPI->TRANSACTION_SETUP = 0;
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK;
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK |
					QSPI_TRANSACTION_SETUP__SAMPLE_DIN__WRITE(0x0f);
}

/**
 * @brief Drive serial SPI command
 * @param[in] cmd 8-bit command to drive on DOUT_0
 */
__PP_FAST void qspi_drive_serial_cmd(uint8_t cmd)
{
	for (int i = 0; i < 8; i++, cmd <<= 1) {
		uint32_t oe = ((cmd & 0x80) ? 0x0003 : 0x0002)
			      << QSPI_TRANSACTION_SETUP__DOUT_0_CTRL__SHIFT;

		CMSDK_QSPI->TRANSACTION_SETUP = oe;
		CMSDK_QSPI->TRANSACTION_SETUP = oe | QSPI_TRANSACTION_SETUP__CLK_VAL__MASK;
	}
}

/**
 * @brief Read SPI input for 8 cycles
 * @return read byte
 */
__PP_FAST uint8_t qspi_read_serial_byte(void)
{
	uint8_t data = 0;
	for (uint8_t i = 0x80; i; i >>= 1) {
		CMSDK_QSPI->TRANSACTION_SETUP = 0;
		CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK;
		CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK |
						QSPI_TRANSACTION_SETUP__SAMPLE_DIN__WRITE(0x02);

		if (CMSDK_QSPI->READ_DATA & 0x02) {
			data |= i;
		}
	}
	return data;
}

/**
 * @brief Drive dummy cycles on QSPI bus
 * @param[in] cycles Number of dummy cycles to drive
 */
__PP_FAST void qspi_dummy(uint8_t cycles)
{
	for (; cycles; cycles--) {
		CMSDK_QSPI->TRANSACTION_SETUP = 0;
		CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CLK_VAL__MASK;
	}
}

/**
 * @brief End QSPI transaction
 */
__PP_FAST void qspi_drive_stop(void)
{
	CMSDK_QSPI->TRANSACTION_SETUP = 0;
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;
}

#if EXECUTING_IN_PLACE
__ramfunc
#endif
static void flash_write_page(struct device const *dev, off_t addr,
			     size_t length, uint8_t const *buffer)
{
	GLOBAL_INT_DISABLE();
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);

	// Apply bank swap
	addr ^= QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__READ(CMSDK_QSPI->REMOTE_AHB_SETUP_4);

	// !!! from this point forward the QSPI bridge will be disabled

	if (QSPI_REMOTE_AHB_SETUP_3__ENABLE_PERFORMANCE_MODE__READ(
		    CMSDK_QSPI->REMOTE_AHB_SETUP_3)) {
		// Exit performance mode
		qspi_drive_start();
		qspi_drive_serial_cmd(SPI_FLASH_RRE);
		qspi_drive_stop();
	}

	// Set WEL
	qspi_drive_start();
	qspi_drive_serial_cmd(SPI_FLASH_WREN);
	qspi_drive_stop();

	// Quad Page Program
	qspi_drive_start();
	uint8_t qpp = (man_id == FLASH_MAN_ID_MACRONIX) ? SPI_FLASH_4PP : SPI_FLASH_QPP;
	qspi_drive_serial_cmd(qpp);
	if (qpp == SPI_FLASH_4PP) {
		// 4XIO PP, address is sent in quad mode
		qspi_drive_byte((addr >> 16) & 0xff);
		qspi_drive_byte((addr >> 8) & 0xff);
		qspi_drive_byte(addr & 0xff);
	} else {
		// regular QPP, address is sent serially
		qspi_drive_serial_cmd((addr >> 16) & 0xff);
		qspi_drive_serial_cmd((addr >> 8) & 0xff);
		qspi_drive_serial_cmd(addr & 0xff);
	}
	for (; length; length--) {
		qspi_drive_byte(*(buffer++));
	}
	qspi_drive_stop();

	// Poll Status Register until WIP clears
	uint8_t status;
	do {
		qspi_drive_start();
		qspi_drive_serial_cmd(SPI_FLASH_RDSR);
		status = qspi_read_serial_byte();
		qspi_drive_stop();
	} while (status & 0x1);

	if (QSPI_REMOTE_AHB_SETUP_3__ENABLE_PERFORMANCE_MODE__READ(
		    CMSDK_QSPI->REMOTE_AHB_SETUP_3)) {
		// Perform 4READ to enter performance enhance mode
		qspi_drive_start();
		qspi_drive_serial_cmd(SPI_FLASH_4READ);
		for (int i = 0; i < 6; i++) { // 6 address cycles
			qspi_drive_nibble(0);
		}
#ifdef QSPI_REMOTE_AHB_SETUP_3__OPCODE_PERFORMANCE_MODE__READ
		uint8_t ind = QSPI_REMOTE_AHB_SETUP_3__OPCODE_PERFORMANCE_MODE__READ(
			CMSDK_QSPI->REMOTE_AHB_SETUP_3);
		qspi_drive_byte(ind); // Performance enhance indicator
#else
		qspi_drive_byte(COMPAT_PERF_MODE_IND);
#endif
		qspi_dummy(4);       // 4 dummy cycles
		qspi_capture_byte(); // data
		qspi_drive_stop();
	}

	// Switch control from QSPI to AHB bridge
	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__REMOTE_AHB_QSPI_HAS_CONTROL__MASK |
					QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);

	// QSPI bridge is restored this point forward
	GLOBAL_INT_RESTORE();
}

static void flash_write_pages(struct device const *dev, off_t addr, size_t length,
			      uint8_t const *buffer)
{

	if (QSPI_REMOTE_AHB_SETUP_3__ENABLE_PERFORMANCE_MODE__READ(
		    CMSDK_QSPI->REMOTE_AHB_SETUP_3)) {
		ext_flash_inval_cache();
		// Issue a dummy read (end of flash) to get back into performance mode,
		// in case we performed an erase, mem-mapped write or went into power down mode
		// prior to page programming. The bridge disables performance mode for these pior
		// operations and does not re-instate perf mode until the CPU issues a read again.
		uint32_t volatile *dummy = (uint32_t volatile *)(DT_REG_ADDR(SOC_NV_FLASH_NODE) +
								 flash_size - sizeof(uint32_t));
		*dummy;
	}

	// When copying from flash to itself, precopy is required
	bool precopy = ((addr >> QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WIDTH) ==
			((uintptr_t)buffer >> QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WIDTH));
	uint8_t copy_buf[precopy ? PAGE_SIZE : 0];

	while (length) {
		uint32_t next_len = PAGE_SIZE - (addr & PAGE_MASK);
		if (next_len > length) {
			next_len = length;
		}
		uint8_t const *page_buf;
		if (precopy) {
			memcpy(copy_buf, buffer, next_len);
			page_buf = copy_buf;
		} else {
			page_buf = buffer;
		}
		flash_write_page(dev, addr, next_len, page_buf);
		addr += next_len;
		buffer += next_len;
		length -= next_len;
	}
}

#if !EXECUTING_IN_PLACE
static uint8_t spi_flash_wait_for_no_wip(const spi_dev_t *spi)
{
	uint8_t ret;

	// FIXME: timeout
	for (;;) {
		ret = spi_read(spi, SPI_FLASH_RDSR);
		if (!(ret & 0x1)) {
			break;
		}
		YIELD();
	}
	return ret;
}

static void spi_flash_write_enable(const spi_dev_t *spi)
{
	do_spi_transaction(spi, 0, SPI_FLASH_WREN, 0, 0x0, 0x0);
}

static void spi_flash_vsr_write_enable(const spi_dev_t *spi)
{
	do_spi_transaction(spi, 0, SPI_FLASH_VWREN, 0, 0x0, 0x0);
}

static bool spi_macronix_make_quad(const spi_dev_t *spi)
{
	spi_flash_wait_for_no_wip(spi);
	spi_flash_write_enable(spi);

	// WRITE STATUS REG - High perf, Quad Enable
	do_spi_transaction(spi, 0, SPI_FLASH_WRSR, 3, 0x0, 0x020040);

	return ((spi_flash_wait_for_no_wip(spi) & 0x40) == 0x40);
}

static bool spi_giga_make_quad(const spi_dev_t *spi)
{
	if (spi_read(spi, SPI_FLASH_RDSR2) & 0x02) {
		// QE already set
		return true;
	}

	spi_flash_wait_for_no_wip(spi);
	spi_flash_write_enable(spi);

	// WRITE STATUS REG - Quad Enable
	do_spi_transaction(spi, 0, SPI_FLASH_WRSR, 2, 0x0, 0x0200);

	spi_flash_wait_for_no_wip(spi);

	return ((spi_read(spi, SPI_FLASH_RDSR2) & 0x02) == 0x02);
}

static bool spi_winbond_make_quad(const spi_dev_t *spi)
{
	if (spi_read(spi, SPI_FLASH_RDSR2) & 0x02) {
		// QE already set
		return true;
	}
	spi_flash_vsr_write_enable(spi);

	// WRITE STATUS REG-2
	do_spi_transaction(spi, 0, 0x31, 1, 0x0, 0x02);

	return ((spi_read(spi, SPI_FLASH_RDSR2) & 0x02) == 0x02);
}

#ifdef FLASH_PD
/**
 * @brief Command external flash device to power down.
 *
 * @note For FlashROM images, __FAST will locate this function in RAM.
 */
__FAST
static void spi_macronix_deep_power_down(const spi_dev_t *spi)
{
	// Also works as Winbond power-down
	do_spi_transaction(spi, 0, SPI_FLASH_DP, 0, 0x0, 0x0);
}
#endif

static void spi_macronix_exit_deep_power_down(const spi_dev_t *spi)
{
	// Winbond release power-down
	// Also works as Macronix release deep power-down
	do_spi_transaction(spi, 0, SPI_FLASH_RDP, 0, 0x0, 0x0);
}

static void spi_micron_make_quad(const spi_dev_t *spi)
{
	// READ ENHANCED VOLATILE CONFIGURATION REGISTER
	uint8_t evcr = spi_read(spi, 0x65);

	spi_flash_wait_for_no_wip(spi);
	spi_flash_write_enable(spi);

	// WRITE ENHANCED VOLATILE CONFIGURATION REGISTER
	do_spi_transaction(spi, 0, 0x61, 1, 0x0, evcr & ~0xd0);
}

/*
 * QSPI interface
 */
static void do_qspi_cmd(uint8_t opcode)
{
	qspi_drive_start();
	qspi_drive_byte(opcode);
	qspi_drive_stop();
}

static void do_qspi_write(uint8_t opcode, uint8_t data)
{
	qspi_drive_start();
	qspi_drive_byte(opcode);
	qspi_drive_byte(data);
	qspi_drive_stop();
}

static uint8_t do_qspi_read(uint8_t opcode)
{
	qspi_drive_start();
	qspi_drive_byte(opcode);
	qspi_capture_byte();
	qspi_drive_stop();

	return (CMSDK_QSPI->READ_DATA);
}

#ifdef FLASH_PD
static void do_qspi_continuous_read_mode_reset(void)
{
	qspi_drive_start();
	qspi_drive_byte(0xff);
	qspi_drive_byte(0xff);
	qspi_drive_byte(0xff);
	qspi_drive_byte(0xff);
	qspi_drive_stop();
}
#endif

#ifdef CMSDK_QSPI_NONSECURE
static void get_bp_divisor(uint32_t bp_freq, uint8_t *divisor)
{
#if (!defined(IS_FOR_SIM) && !defined(__QSPI_REMOTE_AHB_SETUP_8_MACRO__))
	// ASIC timing limits due to package/board propagation delays:
	//  BP freq / QSPI Clock
	//    16/(16 or 32)
	//    32/(16 or 32)
	//    24/(12 or 24)
	//    48/12 only
	//    64/16 only
	//
	//  The SLIP half cycle provides timing margin if the ratio of
	//  bp_freq/qspi_clk is <= 1.
	//  For 48/24 or 64/32, SLIP half-cycle cannot be used.
	//  For 16/32, 32/32, 24/24 SLIP half-cycle can be used.
	//  Since SLIP half-cycle cannot be used in certain combinations
	//  we will limit the qspi_clk divisor below:

	// Divisor values:
	// 0=/1, 1=/2, 2=/4, 3=/8
	switch (bp_freq) {
		case 64000000:
		case 48000000: {
			// limit to 16Mhz or 12Mhz
			*divisor = 2;
		} break;
		default: {
		} break;
	}
#endif // IS_FOR_SIM
}

#define CLOCK_16_MHZ 16000000
#define MAX_SE_STALL_WIP_VALUE \
	((1 << QSPI_REMOTE_AHB_SETUP_6__SE_STALL_WIP__WIDTH) - 1)
#ifdef QSPI_REMOTE_AHB_SETUP_8__PP_STALL_WIP_MSB__WIDTH
// include the extended MSB bits in setup_8
#define MAX_PP_STALL_WIP_VALUE \
	((1 << (QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WIDTH + \
		QSPI_REMOTE_AHB_SETUP_8__PP_STALL_WIP_MSB__WIDTH)) - \
	 1)
#else
#define MAX_PP_STALL_WIP_VALUE \
	((1 << QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WIDTH) - 1)
#endif
#define MAX_STALL_WLE_VALUE \
	((1 << QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__WIDTH) - 1)
#define MAX_STALL_WE2PP_VALUE \
	((1 << QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__WIDTH) - 1)

static void scale_qspi_settings(uint32_t bp_freq, uint32_t qspi_clk)
{
	// The QSPI controller runs at the bp_frequency. Timing
	// settings involving counters are based on a base clock of 16Mhz.
	// These values need to be scaled up for the higher clock rate of
	// these counters.
	if (bp_freq <= CLOCK_16_MHZ) {
		// nothing to scale
		return;
	}

	uint32_t scale_factor = bp_freq / CLOCK_16_MHZ;
	if (bp_freq % CLOCK_16_MHZ) {
		scale_factor++;
	}
	uint32_t ahb_setup5 = CMSDK_QSPI->REMOTE_AHB_SETUP_5;
	uint32_t ahb_setup6 = CMSDK_QSPI->REMOTE_AHB_SETUP_6;

	DEBUG_TRACE_COND(QSPI_FL_DBG, "REMOTE_AHB_SETUP_5: 0x%" PRIx32,
			 CMSDK_QSPI->REMOTE_AHB_SETUP_5);
	DEBUG_TRACE_COND(QSPI_FL_DBG, "REMOTE_AHB_SETUP_6: 0x%" PRIx32,
			 CMSDK_QSPI->REMOTE_AHB_SETUP_6);

	// scale the Sector Erase STALL_WIP
	uint32_t se_stall_wip = QSPI_REMOTE_AHB_SETUP_6__SE_STALL_WIP__READ(ahb_setup6);
	se_stall_wip *= scale_factor;
	DEBUG_TRACE_COND(QSPI_FL_DBG, "SE_STALL_WIP old 0x%" PRIx32 " new: 0x%" PRIx32,
			 QSPI_REMOTE_AHB_SETUP_6__SE_STALL_WIP__READ(ahb_setup6), se_stall_wip);
	if (se_stall_wip > MAX_SE_STALL_WIP_VALUE) {
		se_stall_wip = MAX_SE_STALL_WIP_VALUE;
		DEBUG_TRACE("!SE_STALL_WIP saturates to: 0x%" PRIx32, se_stall_wip);
	}
	QSPI_REMOTE_AHB_SETUP_6__SE_STALL_WIP__MODIFY(ahb_setup6, se_stall_wip);

	// scale Page Program STALL WIP
	uint32_t pp_stall_wip = QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__READ(ahb_setup5);
	pp_stall_wip *= scale_factor;
	DEBUG_TRACE_COND(QSPI_FL_DBG, "PP_STALL_WIP old 0x%" PRIx32 " new: 0x%" PRIx32,
			 QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__READ(ahb_setup5), pp_stall_wip);
	if (pp_stall_wip > MAX_PP_STALL_WIP_VALUE) {
		pp_stall_wip = MAX_PP_STALL_WIP_VALUE;
		// PP STALL saturating to the max value is okay and not a warning.
		// The controller only page programs 1 word at a time and on most
		// devices this takes 10s of microseconds.
		DEBUG_TRACE_COND(QSPI_FL_DBG, "!PP_STALL_WIP saturates to: 0x%" PRIx32,
				 pp_stall_wip);
	}
	QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__MODIFY(ahb_setup5, pp_stall_wip);
#ifdef QSPI_REMOTE_AHB_SETUP_8__PP_STALL_WIP_MSB__WIDTH
	QSPI_REMOTE_AHB_SETUP_8__PP_STALL_WIP_MSB__MODIFY(
		CMSDK_QSPI->REMOTE_AHB_SETUP_8,
		pp_stall_wip >> QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WIDTH);
#endif
	// scale STALL WLE
	uint32_t stall_wle = QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__READ(ahb_setup5);
	stall_wle *= scale_factor;
	DEBUG_TRACE_COND(QSPI_FL_DBG, "STALL_WLE old 0x%" PRIx32 " new: 0x%" PRIx32,
			 QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__READ(ahb_setup5), stall_wle);
	if (stall_wle > MAX_STALL_WLE_VALUE) {
		stall_wle = MAX_STALL_WLE_VALUE;
		DEBUG_TRACE("!STALL_WLE saturates to: 0x%" PRIx32, stall_wle);
	}
	QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__MODIFY(ahb_setup5, stall_wle);

	// scale STALL WE2PP
	uint32_t stall_we2pp = QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__READ(ahb_setup5);
	stall_we2pp *= scale_factor;
	DEBUG_TRACE_COND(QSPI_FL_DBG, "STALL_WE2PP old 0x%" PRIx32 " new: 0x%" PRIx32,
			 QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__READ(ahb_setup5), stall_we2pp);
	if (stall_we2pp > MAX_STALL_WE2PP_VALUE) {
		stall_we2pp = MAX_STALL_WE2PP_VALUE;
		DEBUG_TRACE("!STALL_WLE saturates to: 0x%" PRIx32, stall_we2pp);
	}
	QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__MODIFY(ahb_setup5, stall_we2pp);

	CMSDK_QSPI->REMOTE_AHB_SETUP_5 = ahb_setup5;
	CMSDK_QSPI->REMOTE_AHB_SETUP_6 = ahb_setup6;

	DEBUG_TRACE_COND(QSPI_FL_DBG, "Adj: REMOTE_AHB_SETUP_5: 0x%" PRIx32,
			 CMSDK_QSPI->REMOTE_AHB_SETUP_5);
	DEBUG_TRACE_COND(QSPI_FL_DBG, "Adj: REMOTE_AHB_SETUP_6: 0x%" PRIx32,
			 CMSDK_QSPI->REMOTE_AHB_SETUP_6);
}

#undef MAX_SE_STALL_WIP_VALUE
#undef MAX_PP_STALL_WIP_VALUE
#undef MAX_STALL_WLE_VALUE
#undef MAX_STALL_WE2PP_VALUE

// final settings, must be called with QSPI clock enabled.
static void adjust_qspi_settings(uint8_t flash_man_id)
{
	uint32_t ahb_setup = CMSDK_QSPI->REMOTE_AHB_SETUP;
	uint32_t qspi_clk;
	uint32_t bp_freq = atm_bp_clock_get();

	if (ahb_setup & QSPI_REMOTE_AHB_SETUP__HYPER__MASK) {
		// hyper can only be used with a 16Mhz bp clock
		__ASSERT(bp_freq == CLOCK_16_MHZ, "bp_freq: %d, ahb_setup: %#x)", bp_freq,
			 ahb_setup);
		qspi_clk = 32000000;
	} else {
		uint8_t divisor = QSPI_REMOTE_AHB_SETUP__CLKDIVSEL__READ(ahb_setup);
		if (!divisor) {
			// divisor not set, check final value
			get_bp_divisor(bp_freq, &divisor);
		}
		QSPI_REMOTE_AHB_SETUP__CLKDIVSEL__MODIFY(ahb_setup, divisor);
		qspi_clk = bp_freq / (1 << divisor);
		// QSPI only supports performance mode when clkdivsel=0
		//   (i.e. bp_freq == qspi_clk).
		if (divisor) {
			uint32_t ahb_setup3 = CMSDK_QSPI->REMOTE_AHB_SETUP_3;
			QSPI_REMOTE_AHB_SETUP_3__ENABLE_PERFORMANCE_MODE__CLR(ahb_setup3);
			CMSDK_QSPI->REMOTE_AHB_SETUP_3 = ahb_setup3;
		}
	}

	if (qspi_clk >= 24000000) {
		QSPI_REMOTE_AHB_SETUP__SLIP_HALF_CYCLE__SET(ahb_setup);
	} else {
		QSPI_REMOTE_AHB_SETUP__SLIP_HALF_CYCLE__CLR(ahb_setup);
	}
#ifdef QSPI_SINGLE_MODE
	if (flash_man_id == FLASH_MAN_ID_GIGA) {
		if (qspi_clk >= 48000000) {
			QSPI_REMOTE_AHB_SETUP__OPCODE__MODIFY(ahb_setup, 0x0b);
		}
	}
#endif
	CMSDK_QSPI->REMOTE_AHB_SETUP = ahb_setup;
	scale_qspi_settings(bp_freq, qspi_clk);
}
#else
static void adjust_qspi_settings(uint8_t flash_man_id)
{
	// nothing to adjust for legacy QSPI
}
#endif // CMSDK_QSPI_NONSECURE

static bool macronix_flash_init(uint8_t mem_cap)
{
	if (!spi_macronix_make_quad(&spi2_8MHz_0)) {
		// Quad mode not working
		WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
		return false;
	}

	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);

	/*
	 * Switch to AHB interface
	 */
	CMSDK_QSPI->REMOTE_AHB_SETUP =
		QSPI_REMOTE_AHB_SETUP__SKEW_CSN_ACT_WEN__MASK | // with div by 1
		QSPI_REMOTE_AHB_SETUP__ENABLE_CACHE__MASK |
		QSPI_REMOTE_AHB_SETUP__ENABLE_CLOCKS__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_WORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_HALFWORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__QUAD_OVERHEAD__MASK |
#ifdef QSPI_REMOTE_AHB_SETUP__DUAL_OVERHEAD__MASK
		QSPI_REMOTE_AHB_SETUP__DUAL_OVERHEAD__MASK |
#endif
		QSPI_REMOTE_AHB_SETUP__RDATA_BYTE_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__CLKDIVSEL__WRITE(0) | // div by 1
		QSPI_REMOTE_AHB_SETUP__OPCODE__WRITE(SPI_FLASH_4READ) |
		QSPI_REMOTE_AHB_SETUP__IS_OPCODE__MASK | QSPI_REMOTE_AHB_SETUP__MODE__WRITE(2) |
		QSPI_REMOTE_AHB_SETUP__DUMMY_CYCLES__WRITE(4);

	// Make sure writes are disabled first
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WRDI);

	CMSDK_QSPI->REMOTE_AHB_SETUP_3 =
		QSPI_REMOTE_AHB_SETUP_3__CHECK_WLE__MASK |
		QSPI_REMOTE_AHB_SETUP_3__WIP_BIT__WRITE(0) |
		QSPI_REMOTE_AHB_SETUP_3__WIP_POLARITY__MASK |
		QSPI_REMOTE_AHB_SETUP_3__WLE_BIT__WRITE(1) |
		QSPI_REMOTE_AHB_SETUP_3__WLE_POLARITY__MASK |
		QSPI_REMOTE_AHB_SETUP_3__ENABLE_PERFORMANCE_MODE__MASK |
		QSPI_REMOTE_AHB_SETUP_3__OPCODE_PERFORMANCE_MODE__WRITE(0x5a);

	CMSDK_QSPI->REMOTE_AHB_SETUP_4 = QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WRITE(1 << mem_cap);

	CMSDK_QSPI->REMOTE_AHB_SETUP_5 = QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WRITE(1023);

	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__REMOTE_AHB_QSPI_HAS_CONTROL__MASK |
					QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;

	adjust_qspi_settings(FLASH_MAN_ID_MACRONIX);

	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);
	return true;
}

static bool giga_flash_init(uint8_t mem_cap, uint8_t flash_man_id)
{
	if (!spi_giga_make_quad(&spi2_8MHz_0)) {
		// Quad mode not working
		WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
		return false;
	}

	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);

	/*
	 * Switch to AHB interface
	 */
	CMSDK_QSPI->REMOTE_AHB_SETUP =
		QSPI_REMOTE_AHB_SETUP__SKEW_CSN_ACT_WEN__MASK | // with div by 1
		QSPI_REMOTE_AHB_SETUP__SERIALIZE_PP_ADDRESS__MASK |
		QSPI_REMOTE_AHB_SETUP__ENABLE_CACHE__MASK |
		QSPI_REMOTE_AHB_SETUP__ENABLE_CLOCKS__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_WORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_HALFWORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__QUAD_OVERHEAD__MASK |
#ifdef QSPI_REMOTE_AHB_SETUP__DUAL_OVERHEAD__MASK
		QSPI_REMOTE_AHB_SETUP__DUAL_OVERHEAD__MASK |
#endif
		QSPI_REMOTE_AHB_SETUP__RDATA_BYTE_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__CLKDIVSEL__WRITE(0) | // div by 1
		QSPI_REMOTE_AHB_SETUP__OPCODE__WRITE(SPI_FLASH_4READ) |
		QSPI_REMOTE_AHB_SETUP__IS_OPCODE__MASK | QSPI_REMOTE_AHB_SETUP__MODE__WRITE(2) |
		QSPI_REMOTE_AHB_SETUP__DUMMY_CYCLES__WRITE(4);

	// Make sure writes are disabled first
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WRDI);

	uint32_t ras3 = QSPI_REMOTE_AHB_SETUP_3__CHECK_WLE__MASK |
			QSPI_REMOTE_AHB_SETUP_3__WIP_BIT__WRITE(0) |
			QSPI_REMOTE_AHB_SETUP_3__WIP_POLARITY__MASK |
			QSPI_REMOTE_AHB_SETUP_3__WLE_BIT__WRITE(1) |
			QSPI_REMOTE_AHB_SETUP_3__WLE_POLARITY__MASK |
			QSPI_REMOTE_AHB_SETUP_3__ENABLE_PERFORMANCE_MODE__MASK;

	if (man_id == FLASH_MAN_ID_PUYA) {
		ras3 |= QSPI_REMOTE_AHB_SETUP_3__OPCODE_PERFORMANCE_MODE__WRITE(PUYA_PERF_MODE_OP);
	} else {
		ras3 |= QSPI_REMOTE_AHB_SETUP_3__OPCODE_PERFORMANCE_MODE__WRITE(GIGA_PERF_MODE_OP);
	}

	CMSDK_QSPI->REMOTE_AHB_SETUP_3 = ras3;

	CMSDK_QSPI->REMOTE_AHB_SETUP_4 = QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WRITE(1 << mem_cap);

	CMSDK_QSPI->REMOTE_AHB_SETUP_5 = QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WRITE(1023);

	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__REMOTE_AHB_QSPI_HAS_CONTROL__MASK |
					QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;

	adjust_qspi_settings(flash_man_id);

	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);
	return true;
}

static bool winbond_flash_init(uint8_t mem_cap)
{
	if (!spi_winbond_make_quad(&spi2_8MHz_0)) {
		// Quad mode not working
		WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
		return false;
	}

	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);

	/*
	 * Switch to AHB interface
	 */
	CMSDK_QSPI->REMOTE_AHB_SETUP =
		QSPI_REMOTE_AHB_SETUP__SKEW_CSN_ACT_WEN__MASK | // with div by 1
		QSPI_REMOTE_AHB_SETUP__SERIALIZE_PP_ADDRESS__MASK |
		QSPI_REMOTE_AHB_SETUP__ENABLE_CACHE__MASK |
		QSPI_REMOTE_AHB_SETUP__ENABLE_CLOCKS__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_WORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_HALFWORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__QUAD_OVERHEAD__MASK |
#ifdef QSPI_REMOTE_AHB_SETUP__DUAL_OVERHEAD__MASK
		QSPI_REMOTE_AHB_SETUP__DUAL_OVERHEAD__MASK |
#endif
		QSPI_REMOTE_AHB_SETUP__RDATA_BYTE_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__CLKDIVSEL__WRITE(0) | // div by 1
		QSPI_REMOTE_AHB_SETUP__OPCODE__WRITE(SPI_FLASH_4READ) |
		QSPI_REMOTE_AHB_SETUP__IS_OPCODE__MASK | QSPI_REMOTE_AHB_SETUP__MODE__WRITE(2) |
		QSPI_REMOTE_AHB_SETUP__DUMMY_CYCLES__WRITE(4);

	// Make sure writes are disabled first
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WRDI);

	CMSDK_QSPI->REMOTE_AHB_SETUP_3 = QSPI_REMOTE_AHB_SETUP_3__CHECK_WLE__MASK |
					 QSPI_REMOTE_AHB_SETUP_3__WIP_BIT__WRITE(0) |
					 QSPI_REMOTE_AHB_SETUP_3__WIP_POLARITY__MASK |
					 QSPI_REMOTE_AHB_SETUP_3__WLE_BIT__WRITE(1) |
					 QSPI_REMOTE_AHB_SETUP_3__WLE_POLARITY__MASK;

	CMSDK_QSPI->REMOTE_AHB_SETUP_4 = QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WRITE(1 << mem_cap);

	CMSDK_QSPI->REMOTE_AHB_SETUP_5 = QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WRITE(1023);

	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__REMOTE_AHB_QSPI_HAS_CONTROL__MASK |
					QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;

	adjust_qspi_settings(FLASH_MAN_ID_WINBOND);

	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);
	return true;
}

#ifdef FLASH_PD
/**
 * @brief Place external flash device into deep power down.
 *
 * @note For FlashROM images, __FAST will locate this function in RAM.
 */
__FAST
static void fast_macronix_deep_power_down(void)
{
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);
	{
		do_qspi_continuous_read_mode_reset();

		// Switch control from AHB bridge to SPI2
		CMSDK_QSPI->TRANSACTION_SETUP =
			QSPI_TRANSACTION_SETUP__REMOTE_SPI_HAS_CONTROL__MASK |
			QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;
	}
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);

	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__CLK_ENABLE);
	{
		spi_macronix_deep_power_down(&spi2_8MHz_0);
	}
	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
}

/**
 * @brief Place external flash device into deep power down.
 *
 * @note For FlashROM images, __FAST will locate this function in RAM.
 */
__FAST
static void fast_winbond_deep_power_down(void)
{
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);
	{
		// Switch control from AHB bridge to SPI2
		CMSDK_QSPI->TRANSACTION_SETUP =
			QSPI_TRANSACTION_SETUP__REMOTE_SPI_HAS_CONTROL__MASK |
			QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;

		WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__CLK_ENABLE);
		{
			spi_macronix_deep_power_down(&spi2_8MHz_0);
		}
		WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);

		// Switch control from SPI2 to AHB bridge
		CMSDK_QSPI->TRANSACTION_SETUP =
			QSPI_TRANSACTION_SETUP__REMOTE_AHB_QSPI_HAS_CONTROL__MASK |
			QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;
	}
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);
}

__UNUSED static void (*qspi_flash_deep_power_down)(void);
#endif

static bool micron_flash_init(uint8_t mem_cap)
{
	/*
	 * Switch to Quad Mode
	 */
	spi_micron_make_quad(&spi2_8MHz_0);

	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);

	// READ ID isn't support in quad mode
	// Instead use MULTIPLE I/O READ ID; dummy clock = 0; opcode = 0xaf
	uint8_t qspi_flash_id = do_qspi_read(0xaf);
	if (qspi_flash_id != FLASH_MAN_ID_MICRON) {
		LOG_INF("QSPI read ID %#x", qspi_flash_id);
		// Quad not working
		WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__SRESET);
		return false;
	}

#ifdef USE_MICRON_XIP_MODE
	/*
	 * Switch to Micron XIP Mode
	 * Micron XIP mode requires only an address (no inst) to output data.
	 */
	do_qspi_cmd(SPI_FLASH_WREN);

	// WRITE VOLATILE CONFIGURATION REGISTER
	// 7:4 = dummy cycles = 1
	// 3 = XIP; watch out! it's active low for some odd reason
	// 2 = reserved; live with default which is 1'b0
	// 1:0 = wrap; live with default which is 2'b11
	// So need to write 0001_0011 = 0x13
	do_qspi_write(0x81, 0x13);

	// FAST READ with XIP Confirmation Bit = 0
	qspi_drive_start();
	qspi_drive_byte(SPI_FLASH_FREAD);
	int i;
	for (i = 0; i < 8; i++) { // address + dummy
		qspi_drive_nibble(0);
	}
	qspi_capture_byte();
	uint8_t fr_byte = CMSDK_QSPI->READ_DATA;
	qspi_drive_stop();
	LOG_INF("FAST READ of 0x0: %#x", fr_byte);
#else // USE_MICRON_XIP_MODE
	do_qspi_cmd(SPI_FLASH_WREN);

	// WRITE VOLATILE CONFIGURATION REGISTER
	// 7:4 = dummy cycles = 1
	// 3 = XIP; watch out! it's active low for some odd reason
	// 2 = reserved; live with default which is 1'b0
	// 1:0 = wrap; live with default which is 2'b11
	// So need to write 0001_1011 = 0x1b
	do_qspi_write(0x81, 0x1b);
#endif // USE_MICRON_XIP_MODE

	/*
	 * Switch to AHB interface
	 */
	CMSDK_QSPI->REMOTE_AHB_SETUP =
		QSPI_REMOTE_AHB_SETUP__SKEW_CSN_ACT_WEN__MASK | // with div by 1
		QSPI_REMOTE_AHB_SETUP__ENABLE_CACHE__MASK |
		QSPI_REMOTE_AHB_SETUP__ENABLE_CLOCKS__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_WORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__WDATA_HALFWORD_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__RDATA_BYTE_SWAP__MASK |
		QSPI_REMOTE_AHB_SETUP__CLKDIVSEL__WRITE(0) | // div by 1
#ifndef USE_MICRON_XIP_MODE
		QSPI_REMOTE_AHB_SETUP__OPCODE__WRITE(SPI_FLASH_4READ) |
		QSPI_REMOTE_AHB_SETUP__IS_OPCODE__MASK |
#endif
		QSPI_REMOTE_AHB_SETUP__MODE__WRITE(2) |
		QSPI_REMOTE_AHB_SETUP__DUMMY_CYCLES__WRITE(1);

	// Make sure writes are disabled first
	CMSDK_QSPI->REMOTE_AHB_SETUP_2 =
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_SE__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WIP__WRITE(SPI_FLASH_RDSR) |
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_PP__WRITE(0x00) | // NOP
		QSPI_REMOTE_AHB_SETUP_2__OPCODE_WE__WRITE(SPI_FLASH_WRDI);

	CMSDK_QSPI->REMOTE_AHB_SETUP_3 = QSPI_REMOTE_AHB_SETUP_3__CHECK_WLE__MASK |
					 QSPI_REMOTE_AHB_SETUP_3__WIP_BIT__WRITE(0) |
					 QSPI_REMOTE_AHB_SETUP_3__WIP_POLARITY__MASK |
					 QSPI_REMOTE_AHB_SETUP_3__WLE_BIT__WRITE(1) |
					 QSPI_REMOTE_AHB_SETUP_3__WLE_POLARITY__MASK;

	CMSDK_QSPI->REMOTE_AHB_SETUP_4 = QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WRITE(1 << mem_cap);

	CMSDK_QSPI->REMOTE_AHB_SETUP_5 = QSPI_REMOTE_AHB_SETUP_5__STALL_WE2PP__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__STALL_WLE__WRITE(32) |
					 QSPI_REMOTE_AHB_SETUP_5__PP_STALL_WIP__WRITE(1023);

	CMSDK_QSPI->TRANSACTION_SETUP = QSPI_TRANSACTION_SETUP__REMOTE_AHB_QSPI_HAS_CONTROL__MASK |
					QSPI_TRANSACTION_SETUP__CSN_VAL__MASK;

	adjust_qspi_settings(FLASH_MAN_ID_MICRON);

	WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_DISABLE);
	return true;
}
/*
 * Configure pinumx for QSPI signals
 */
static void flash_init_pinmux(void)
{
	PIN_SELECT(DT_INST_PROP(0, clk_pin), QSPI_CLK);
	PIN_SELECT(DT_INST_PROP(0, csn_pin), QSPI_CSN);
	PIN_SELECT(DT_INST_PROP(0, d0_pin), QSPI_D0);
	PIN_SELECT(DT_INST_PROP(0, d1_pin), QSPI_D1);
	PIN_SELECT(DT_INST_PROP(0, d2_pin), QSPI_D2);
	PIN_SELECT(DT_INST_PROP(0, d3_pin), QSPI_D3);
	PIN_PULL_CLR(DT_INST_PROP(0, clk_pin));
	PIN_PULL_CLR(DT_INST_PROP(0, csn_pin));
#ifdef CONFIG_SOC_SERIES_ATM33
	PIN_PULLUP(DT_INST_PROP(0, d0_pin));
	PIN_PULLUP(DT_INST_PROP(0, d1_pin));
#else
	PIN_PULL_CLR(DT_INST_PROP(0, d0_pin));
	PIN_PULL_CLR(DT_INST_PROP(0, d1_pin));
#endif
	PIN_PULLUP(DT_INST_PROP(0, d2_pin));
	PIN_PULLUP(DT_INST_PROP(0, d3_pin));
}

/*
 * external_flash_wakeup
 *
 * Flash might have been in deep power down during hibernation,
 * so wake it up well before first attempted access.
 */
static void external_flash_wakeup(void)
{
	flash_init_pinmux();

	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__CLK_ENABLE);
	{
		spi_macronix_exit_deep_power_down(&spi2_8MHz_0);
	}
	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
}

/*
 * Check SPI2 for flash device
 */
static bool flash_discover(void)
{
	WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__CLK_ENABLE);

	// READ ID opcode and set up AHB bridge
	uint8_t mem_cap;
	int i;
	for (i = 0; i < 2; i++) {
		uint32_t ext_flash_id = spi_read_3(&spi2_8MHz_0, SPI_FLASH_RDID);
		LOG_INF("SPI2 read ID %#" PRIx32, ext_flash_id);
		man_id = ext_flash_id & 0xff;
		mem_cap = ext_flash_id >> 16;
		// Bridge only supports 3 byte addressing
		if (mem_cap > QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WIDTH) {
			mem_cap = QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WIDTH;
		}
		flash_size = 1 << mem_cap;
		if (man_id == FLASH_MAN_ID_MICRON) {
			if (!micron_flash_init(mem_cap)) {
				return false;
			}
			break;
		} else if (man_id == FLASH_MAN_ID_MACRONIX) {
			if (!macronix_flash_init(mem_cap)) {
				return false;
			}

#ifdef FLASH_PD
			// Deep Power-down
			qspi_flash_deep_power_down = fast_macronix_deep_power_down;
			macronix_flash_enable_pm();
#endif
			break;
		} else if ((man_id == FLASH_MAN_ID_GIGA) || (man_id == FLASH_MAN_ID_FUDAN) ||
			   (man_id == FLASH_MAN_ID_PUYA) || (man_id == FLASH_MAN_ID_GIANTEC)) {
			if (!giga_flash_init(mem_cap, man_id)) {
				return false;
			}

#ifdef FLASH_PD
			// Deep Power-down
			qspi_flash_deep_power_down = fast_macronix_deep_power_down;
			giga_flash_enable_pm();
#endif
			break;
		} else if (man_id == FLASH_MAN_ID_WINBOND) {
			if (!winbond_flash_init(mem_cap)) {
				return false;
			}

#ifdef FLASH_PD
			// Power-down
			qspi_flash_deep_power_down = fast_winbond_deep_power_down;
			winbond_flash_enable_pm();
#endif
			break;
		}

		/* Might be a micron in qspi mode */

		WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__CLK_ENABLE);
		{
			do_qspi_cmd(SPI_FLASH_WREN);
			// WRITE VOLATILE CONFIGURATION REGISTER
			do_qspi_write(0x81, 0xfb);

			// READ ENHANCED VOLATILE CONFIGURATION REGISTER
			uint8_t evcr = do_qspi_read(0x65);

			do_qspi_cmd(SPI_FLASH_WREN);
			// WRITE ENHANCED VOLATILE CONFIGURATION REGISTER
			do_qspi_write(0x61, evcr | 0xd0);
		}
		// Switch from QSPI to SPI
		WRPR_CTRL_SET(CMSDK_QSPI, WRPR_CTRL__SRESET);
	}
	if (i == 2) {
		// No flash connected
		WRPR_CTRL_SET(CMSDK_SPI2, WRPR_CTRL__SRESET);
		return false;
	}

	return true;
}

#endif // EXECUTING_IN_PLACE

static void recover_man_id(void)
{
	uint32_t ras = CMSDK_QSPI->REMOTE_AHB_SETUP;
#ifdef QSPI_REMOTE_AHB_SETUP__IS_MACRONIX__MASK
	if (ras & QSPI_REMOTE_AHB_SETUP__IS_MACRONIX__MASK)
#else
	if (ras & QSPI_REMOTE_AHB_SETUP__QUAD_OVERHEAD__MASK)
#endif
	{
		if (ras & QSPI_REMOTE_AHB_SETUP__SERIALIZE_PP_ADDRESS__MASK) {
			uint32_t ras3 = CMSDK_QSPI->REMOTE_AHB_SETUP_3;
#ifdef QSPI_REMOTE_AHB_SETUP_3__EXPM__MASK
			if (ras3 & QSPI_REMOTE_AHB_SETUP_3__EXPM__MASK) {
#else
			uint32_t opcode =
				QSPI_REMOTE_AHB_SETUP_3__OPCODE_PERFORMANCE_MODE__READ(ras3);
			if ((opcode == GIGA_PERF_MODE_OP) || (opcode == PUYA_PERF_MODE_OP)) {
				if (opcode == PUYA_PERF_MODE_OP) {
					man_id = FLASH_MAN_ID_PUYA;
				} else
#endif
				{
#ifdef CONFIG_SOC_FLASH_ATM_FORCE_PUYA
					// legacy flash init cannot differentiate
					// GIGA from PUYA or FUDAN
					man_id = FLASH_MAN_ID_PUYA;
#else
					// Can't tell GIGA apart from FUDAN
					man_id = FLASH_MAN_ID_GIGA;
#endif
				}
#ifdef FLASH_PD
				giga_flash_enable_pm();
#endif
			} else {
				man_id = FLASH_MAN_ID_WINBOND;
#ifdef FLASH_PD
				winbond_flash_enable_pm();
#endif
			}
		} else {
			man_id = FLASH_MAN_ID_MACRONIX;
#ifdef FLASH_PD
			macronix_flash_enable_pm();
#endif
		}
	} else {
		man_id = FLASH_MAN_ID_MICRON;
	}

	uint32_t qspi_ras4 = CMSDK_QSPI->REMOTE_AHB_SETUP_4;
	uint8_t mem_cap = u32_count_trailing_zeros(qspi_ras4);
	// Bridge only supports 3 byte addressing
	if (mem_cap > QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WIDTH) {
		mem_cap = QSPI_REMOTE_AHB_SETUP_4__INVERT_ADDR__WIDTH;
	}
	flash_size = 1 << mem_cap;

	LOG_INF("recovered man_id:%#x, size:%#x", man_id, flash_size);
}

static int flash_atm_init(struct device const *dev)
{
	LOG_DBG("flash_atm base:0x%08lx", (unsigned long)DT_REG_ADDR(SOC_NV_FLASH_NODE));

#if !EXECUTING_IN_PLACE
	if (WRPR_CTRL_GET(CMSDK_QSPI) == WRPR_CTRL__CLK_DISABLE) {
		// flash was already initialized by a bootloader
		// just recover the manufacturer ID
		recover_man_id();
		INIT_FLASH_BREAKIN();
		return 0;
	}

	external_flash_wakeup();
	bool found = flash_discover();

	if (found) {
		LOG_INF("man_id:%#x", man_id);
	}
#else
	recover_man_id();
#endif
	INIT_FLASH_BREAKIN();
	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_atm_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_atm_api);
