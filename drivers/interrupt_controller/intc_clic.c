/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 * Copyright (c) 2025 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Core Local Interrupt Controller
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/icsr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/riscv_clic.h>
#include "intc_clic.h"

#if DT_HAS_COMPAT_STATUS_OKAY(riscv_clic)
#define DT_DRV_COMPAT riscv_clic
#elif DT_HAS_COMPAT_STATUS_OKAY(nuclei_eclic)
#define DT_DRV_COMPAT nuclei_eclic
#else
#error "Unknown CLIC controller compatible for this configuration"
#endif

struct clic_data {
	uint8_t nlbits;
	uint8_t intctlbits;
};

struct clic_config {
	mem_addr_t base;
};

struct pmp_stack_guard_key_t {
	unsigned long mstatus;
	unsigned int irq_key;
};

/*
 * M-mode CLIC memory-mapped registers are accessible only in M-mode.
 * Temporarily disable the PMP stack guard (set mstatus.MPRV = 0) to configure
 * CLIC registers, then restore the PMP stack guard using these functions.
 */
static ALWAYS_INLINE void disable_pmp_stack_guard(struct pmp_stack_guard_key_t *key)
{
	if (IS_ENABLED(CONFIG_PMP_STACK_GUARD)) {
		key->irq_key = irq_lock();
		key->mstatus = csr_read_clear(mstatus, MSTATUS_MPRV);
	} else {
		ARG_UNUSED(key);
	}
}

static ALWAYS_INLINE void restore_pmp_stack_guard(struct pmp_stack_guard_key_t key)
{
	if (IS_ENABLED(CONFIG_PMP_STACK_GUARD)) {
		csr_write(mstatus, key.mstatus);
		irq_unlock(key.irq_key);
	} else {
		ARG_UNUSED(key);
	}
}

static ALWAYS_INLINE void write_clic32(const struct device *dev, uint32_t offset, uint32_t value)
{
	const struct clic_config *config = dev->config;
	mem_addr_t reg_addr = config->base + offset;
	struct pmp_stack_guard_key_t key;

	disable_pmp_stack_guard(&key);
	sys_write32(value, reg_addr);
	restore_pmp_stack_guard(key);
}

static ALWAYS_INLINE uint32_t read_clic32(const struct device *dev, uint32_t offset)
{
	const struct clic_config *config = dev->config;
	mem_addr_t reg_addr = config->base + offset;
	struct pmp_stack_guard_key_t key;
	uint32_t reg;

	disable_pmp_stack_guard(&key);
	reg = sys_read32(reg_addr);
	restore_pmp_stack_guard(key);

	return reg;
}

static ALWAYS_INLINE void write_clic8(const struct device *dev, uint32_t offset, uint8_t value)
{
	const struct clic_config *config = dev->config;
	mem_addr_t reg_addr = config->base + offset;
	struct pmp_stack_guard_key_t key;

	disable_pmp_stack_guard(&key);
	sys_write8(value, reg_addr);
	restore_pmp_stack_guard(key);
}

static ALWAYS_INLINE uint8_t read_clic8(const struct device *dev, uint32_t offset)
{
	const struct clic_config *config = dev->config;
	mem_addr_t reg_addr = config->base + offset;
	struct pmp_stack_guard_key_t key;
	uint32_t reg;

	disable_pmp_stack_guard(&key);
	reg = sys_read8(reg_addr);
	restore_pmp_stack_guard(key);

	return reg;
}

/**
 * @brief Enable interrupt
 */
void riscv_clic_irq_enable(uint32_t irq)
{
#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	const struct device *dev = DEVICE_DT_INST_GET(0);
	union CLICINTIE clicintie = {.b = {.IE = 0x1}};

	write_clic8(dev, CLIC_INTIE(irq), clicintie.w);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	micsr2_set(CLIC_INTIE(irq), BIT(irq % 32));
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif
}

/**
 * @brief Disable interrupt
 */
