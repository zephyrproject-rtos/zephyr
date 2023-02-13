/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2020-2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 Interrupt Unit device driver
 *
 * The ARCv2 interrupt unit has 16 allocated exceptions associated with
 * vectors 0 to 15 and 240 interrupts associated with vectors 16 to 255.
 * The interrupt unit is optional in the ARCv2-based processors. When
 * building a processor, you can configure the processor to include an
 * interrupt unit. The ARCv2 interrupt unit is highly programmable.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#ifdef CONFIG_ARC_CONNECT
static void arc_shared_intc_init(void)
{
	/*
	 * Initialize all IDU interrupts:
	 * - select round-robbin
	 * - disable all lines
	 */
	BUILD_ASSERT(CONFIG_NUM_IRQS > ARC_CONNECT_IDU_IRQ_START);
	__ASSERT(z_arc_v2_core_id() == ARC_MP_PRIMARY_CPU_ID,
		 "idu interrupts must be inited from primary core");

	z_arc_connect_idu_disable();

	for (uint32_t i = 0; i < (CONFIG_NUM_IRQS - ARC_CONNECT_IDU_IRQ_START); i++) {
		/*
		 * TODO: don't use z_arc_connect_idu* functions to avoid
		 * locking/unlocking every time.
		 */

		/* Disable (mask) line */
		z_arc_connect_idu_set_mask(i, 0x1);
		z_arc_connect_idu_set_mode(i, ARC_CONNECT_INTRPT_TRIGGER_LEVEL,
					   ARC_CONNECT_DISTRI_MODE_ROUND_ROBIN);

		/*
		 * Fake round-robin: we allow to distribute interrupts only to primary core as
		 * secondary cores may be not initialized yet.
		 */
		z_arc_connect_idu_set_dest(i, BIT(ARC_MP_PRIMARY_CPU_ID));
	}

	z_arc_connect_idu_enable();

}

/* Allow to schedule IRQ to all cores after we bring up all secondary cores */
static int arc_shared_intc_update_post_smp(const struct device *unused)
{
	__ASSERT(z_arc_v2_core_id() == ARC_MP_PRIMARY_CPU_ID,
		 "idu interrupts must be updated from primary core");

	z_arc_connect_idu_disable();

	for (uint32_t i = 0; i < (CONFIG_NUM_IRQS - ARC_CONNECT_IDU_IRQ_START); i++) {
		/* TODO: take arc_connect_spinlock one time to avoid locking/unlocking every time */
		z_arc_connect_idu_set_dest(i, GENMASK(CONFIG_MP_NUM_CPUS, 0));
	}

	z_arc_connect_idu_enable();

	return 0;
}

SYS_INIT(arc_shared_intc_update_post_smp, SMP, 0);
#endif /* CONFIG_ARC_CONNECT */


/* lowest IRQ priority */
#ifdef CONFIG_ARC_SECURE_FIRMWARE
#define ARC_IRQ_DEFAULT_PRIORITY ((CONFIG_NUM_IRQ_PRIO_LEVELS - 1) | _ARC_V2_IRQ_PRIORITY_SECURE)
#else
#define ARC_IRQ_DEFAULT_PRIORITY (CONFIG_NUM_IRQ_PRIO_LEVELS - 1)
#endif

static inline void arc_core_intc_init_nolock(uint32_t irq, uint32_t state)
{
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY, ARC_IRQ_DEFAULT_PRIORITY);
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, _ARC_V2_INT_LEVEL);
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, state);
}

/*
 * Initialize the core private interrupt controller.
 *
 * This function must be called on each CPU in case of SMP system.
 *
 * NOTE: core interrupts are still globally disabled at this point (STATUS32.IE = 0), so there is
 * no need to protect the window between a write to IRQ_SELECT and subsequent writes to the
 * selected IRQ's registers with locks.
 */
void arc_core_private_intc_init(void)
{
	/*
	 * Interrupts from 0 to 15 are exceptions and they are ignored by IRQ auxiliary registers.
	 * We skip those interrupt lines while setting up core private interrupt controller.
	 */
	BUILD_ASSERT(CONFIG_GEN_IRQ_START_VECTOR == 16);

	/*
	 * System with IDU case (most likely multi-core system):
	 *  - disable private IRQs: they will be enabled with irq_enable before usage
	 *  - enable shared (IDU) IRQs: their enabling / disabling is controlled via IDU, so we
	 *    always pass them via core private interrupt controller.
	 * System without IDU case (single-core system):
	 *  - disable all IRQs: they will be enabled with irq_enable before usage
	 */
#ifdef CONFIG_ARC_CONNECT
	for (uint32_t irq = CONFIG_GEN_IRQ_START_VECTOR; irq < ARC_CONNECT_IDU_IRQ_START; irq++) {
		arc_core_intc_init_nolock(irq, _ARC_V2_INT_DISABLE);
	}

	for (uint32_t irq = ARC_CONNECT_IDU_IRQ_START; irq < CONFIG_NUM_IRQS; irq++) {
		arc_core_intc_init_nolock(irq, _ARC_V2_INT_ENABLE);
	}
#else
	for (uint32_t irq = CONFIG_GEN_IRQ_START_VECTOR; irq < CONFIG_NUM_IRQS; irq++) {
		arc_core_intc_init_nolock(irq, _ARC_V2_INT_DISABLE);
	}
#endif /* CONFIG_ARC_CONNECT */
}

static int arc_irq_init(const struct device *unused)
{
#ifdef CONFIG_ARC_CONNECT
	arc_shared_intc_init();
#endif /* CONFIG_ARC_CONNECT */

	/*
	 * We initialize per-core part for core 0 here,
	 * for rest cores it will be initialized in slave_start.
	 */
	arc_core_private_intc_init();

	return 0;
}

SYS_INIT(arc_irq_init, PRE_KERNEL_1, 0);
