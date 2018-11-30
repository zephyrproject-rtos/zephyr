/*
 * Copyright (c) 2018, Cypress Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

#include "cy_sysint.h"
#include "cy_wdt.h"
#include <init.h>
#include <misc/printk.h>
#include <string.h>

#define SIMPLE_RESOURCES_PROTECTION

#ifdef SIMPLE_RESOURCES_PROTECTION
#include "cyprotection_simple_config.h"
#else
#include "cyprotection_full_config.h"
#endif

#define PRIVIL_ACCESS 1
#define UNPRIVIL_ACCESS 0
#define SECURE_ACCESS 0
#define UNSECURE_ACCESS 1

#define REGION_SIZE 0x1

#define CM0_MEMORY_ADDR (CONFIG_SRAM_BASE_ADDRESS + 0x500)
#define CM4_MEMORY_ADDR (0x08024000 + 0x500)

#define SLEEP_TIME 500

#ifdef SIMPLE_RESOURCES_PROTECTION
static cy_en_prot_status_t protect_cm0(void)
{
	cy_en_prot_status_t status = CY_PROT_SUCCESS;

	/* smpu */
	status = smpu_protect(flash_spm_smpu_config,
		ARRAY_SIZE(flash_spm_smpu_config));
	if (status == CY_PROT_SUCCESS) {
		status = smpu_protect(sram_spm_smpu_config,
			ARRAY_SIZE(sram_spm_smpu_config));
	}

	/* fixed region ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_rg_protect(fixed_rg_ppu_config,
			ARRAY_SIZE(fixed_rg_ppu_config));
	}
	/* fixed slave ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_sl_protect(fixed_sl_ppu_config,
			ARRAY_SIZE(fixed_sl_ppu_config));
	}
	/* programmable ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_prog_protect(prog_ppu_config,
			ARRAY_SIZE(prog_ppu_config));
	}
	/* fixed group ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_gr_protect(fixed_gr_ppu_config,
			ARRAY_SIZE(fixed_gr_ppu_config));
	}

	/* bus masters */
	if (status == CY_PROT_SUCCESS) {
		status = bus_masters_protect(bus_masters_config,
			ARRAY_SIZE(bus_masters_config));
	}

	return status;
}
#else
static cy_en_prot_status_t protect_cm0(void)
{
	cy_en_prot_status_t status = CY_PROT_SUCCESS;

	/* smpu */
	status = smpu_protect(flash_spm_smpu_config,
		ARRAY_SIZE(flash_spm_smpu_config));
	if (status == CY_PROT_SUCCESS) {
		status = smpu_protect(sram_spm_smpu_config,
			ARRAY_SIZE(sram_spm_smpu_config));
	}
	if (status == CY_PROT_SUCCESS) {
		status = smpu_protect_unprotected(&spm_other_smpu_config);
	}

	/* fixed region ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_rg_protect(fixed_rg_pc0_ppu_config,
			ARRAY_SIZE(fixed_rg_pc0_ppu_config));
	}
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_rg_protect(fixed_rg_spm_ppu_config,
			ARRAY_SIZE(fixed_rg_spm_ppu_config));
	if (status == CY_PROT_SUCCESS) {
	}
		status = ppu_fixed_rg_protect(fixed_rg_any_ppu_config,
			ARRAY_SIZE(fixed_rg_any_ppu_config));
	}
	/* fixed slave ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_sl_protect(fixed_sl_pc0_ppu_config,
			ARRAY_SIZE(fixed_sl_pc0_ppu_config));
	}
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_sl_protect(fixed_sl_spm_ppu_config,
			ARRAY_SIZE(fixed_sl_spm_ppu_config));
	}
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_sl_protect(fixed_sl_any_ppu_config,
			ARRAY_SIZE(fixed_sl_any_ppu_config));
	}
	/* programmable ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_prog_protect(prog_pc0_ppu_config,
			ARRAY_SIZE(prog_pc0_ppu_config));
	}
	if (status == CY_PROT_SUCCESS) {
		status = ppu_prog_protect(prog_spm_ppu_config,
			ARRAY_SIZE(prog_spm_ppu_config));
	}
	/* fixed group ppu */
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_gr_protect(fixed_gr_pc0_ppu_config,
			ARRAY_SIZE(fixed_gr_pc0_ppu_config));
	}
	if (status == CY_PROT_SUCCESS) {
		status = ppu_fixed_gr_protect(fixed_gr_spm_ppu_config,
			ARRAY_SIZE(fixed_gr_spm_ppu_config));
	}

	/* bus masters */
	if (status == CY_PROT_SUCCESS) {
		status = bus_masters_protect(bus_masters_config,
			ARRAY_SIZE(bus_masters_config));
	}

	return status;
}
#endif

