/* SPDX-License-Identifier: Apache-2.0 */

/**
 * @brief CLINT-based IPI implementation.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>
#include <soc.h>

extern void z_sched_ipi(void);

static inline void clint_ipi_send(uint8_t target_hart)
{
	volatile uint32_t *clint_ipi = (uint32_t *)RISCV_MSIP_BASE;

	clint_ipi[target_hart] = 1;
}

static inline void clint_ipi_clear(uint8_t target_hart)
{
	volatile uint32_t *clint_ipi = (uint32_t *)RISCV_MSIP_BASE;

	clint_ipi[target_hart] = 0;
}

/* CLINT implementation of sched_ipi */
FUNC_ALIAS(clint_sched_ipi, arch_sched_ipi, void);
void clint_sched_ipi(void)
{
	/* Note: protect the thread from switching to other CPUs, so we can ensure
	 * that it correctly boardcast IPI to all the other CPUs. */
	unsigned int key = arch_irq_lock();

	/* boardcast IPI to all CPUs except itself. */
	uint8_t self = _current_cpu->id;

	for (uint8_t hart_id=0; hart_id<CONFIG_MP_NUM_CPUS; hart_id++)
	{
		if(hart_id != self)
		{
			clint_ipi_send(hart_id);
		}
	}

	arch_irq_unlock(key);
}

static void clint_ipi_handler(void)
{
	/* IRQ is disabled in IPI handler */
	clint_ipi_clear(_current_cpu->id);

	z_sched_ipi();
}

/**
 * @brief Initialize CLINT-based IPI driver.
 * @return N/A
 */
static int clint_ipi_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Setup IRQ handler for CLINT IPI driver */
	IRQ_CONNECT(RISCV_MACHINE_SOFT_IRQ,
	            0,
	            clint_ipi_handler,
	            NULL,
	            0);

	/* Enable IRQ for CLINT IPI driver */
	irq_enable(RISCV_MACHINE_SOFT_IRQ);

	return 0;
}

SYS_INIT(clint_ipi_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
