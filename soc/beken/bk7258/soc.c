/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <soc.h>

/*
 * Addresses are the secure aliases, which is what the BootROM leaves the core
 * running in. The non-secure alias of any peripheral is at +0x10000000.
 */
#define BK7258_AON_WDT_BASE 0x44000600
#define BK7258_WDT_BASE     0x44800000

/* CPU0's interrupt gate: sources 0 to 31, then 32 to 63 */
#define BK7258_SYS_CPU0_INT_EN 0x44010080

/* Both watchdogs share a control register: period in bits 15:0, key in 23:16 */
#define BK7258_WDT_CTRL_OFFSET 0x10
#define BK7258_WDT_KEY_FIRST   0x5a0000
#define BK7258_WDT_KEY_SECOND  0xa50000

/* NMI watchdog global control */
#define BK7258_WDT_GLOBAL_CTRL_OFFSET   0x08
#define BK7258_WDT_CLK_GATE_BYPASS      BIT(1)

/*
 * Stop both watchdogs. They are running when the BootROM hands over: left
 * alone, the NMI watchdog raises an NMI about 2.5 s into boot and the AON
 * watchdog resets the chip about 45 s in. Writing a period of zero with the
 * two-key sequence disables a watchdog.
 */
static void watchdog_disable(void)
{
	sys_write32(BK7258_WDT_KEY_FIRST, BK7258_AON_WDT_BASE);
	sys_write32(BK7258_WDT_KEY_SECOND, BK7258_AON_WDT_BASE);

	sys_set_bits(BK7258_WDT_BASE + BK7258_WDT_GLOBAL_CTRL_OFFSET,
		     BK7258_WDT_CLK_GATE_BYPASS);
	sys_write32(BK7258_WDT_KEY_FIRST, BK7258_WDT_BASE + BK7258_WDT_CTRL_OFFSET);
	sys_write32(BK7258_WDT_KEY_SECOND, BK7258_WDT_BASE + BK7258_WDT_CTRL_OFFSET);
}

static struct k_spinlock irq_gate_lock;

void bk7258_irq_gate_enable(unsigned int irq)
{
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(irq < CONFIG_NUM_IRQS);

	key = k_spin_lock(&irq_gate_lock);
	sys_set_bits(BK7258_SYS_CPU0_INT_EN + ((irq / 32U) * 4U), BIT(irq % 32U));
	k_spin_unlock(&irq_gate_lock, key);
}

void soc_early_init_hook(void)
{
	watchdog_disable();
}