void main(void)
{
	u8_t access_flag;
	u32_t *mem_addr;

	/* Disable watchdog */
	Cy_WDT_Unlock();
	Cy_WDT_Disable();

	printk("PSoC6 Secure core has started\n\n");

	if (protect_cm0() == CY_PROT_SUCCESS) {
		printk("PSoC6 CM0+ resources protected\n\n");
	} else {
		printk("PSoC6 CM0+ resourses protection failed\n");
		while (1) {
		}
	}

	printk("Check access rights before executing CM4 requests\n\n");
	access_flag = isPeripheralAccessAllowed((u32_t)SCB2, REGION_SIZE,
		PRIVIL_ACCESS, UNSECURE_ACCESS, CY_PROT_PC6, CY_PROT_PERM_RW);
	printk("CM4 privileged unsecured access to SCB2 is %s\n",
		access_flag ? "allowed" : "forbidden");
	access_flag = isPeripheralAccessAllowed((u32_t)SCB2, REGION_SIZE,
		UNPRIVIL_ACCESS, SECURE_ACCESS, CY_PROT_PC1, CY_PROT_PERM_RW);
	printk("CM0+ unprivileged secured access to SCB2 is %s\n",
		access_flag ? "allowed" : "forbidden");
	access_flag = isPeripheralAccessAllowed((u32_t)SCB2, REGION_SIZE,
		PRIVIL_ACCESS, SECURE_ACCESS, CY_PROT_PC1, CY_PROT_PERM_RW);
	printk("CM0+ privileged secured access to SCB2 is %s\n\n",
		access_flag ? "allowed" : "forbidden");

	access_flag = isMemoryAccessAllowed(CM4_MEMORY_ADDR, REGION_SIZE,
		UNPRIVIL_ACCESS, UNSECURE_ACCESS, CY_PROT_PC6, CY_PROT_PERM_RW);
	printk("CM4 unprivileged unsecured access to CM4 memory is %s\n",
		access_flag ? "allowed" : "forbidden");
	access_flag = isMemoryAccessAllowed(CM4_MEMORY_ADDR, REGION_SIZE,
		PRIVIL_ACCESS, SECURE_ACCESS, CY_PROT_PC1, CY_PROT_PERM_RW);
	printk("CM0 privileged secured access to CM4 memory is %s\n\n",
		access_flag ? "allowed" : "forbidden");

	access_flag = isMemoryAccessAllowed(CM0_MEMORY_ADDR, REGION_SIZE,
		UNPRIVIL_ACCESS, UNSECURE_ACCESS, CY_PROT_PC6, CY_PROT_PERM_RW);
	printk("CM4 unprivileged unsecured access to CM0 memory is %s\n",
		access_flag ? "allowed" : "forbidden");
	access_flag = isMemoryAccessAllowed(CM0_MEMORY_ADDR, REGION_SIZE,
		PRIVIL_ACCESS, SECURE_ACCESS, CY_PROT_PC1, CY_PROT_PERM_RW);
	printk("CM0 privileged secured access to CM0 memory is %s\n\n",
		access_flag ? "allowed" : "forbidden");

	printk("Read from CM4 memory\n");
	mem_addr = (u32_t *)CM4_MEMORY_ADDR;
	printk("Content of CM4 memory by address 0x%x is 0x%x\n\n",
		(u32_t)mem_addr, *mem_addr);
	printk("Read from CM0 memory\n");
	mem_addr = (u32_t *)CM0_MEMORY_ADDR;
	printk("Content of CM0 memory by address 0x%x is 0x%x\n",
		(u32_t)mem_addr, *mem_addr);

	/* Wait a bit to avoid terminal output corruption */
	k_sleep(SLEEP_TIME);

	/* Enable CM4 */
	Cy_SysEnableCM4(CONFIG_SLAVE_BOOT_ADDRESS_PSOC6);

	while (1) {
	}
}
