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

struct imsic_data {
	/* empty for MVP */
};

/* Forward declaration */
static void imsic_mext_isr(const void *arg);

static int imsic_init(const struct device *dev)
{
	const struct imsic_cfg *cfg = dev->config;

	/* MINIMAL: Only write EIDELIVERY and EITHRESHOLD */
	/* Enable delivery in MMSI mode (bits [30:29] = 10 = 0x40000000) */
	uint32_t eidelivery_value = EIDELIVERY_ENABLE | EIDELIVERY_MODE_MMSI;
	LOG_INF("Setting EIDELIVERY=0x%08x (ENABLE=0x%x, MODE_MMSI=0x%x)", eidelivery_value,
		(unsigned int)EIDELIVERY_ENABLE, (unsigned int)EIDELIVERY_MODE_MMSI);
	write_imsic_csr(ICSR_EIDELIVERY, eidelivery_value);

	/* Set EITHRESHOLD to 0 to allow all interrupts (no priority filtering) */
	write_imsic_csr(ICSR_EITHRESH, 0);

	/* Read back to verify */
	uint32_t readback = read_imsic_csr(ICSR_EIDELIVERY);
	uint32_t thresh_readback = read_imsic_csr(ICSR_EITHRESH);

	LOG_INF("IMSIC init hart=%u num_ids=%u nr_irqs=%u", cfg->hart_id, cfg->num_ids,
		cfg->nr_irqs);
	LOG_INF("  EIDELIVERY=0x%08x EITHRESHOLD=0x%08x", readback, thresh_readback);

	return 0;
}

/* Runtime API: claim interrupt (read and acknowledge) */
uint32_t riscv_imsic_claim(void)
{
	uint32_t topei;
	__asm__ volatile("csrrw %0, " STRINGIFY(CSR_MTOPEI) ", x0" : "=r"(topei));
	return topei & MTOPEI_EIID_MASK; /* Extract EIID from mtopei */
}

/* Runtime API: complete interrupt (write back to clear) */
void riscv_imsic_complete(uint32_t eiid)
{
	/* Write back to mtopei to complete the claim */
	__asm__ volatile("csrw " STRINGIFY(CSR_MTOPEI) ", %0" : : "r"(eiid));
}

/* Enable an EIID in IMSIC EIE - operates on CURRENT CPU's IMSIC via CSRs */
void riscv_imsic_enable_eiid(uint32_t eiid)
{
	/* IMSIC implements EIE0..EIE7, 32 IDs each */
	uint32_t reg_index = eiid / 32U; /* 0..7 */
	uint32_t bit = eiid % 32U;       /* 0..31 */
	uint32_t icsr_addr = ICSR_EIE0 + reg_index;

	/* CSR writes execute on current hart and route to that hart's IMSIC */
	uint32_t cur = read_imsic_csr(icsr_addr);
	uint32_t cpu_id = arch_proc_id();
	LOG_INF("IMSIC enable EIID %u on CPU %u: EIE[%u] before=0x%08x", eiid, cpu_id, reg_index,
		cur);
	cur |= BIT(bit);
	write_imsic_csr(icsr_addr, cur);

	/* Read back to verify */
	uint32_t verify = read_imsic_csr(icsr_addr);
	LOG_INF("IMSIC enable EIID %u on CPU %u: EIE[%u] after=0x%08x (bit %u)", eiid, cpu_id,
		reg_index, verify, bit);
}

/* Disable an EIID in IMSIC EIE - operates on CURRENT CPU's IMSIC via CSRs */
void riscv_imsic_disable_eiid(uint32_t eiid)
{
	uint32_t reg_index = eiid / 32U;
	uint32_t bit = eiid % 32U;
	uint32_t icsr_addr = ICSR_EIE0 + reg_index;

	uint32_t cur = read_imsic_csr(icsr_addr);
	cur &= ~BIT(bit);
	write_imsic_csr(icsr_addr, cur);

	LOG_DBG("IMSIC disable EIID %u on CPU %u", eiid, arch_proc_id());
}

/* Check if an EIID is enabled on CURRENT CPU's IMSIC */
int riscv_imsic_is_enabled(uint32_t eiid)
{
	uint32_t reg_index = eiid / 32U;
	uint32_t bit = eiid % 32U;
	uint32_t icsr_addr = ICSR_EIE0 + reg_index;

	uint32_t cur = read_imsic_csr(icsr_addr);
	return !!(cur & BIT(bit));
}