void riscv_clic_irq_disable(uint32_t irq)
{
#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	const struct device *dev = DEVICE_DT_INST_GET(0);
	union CLICINTIE clicintie = {.b = {.IE = 0x0}};

	write_clic8(dev, CLIC_INTIE(irq), clicintie.w);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	micsr2_clear(CLIC_INTIE(irq), BIT(irq % 32));
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif
}

/**
 * @brief Get enable status of interrupt
 */
int riscv_clic_irq_is_enabled(uint32_t irq)
{
	int is_enabled = 0;

#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	const struct device *dev = DEVICE_DT_INST_GET(0);
	union CLICINTIE clicintie = {.w = read_clic8(dev, CLIC_INTIE(irq))};

	is_enabled = clicintie.b.IE;
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	is_enabled = micsr2_read(CLIC_INTIE(irq)) & BIT(irq % 32);
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif

	return !!is_enabled;
}

/**
 * @brief Set priority and level of interrupt
 */
void riscv_clic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_data *data = dev->data;
#ifdef CONFIG_RISCV_ISA_EXT_SMCSRIND
	uint32_t bit_offset = 8 * (irq % 4);
#endif

	/*
	 * Set the interrupt level and the interrupt priority.
	 * Examples of mcliccfg settings:
	 * CLICINTCTLBITS mnlbits clicintctl[i] interrupt levels
	 *       0         2      ........      255
	 *       1         2      l.......      127,255
	 *       2         2      ll......      63,127,191,255
	 *       3         3      lll.....      31,63,95,127,159,191,223,255
	 *       4         1      lppp....      127,255
	 * "." bits are non-existent bits for level encoding, assumed to be 1
	 * "l" bits are available variable bits in level specification
	 * "p" bits are available variable bits in priority specification
	 */
	const uint8_t max_level = BIT_MASK(data->nlbits);
	const uint8_t max_prio = BIT_MASK(data->intctlbits - data->nlbits);
	uint8_t intctrl = (MIN(pri, max_prio) << (8U - data->intctlbits)) |
			  (MIN(pri, max_level) << (8U - data->nlbits)) |
			  BIT_MASK(8U - data->intctlbits);

#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	write_clic8(dev, CLIC_INTCTRL(irq), intctrl);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	uint32_t clicintctl;

	clicintctl = micsr_read(CLIC_INTCTRL(irq));
	clicintctl &= ~GENMASK(bit_offset + 7, bit_offset);
	clicintctl |= intctrl << bit_offset;
	micsr_write(CLIC_INTCTRL(irq), clicintctl);
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif

	/* Set the IRQ operates in machine mode, non-vectoring and the trigger type. */
	union CLICINTATTR clicattr = {.b = {.mode = 0x3, .shv = 0x0, .trg = flags & BIT_MASK(3)}};

#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	write_clic8(dev, CLIC_INTATTR(irq), clicattr.w);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	uint32_t clicintattr;

	clicintattr = micsr2_read(CLIC_INTATTR(irq));
	clicintattr &= ~GENMASK(bit_offset + 7, bit_offset);
	clicintattr |= clicattr.w << bit_offset;
	micsr2_write(CLIC_INTATTR(irq), clicintattr);
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif
}

/**
 * @brief Set vector mode of interrupt
 */
void riscv_clic_irq_vector_set(uint32_t irq)
{
#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	const struct device *dev = DEVICE_DT_INST_GET(0);
	union CLICINTATTR clicattr = {.w = read_clic8(dev, CLIC_INTATTR(irq))};

	/* Set Selective Hardware Vectoring. */
	clicattr.b.shv = 1;
	write_clic8(dev, CLIC_INTATTR(irq), clicattr.w);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	uint32_t clicintattr;
	union CLICINTATTR clicattr = {.b = {.shv = 1}};
	uint32_t bit_offset = 8 * (irq % 4);

	clicintattr = micsr2_read(CLIC_INTATTR(irq));
	clicintattr |= clicattr.w << bit_offset;
	micsr2_write(CLIC_INTATTR(irq), clicintattr);
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif
}

