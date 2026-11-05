/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rh850.h>

/* INTC2 (shared): exactly one instance */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(renesas_rh850_intc2) == 1,
	     "Exactly one renesas,rh850-intc2 (INTC2) instance is required");

#define RH850_INTC2_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_rh850_intc2)

/* Maximum number of interrupt lines for each controller type, retrieved from device tree */
#define RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES                                                         \
	(DT_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_rh850_intc1), num_lines))
#define RH850_IRQ_INTC2_VECTOR_MAX_ENTRIES (DT_PROP(RH850_INTC2_NODE, num_lines))
#define RH850_IRQ_FEINT_VECTOR_MAX_ENTRIES                                                         \
	(DT_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_rh850_feinc), num_lines))
#define RH850_IRQ_FENMI_VECTOR_MAX_ENTRIES (1UL)

/* Combined interrupt vector counts for range validation */
#define RH850_IRQ_INTC_VECTOR_MAX_ENTRIES                                                          \
	(RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES + RH850_IRQ_INTC2_VECTOR_MAX_ENTRIES)

#define RH850_IRQ_INT_VECTOR_MAX_ENTRIES                                                           \
	(RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES + RH850_IRQ_INTC2_VECTOR_MAX_ENTRIES +                 \
	 RH850_IRQ_FEINT_VECTOR_MAX_ENTRIES + RH850_IRQ_FENMI_VECTOR_MAX_ENTRIES)

/* EICn (16-bit) register fields */
#define RH850_EIC_EIP_MASK GENMASK(3, 0)
#define RH850_EIC_EITB     BIT(6)
#define RH850_EIC_EIMK     BIT(7)

/* EIBDn (32-bit) register fields */
#define RH850_EIBD_PEID_MASK GENMASK(2, 0)

/* Register address calculation */
#define RH850_IRQ_EIC_REG(base, irq)  ((mm_reg_t)(base) + (2U * (irq)))
#define RH850_IRQ_EIBD_REG(base, irq) ((mm_reg_t)(base) + (4U * (irq)))

/* Per-PE register tables, indexed by hardware PEID */
#define RH850_IRQ_GET_EIC_REG(n)                                                                   \
	[DT_PROP(n, renesas_pe_id)] = DT_REG_ADDR_BY_NAME(n, EIC),
#define RH850_IRQ_GET_FEINTMSK_REG(n)                                                              \
	[DT_PROP(n, renesas_pe_id)] = DT_REG_ADDR_BY_NAME(n, FEINTMSK),

static const mm_reg_t intc1_eic_reg[] = {
	DT_FOREACH_STATUS_OKAY(renesas_rh850_intc1, RH850_IRQ_GET_EIC_REG)};

static const mm_reg_t intc2_eic_reg = DT_REG_ADDR_BY_NAME(RH850_INTC2_NODE, EIC);

static const mm_reg_t intc2_eibd_reg = DT_REG_ADDR_BY_NAME(RH850_INTC2_NODE, EIBD);

static const mm_reg_t feinc_feintmsk_reg[] = {
	DT_FOREACH_STATUS_OKAY(renesas_rh850_feinc, RH850_IRQ_GET_FEINTMSK_REG)};

/* Returns the INTC1 EICn address of the given PE */
static mm_reg_t intc1_eic_addr(uint32_t core_id, unsigned int irq)
{
	__ASSERT((core_id < ARRAY_SIZE(intc1_eic_reg)) && (intc1_eic_reg[core_id] != 0U),
		 "No INTC1 instance for PE%u", core_id);

	return RH850_IRQ_EIC_REG(intc1_eic_reg[core_id], irq);
}

/* Returns the FEINTMSK address of the given PE */
static mm_reg_t feintmsk_addr(uint32_t core_id)
{
	__ASSERT((core_id < ARRAY_SIZE(feinc_feintmsk_reg)) && (feinc_feintmsk_reg[core_id] != 0U),
		 "No FEINC instance for PE%u", core_id);

	return feinc_feintmsk_reg[core_id];
}

