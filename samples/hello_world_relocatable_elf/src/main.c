/*
 * Copyright (c) Qualcomm Technologies, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <zephyr/sys/__assert.h>

#ifdef CONFIG_GEN_SW_ISR_TABLE
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sw_isr_table.h>
#endif

#ifdef CONFIG_RISCV_APLIC_DIRECT
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

#ifdef CONFIG_GEN_SW_ISR_TABLE
/*
 * The ISR table is generated from zephyr_pre0.elf, so its handler addresses
 * are only valid at run time if they were adjusted to the running image. Check
 * that every handler lies in the text region, and that the entries equal to
 * the run-time address of z_irq_spurious (unconnected lines) and the others
 * (connected lines) are both present. An unadjusted table or a wrong delta
 * leaves no entry equal to z_irq_spurious. No interrupt is triggered.
 */
static int check_isr_table(void)
{
	const uintptr_t text_start = (uintptr_t)__text_region_start;
	const uintptr_t text_end = (uintptr_t)__text_region_end;
	size_t spurious = 0;
	size_t connected = 0;

	for (size_t i = 0; i < IRQ_TABLE_SIZE; i++) {
		const uintptr_t isr = (uintptr_t)_sw_isr_table[i].isr;

		if ((isr < text_start) || (isr >= text_end)) {
			printf("ISR table FAILED: entry %zu handler %#lx outside %#lx-%#lx\n", i,
			       (unsigned long)isr, (unsigned long)text_start,
			       (unsigned long)text_end);
			return -EFAULT;
		}

		if (_sw_isr_table[i].isr == z_irq_spurious) {
			spurious++;
		} else {
			connected++;
		}
	}

	if ((spurious == 0U) || (connected == 0U)) {
		printf("ISR table FAILED: %zu spurious, %zu connected entries\n", spurious,
		       connected);
		return -EFAULT;
	}

	printf("ISR table OK: %zu entries checked, %zu connected\n", (size_t)IRQ_TABLE_SIZE,
	       connected);

	return 0;
}
#endif /* CONFIG_GEN_SW_ISR_TABLE */

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

#define SAMPLE_IRQ_L2(id)   (IRQ_TO_L2(id) | CONFIG_2ND_LVL_INTR_00_OFFSET)
#define SAMPLE_IRQ_PRIORITY 1U

/* First APLIC sources not used in DT, one per kind of ISR argument. */
#define SAMPLE_IRQ_ID_RODATA 9U
#define SAMPLE_IRQ_ID_DATA   11U
#define SAMPLE_IRQ_ID_BSS    12U

static K_SEM_DEFINE(isr_sem, 0, 1);
static const void *volatile isr_arg;

static void sample_isr(const void *arg)
{
	isr_arg = arg;
	k_sem_give(&isr_sem);
}

/*
 * Trigger an already connected interrupt and check that the handler gets
 * the expected argument.
 */
static int sample_trigger_irq(uint32_t irq_id, const char *kind, const void *expected)
{
	const uint32_t irq_l2 = SAMPLE_IRQ_L2(irq_id);
	int ret;

	isr_arg = NULL;
	irq_enable(irq_l2);

	/* Set the source pending, as if its input had a rising edge */
	sys_write32(irq_id, APLIC_BASE + APLIC_SETIPNUM);

	ret = k_sem_take(&isr_sem, K_SECONDS(1));

	irq_disable(irq_l2);

	if (ret != 0) {
		printf("Interrupt FAILED: IRQ %u (%s) not handled (%d)\n", irq_id, kind, ret);
		return ret;
	}

	if (isr_arg != expected) {
		printf("Interrupt FAILED: IRQ %u (%s) ISR argument %p, expected %p\n", irq_id,
		       kind, isr_arg, expected);
		return -EFAULT;
	}

	printf("Interrupt OK: IRQ %u (%s) handled, argument %p\n", irq_id, kind, isr_arg);

	return 0;
}

/*
 * The ISR argument is not necessarily in the same memory region as the ISR
 * table: it can be a const object (.rodata, like the device pointers drivers
 * pass), or a mutable one in .data or .bss. Each kind gets its own line.
 */
static int sample_aplic_direct_irq(void)
{
	/* Their addresses are the ISR arguments: any non-NULL object would do. */
	static const uint32_t cookie_rodata = 0xC0FFEEU;
	static uint32_t cookie_data = 0xDA7AU;
	static uint32_t cookie_bss;
	int ret;

	IRQ_CONNECT(SAMPLE_IRQ_L2(SAMPLE_IRQ_ID_RODATA), SAMPLE_IRQ_PRIORITY, sample_isr,
		    &cookie_rodata, APLIC_SM_EDGE_RISE);
	IRQ_CONNECT(SAMPLE_IRQ_L2(SAMPLE_IRQ_ID_DATA), SAMPLE_IRQ_PRIORITY, sample_isr,
		    &cookie_data, APLIC_SM_EDGE_RISE);
	IRQ_CONNECT(SAMPLE_IRQ_L2(SAMPLE_IRQ_ID_BSS), SAMPLE_IRQ_PRIORITY, sample_isr,
		    &cookie_bss, APLIC_SM_EDGE_RISE);

	ret = sample_trigger_irq(SAMPLE_IRQ_ID_RODATA, ".rodata", &cookie_rodata);
	if (ret != 0) {
		return ret;
	}

	ret = sample_trigger_irq(SAMPLE_IRQ_ID_DATA, ".data", &cookie_data);
	if (ret != 0) {
		return ret;
	}

	return sample_trigger_irq(SAMPLE_IRQ_ID_BSS, ".bss", &cookie_bss);
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

#ifdef CONFIG_GEN_SW_ISR_TABLE
	if (check_isr_table() != 0) {
		return -1;
	}
#endif

#ifdef CONFIG_RISCV_APLIC_DIRECT
	if (sample_aplic_direct_irq() != 0) {
		return -1;
	}
#endif

	return 0;
}
