/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/sys/barrier.h>

unsigned int sw_irq_number = (unsigned int)(-1);
static volatile bool custom_init_called;
static volatile bool custom_enable_called;
static volatile bool custom_disable_called;
static volatile bool custom_set_priority_called;
static volatile bool custom_eoi_called;
static volatile bool irq_handler_called;

/* Define out custom SoC interrupt controller interface methods.
 * These closely match the normal Cortex-M implementations.
 */

#define NUM_IRQS_PER_REG  32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

void z_soc_irq_init(void)
{
	int irq = 0;

	for (; irq < CONFIG_NUM_IRQS; irq++) {
		NVIC_SetPriority((IRQn_Type)irq, _IRQ_PRIO_OFFSET);
	}

	custom_init_called = true;
}

void z_soc_irq_enable(unsigned int irq)
{
	if (irq == sw_irq_number) {
		custom_enable_called = true;
	}
	NVIC_EnableIRQ((IRQn_Type)irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	if (irq == sw_irq_number) {
		custom_disable_called = true;
	}
	NVIC_DisableIRQ((IRQn_Type)irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return NVIC->ISER[REG_FROM_IRQ(irq)] & BIT(BIT_FROM_IRQ(irq));
}

void z_soc_irq_eoi(unsigned int irq)
{
	if (irq == sw_irq_number) {
		custom_eoi_called = true;
	}
}

inline __attribute__((always_inline)) unsigned int z_soc_irq_get_active(void)
{
	return __get_IPSR();
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	if (irq == sw_irq_number) {
		custom_set_priority_called = true;
	}

	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) && (flags & IRQ_ZERO_LATENCY)) {
		prio = _EXC_ZERO_LATENCY_IRQS_PRIO;
	} else {
		prio += _IRQ_PRIO_OFFSET;
	}

	NVIC_SetPriority((IRQn_Type)irq, prio);
}

void arm_isr_handler(const void *args)
{
	ARG_UNUSED(args);

#if defined(CONFIG_CPU_CORTEX_M) && defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Clear Floating Point Status and Control Register (FPSCR),
	 * to prevent from having the interrupt line set to pending again,
	 * in case FPU IRQ is selected by the test as "Available IRQ line"
	 */
#if defined(CONFIG_ARMV8_1_M_MAINLINE)
	/*
	 * For ARMv8.1-M with FPU, the FPSCR[18:16] LTPSIZE field must be set
	 * to 0b100 for "Tail predication not applied" as it's reset value
	 */
	__set_FPSCR(4 << FPU_FPDSCR_LTPSIZE_Pos);
#else
	__set_FPSCR(0);
#endif
#endif

	/* IRQ numbers are offset by 16 on Cortex-M. */
	unsigned int this_irq = z_soc_irq_get_active() - 16;

	TC_PRINT("Got IRQ: %u\n", this_irq);

	zassert_equal(this_irq, sw_irq_number, "Unexpected active IRQ\n");
	irq_handler_called = true;
}

/**
 * @brief Test custom interrupt controller handling with CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER.
 * @addtogroup kernel_interrupt_tests
 * @ingroup all_tests
 * @{
 */

ZTEST(arm_custom_interrupt, test_arm_custom_interrupt)
{
	zassert_true(custom_init_called, "Custom IRQ init not called\n");

	/* Determine an NVIC IRQ line that is not currently in use. */
	int i;

	for (i = CONFIG_NUM_IRQS - 1; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			NVIC_SetPendingIRQ(i);

			if (NVIC_GetPendingIRQ(i)) {
				/* If the NVIC line is pending, it is
				 * guaranteed that it is implemented; clear the
				 * line.
				 */
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering.
					 */
					break;
				}
			}
		}
	}

	zassert_true(i >= 0, "No available IRQ line to use in the test\n");

	TC_PRINT("Available IRQ line: %u\n", i);
	sw_irq_number = i;

	zassert_false(custom_set_priority_called, "Custom set priority flag set\n");
	arch_irq_connect_dynamic(sw_irq_number, 0 /* highest priority */, arm_isr_handler, NULL, 0);
	zassert_true(custom_set_priority_called, "Custom set priority not called\n");

	NVIC_ClearPendingIRQ(i);

	zassert_false(arch_irq_is_enabled(sw_irq_number), "SW IRQ already enabled\n");
	zassert_false(custom_enable_called, "Custom IRQ enable flag is set\n");
	irq_enable(sw_irq_number);
	zassert_true(custom_enable_called, "Custom IRQ enable not called\n");
	zassert_true(arch_irq_is_enabled(sw_irq_number), "SW IRQ is not enabled\n");

	for (int j = 1; j <= 3; j++) {
		custom_eoi_called = false;
		irq_handler_called = false;
		custom_set_priority_called = false;

		/* Set the dynamic IRQ to pending state. */
		NVIC_SetPendingIRQ(i);

		/*
		 * Instruction barriers to make sure the NVIC IRQ is
		 * set to pending state before 'test_flag' is checked.
		 */
		barrier_dsync_fence_full();
		barrier_isync_fence_full();

		/* Returning here implies the thread was not aborted. */

		/* Confirm test flag is set by the ISR handler. */
		zassert_true(custom_eoi_called, "Custom EOI handler not called\n");
		zassert_true(irq_handler_called, "ISR handler not called\n");
	}

	zassert_false(custom_disable_called, "Custom IRQ disable flag is set\n");
	irq_disable(sw_irq_number);
	zassert_true(custom_disable_called, "Custom IRQ disable not called\n");
}

/**
 * @}
 */
