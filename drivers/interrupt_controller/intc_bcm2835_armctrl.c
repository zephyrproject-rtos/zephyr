/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2835_armctrl_ic

#include <errno.h>
#include <zephyr/arch/arm/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bcm2835_armctrl, CONFIG_INTC_LOG_LEVEL);

#define BCM2835_ARMCTRL_BASE	DT_INST_REG_ADDR(0)

#define BCM2835_IRQ_BASIC_PENDING	(BCM2835_ARMCTRL_BASE + 0x200)
#define BCM2835_IRQ_PENDING_1		(BCM2835_ARMCTRL_BASE + 0x204)
#define BCM2835_IRQ_PENDING_2		(BCM2835_ARMCTRL_BASE + 0x208)
#define BCM2835_FIQ_CONTROL		(BCM2835_ARMCTRL_BASE + 0x20c)
#define BCM2835_ENABLE_IRQS_1		(BCM2835_ARMCTRL_BASE + 0x210)
#define BCM2835_ENABLE_IRQS_2		(BCM2835_ARMCTRL_BASE + 0x214)
#define BCM2835_ENABLE_BASIC_IRQS	(BCM2835_ARMCTRL_BASE + 0x218)
#define BCM2835_DISABLE_IRQS_1		(BCM2835_ARMCTRL_BASE + 0x21c)
#define BCM2835_DISABLE_IRQS_2		(BCM2835_ARMCTRL_BASE + 0x220)
#define BCM2835_DISABLE_BASIC_IRQS	(BCM2835_ARMCTRL_BASE + 0x224)

#define BCM2835_GPU_IRQ_COUNT		64U
#define BCM2835_BASIC_IRQ_BASE		64U
#define BCM2835_BASIC_IRQ_COUNT		8U
#define BCM2835_TOTAL_IRQS		(BCM2835_GPU_IRQ_COUNT + BCM2835_BASIC_IRQ_COUNT)
#define BCM2835_SPURIOUS_IRQ		(CONFIG_NUM_IRQS + 1U)

static struct k_spinlock bcm2835_armctrl_lock;
static uint32_t bcm2835_enabled_gpu1;
static uint32_t bcm2835_enabled_gpu2;
static uint32_t bcm2835_enabled_basic;

static inline bool bcm2835_irq_is_basic(unsigned int irq)
{
	return (irq >= BCM2835_BASIC_IRQ_BASE) && (irq < BCM2835_TOTAL_IRQS);
}

static inline bool bcm2835_irq_is_gpu1(unsigned int irq)
{
	return irq < 32U;
}

static inline bool bcm2835_irq_is_gpu2(unsigned int irq)
{
	return (irq >= 32U) && (irq < BCM2835_GPU_IRQ_COUNT);
}

static inline bool bcm2835_irq_is_valid(unsigned int irq)
{
	return irq < BCM2835_TOTAL_IRQS;
}

static inline uint32_t bcm2835_irq_bit(unsigned int irq)
{
	if (bcm2835_irq_is_basic(irq)) {
		return BIT(irq - BCM2835_BASIC_IRQ_BASE);
	}

	if (bcm2835_irq_is_gpu2(irq)) {
		return BIT(irq - 32U);
	}

	return BIT(irq);
}

unsigned int z_soc_irq_get_active(void)
{
	uint32_t gpu_pending;
	uint32_t basic_bits;

	/*
	 * Service GPU IRQ pending (IRQ_PENDING_1 / _2) before IRQ_BASIC_PENDING.
	 * Linux irq-bcm2835.c documents that register 0x200 mixes ARM status bits,
	 * shortcuts, and "check bank 1/2" flags — it is not eight independent
	 * lines that should preempt GPU IRQ demux. Reading GPU banks first also
	 * matches hardware priority for mini-UART (GPU irq 29) under QEMU.
	 * Only bits enabled in software are considered.
	 */
	gpu_pending = sys_read32(BCM2835_IRQ_PENDING_1) & bcm2835_enabled_gpu1;
	if (gpu_pending != 0U) {
		return u32_count_trailing_zeros(gpu_pending);
	}

	gpu_pending = sys_read32(BCM2835_IRQ_PENDING_2) & bcm2835_enabled_gpu2;
	if (gpu_pending != 0U) {
		return 32U + u32_count_trailing_zeros(gpu_pending);
	}

	basic_bits = (sys_read32(BCM2835_IRQ_BASIC_PENDING) & GENMASK(7, 0)) &
		     bcm2835_enabled_basic;
	if (basic_bits != 0U) {
		return BCM2835_BASIC_IRQ_BASE + u32_count_trailing_zeros(basic_bits);
	}

	return BCM2835_SPURIOUS_IRQ;
}