void intc_rh850_irq_enable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "trying to enable invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "trying to enable reserved interrupt (%u)",
		 irq);

	unsigned int key = irq_lock();
	uint32_t core_id = rh850_get_core_id();

	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		/* INTC1: select table reference method, then unmask */
		mm_reg_t eic = intc1_eic_addr(core_id, irq);

		sys_write16(sys_read16(eic) | RH850_EIC_EITB, eic);
		sys_write16(sys_read16(eic) & (uint16_t)~RH850_EIC_EIMK, eic);
	} else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		/* INTC2: select table reference method, unmask, then bind to this PE */
		mm_reg_t eic = RH850_IRQ_EIC_REG(intc2_eic_reg, irq);
		mm_reg_t eibd = RH850_IRQ_EIBD_REG(intc2_eibd_reg, irq);

		sys_write16(sys_read16(eic) | RH850_EIC_EITB, eic);
		sys_write16(sys_read16(eic) & (uint16_t)~RH850_EIC_EIMK, eic);
		sys_write32((sys_read32(eibd) & ~RH850_EIBD_PEID_MASK) |
				    FIELD_PREP(RH850_EIBD_PEID_MASK, core_id),
			    eibd);
	} else if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		/* FEINT: clear the mask bit */
		mm_reg_t msk = feintmsk_addr(core_id);

		sys_write32(sys_read32(msk) & ~BIT(irq - RH850_IRQ_INTC_VECTOR_MAX_ENTRIES), msk);
	} else {
		/* Not handled by this controller */
	}

	irq_unlock(key);
}

void intc_rh850_irq_disable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "trying to disable invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "trying to disable reserved interrupt (%u)",
		 irq);

	unsigned int key = irq_lock();
	uint32_t core_id = rh850_get_core_id();

	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		/* INTC1: set the mask bit */
		mm_reg_t eic = intc1_eic_addr(core_id, irq);

		sys_write16(sys_read16(eic) | RH850_EIC_EIMK, eic);
	} else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		/* INTC2: set the mask bit */
		mm_reg_t eic = RH850_IRQ_EIC_REG(intc2_eic_reg, irq);

		sys_write16(sys_read16(eic) | RH850_EIC_EIMK, eic);
	} else if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		/* FEINT: set the mask bit */
		mm_reg_t msk = feintmsk_addr(core_id);

		sys_write32(sys_read32(msk) | BIT(irq - RH850_IRQ_INTC_VECTOR_MAX_ENTRIES), msk);
	} else {
		/* Not handled by this controller */
	}

	irq_unlock(key);
}

bool intc_rh850_irq_is_enabled(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "is_enabled on invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "is_enabled on reserved interrupt (%u)", irq);

	uint32_t core_id = rh850_get_core_id();

	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		/* INTC1: enabled when the mask bit is clear */
		return (sys_read16(intc1_eic_addr(core_id, irq)) & RH850_EIC_EIMK) == 0U;
	} else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		/* INTC2: enabled when the mask bit is clear */
		return (sys_read16(RH850_IRQ_EIC_REG(intc2_eic_reg, irq)) & RH850_EIC_EIMK) == 0U;
	} else if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		/* FEINT: enabled when the mask bit is clear */
		return (sys_read32(feintmsk_addr(core_id)) &
			BIT(irq - RH850_IRQ_INTC_VECTOR_MAX_ENTRIES)) == 0U;
	} else {
		return false;
	}
}

void intc_rh850_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	__ASSERT(irq < CONFIG_NUM_IRQS, "irq_priority_set on invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "irq_priority_set on reserved interrupt (%u)",
		 irq);
	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS, "invalid priority (%u) for interrupt %u", prio,
		 irq);

	unsigned int key = irq_lock();
	mm_reg_t eic;

	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		/* INTC1: EICn.EIP[3:0] of this PE */
		eic = intc1_eic_addr(rh850_get_core_id(), irq);
	} else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		/* INTC2: EICn.EIP[3:0] */
		eic = RH850_IRQ_EIC_REG(intc2_eic_reg, irq);
	} else {
		/* FEINT channels have no priority setting */
		irq_unlock(key);
		return;
	}

	sys_write16((sys_read16(eic) & (uint16_t)~RH850_EIC_EIP_MASK) |
			    (uint16_t)FIELD_PREP(RH850_EIC_EIP_MASK, prio),
		    eic);

	irq_unlock(key);
}
