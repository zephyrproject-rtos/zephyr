/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Basic initialization for the CC2650 System on Chip.
 */


#include <toolchain/gcc.h>
#include <init.h>
#include <sys/sys_io.h>

#include "soc.h"


#define CCFG_SIZE	88

/* The bootloader of the SoC (in ROM) reads configuration
 * data (CCFG) at a fixed address (last page of flash).
 * The most notable information being whether to run the
 * code stored in flash or not.
 *
 * We put configuration data in a specific section so that
 * the linker script can map it accordingly.
 */
const u32_t
__ti_ccfg_section
ti_ccfg[CCFG_SIZE / sizeof(u32_t)] = {
	0x00008001, /* EXT_LF_CLK: default values */
	0xFF13FFFF, /* MODE_CONF_1: default values */
	0x0058FFFF, /* SIZE_AND_DIS_FLAGS: 88 bytes long, no external osc. */
	0xFFFFFFFF, /* MODE_CONF: default values */
	0xFFFFFFFF, /* VOLT_LOAD_0: default values */
	0xFFFFFFFF, /* VOLT_LOAD_1: default values */
	0xFFFFFFFF, /* RTC_OFFSET: default values */
	0xFFFFFFFF, /* FREQ_OFFSET: default values */
	0xFFFFFFFF, /* IEEE_MAC_0: use MAC address from FCFG */
	0xFFFFFFFF, /* IEEE_MAC_1: use MAC address from FCFG */
	0xFFFFFFFF, /* IEEE_BLE_0: use BLE address from FCFG */
	0xFFFFFFFF, /* IEEE_BLE_1: use BLE address from FCFG */
	/* BL_CONFIG: disable backdoor and bootloader,
	 * default pin, default active level (high)
	 */
	CC2650_CCFG_BACKDOOR_DISABLED |
		(0xFF << CC2650_CCFG_BL_CONFIG_BL_PIN_NUMBER_POS) |
		(0x1  << CC2650_CCFG_BL_CONFIG_BL_LEVEL_POS) |
		0x00FE0000 | /* reserved */
		CC2650_CCFG_BOOTLOADER_DISABLED,
	0xFFFFFFFF, /* ERASE_CONF: default values (banks + chip erase) */
	/* CCFG_TI_OPTIONS: disable TI failure analysis */
	CC2650_CCFG_TI_FA_DISABLED |
		0xFFFFFF00, /* reserved */
	0xFFC5C5C5, /* CCFG_TAP_DAP_0: default values */
	0xFFC5C5C5, /* CCFG_TAP_DAP_1: default values */
	/* IMAGE_VALID_CONF: authorize program on flash to run */
	CC2650_CCFG_IMAGE_IS_VALID,
	/* Make all flash chip programmable + erasable
	 * (which is default)
	 */
	0xFFFFFFFF, /* CCFG_PROT_31_0 */
	0xFFFFFFFF, /* CCFG_PROT_61_32 */
	0xFFFFFFFF, /* CCFG_PROT_95_64 */
	0xFFFFFFFF  /* CCFG_PROT_127_96 */
};

/* PRCM Registers */
static const u32_t clkloadctl =
	REG_ADDR(DT_TI_CC2650_PRCM_40082000_BASE_ADDRESS,
		 CC2650_PRCM_CLKLOADCTL);
static const u32_t gpioclkgr =
	REG_ADDR(DT_TI_CC2650_PRCM_40082000_BASE_ADDRESS,
		 CC2650_PRCM_GPIOCLKGR);
static const u32_t pdctl0 =
	REG_ADDR(DT_TI_CC2650_PRCM_40082000_BASE_ADDRESS,
		 CC2650_PRCM_PDCTL0);
static const u32_t pdstat0 =
	REG_ADDR(DT_TI_CC2650_PRCM_40082000_BASE_ADDRESS,
		 CC2650_PRCM_PDSTAT0);
#ifdef CONFIG_SERIAL
static const u32_t uartclkgr =
	REG_ADDR(DT_TI_CC2650_PRCM_40082000_BASE_ADDRESS,
		 CC2650_PRCM_UARTCLKGR);
#endif

/* Setup power and clock for needed hardware modules. */
static void setup_modules_prcm(void)
{
#if defined(CONFIG_GPIO_CC2650) || \
	defined(CONFIG_SERIAL)

	/* Setup power */
#if defined(CONFIG_GPIO_CC2650)
	sys_set_bit(pdctl0, CC2650_PRCM_PDCTL0_PERIPH_ON_POS);
#endif
#ifdef CONFIG_SERIAL
	sys_set_bit(pdctl0, CC2650_PRCM_PDCTL0_SERIAL_ON_POS);
#endif

	/* Setup clocking */
#ifdef CONFIG_GPIO_CC2650
	sys_set_bit(gpioclkgr, CC2650_PRCM_GPIOCLKGR_CLK_EN_POS);
#endif
#ifdef CONFIG_SERIAL
	sys_set_bit(uartclkgr, CC2650_PRCM_UARTCLKGR_CLK_EN_POS);
#endif
	/* Reload clocking configuration for device */
	sys_set_bit(clkloadctl, CC2650_PRCM_CLKLOADCTL_LOAD_POS);

	/* Wait for power to be completely on, to avoid bus faults
	 * when accessing modules' registers.
	 */
#if defined(CONFIG_GPIO_CC2650)
	while (!(sys_read32(pdstat0) &
	       BIT(CC2650_PRCM_PDSTAT0_PERIPH_ON_POS))) {
		continue;
	}
#endif
#if defined(CONFIG_SERIAL)
	while (!(sys_read32(pdstat0) &
	       BIT(CC2650_PRCM_PDSTAT0_SERIAL_ON_POS))) {
		continue;
	}
#endif

#endif
}

static int ti_cc2650(struct device *dev)
{
	ARG_UNUSED(dev);

	NMI_INIT();
	setup_modules_prcm();
	return 0;
}

SYS_INIT(ti_cc2650, PRE_KERNEL_1, 0);

int bit_is_set(u32_t reg, u8_t bit)
{
	return sys_read32(reg) & BIT(bit);
}
