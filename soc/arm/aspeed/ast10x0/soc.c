/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 ASPEED Technology Inc.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <soc.h>

extern char __bss_nc_start__[];
extern char __bss_nc_end__[];

/* SCU registers */
#define JTAG_PINMUX_REG		0x41c

/* ASPEED System reset contrl/status register */
#define SYS_WDT4_SW_RESET	BIT(31)
#define SYS_WDT4_ARM_RESET	BIT(30)
#define SYS_WDT4_FULL_RESET	BIT(29)
#define SYS_WDT4_SOC_RESET	BIT(28)
#define SYS_WDT3_SW_RESET	BIT(27)
#define SYS_WDT3_ARM_RESET	BIT(26)
#define SYS_WDT3_FULL_RESET	BIT(25)
#define SYS_WDT3_SOC_RESET	BIT(24)
#define SYS_WDT2_SW_RESET	BIT(23)
#define SYS_WDT2_ARM_RESET	BIT(22)
#define SYS_WDT2_FULL_RESET	BIT(21)
#define SYS_WDT2_SOC_RESET	BIT(20)
#define SYS_WDT1_SW_RESET	BIT(19)
#define SYS_WDT1_ARM_RESET	BIT(18)
#define SYS_WDT1_FULL_RESET	BIT(17)
#define SYS_WDT1_SOC_RESET	BIT(16)

#define SYS_FLASH_ABR_RESET	BIT(2)
#define SYS_EXT_RESET		BIT(1)
#define SYS_PWR_RESET_FLAG	BIT(0)

#define BIT_WDT_SOC(x)	SYS_WDT ## x ## _SOC_RESET
#define BIT_WDT_FULL(x)	SYS_WDT ## x ## _FULL_RESET
#define BIT_WDT_ARM(x)	SYS_WDT ## x ## _ARM_RESET
#define BIT_WDT_SW(x)	SYS_WDT ## x ## _SW_RESET

#define HANDLE_WDTx_RESET(x, event_log, event_log_reg) \
	if (event_log & (BIT_WDT_SOC(x) | BIT_WDT_FULL(x) | BIT_WDT_ARM(x) | BIT_WDT_SW(x))) { \
		printk("RST: WDT%d ", x); \
		if (event_log & BIT_WDT_SOC(x)) { \
			printk("SOC "); \
			sys_write32(BIT_WDT_SOC(x), event_log_reg); \
		} \
		if (event_log & BIT_WDT_FULL(x)) { \
			printk("FULL "); \
			sys_write32(BIT_WDT_FULL(x), event_log_reg); \
		} \
		if (event_log & BIT_WDT_ARM(x)) { \
			printk("ARM "); \
			sys_write32(BIT_WDT_ARM(x), event_log_reg); \
		} \
		if (event_log & BIT_WDT_SW(x)) { \
			printk("SW "); \
			sys_write32(BIT_WDT_SW(x), event_log_reg); \
		} \
		printk("\n"); \
	} \
	(void)(x)

/* secure boot header : provide image size to bootROM for SPI boot */
struct sb_header {
	uint32_t key_location;
	uint32_t enc_img_addr;
	uint32_t img_size;
	uint32_t sign_location;
	uint32_t header_rev[2];
	uint32_t patch_location;
	uint32_t checksum;
};

struct sb_header sbh __attribute((used, section(".sboot"))) = {
	.img_size = (uint32_t)&__bss_start,
};

void z_arm_platform_init(void)
{
	uint32_t jtag_pinmux;
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));

	/* enable JTAG pins */
	jtag_pinmux = sys_read32(base + JTAG_PINMUX_REG);
	jtag_pinmux |= (0x1f << 25);
	sys_write32(jtag_pinmux, base + JTAG_PINMUX_REG);

	/* clear non-cached .bss */
	if (CONFIG_SRAM_NC_SIZE > 0) {
		(void)memset(__bss_nc_start__, 0, __bss_nc_end__ - __bss_nc_start__);
	}

	cache_instr_enable();
}

void aspeed_print_abr_wdt_mode(void)
{
	/* ABR enable */
	if (sys_read32(HW_STRAP2_SCU510) & BIT(11)) {
		printk("FMC ABR: Enable");
		if (sys_read32(HW_STRAP2_SCU510) & BIT(12))
			printk(", Single flash");
		else
			printk(", Dual flashes");

		printk(", Source: %s (%d)",
			(sys_read32(ASPEED_FMC_WDT2_CTRL) & BIT(4)) ? "Alternate" : "Primary",
			(sys_read32(HW_STRAP1_SCU500) & BIT(3)) ? 1 : 0);

		if (sys_read32(HW_STRAP2_SCU510) & GENMASK(15, 13)) {
			printk(", bspi sz: %ldMB",
				BIT((sys_read32(HW_STRAP2_SCU510) >> 13) & 0x7) / 2);
		}
		printk("\n");
	}
}

void aspeed_print_sysrst_info(void)
{
	uint32_t rest1 = sys_read32(SYS_RESET_LOG_REG1);
	uint32_t rest2 = sys_read32(SYS_RESET_LOG_REG2);

	if (rest1 & SYS_PWR_RESET_FLAG) {
		printk("RST: Power On\n");
		sys_write32(rest1, SYS_RESET_LOG_REG1);
	} else {
		HANDLE_WDTx_RESET(4, rest1, SYS_RESET_LOG_REG1);
		HANDLE_WDTx_RESET(3, rest1, SYS_RESET_LOG_REG1);
		HANDLE_WDTx_RESET(2, rest1, SYS_RESET_LOG_REG1);
		HANDLE_WDTx_RESET(1, rest1, SYS_RESET_LOG_REG1);

		if (rest1 & SYS_FLASH_ABR_RESET) {
			printk("RST: SYS_FLASH_ABR_RESET\n");
			sys_write32(SYS_FLASH_ABR_RESET, SYS_RESET_LOG_REG1);
		}

		if (rest1 & SYS_EXT_RESET) {
			printk("RST: External\n");
			sys_write32(SYS_EXT_RESET, SYS_RESET_LOG_REG1);
		}
	}

	ARG_UNUSED(rest2);

	aspeed_print_abr_wdt_mode();
}
