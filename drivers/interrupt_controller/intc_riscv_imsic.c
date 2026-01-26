/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT riscv_imsic

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(intc_riscv_imsic, CONFIG_LOG_DEFAULT_LEVEL);

struct imsic_cfg {
	uintptr_t reg_base;
	uint32_t num_ids;
	uint32_t hart_id;
	uint32_t nr_irqs; /* Effective IRQ limit for bounds checking */
};

/* Forward declaration */
static void imsic_mext_isr(const void *arg);

static int imsic_init(const struct device *dev)
{
	/* MINIMAL: Only write EIDELIVERY and EITHRESHOLD */
	/* Enable delivery in MMSI mode (bits [30:29] = 10 = 0x40000000) */
	uint32_t eidelivery_value = EIDELIVERY_ENABLE | EIDELIVERY_MODE_MMSI;

	LOG_DBG("Setting EIDELIVERY=0x%08x (ENABLE=0x%x, MODE_MMSI=0x%x)", eidelivery_value,
		(unsigned int)EIDELIVERY_ENABLE, (unsigned int)EIDELIVERY_MODE_MMSI);
	micsr_write(ICSR_EIDELIVERY, eidelivery_value);

	/* Set EITHRESHOLD to 0 to allow all interrupts (no priority filtering) */
	micsr_write(ICSR_EITHRESH, 0);

	LOG_DBG("IMSIC init hart=%u num_ids=%u nr_irqs=%u",
		((const struct imsic_cfg *)dev->config)->hart_id,
		((const struct imsic_cfg *)dev->config)->num_ids,
		((const struct imsic_cfg *)dev->config)->nr_irqs);
	LOG_DBG("  EIDELIVERY=0x%08lx EITHRESHOLD=0x%08lx",
		(unsigned long)micsr_read(ICSR_EIDELIVERY),
		(unsigned long)micsr_read(ICSR_EITHRESH));

	return 0;
}

/* Runtime API: claim interrupt (atomic read and clear) */
inline uint32_t riscv_imsic_claim(void)
{
	/* Atomic read and claim: csrrw reads ID and clears its pending bit */
	uint32_t topei = csr_swap(CSR_MTOPEI, 0);

	return topei & MTOPEI_EIID_MASK;
}

/* Helper to calculate EIE register index and bit position for an EIID */
static inline void eiid_to_eie_index(uint32_t eiid, uint32_t *reg_index, uint32_t *bit)
{
	*reg_index = eiid / __riscv_xlen;
	*bit = eiid % __riscv_xlen;
}

/* Enable an EIID in IMSIC EIE - operates on CURRENT CPU's IMSIC via CSRs */
void riscv_imsic_enable_eiid(uint32_t eiid)
{
	uint32_t reg_index, bit;

	eiid_to_eie_index(eiid, &reg_index, &bit);
	uint32_t icsr_addr = ICSR_EIE0 + reg_index;

	LOG_DBG("IMSIC enable EIID %u on CPU %u: EIE[%u] bit %u", eiid, arch_proc_id(),
		reg_index, bit);
	micsr_set(icsr_addr, BIT(bit));
}

/* Disable an EIID in IMSIC EIE - operates on CURRENT CPU's IMSIC via CSRs */
void riscv_imsic_disable_eiid(uint32_t eiid)
{
	uint32_t reg_index, bit;

	eiid_to_eie_index(eiid, &reg_index, &bit);
	uint32_t icsr_addr = ICSR_EIE0 + reg_index;

	micsr_clear(icsr_addr, BIT(bit));
	LOG_DBG("IMSIC disable EIID %u on CPU %u", eiid, arch_proc_id());
}

/* Check if an EIID is enabled on CURRENT CPU's IMSIC */
int riscv_imsic_is_enabled(uint32_t eiid)
{
	uint32_t reg_index, bit;

	eiid_to_eie_index(eiid, &reg_index, &bit);
	uint32_t icsr_addr = ICSR_EIE0 + reg_index;

	return !!(micsr_read(icsr_addr) & BIT(bit));
}

