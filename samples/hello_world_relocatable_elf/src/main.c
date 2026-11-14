/*
 * Copyright (c) Qualcomm Technologies, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/sys/__assert.h>

#ifdef CONFIG_RISCV_APLIC_DIRECT
#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#endif

/*
 * Offset between the link-time and run-time address, stored by the
 * early self-relocation code.
 */
extern uintptr_t z_load_offset;

int main(void);

/* Statically initialized, so it only holds the run-time address if the dynamic relocations ran. */
static int (*volatile main_ptr)(void) = main;

#ifdef CONFIG_RISCV_APLIC_DIRECT
/*
 * Register an interrupt with IRQ_CONNECT() and trigger it through the APLIC
 * in direct delivery mode.
 *
 * IRQ_CONNECT() places the handler and its argument in the ISR table that the
 * build generates from zephyr_pre0.elf, so the interrupt only reaches the
 * handler with the expected argument if that table was relocated to the
 * run-time address.
 */

#define APLIC_BASE DT_REG_ADDR(DT_NODELABEL(aplic))

#define SAMPLE_IRQ_ID       9U /* First APLIC source not used in DT */
#define SAMPLE_IRQ_ID_L2    (IRQ_TO_L2(SAMPLE_IRQ_ID) | CONFIG_2ND_LVL_INTR_00_OFFSET)
#define SAMPLE_IRQ_PRIORITY 1U

static K_SEM_DEFINE(isr_sem, 0, 1);
static const void *volatile isr_arg;

static void sample_isr(const void *arg)
{
	isr_arg = arg;
	k_sem_give(&isr_sem);
}

static int sample_aplic_direct_irq(void)
{
	/* Its address is the ISR argument: any non-NULL object would do. */
	static const uint32_t isr_cookie = 0xC0FFEEU;
	int ret;

	IRQ_CONNECT(SAMPLE_IRQ_ID_L2, SAMPLE_IRQ_PRIORITY, sample_isr, &isr_cookie,
		    APLIC_SM_EDGE_RISE);
	irq_enable(SAMPLE_IRQ_ID_L2);

	/* Set the source pending, as if its input had a rising edge */
	sys_write32(SAMPLE_IRQ_ID, APLIC_BASE + APLIC_SETIPNUM);

	ret = k_sem_take(&isr_sem, K_SECONDS(1));

	irq_disable(SAMPLE_IRQ_ID_L2);

	if (ret != 0) {
		printf("Interrupt FAILED: IRQ %u not handled (%d)\n", SAMPLE_IRQ_ID, ret);
		return ret;
	}

	if (isr_arg != &isr_cookie) {
		printf("Interrupt FAILED: ISR argument %p, expected %p\n", isr_arg,
		       (const void *)&isr_cookie);
		return -EFAULT;
	}

	printf("Interrupt OK: IRQ %u handled, argument %p, argument value 0x%x\n", SAMPLE_IRQ_ID,
	       isr_arg, *(const uint32_t *)isr_arg);

	return 0;
}
#endif /* CONFIG_RISCV_APLIC_DIRECT */

int main(void)
{
	uintptr_t run_addr;
	uintptr_t link_addr;

	/* PC-relative, so this is the address main() is really executing at. */
	__asm__ volatile("lla %0, %1" : "=r"(run_addr) : "i"(main));
	link_addr = run_addr - z_load_offset;

	__ASSERT(z_load_offset == SAMPLE_LOAD_OFFSET,
		 "load offset %#lx, expected %#lx", (unsigned long)z_load_offset,
		 (unsigned long)SAMPLE_LOAD_OFFSET);
	__ASSERT((uintptr_t)main_ptr == run_addr,
		 "%s() relocated to %#lx but runs at %#lx", __func__,
		 (unsigned long)(uintptr_t)main_ptr, (unsigned long)run_addr);

	printf("%s() linked at:  %#lx\n", __func__, (unsigned long)link_addr);
	printf("%s() running at: %#lx\n", __func__, (unsigned long)run_addr);
	printf("Relocation OK: load offset %#lx\n", (unsigned long)z_load_offset);

#ifdef CONFIG_RISCV_APLIC_DIRECT
	if (sample_aplic_direct_irq() != 0) {
		return -1;
	}
#endif

	return 0;
}
