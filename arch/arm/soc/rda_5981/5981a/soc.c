/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>

#include "soc_registers.h"
#include "soc_cfg.h"

#define SYS_CLK_FREQUENCY_40M                            (40000000UL)
#define SYS_CLK_FREQUENCY_80M                            (80000000UL)
#define SYS_CLK_FREQUENCY_160M                           (160000000UL)
#define BUS_CLK_FREQUENCY_40M                            (40000000UL)
#define BUS_CLK_FREQUENCY_80M                            (80000000UL)

uint32_t SystemCoreClock __used = SYS_CLK_FREQUENCY_160M;
uint32_t AHBBusClock __used = BUS_CLK_FREQUENCY_80M;

static void clock_init(void)
{
	struct scu_ctrl *cfg = (struct scu_ctrl *)RDA_SCU_BASE;

	/* Determine clock frequency according to
	 * SCU core config register values
	 */
	switch ((cfg->corecfg >> 12) & 0x03UL) {
	case 0:
		SystemCoreClock = SYS_CLK_FREQUENCY_40M;
		break;
	case 1:
		SystemCoreClock = SYS_CLK_FREQUENCY_80M;
		break;
	case 2:
	case 3:
		SystemCoreClock = SYS_CLK_FREQUENCY_160M;
		break;
	}

	switch ((cfg->corecfg >> 11) & 0x01UL) {
	case 0:
		AHBBusClock = BUS_CLK_FREQUENCY_40M;
		break;
	case 1:
		AHBBusClock = BUS_CLK_FREQUENCY_80M;
	break;
	}

}

static void relocate_vector_table(void)
{
	int i;
	uint32_t *vectors;
	uint32_t *flash_vectors =
		(uint32_t *)(CONFIG_FLASH_BASE_ADDRESS & 0xffffff80);

	vectors = (uint32_t *)RDA_IRAM_BASE;
	for (i = 0; i < RDA_HAL_IRQ_NUM; i++) {
		vectors[i] = flash_vectors[i];
	}

	SCB->VTOR = (uint32_t)RDA_IRAM_BASE;
}

static void enable_global_irq(uint32_t mask)
{
	__asm__ volatile ("MSR primask, %0" : : "r" (mask) : "memory");
}

static void reboot_vect(void)
{
	SCB->AIRCR = (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
		(SCB_AIRCR_VECTRESET_Msk));

	/* the reboot is not immediate, so wait here until it takes effect */
	for (;;) {
		;
	}
}

static int rda5981_init(struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* fpu enabled by preC if CONFIG_FLOAT */

	/* Setup the vector table offset register (VTOR),
	 * which is located at the beginning of flash area.
	 */
	/*_scs_relocate_vector_table((void *)CONFIG_FLASH_BASE_ADDRESS); */
	relocate_vector_table();

	enable_global_irq(0);

	/* Clear all faults*/
	_ClearFaults();

	clock_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	if (!rda_ccfg_boot())
		reboot_vect();

	irq_unlock(key);

	return 0;
}

SYS_INIT(rda5981_init, PRE_KERNEL_1, 0);