void z_soc_irq_eoi(unsigned int irq)
{
	ARG_UNUSED(irq);

	barrier_dsync_fence_full();
}

void z_soc_irq_init(void)
{
	/*
	 * Early interrupt-controller init runs before scheduler start on the
	 * current single-core BCM2835 bring-up path, so no concurrent access
	 * exists yet.
	 */
	sys_write32(0U, BCM2835_FIQ_CONTROL);
	sys_write32(UINT32_MAX, BCM2835_DISABLE_IRQS_1);
	sys_write32(UINT32_MAX, BCM2835_DISABLE_IRQS_2);
	sys_write32(UINT32_MAX, BCM2835_DISABLE_BASIC_IRQS);

	bcm2835_enabled_gpu1 = 0U;
	bcm2835_enabled_gpu2 = 0U;
	bcm2835_enabled_basic = 0U;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);

	/*
	 * BCM2835 ARMCTRL does not provide a normal per-IRQ priority scheme.
	 * Priority routing is limited to the separate FIQ path, which is left
	 * disabled during the initial Zephyr bring-up.
	 */
}

void z_soc_irq_enable(unsigned int irq)
{
	k_spinlock_key_t key;
	uint32_t bit;

	if (!bcm2835_irq_is_valid(irq)) {
		LOG_ERR("invalid irq %u", irq);
		return;
	}

	key = k_spin_lock(&bcm2835_armctrl_lock);
	bit = bcm2835_irq_bit(irq);

	if (bcm2835_irq_is_gpu1(irq)) {
		sys_write32(bit, BCM2835_ENABLE_IRQS_1);
		bcm2835_enabled_gpu1 |= bit;
	} else if (bcm2835_irq_is_gpu2(irq)) {
		sys_write32(bit, BCM2835_ENABLE_IRQS_2);
		bcm2835_enabled_gpu2 |= bit;
	} else {
		sys_write32(bit, BCM2835_ENABLE_BASIC_IRQS);
		bcm2835_enabled_basic |= bit;
	}

	k_spin_unlock(&bcm2835_armctrl_lock, key);
}

void z_soc_irq_disable(unsigned int irq)
{
	k_spinlock_key_t key;
	uint32_t bit;

	if (!bcm2835_irq_is_valid(irq)) {
		LOG_ERR("invalid irq %u", irq);
		return;
	}

	key = k_spin_lock(&bcm2835_armctrl_lock);
	bit = bcm2835_irq_bit(irq);

	if (bcm2835_irq_is_gpu1(irq)) {
		sys_write32(bit, BCM2835_DISABLE_IRQS_1);
		bcm2835_enabled_gpu1 &= ~bit;
	} else if (bcm2835_irq_is_gpu2(irq)) {
		sys_write32(bit, BCM2835_DISABLE_IRQS_2);
		bcm2835_enabled_gpu2 &= ~bit;
	} else {
		sys_write32(bit, BCM2835_DISABLE_BASIC_IRQS);
		bcm2835_enabled_basic &= ~bit;
	}

	k_spin_unlock(&bcm2835_armctrl_lock, key);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	uint32_t bit;
	int enabled;

	if (!bcm2835_irq_is_valid(irq)) {
		LOG_ERR("invalid irq %u", irq);
		return -EINVAL;
	}

	bit = bcm2835_irq_bit(irq);

	if (bcm2835_irq_is_gpu1(irq)) {
		enabled = (bcm2835_enabled_gpu1 & bit) != 0U;
	} else if (bcm2835_irq_is_gpu2(irq)) {
		enabled = (bcm2835_enabled_gpu2 & bit) != 0U;
	} else {
		enabled = (bcm2835_enabled_basic & bit) != 0U;
	}

	return enabled;
}