/* Separate IRQ registration for hart 0 vs other harts to avoid duplicate registration */
static void imsic_irq_config_func_0(void)
{
	/* Only hart 0 (instance 0) registers the global MEXT IRQ handler */
	IRQ_CONNECT(RISCV_IRQ_MEXT, 0, imsic_mext_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(RISCV_IRQ_MEXT);
	LOG_DBG("Registered MEXT IRQ handler from hart 0 IMSIC instance");
}

#define IMSIC_IRQ_CONFIG_FUNC_DEFINE_SECONDARY(inst)                                               \
	static void imsic_irq_config_func_##inst(void)                                             \
	{                                                                                          \
		/* Secondary harts just enable MEXT locally, no IRQ_CONNECT */                     \
		irq_enable(RISCV_IRQ_MEXT);                                                        \
		LOG_DBG("Hart %u IMSIC: enabled MEXT locally (no IRQ_CONNECT)",                    \
			DT_INST_PROP(inst, riscv_hart_id));                                        \
	}

/* Generate secondary IRQ config functions for instances 1+ */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
IMSIC_IRQ_CONFIG_FUNC_DEFINE_SECONDARY(1)
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 2
IMSIC_IRQ_CONFIG_FUNC_DEFINE_SECONDARY(2)
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 3
IMSIC_IRQ_CONFIG_FUNC_DEFINE_SECONDARY(3)
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 4
IMSIC_IRQ_CONFIG_FUNC_DEFINE_SECONDARY(4)
#endif

#define IMSIC_INIT(inst)                                                                           \
	static const struct imsic_cfg imsic_cfg_##inst = {                                         \
		.reg_base = DT_INST_REG_ADDR(inst),                                                \
		.num_ids = DT_INST_PROP(inst, riscv_num_ids),                                      \
		.hart_id = DT_INST_PROP(inst, riscv_hart_id),                                      \
		.nr_irqs = MIN(DT_INST_PROP(inst, riscv_num_ids), CONFIG_NUM_IRQS),                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, imsic_init, NULL, NULL, &imsic_cfg_##inst, PRE_KERNEL_1,       \
			      CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IMSIC_INIT)

/* Call IRQ config functions at POST_KERNEL level to register MEXT handler */
static int imsic_irq_init(void)
{
	/* Call instance 0 (always present) */
	imsic_irq_config_func_0();

	/* Call secondary instance functions if they exist */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
	imsic_irq_config_func_1();
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 2
	imsic_irq_config_func_2();
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 3
	imsic_irq_config_func_3();
#endif
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 4
	imsic_irq_config_func_4();
#endif

	return 0;
}

SYS_INIT(imsic_irq_init, POST_KERNEL, CONFIG_INTC_INIT_PRIORITY);

/* Include for irq_from_level_2() */
#include <zephyr/irq_multilevel.h>

/*
 * MEXT interrupt handler: claim EIID from IMSIC and dispatch to registered ISR
 *
 * With 1:1 mapping, EIID equals IRQ number directly.
 */
static void imsic_mext_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct imsic_cfg *cfg = dev->config;

	LOG_DBG("MEXT ISR entered");

	uint32_t eiid = riscv_imsic_claim();

	if (eiid == 0U) {
		return; /* Spurious or already claimed */
	}

	/* 1:1 mapping: EIID is the IRQ number */
	uint32_t irq = eiid;

	LOG_DBG("MEXT claimed EIID/IRQ %u", irq);

	/* Bounds check */
	if (irq >= cfg->nr_irqs) {
		LOG_ERR("IRQ %u out of range (>= %u)", irq, cfg->nr_irqs);
		z_irq_spurious(NULL);
	}

	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
}

#ifdef CONFIG_SMP
/**
 * @brief Initialize IMSIC on secondary CPUs
 *
 * This function is called on each secondary CPU during SMP boot to initialize
 * the IMSIC interrupt controller on that CPU. It configures the EIDELIVERY
 * and EITHRESHOLD CSRs to enable interrupt delivery.
 *
 * This follows the same pattern as smp_timer_init() for the CLINT timer.
 *
 * Note: IMSIC CSRs (accessed via ISELECT/IREG) are local to each CPU.
 * When this function executes on CPU N, it configures that CPU's IMSIC file.
 */
void z_riscv_imsic_secondary_init(void)
{
	LOG_DBG("IMSIC secondary init on CPU %u", arch_proc_id());

	/* Enable interrupt delivery in MMSI mode */
	/* EIDELIVERY[0] = 1: Enable delivery */
	/* EIDELIVERY[30:29] = 10: MMSI mode (0x40000000) */
	uint32_t eidelivery_value = EIDELIVERY_ENABLE | EIDELIVERY_MODE_MMSI;

	micsr_write(ICSR_EIDELIVERY, eidelivery_value);

	/* Set EITHRESHOLD to 0 to allow all interrupt priorities */
	micsr_write(ICSR_EITHRESH, 0);

	/* Enable MEXT interrupt on this CPU */
	irq_enable(RISCV_IRQ_MEXT);

	/* Read back to verify initialization */
	unsigned long eidelivery_readback = micsr_read(ICSR_EIDELIVERY);

	LOG_DBG("CPU %u IMSIC initialized: EIDELIVERY=0x%08lx EITHRESH=0x%08lx", arch_proc_id(),
		eidelivery_readback, (unsigned long)micsr_read(ICSR_EITHRESH));

	/* Sanity check: verify EIDELIVERY enable bit is set */
	if (!(eidelivery_readback & EIDELIVERY_ENABLE)) {
		LOG_ERR("CPU %u IMSIC EIDELIVERY enable bit not set! Got 0x%08lx", arch_proc_id(),
			eidelivery_readback);
	}
}
#endif /* CONFIG_SMP */
