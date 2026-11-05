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
#include <zephyr/isr_table_metadata.h>
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

#if defined(CONFIG_GEN_SW_ISR_TABLE_ARRAY)
/* The software ISR table itself, an array of these structures indexed by the
 * irq line
 */
extern
#ifndef CONFIG_SRAM_SW_ISR_TABLE
	const
#endif /* CONFIG_SRAM_SW_ISR_TABLE */
	struct _isr_table_entry _sw_isr_table[];
#elif defined(CONFIG_GEN_SW_ISR_TABLE_SWITCH)
#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
extern void get_isr_entry(int irq_number, struct _isr_table_entry *entry);
#else
extern void __sw_isr_table get_isr_entry(int irq_number, struct _isr_table_entry *entry);
#endif
#endif /* CONFIG_GEN_SW_ISR_TABLE_ARRAY */

#if defined(CONFIG_GEN_SW_ISR_TABLE)
/**
 * @brief Copy an entry using its software ISR table index.
 *
 * @param irq_index Normalized table index after controller mapping and start-vector adjustment.
 * @param entry Storage for the copied handler and argument.
 */
static inline void z_isr_get_entry(uint32_t irq_index, struct _isr_table_entry *entry)
{
#if defined(CONFIG_GEN_SW_ISR_TABLE_ARRAY)
	*entry = _sw_isr_table[irq_index];
#else
	get_isr_entry(irq_index, entry);
#endif
}
#endif

#if defined(CONFIG_GEN_SW_ISR_TABLE_SWITCH)
/**
 * @brief Invoke a switch-selected ISR using its software table index.
 *
 * @param irq_index Normalized table index, as accepted by z_isr_get_entry().
 */
void z_isr_invoke(uint32_t irq_index);
#endif

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
 * Local declarations use z_isr_local_record followed by section-name bytes.
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

#ifdef CONFIG_SHARED_INTERRUPTS
struct z_shared_isr_table_entry {
	struct _isr_table_entry clients[CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS];
	size_t client_num;
};

void z_shared_isr(const void *data);

extern
#ifndef CONFIG_SRAM_SW_ISR_TABLE
const
#endif
struct z_shared_isr_table_entry z_shared_sw_isr_table[];
#endif /* CONFIG_SHARED_INTERRUPTS */

/** This interrupt gets put directly in the vector table */
#define ISR_FLAG_DIRECT BIT(0)

#define _MK_ISR_NAME(x, y) __MK_ISR_NAME(x, y)
#define __MK_ISR_NAME(x, y) __isr_ ## x ## _irq_ ## y


#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)

#if defined(CONFIG_SRAM_SW_ISR_TABLE)
#define Z_ISR_TABLE_STORAGE
#else
#define Z_ISR_TABLE_STORAGE const
#endif

#define _MK_ISR_ELEMENT_NAME(func, id) __MK_ISR_ELEMENT_NAME(func, id)
#define __MK_ISR_ELEMENT_NAME(func, id) __isr_table_entry_ ## func ## _irq_ ## id

#define _MK_IRQ_ELEMENT_NAME(func, id) __MK_ISR_ELEMENT_NAME(func, id)
#define __MK_IRQ_ELEMENT_NAME(func, id) __irq_table_entry_ ## func ## _irq_ ## id

/* Include the handler name alongside the translation unit's registration counter. */
#define _MK_ISR_SECTION_NAME(prefix, file, func, counter)                                          \
	"." Z_STRINGIFY(prefix) "." file "." Z_STRINGIFY(func) "." Z_STRINGIFY(counter)

#define _MK_ISR_ELEMENT_SECTION(func, counter) _MK_ISR_SECTION_NAME(irq, __FILE__, func, counter)
#define _MK_IRQ_ELEMENT_SECTION(func, counter) _MK_ISR_SECTION_NAME(isr, __FILE__, func, counter)

/* Separated macro to create ISR table entry only.
 * Used by Z_ISR_DECLARE and ISR tables generation script.
 */
#define _Z_ISR_TABLE_ENTRY(func, param, sect) \
	static Z_DECL_ALIGN(struct _isr_table_entry) Z_ISR_TABLE_STORAGE                  \
		Z_GENERIC_SECTION_LITERAL(sect)                                           \
		__used __noasan _MK_ISR_ELEMENT_NAME(func, __COUNTER__) = {              \
			.arg = (const void *)(param),                                     \
			.isr = (void (*)(const void *))(func)                             \
	}

#define Z_ISR_DECLARE_RECORD(irq_p, flags_p, func, sect, counter)                                \
	static Z_ISR_LOCAL_METADATA_STORAGE const struct {                                    \
		struct z_isr_local_record header;                                              \
		char sname[sizeof(sect)];                                                      \
	} _MK_ISR_NAME(func, counter) Z_GENERIC_SECTION(.intList)                              \
		__aligned(Z_ISR_LOCAL_RECORD_ALIGNMENT) __used __noasan = {                    \
		.header = {                                                                   \
			.record_size = sizeof(_MK_ISR_NAME(func, counter)),                    \
			.irq = (irq_p),                                                       \
			.flags = (flags_p),                                                   \
			.name_size = sizeof(sect),                                            \
		},                                                                             \
		.sname = sect,                                                                 \
	}

