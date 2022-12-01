/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <hpm_common.h>
#include <hpm_soc.h>
#include <hpm_pmp_drv.h>
#include <hpm_pllctl_drv.h>
#ifdef CONFIG_XIP
#include <hpm_bootheader.h>
#endif

#ifdef CONFIG_XIP
/**
 * @brief FLASH configuration option definitions:
 * option[0]:
 *    [31:16] 0xfcf9 - FLASH configuration option tag
 *    [15:4]  0 - Reserved
 *    [3:0]   option words (exclude option[0])
 * option[1]:
 *    [31:28] Flash probe type
 *      0 - SFDP SDR / 1 - SFDP DDR
 *      2 - 1-4-4 Read (0xEB, 24-bit address) / 3 - 1-2-2 Read(0xBB, 24-bit address)
 *      4 - HyperFLASH 1.8V / 5 - HyperFLASH 3V
 *      6 - OctaBus DDR (SPI -> OPI DDR)
 *      8 - Xccela DDR (SPI -> OPI DDR)
 *      10 - EcoXiP DDR (SPI -> OPI DDR)
 *    [27:24] Command Pads after Power-on Reset
 *      0 - SPI / 1 - DPI / 2 - QPI / 3 - OPI
 *    [23:20] Command Pads after Configuring FLASH
 *      0 - SPI / 1 - DPI / 2 - QPI / 3 - OPI
 *    [19:16] Quad Enable Sequence (for the device support SFDP 1.0 only)
 *      0 - Not needed
 *      1 - QE bit is at bit 6 in Status Register 1
 *      2 - QE bit is at bit1 in Status Register 2
 *      3 - QE bit is at bit7 in Status Register 2
 *      4 - QE bit is at bit1 in Status Register 2 and should be programmed by 0x31
 *    [15:8] Dummy cycles
 *      0 - Auto-probed / detected / default value
 *      Others - User specified value, for DDR read, the dummy cycles should be
 *				2 * cycles on FLASH datasheet
 *    [7:4] Misc.
 *      0 - Not used
 *      1 - SPI mode
 *      2 - Internal loopback
 *      3 - External DQS
 *    [3:0] Frequency option
 *      1 - 30MHz / 2 - 50MHz / 3 - 66MHz / 4 - 80MHz / 5 - 100MHz
 *				/ 6 - 120MHz / 7 - 133MHz / 8 - 166MHz
 *
 * option[2] (Effective only if the bit[3:0] in option[0] > 1)
 *    [31:20]  Reserved
 *    [19:16] IO voltage
 *      0 - 3V / 1 - 1.8V
 *    [15:12] Pin group
 *      0 - 1st group / 1 - 2nd group
 *    [11:8] Connection selection
 *      0 - CA_CS0 / 1 - CB_CS0 / 2 - CA_CS0 + CB_CS0
 *			(Two FLASH connected to CA and CB respectively)
 *    [7:0] Drive Strength
 *      0 - Default value
 * option[3] (Effective only if the bit[3:0] in option[0] > 2,
 *				required only for the QSPI NOR FLASH that not supports
 *              JESD216)
 *    [31:16] reserved
 *    [15:12] Sector Erase Command Option, not required here
 *    [11:8]  Sector Size Option, not required here
 *    [7:0] Flash Size Option
 *      0 - 4MB / 1 - 8MB / 2 - 16MB
 */
__attribute__ ((section(".nor_cfg_option"))) const uint32_t option[4] = {
		0xfcf90001, 0x00000007, 0x0, 0x0
	};
uint32_t __fw_size__[] = {32768};
#endif
__weak void _init_noncache(void)
{
	uint32_t i, size;

	extern uint8_t __noncacheable_data_load_start[];
	extern uint8_t __noncacheable_bss_start__[], __noncacheable_bss_end__[];
	extern uint8_t __noncacheable_init_start__[], __noncacheable_init_end__[];

	/* noncacheable bss section */
	size = __noncacheable_bss_end__ - __noncacheable_bss_start__;
	for (i = 0; i < size; i++) {
		*(__noncacheable_bss_start__ + i) = 0;
	}

	/* noncacheable init section LMA: etext + data length + ramfunc length */
	size = __noncacheable_init_end__ - __noncacheable_init_start__;
	for (i = 0; i < size; i++) {
		*(__noncacheable_init_start__ + i) = *(__noncacheable_data_load_start + i);
	}
}

void soc_init_pmp(void)
{
	extern uint32_t __noncacheable_start__[];
	extern uint32_t __noncacheable_end__[];

	uint32_t start_addr = (uint32_t) __noncacheable_start__;
	uint32_t end_addr = (uint32_t) __noncacheable_end__;
	uint32_t length = end_addr - start_addr;
	pmp_entry_t pmp_entry[1];

	if (length == 0) {
		return;
	}

	/* Ensure the address and the length are power of 2 aligned */
	__ASSERT((length & (length - 1U)) == 0U,
		"Length are power of 2 aligned");
	__ASSERT((start_addr & (length - 1U)) == 0U,
		"Address are power of 2 aligned");

	pmp_entry[0].pmp_addr = PMP_NAPOT_ADDR(start_addr, length);
	pmp_entry[0].pmp_cfg.val = PMP_CFG(READ_EN, WRITE_EN,
			EXECUTE_EN, ADDR_MATCH_NAPOT, REG_UNLOCK);
	pmp_entry[0].pma_addr = PMA_NAPOT_ADDR(start_addr, length);
	pmp_entry[0].pma_cfg.val = PMA_CFG(ADDR_MATCH_NAPOT, MEM_TYPE_MEM_NON_CACHE_BUF, AMO_EN);

	pmp_config(&pmp_entry[0], ARRAY_SIZE(pmp_entry));
}

static int hpmicro_soc_init(void)
{
	uint32_t key;

	key = irq_lock();
	soc_init_pmp();
	irq_unlock(key);

	return 0;
}

SYS_INIT(hpmicro_soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