/* Read pending from EIP0 */
uint32_t riscv_imsic_get_pending(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* Return pending bits for first 64 IDs as a quick probe (EIP0|EIP1<<32) */
	uint32_t p0 = read_imsic_csr(ICSR_EIP0);
	uint32_t p1 = read_imsic_csr(ICSR_EIP1);
	return p0 | (p1 ? 0x80000000U : 0U); /* signal >32 pending via MSB */
}

/* Separate IRQ registration for hart 0 vs other harts to avoid duplicate registration */
static void imsic_irq_config_func_0(void)
{
	/* Only hart 0 (instance 0) registers the global MEXT IRQ handler */
	IRQ_CONNECT(11, 0, imsic_mext_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(11);
	LOG_INF("Registered MEXT IRQ handler from hart 0 IMSIC instance");
}

#define IMSIC_IRQ_CONFIG_FUNC_DEFINE_SECONDARY(inst)                                               \
	static void imsic_irq_config_func_##inst(void)                                             \
	{                                                                                          \
		/* Secondary harts just enable MEXT locally, no IRQ_CONNECT */                     \
		irq_enable(11);                                                                    \
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
	static struct imsic_data imsic_data_##inst;                                                \
	static const struct imsic_cfg imsic_cfg_##inst = {                                         \
		.reg_base = DT_INST_REG_ADDR(inst),                                                \
		.num_ids = DT_INST_PROP(inst, riscv_num_ids),                                      \
		.hart_id = DT_INST_PROP(inst, riscv_hart_id),                                      \
		.nr_irqs = MIN(DT_INST_PROP(inst, riscv_num_ids), CONFIG_NUM_IRQS),                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, imsic_init, NULL, &imsic_data_##inst, &imsic_cfg_##inst,       \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

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

/* Global device pointer for easy access */
static const struct device *imsic_device;

/* Zephyr integration functions */
const struct device *riscv_imsic_get_dev(void)
{
	if (!imsic_device) {
		imsic_device = DEVICE_DT_GET_ANY(riscv_imsic);
	}
	return imsic_device;
}

/* Redundant Zephyr integration functions removed - now handled by unified AIA driver */

/* MEXT interrupt handler: claim EIID from IMSIC and dispatch to registered ISR */
void imsic_mext_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct imsic_cfg *cfg = dev->config;

	LOG_DBG("MEXT ISR entered");

	while (1) {
		uint32_t eiid = riscv_imsic_claim();
		if (eiid == 0U) {
			break; /* No more pending interrupts */
		}

		LOG_INF("MEXT claimed EIID %u, dispatching to ISR table", eiid);

		/* Bounds check - critical for safety (following PLIC pattern) */
		if (eiid >= cfg->nr_irqs) {
			LOG_ERR("EIID %u out of range (>= %u)", eiid, cfg->nr_irqs);
			riscv_imsic_complete(eiid);
			z_irq_spurious(NULL); /* Fatal - doesn't return */
		}

		/* Dispatch to ISR table using EIID as direct index (AIA flat namespace) */
		_sw_isr_table[eiid].isr(_sw_isr_table[eiid].arg);

		riscv_imsic_complete(eiid);
	}
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
	unsigned int cpu_id = arch_proc_id();

	LOG_INF("IMSIC secondary init on CPU %u", cpu_id);

	/* Enable interrupt delivery in MMSI mode */
	/* EIDELIVERY[0] = 1: Enable delivery */
	/* EIDELIVERY[30:29] = 10: MMSI mode (0x40000000) */
	uint32_t eidelivery_value = EIDELIVERY_ENABLE | EIDELIVERY_MODE_MMSI;
	write_imsic_csr(ICSR_EIDELIVERY, eidelivery_value);

	/* Set EITHRESHOLD to 0 to allow all interrupt priorities */
	write_imsic_csr(ICSR_EITHRESH, 0);

	/* Enable MEXT interrupt on this CPU */
	irq_enable(RISCV_IRQ_MEXT);

	/* Read back to verify initialization */
	uint32_t eidelivery_readback = read_imsic_csr(ICSR_EIDELIVERY);
	uint32_t eithresh_readback = read_imsic_csr(ICSR_EITHRESH);

	LOG_INF("CPU %u IMSIC initialized: EIDELIVERY=0x%08x EITHRESH=0x%08x", cpu_id,
		eidelivery_readback, eithresh_readback);

	/* Sanity check: verify EIDELIVERY enable bit is set */
	if (!(eidelivery_readback & EIDELIVERY_ENABLE)) {
		LOG_ERR("CPU %u IMSIC EIDELIVERY enable bit not set! Got 0x%08x", cpu_id,
			eidelivery_readback);
	}
}
#endif /* CONFIG_SMP */