#define Z_ISR_DECLARE_C(irq, flags, func, param, counter) \
	_Z_ISR_DECLARE_C(irq, flags, func, param, counter)

#define _Z_ISR_DECLARE_C(irq, flags, func, param, counter)                                         \
	_Z_ISR_TABLE_ENTRY(func, param, _MK_ISR_ELEMENT_SECTION(func, counter));                   \
	Z_ISR_DECLARE_RECORD(irq, flags, func, _MK_ISR_ELEMENT_SECTION(func, counter), counter)

/* Keep the typed entry in its original compilation unit. The registration
 * record names its input section so the generator can arrange it by IRQ index.
 */
#define Z_ISR_DECLARE(irq, flags, func, param)                                            \
	BUILD_ASSERT(((flags) & ISR_FLAG_DIRECT) == 0, "Use Z_ISR_DECLARE_DIRECT macro"); \
	Z_ISR_DECLARE_C(irq, flags, func, param, __COUNTER__)


/* A static label address prevents cloning or importing the registration's
 * containing function. Otherwise ThinLTO can emit the same slot in several
 * backend objects. The owner reference is discarded at link time.
 */
#define _Z_ISR_DIRECT_TABLE_ENTRY_CODE(func, sect)                                             \
	{                                                                                   \
		__label__ z_isr_vector_owner;                                                \
		static __typeof__(&(func)) const                                             \
			_CONCAT(z_isr_vector_ref_, __COUNTER__)                               \
			Z_GENERIC_SECTION(.isr_vector_refs) __used __noasan = &(func);        \
		static const void *const z_isr_vector_owner_ref                              \
			Z_GENERIC_SECTION(.isr_vector_owners) __used __noasan =              \
				&&z_isr_vector_owner;                                       \
z_isr_vector_owner:                                                         \
		__asm__ volatile(ARCH_IRQ_VECTOR_SECTION_OPEN(sect)                          \
				 ARCH_IRQ_VECTOR_JUMP_OPERAND                                \
				 ARCH_IRQ_VECTOR_SECTION_CLOSE : : "i" (func));              \
	}

/* Address entries may be declared at file or block scope. Instruction entries
 * use a C operand so LTO preserves the target and rewrites its symbol name.
 */
#define _Z_ISR_DIRECT_TABLE_ENTRY(func, sect)                                                      \
	COND_CODE_1(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS, (                                     \
			static Z_DECL_ALIGN(uintptr_t) const                                       \
			Z_GENERIC_SECTION_LITERAL(sect)                                           \
			__used __noasan _MK_IRQ_ELEMENT_NAME(func, __COUNTER__) =             \
				((uintptr_t)(func));                                           \
		), (                                                                               \
			_Z_ISR_DIRECT_TABLE_ENTRY_CODE(func, sect)                               \
		))

/* Generated default targets are global symbols and are emitted at file scope.
 * Basic asm is not a C reference; the discarded typed root preserves C targets.
 */
#define _Z_ISR_DIRECT_TABLE_DEFAULT(func, sect)                                                \
	COND_CODE_1(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS, (                                \
		_Z_ISR_DIRECT_TABLE_ENTRY(func, sect)                                         \
	), (                                                                                \
		static __typeof__(&(func)) const                                             \
			_CONCAT(z_isr_vector_ref_, __COUNTER__)                               \
			Z_GENERIC_SECTION(.isr_vector_refs) __used __noasan = &(func);        \
		__asm__(PUSHSECTION_DIRECTIVE " \"" sect "\",\"ax\"\n\t"                  \
			ARCH_IRQ_VECTOR_JUMP_CODE(func)                                      \
			POPSECTION_DIRECTIVE "\n\t");                                      \
	))

#define Z_ISR_DECLARE_DIRECT_C(irq, flags, func, counter) \
	_Z_ISR_DECLARE_DIRECT_C(irq, flags, func, counter)

#define _Z_ISR_DECLARE_DIRECT_C(irq, flags, func, counter)                                         \
	_Z_ISR_DIRECT_TABLE_ENTRY(func, _MK_IRQ_ELEMENT_SECTION(func, counter));                   \
	Z_ISR_DECLARE_RECORD(irq, ISR_FLAG_DIRECT | (flags), func,                                 \
			     _MK_IRQ_ELEMENT_SECTION(func, counter), counter)

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
void z_isr_install(unsigned int irq, void (*routine)(const void *parameter),
		   const void *param);

#ifdef CONFIG_SHARED_INTERRUPTS
int z_isr_uninstall(unsigned int irq, void (*routine)(const void *parameter),
		    const void *param);
#endif /* CONFIG_SHARED_INTERRUPTS */
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_SW_ISR_TABLE_H_ */
