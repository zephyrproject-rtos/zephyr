/*
 * Copyright (c) 2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software-managed ISR table
 *
 * Data types for a software-managed ISR table, with a parameter per-ISR.
 */

#ifndef ZEPHYR_INCLUDE_SW_ISR_TABLE_H_
#define ZEPHYR_INCLUDE_SW_ISR_TABLE_H_

#if !defined(_ASMLANGUAGE)
#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default vector for the IRQ vector table */
extern void _isr_wrapper(void);

/* Spurious interrupt handler. Throws an error if called */
extern void z_irq_spurious(const void *unused);

/*
 * Note the order: arg first, then ISR. This allows a table entry to be
 * loaded arg -> r0, isr -> r3 in _isr_wrapper with one ldmia instruction,
 * on ARM Cortex-M (Thumb2).
 */
struct _isr_table_entry {
	const void *arg;
	void (*isr)(const void *);
};

/* The software ISR table itself, an array of these structures indexed by the
 * irq line
 */
extern struct _isr_table_entry _sw_isr_table[];

/*
 * Data structure created in a special binary .intlist section for each
 * configured interrupt. gen_irq_tables.py pulls this out of the binary and
 * uses it to create the IRQ vector table and the _sw_isr_table.
 *
 * More discussion in include/linker/intlist.ld
 */
struct _isr_list {
	/** IRQ line number */
	int32_t irq;
	/** Flags for this IRQ, see ISR_FLAG_* definitions */
	int32_t flags;
	/** ISR to call */
	void *func;
	/** Parameter for non-direct IRQs */
	const void *param;
};

/**
 * We need to somehow map the interrupt controller instance_id to its offset
 * TODO: Populate this LUT somewhere and put into somewhere
 */
#ifdef CONFIG_INTC_MULTI_INSTANCE
struct z_isr_offset_table_entry {
	const struct device *dev;
	unsigned int instance_id;
	unsigned int isr_offset;
};

extern const struct z_isr_offset_table_entry z_isr_offset_table[];
#endif /* CONFIG_INTC_MULTI_INSTANCE */

#ifdef CONFIG_SHARED_INTERRUPTS
struct z_shared_isr_client {
	void (*isr)(const void *arg);
	const void *arg;
};

struct z_shared_isr_table_entry {
	struct z_shared_isr_client clients[CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS];
	size_t client_num;
};

void z_shared_isr(const void *data);

extern struct z_shared_isr_table_entry z_shared_sw_isr_table[];
#endif /* CONFIG_SHARED_INTERRUPTS */

/**
 * @brief Helper function used to compute the index in _sw_isr_table
 * based on passed IRQ.
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return corresponding index in _sw_isr_table
 */
unsigned int z_get_sw_isr_table_idx(unsigned int irq);

/**
 * @brief Get the corresponding interrupt controller for @a instance_id
 *
 * @param instance_id Instance ID of the IRQ
 *
 * @return corresponding interrupt controller in z_isr_offset_table, NULL if not found
*/
static inline const struct device *z_get_instance_dev_from_id(unsigned int instance_id)
{
#ifdef CONFIG_INTC_MULTI_INSTANCE
	for (size_t i = 0; i < CONFIG_INTC_NUM_INSTANCE; i++) {
		if(instance_id == z_isr_offset_table[i].instance_id) {
			return z_isr_offset_table[i].dev;
		}
	}
	__ASSERT(false, "entry not found");
#else
	ARG_UNUSED(instance_id);
#endif
	return NULL;
}

/**
 * @brief Get the corresponding instance_id for interrupt controller @a dev
 *
 * @param dev Interrupt controller device
 *
 * @return corresponding instance ID in z_isr_offset_table, UINT32_MAX if not found
*/
static inline unsigned int z_get_instance_id_from_dev(const struct device *dev)
{
#ifdef CONFIG_INTC_MULTI_INSTANCE
	for (size_t i = 0; i < CONFIG_INTC_NUM_INSTANCE; i++) {
		if(dev == z_isr_offset_table[i].dev) {
			return z_isr_offset_table[i].instance_id;
		}
	}
	__ASSERT(false, "entry not found");
	return UINT32_MAX;
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

/**
 * @brief Get the corresponding offset for interrupt controller @a dev
 *
 * @param dev Interrupt controller device
 *
 * @return corresponding ISR offset in z_isr_offset_table, UINT32_MAX if not found
*/
static inline unsigned int z_get_instance_isr_offset_from_dev(const struct device *dev)
{
#ifdef CONFIG_INTC_MULTI_INSTANCE
	for (size_t i = 0; i < CONFIG_INTC_NUM_INSTANCE; i++) {
		if(dev == z_isr_offset_table[i].dev) {
			return z_isr_offset_table[i].isr_offset;
		}
	}
	__ASSERT(false, "entry not found");
	return UINT32_MAX;
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

/** This interrupt gets put directly in the vector table */
#define ISR_FLAG_DIRECT BIT(0)

#define _MK_ISR_NAME(x, y) __MK_ISR_NAME(x, y)
#define __MK_ISR_NAME(x, y) __isr_ ## x ## _irq_ ## y

/* Create an instance of struct _isr_list which gets put in the .intList
 * section. This gets consumed by gen_isr_tables.py which creates the vector
 * and/or SW ISR tables.
 */
#define Z_ISR_DECLARE(irq, flags, func, param) \
	static Z_DECL_ALIGN(struct _isr_list) Z_GENERIC_SECTION(.intList) \
		__used _MK_ISR_NAME(func, __COUNTER__) = \
			{irq, flags, (void *)&func, (const void *)param}

#define IRQ_TABLE_SIZE (CONFIG_NUM_IRQS - CONFIG_GEN_IRQ_START_VECTOR)

#ifdef CONFIG_DYNAMIC_INTERRUPTS
void z_isr_install(unsigned int irq, void (*routine)(const void *),
		   const void *param);

#ifdef CONFIG_SHARED_INTERRUPTS
int z_isr_uninstall(unsigned int irq, void (*routine)(const void *),
		    const void *param);
#endif /* CONFIG_SHARED_INTERRUPTS */
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_SW_ISR_TABLE_H_ */
