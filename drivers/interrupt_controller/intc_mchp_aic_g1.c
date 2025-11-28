/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <soc.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT microchip_aic_g1_intc

enum AIC_TYPE {
	AIC_NON_SECURE,
	AIC_SECURE,
	AIC_TYPE_COUNT,
};

static aic_registers_t *aic_reg[AIC_TYPE_COUNT];

struct mchp_aic_dev_cfg {
	const aic_registers_t *regs;
	enum AIC_TYPE type;
};

static struct k_spinlock lock;

static aic_registers_t *mchp_aic_get_aic(uint32_t source)
{
	ARG_UNUSED(source);

#ifdef SFR_AICREDIR_NSAIC_Msk
	if (SFR_REGS->SFR_AICREDIR & SFR_AICREDIR_NSAIC_Msk == SFR_AICREDIR_NSAIC(1)) {
		return aic_reg[AIC_NON_SECURE];
	}
	__ASSERT(false, "Interrupts managed by the AICs corresponding to the "
			"Secure State of the peripheral to be implemented");
#endif

	return aic_reg[AIC_NON_SECURE];
}

void z_aic_irq_enable(unsigned int irq)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	aic_registers_t *aic = mchp_aic_get_aic(irq);

	aic->AIC_SSR = AIC_SSR_INTSEL(irq);
	aic->AIC_IECR = AIC_IECR_INTEN_Msk;

	k_spin_unlock(&lock, key);
}

void z_aic_irq_disable(unsigned int irq)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	aic_registers_t *aic = mchp_aic_get_aic(irq);

	aic->AIC_SSR = AIC_SSR_INTSEL(irq);
	aic->AIC_IDCR = AIC_IDCR_INTD_Msk;

	k_spin_unlock(&lock, key);
}

int z_aic_irq_is_enabled(unsigned int irq)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	aic_registers_t *aic = mchp_aic_get_aic(irq);
	unsigned int imr;

	aic->AIC_SSR = AIC_SSR_INTSEL(irq);
	imr = aic->AIC_IMR;

	k_spin_unlock(&lock, key);

	return ((imr & AIC_IMR_INTM_Msk) == 0) ? 0 : 1;
}

bool z_soc_irq_is_pending(unsigned int irq)
{
	aic_registers_t *aic = mchp_aic_get_aic(irq);
	unsigned int ipr, mask;

	if (irq <= 31) {
		ipr = aic->AIC_IPR0;
		mask = 1 << irq;
	} else if (irq <= 63) {
		ipr = aic->AIC_IPR1;
		mask = 1 << (irq - 32);
	} else if (irq <= 95) {
		ipr = aic->AIC_IPR2;
		mask = 1 << (irq - 63);
	} else if (irq <= 127) {
		ipr = aic->AIC_IPR3;
		mask = 1 << (irq - 96);
	} else {
		return 0;
	}

	return ((ipr & mask) == 0) ? 0 : 1;
}

void z_soc_irq_set_pending(unsigned int irq)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	aic_registers_t *aic = mchp_aic_get_aic(irq);

	aic->AIC_SSR = AIC_SSR_INTSEL(irq);
	aic->AIC_ISCR = AIC_ISCR_INTSET_Msk;

	k_spin_unlock(&lock, key);
}

void z_soc_irq_clear_pending(unsigned int irq)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	aic_registers_t *aic = mchp_aic_get_aic(irq);

	aic->AIC_SSR = AIC_SSR_INTSEL(irq);
	aic->AIC_ICCR = AIC_ICCR_INTCLR_Msk;

	k_spin_unlock(&lock, key);
}

void z_aic_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	aic_registers_t *aic = mchp_aic_get_aic(irq);

	aic->AIC_SSR = AIC_SSR_INTSEL(irq);

#if defined(AIC_SMR_PRIORITY)
	aic->AIC_SMR = AIC_SMR_SRCTYPE(flags) | AIC_SMR_PRIORITY(prio);
#elif defined(AIC_SMR_PRIOR)
	aic->AIC_SMR = AIC_SMR_SRCTYPE(flags) | AIC_SMR_PRIOR(prio);
#else
#error "Definition for AIC_SMR register not found"
#endif

	k_spin_unlock(&lock, key);
}

unsigned int z_aic_irq_get_active(void)
{
	aic_registers_t *aic = aic_reg[AIC_NON_SECURE];

	(void)aic->AIC_IVR;

	return aic->AIC_ISR;
}

void z_aic_irq_eoi(unsigned int irq)
{
	aic_registers_t *aic = mchp_aic_get_aic(irq);

	aic->AIC_EOICR = AIC_EOICR_ENDIT(1);
}

void z_aic_irq_init(void)
{
	/* nothing to initialize */
}

int mchp_aic_init(const struct device *dev)
{
	const struct mchp_aic_dev_cfg *const cfg = dev->config;
	aic_registers_t *aic = (aic_registers_t *)(uintptr_t)cfg->regs;
	k_spinlock_key_t key;
	int i;

	aic_reg[cfg->type] = aic;

	/* No debugging in AIC: Debug (Protect) Control Register */
	aic->AIC_DCR = AIC_DCR_GMSK(0) | AIC_DCR_PROT(0);

	/*
	 * Spurious Interrupt ID in Spurious Vector Register.
	 * When there is no current interrupt, the IRQ Vector Register reads
	 * the value stored in AIC_SPU
	 */
	aic->AIC_SPU = AIC_SPU_Msk;

	/* Perform 8 End Of Interrupt Command to make sure AIC will not Lock out nIRQ */
	for (i = 0; i < 8; i++) {
		aic->AIC_EOICR = AIC_EOICR_ENDIT(1);
	}

	key = k_spin_lock(&lock);
	/* Disable and clear all interrupts initially */
	for (i = 0; i < AIC_SSR_Msk + 1; i++) {
		aic->AIC_SSR = AIC_SSR_INTSEL(i);
		aic->AIC_IDCR = AIC_IDCR_INTD_Msk;
		aic->AIC_ICCR = AIC_ICCR_INTCLR_Msk;
	}
	k_spin_unlock(&lock, key);

	return 0;
}

#define MCHP_AIC_INIT(n)									\
		BUILD_ASSERT(n < AIC_TYPE_COUNT, "Too many AIC instances to support");		\
												\
		static const struct mchp_aic_dev_cfg mchp_aic##n##_config = {			\
			.regs = (const aic_registers_t *)DT_INST_REG_ADDR(n),			\
			.type = DT_INST_ENUM_IDX(n, type),					\
		};										\
												\
		DEVICE_DT_INST_DEFINE(n, mchp_aic_init, NULL, NULL,				\
				      &mchp_aic##n##_config,					\
				      PRE_KERNEL_1,						\
				      CONFIG_INTC_INIT_PRIORITY,				\
				      NULL);

DT_INST_FOREACH_STATUS_OKAY(MCHP_AIC_INIT)
