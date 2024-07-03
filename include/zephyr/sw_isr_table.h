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
#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default vector for the IRQ vector table */
void _isr_wrapper(void);

/* Spurious interrupt handler. Throws an error if called */
void z_irq_spurious(const void *unused);

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

struct _irq_parent_entry {
	const struct device *dev;
	unsigned int level;
	unsigned int irq;
	unsigned int offset;
};

/**
 * @cond INTERNAL_HIDDEN
 */

/* Mapping between aggregator level to order */
#define Z_STR_L2 2ND
#define Z_STR_L3 3RD
/**
 * @brief Get the Software ISR table offset Kconfig for the given aggregator level
 *
 * @param l Aggregator level, must be 2 or 3
 *
 * @return `CONFIG_2ND_LVL_ISR_TBL_OFFSET` if second level aggregator,
 * `CONFIG_3RD_LVL_ISR_TBL_OFFSET` if third level aggregator
 */
#define Z_SW_ISR_TBL_KCONFIG_BY_ALVL(l) CONCAT(CONFIG_, CONCAT(Z_STR_L, l), _LVL_ISR_TBL_OFFSET)

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Get an interrupt controller node's level base ISR table offset.
 *
 * @param node_id node identifier of the interrupt controller
 *
 * @return `CONFIG_2ND_LVL_ISR_TBL_OFFSET` if node_id is a second level aggregator,
 * `CONFIG_3RD_LVL_ISR_TBL_OFFSET` if it is a third level aggregator
 */
#define INTC_BASE_ISR_TBL_OFFSET(node_id)                                                          \
	Z_SW_ISR_TBL_KCONFIG_BY_ALVL(DT_INTC_GET_AGGREGATOR_LEVEL(node_id))

/**
 * @brief Get the SW ISR table offset for an instance of interrupt controller
 *
 * @param inst DT_DRV_COMPAT interrupt controller driver instance number
 *
 * @return Software ISR table offset of the interrupt controller
 */
#define INTC_INST_ISR_TBL_OFFSET(inst)                                                             \
	(INTC_BASE_ISR_TBL_OFFSET(DT_DRV_INST(inst)) + (inst * CONFIG_MAX_IRQ_PER_AGGREGATOR))

/**
 * @brief Get the SW ISR table offset for a child interrupt controller
 *
 * @details This macro is a alternative form of the `INTC_INST_ISR_TBL_OFFSET`. This is used by
 * pseudo interrupt controller devices that are child of a main interrupt controller device.
 *
 * @param node_id node identifier of the child interrupt controller
 *
 * @return Software ISR table offset of the child
 */
#define INTC_CHILD_ISR_TBL_OFFSET(node_id)                                                         \
	(INTC_BASE_ISR_TBL_OFFSET(node_id) +                                                       \
	 (DT_NODE_CHILD_IDX(node_id) * CONFIG_MAX_IRQ_PER_AGGREGATOR))

/**
 * @brief Register an  interrupt controller with the software ISR table
 *
 * @param _name Name of the interrupt controller (must be unique)
 * @param _dev Pointer to the interrupt controller device instance
 * @param _irq Interrupt controller IRQ number
 * @param _offset Software ISR table offset of the interrupt controller
 * @param _level Interrupt controller aggregator level
 */
#define IRQ_PARENT_ENTRY_DEFINE(_name, _dev, _irq, _offset, _level)                                \
	static const STRUCT_SECTION_ITERABLE_ALTERNATE(intc_table, _irq_parent_entry, _name) = {   \
		.dev = _dev,                                                                       \
		.level = _level,                                                                   \
		.irq = _irq,                                                                       \
		.offset = _offset,                                                                 \
	}

/*
 * Data structure created in a special binary .intlist section for each
 * configured interrupt. gen_irq_tables.py pulls this out of the binary and
 * uses it to create the IRQ vector table and the _sw_isr_table.
 *
 * More discussion in include/linker/intlist.ld
 *
 * This is a version used when CONFIG_ISR_TABLES_LOCAL_DECLARATION is disabled.
 * See _isr_list_sname used otherwise.
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

/*
 * Data structure created in a special binary .intlist section for each
 * configured interrupt. gen_isr_tables.py pulls this out of the binary and
 * uses it to create linker script chunks that would place interrupt table entries
 * in the right place in the memory.
 *
 * This is a version used when CONFIG_ISR_TABLES_LOCAL_DECLARATION is enabled.
 * See _isr_list used otherwise.
 */
struct _isr_list_sname {
	/** IRQ line number */
	int32_t irq;
	/** Flags for this IRQ, see ISR_FLAG_* definitions */
	int32_t flags;
	/** The section name */
	const char sname[];
};

#ifdef CONFIG_SHARED_INTERRUPTS
struct z_shared_isr_table_entry {
	struct _isr_table_entry clients[CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS];
	size_t client_num;
};

void z_shared_isr(const void *data);

extern struct z_shared_isr_table_entry z_shared_sw_isr_table[];
#endif /* CONFIG_SHARED_INTERRUPTS */

/** This interrupt gets put directly in the vector table */
#define ISR_FLAG_DIRECT BIT(0)

#define _MK_ISR_NAME(x, y) __MK_ISR_NAME(x, y)
#define __MK_ISR_NAME(x, y) __isr_ ## x ## _irq_ ## y


#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)

#define _MK_ISR_ELEMENT_NAME(func, id) __MK_ISR_ELEMENT_NAME(func, id)
#define __MK_ISR_ELEMENT_NAME(func, id) __isr_table_entry_ ## func ## _irq_ ## id