/**
 * @brief Set pending bit of an interrupt
 */
void riscv_clic_irq_set_pending(uint32_t irq)
{
#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	const struct device *dev = DEVICE_DT_INST_GET(0);
	union CLICINTIP clicintip = {.b = {.IP = 0x1}};

	write_clic8(dev, CLIC_INTIP(irq), clicintip.w);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	micsr_set(CLIC_INTIP(irq), BIT(irq % 32));
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif
}

static int clic_init(const struct device *dev)
{
	struct clic_data *data = dev->data;

	if (IS_ENABLED(CONFIG_NUCLEI_ECLIC)) {
		/* Configure the interrupt level threshold. */
		union CLICMTH clicmth = {.b = {.mth = 0x0}};

		write_clic32(dev, CLIC_MTH, clicmth.qw);

		/* Detect the number of bits for the clicintctl register. */
		union CLICINFO clicinfo = {.qw = read_clic32(dev, CLIC_INFO)};

		data->intctlbits = clicinfo.b.intctlbits;

		if (data->nlbits > data->intctlbits) {
			data->nlbits = data->intctlbits;
		}
	} else {
		/* Configure the interrupt level threshold by CSR mintthresh. */
		csr_write(CSR_MINTTHRESH, 0x0);
	}

	if (IS_ENABLED(CONFIG_CLIC_SMCLICCONFIG_EXT)) {
#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
		/* Configure the number of bits assigned to interrupt levels. */
		union CLICCFG cliccfg = {.qw = read_clic32(dev, CLIC_CFG)};

		cliccfg.w.nlbits = data->nlbits;
		write_clic32(dev, CLIC_CFG, cliccfg.qw);
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
		union CLICCFG cliccfg = {.qw = micsr_read(CLIC_CFG)};

		cliccfg.w.nlbits = data->nlbits;
		micsr_write(CLIC_CFG, cliccfg.qw);
#else
		#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif
	}

#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
	/* Reset all interrupt control register. */
	for (int i = 0; i < CONFIG_NUM_IRQS; i++) {
		write_clic32(dev, CLIC_CTRL(i), 0);
	}
#elif CONFIG_RISCV_ISA_EXT_SMCSRIND
	/* Reset all clicintip, clicintie register. */
	for (int i = 0; i < CONFIG_NUM_IRQS; i += 32) {
		micsr_write(CLIC_INTIP(i), 0);
		micsr2_write(CLIC_INTIE(i), 0);
	}

	/* Reset all clicintctl, clicintattr register. */
	for (int i = 0; i < CONFIG_NUM_IRQS; i += 4) {
		micsr_write(CLIC_INTCTRL(i), 0);
		micsr2_write(CLIC_INTATTR(i), 0);
	}
#else
	#error "CLIC platforms must support either memory-mapped or SMCSRIND access"
#endif

	return 0;
}

#define CLIC_INTC_DATA_INIT(n)                                                                     \
	static struct clic_data clic_data_##n = {                                                  \
		.nlbits = CONFIG_CLIC_PARAMETER_MNLBITS,                                           \
		.intctlbits = CONFIG_CLIC_PARAMETER_INTCTLBITS,                                    \
	};
#define CLIC_INTC_CONFIG_INIT(n)                                                                   \
	const static struct clic_config clic_config_##n = {                                        \
		.base = COND_CODE_1(CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS,                           \
				   (DT_REG_ADDR(DT_DRV_INST(n))), (0)),                            \
	};
#define CLIC_INTC_DEVICE_INIT(n)                                                                   \
	CLIC_INTC_DATA_INIT(n)                                                                     \
	CLIC_INTC_CONFIG_INIT(n)                                                                   \
	DEVICE_DT_INST_DEFINE(n, &clic_init, NULL, &clic_data_##n, &clic_config_##n, PRE_KERNEL_1, \
			      CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CLIC_INTC_DEVICE_INIT)