#define _MK_IRQ_ELEMENT_NAME(func, id) __MK_ISR_ELEMENT_NAME(func, id)
#define __MK_IRQ_ELEMENT_NAME(func, id) __irq_table_entry_ ## func ## _irq_ ## id

#define _MK_ISR_SECTION_NAME(prefix, file, counter) \
	"." Z_STRINGIFY(prefix)"."file"." Z_STRINGIFY(counter)

#define _MK_ISR_ELEMENT_SECTION(counter) _MK_ISR_SECTION_NAME(irq, __FILE__, counter)
#define _MK_IRQ_ELEMENT_SECTION(counter) _MK_ISR_SECTION_NAME(isr, __FILE__, counter)

/* Separated macro to create ISR table entry only.
 * Used by Z_ISR_DECLARE and ISR tables generation script.
 */
#define _Z_ISR_TABLE_ENTRY(irq, func, param, sect) \
	static Z_DECL_ALIGN(struct _isr_table_entry)                                      \
		__attribute__((section(sect)))                                            \
		__used _MK_ISR_ELEMENT_NAME(func, __COUNTER__) = {                        \
			.arg = (const void *)(param),                                     \
			.isr = (void (*)(const void *))(void *)(func)                     \
	}

#define Z_ISR_DECLARE_C(irq, flags, func, param, counter) \
	_Z_ISR_DECLARE_C(irq, flags, func, param, counter)

#define _Z_ISR_DECLARE_C(irq, flags, func, param, counter)                                \
	_Z_ISR_TABLE_ENTRY(irq, func, param, _MK_ISR_ELEMENT_SECTION(counter));           \
	static struct _isr_list_sname Z_GENERIC_SECTION(.intList)                         \
		__used _MK_ISR_NAME(func, counter) =                                      \
		{irq, flags, _MK_ISR_ELEMENT_SECTION(counter)}

/* Create an entry for _isr_table to be then placed by the linker.
 * An instance of struct _isr_list which gets put in the .intList
 * section is created with the name of the section where _isr_table entry is placed to be then
 * used by isr generation script to create linker script chunk.
 */
#define Z_ISR_DECLARE(irq, flags, func, param)                                            \
	BUILD_ASSERT(((flags) & ISR_FLAG_DIRECT) == 0, "Use Z_ISR_DECLARE_DIRECT macro"); \
	Z_ISR_DECLARE_C(irq, flags, func, param, __COUNTER__)


/* Separated macro to create ISR Direct table entry only.
 * Used by Z_ISR_DECLARE_DIRECT and ISR tables generation script.
 */
#define _Z_ISR_DIRECT_TABLE_ENTRY(irq, func, sect)                                                 \
	COND_CODE_1(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS, (                                     \
			static Z_DECL_ALIGN(uintptr_t)                                             \
			__attribute__((section(sect)))                                             \
			__used _MK_IRQ_ELEMENT_NAME(func, __COUNTER__) = ((uintptr_t)(func));      \
		), (                                                                               \
			static void __attribute__((section(sect))) __attribute__((naked))          \
			__used _MK_IRQ_ELEMENT_NAME(func, __COUNTER__)(void) {                     \
				__asm(ARCH_IRQ_VECTOR_JUMP_CODE(func));                            \
			}                                                                          \
		))

#define Z_ISR_DECLARE_DIRECT_C(irq, flags, func, counter) \
	_Z_ISR_DECLARE_DIRECT_C(irq, flags, func, counter)

#define _Z_ISR_DECLARE_DIRECT_C(irq, flags, func, counter)                                         \
	_Z_ISR_DIRECT_TABLE_ENTRY(irq, func, _MK_IRQ_ELEMENT_SECTION(counter));                    \
	static struct _isr_list_sname Z_GENERIC_SECTION(.intList)                                  \
		__used _MK_ISR_NAME(func, counter) = {                                             \
			irq,                                                                       \
			ISR_FLAG_DIRECT | (flags),                                                 \
			_MK_IRQ_ELEMENT_SECTION(counter)}

/* Create an entry to irq table and place it in specific section which name is then placed
 * in an instance of struct _isr_list to be then used by the isr generation script to create
 * the linker script chunks.
 */
#define Z_ISR_DECLARE_DIRECT(irq, flags, func)                                                     \
	BUILD_ASSERT(IS_ENABLED(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS) ||                        \
		IS_ENABLED(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE),                                  \
		"CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_{ADDRESS,CODE} not set");                         \
	Z_ISR_DECLARE_DIRECT_C(irq, flags, func, __COUNTER__)


#else /* defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION) */

/* Create an instance of struct _isr_list which gets put in the .intList
 * section. This gets consumed by gen_isr_tables.py which creates the vector
 * and/or SW ISR tables.
 */
#define Z_ISR_DECLARE(irq, flags, func, param) \
	static Z_DECL_ALIGN(struct _isr_list) Z_GENERIC_SECTION(.intList) \
		__used _MK_ISR_NAME(func, __COUNTER__) = \
			{irq, flags, (void *)&func, (const void *)param}

/* The version of the Z_ISR_DECLARE that should be used for direct ISR declaration.
 * It is here for the API match the version with CONFIG_ISR_TABLES_LOCAL_DECLARATION enabled.
 */
#define Z_ISR_DECLARE_DIRECT(irq, flags, func) \
	Z_ISR_DECLARE(irq, ISR_FLAG_DIRECT | (flags), func, NULL)

#endif

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
