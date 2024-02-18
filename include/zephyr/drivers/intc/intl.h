/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_INTL_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_INTL_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/irq_handler.h>
#include <zephyr/sys/util.h>

/** Interrupt vector template */
typedef void (*intc_vector_t)(void);

/**
 * @brief Get number of interrupt lines of interrupt controller
 *
 * @param node_id Interrupt controller's devicetree node
 */
#define INTC_DT_NUM_INTLS(node_id) \
	_CONCAT(SYS_DT_IRQ_NUM_INTLS_, DT_DEP_ORD(node_id))

/**
 * @brief Get number of implemented interrupt lines of interrupt controller
 *
 * @param node_id Interrupt controller's devicetree node
 */
#define INTC_DT_NUM_IMPL_INTLS(node_id) \
	_CONCAT(SYS_DT_IRQ_NUM_IMPL_INTLS_, DT_DEP_ORD(node_id))

/**
 * @brief Check if interrupt controller has any implemented interrupt lines
 *
 * @param node_id Interrupt controller's devicetree node
 */
#define INTC_DT_HAS_IMPL_INTLS(node_id) \
	_CONCAT(SYS_DT_IRQ_HAS_IMPL_INTLS_, DT_DEP_ORD(node_id))

/**
 * @brief Invoke macro for each interrupt controller's interrupt line
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO)
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 */
#define INTC_DT_FOREACH_INTL(node_id, fn)							\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_,							\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn)

/**
 * @brief Invoke macro for each interrupt controller's interrupt line with
 * separator
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO, (;))
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 * @param sep Separator enclosed in brackets
 */
#define INTC_DT_FOREACH_INTL_SEP(node_id, fn, sep)						\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_SEP_,							\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn, sep)

/**
 * @brief Invoke macro for each interrupt controller's interrupt line with vargs
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln, bar, baz)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO, 0, 1)
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 * @param ... Variable number of arguments to pass to macro after node_id and intln
 */
#define INTC_DT_FOREACH_INTL_VARGS(node_id, fn, ...)						\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_VARGS_,							\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn, __VA_ARGS__)

/**
 * @brief Invoke macro for each interrupt controller's interrupt line with separator
 * and vargs
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln, bar, baz)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO, (;), 0, 1)
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 * @param sep Separator enclosed in brackets
 * @param ... Variable number of arguments to pass to macro after node_id and intln
 */
#define INTC_DT_FOREACH_INTL_SEP_VARGS(node_id, fn, sep, ...)					\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_SEP_VARGS_,						\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn, sep, __VA_ARGS__)

/**
 * @brief Invoke macro for each interrupt controller's implemented interrupt lines
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO)
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 */
#define INTC_DT_FOREACH_IMPL_INTL(node_id, fn)							\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_,							\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn)

/**
 * @brief Invoke macro for each interrupt controller's implemented interrupt lines with
 * separator
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO, (;))
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 * @param sep Separator enclosed in brackets
 */
#define INTC_DT_FOREACH_IMPL_INTL_SEP(node_id, fn, sep)						\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_SEP_,						\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn, sep)

/**
 * @brief Invoke macro for each interrupt controller's implemented interrupt lines
 * with vargs
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln, bar, baz)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO, 0, 1)
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 * @param ... Variable number of arguments to pass to macro after node_id and intln
 */
#define INTC_DT_FOREACH_IMPL_INTL_VARGS(node_id, fn, ...)					\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_VARGS_,						\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn, __VA_ARGS__)

/**
 * @brief Invoke macro for each interrupt controller's implemented interrupt lines
 * with separator and vargs
 *
 * Example usage:
 *
 *     #define FOO(node_id, intln, bar, baz)
 *
 *     INTC_DT_FOREACH_INTL(DT_NODELABEL(foo), FOO, (;), 0, 1)
 *
 * @param node_id Interrupt controller's devicetree node
 * @param fn Macro to invoke
 * @param sep Separator enclosed in brackets
 * @param ... Variable number of arguments to pass to macro after node_id and intln
 */
#define INTC_DT_FOREACH_IMPL_INTL_SEP_VARGS(node_id, fn, sep, ...)				\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_SEP_VARGS_,					\
		DT_DEP_ORD(node_id)								\
	)(node_id, fn, sep, __VA_ARGS__)

/**
 * @brief Synthesize interrupt line handler's symbol by node
 *
 * @param node_id Interrupt controller's devicetree node
 * @param intln Interrupt controller's interrupt line number
 */
#define INTC_DT_INTL_HANDLER_SYMBOL(node_id, intln) \
	_CONCAT_4(__sys_intl_handler_, DT_DEP_ORD(node_id), _, intln)

/**
 * @brief Synthesize vector table's vector's symbol
 *
 * @param node_id Interrupt controller's devicetree node
 * @param intln Interrupt controller's interrupt line number
 */
#define INTC_DT_VECTOR_SYMBOL(node_id, intln) \
	_CONCAT_4(__intc_vector_, DT_DEP_ORD(node_id), _, intln)

/**
 * @brief Device driver instance variants of INTC_DT_ macros
 * @{
 */

#define INTC_DT_INST_NUM_INTLS(inst) \
	INTC_DT_NUM_INTLS(DT_DRV_INST(inst))

#define INTC_DT_INST_NUM_IMPL_INTLS(inst) \
	INTC_DT_NUM_IMPL_INTLS(DT_DRV_INST(inst))

#define INTC_DT_INST_HAS_IMPL_INTLS(inst) \
	INTC_DT_HAS_IMPL_INTLS(DT_DRV_INST(inst))

#define INTC_DT_INST_FOREACH_INTL(inst, fn)							\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_,							\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn)

#define INTC_DT_INST_FOREACH_INTL_SEP(inst, fn, sep)						\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_SEP_,							\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn, sep)

#define INTC_DT_INST_FOREACH_INTL_VARGS(inst, fn, ...)						\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_VARGS_,							\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn, __VA_ARGS__)

#define INTC_DT_INST_FOREACH_INTL_SEP_VARGS(inst, fn, sep, ...)					\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_INTL_SEP_VARGS_,						\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn, sep, __VA_ARGS__)

#define INTC_DT_INST_FOREACH_IMPL_INTL(inst, fn)						\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_,							\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn)

#define INTC_DT_INST_FOREACH_IMPL_INTL_SEP(inst, fn, sep)					\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_SEP_,						\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn, sep)

#define INTC_DT_INST_FOREACH_IMPL_INTL_VARGS(inst, fn, ...)					\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_VARGS_,						\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn, __VA_ARGS__)

#define INTC_DT_INST_FOREACH_IMPL_INTL_SEP_VARGS(inst, fn, sep, ...)				\
	_CONCAT(										\
		SYS_DT_IRQ_FOREACH_IMPL_INTL_SEP_VARGS_,					\
		DT_INST_DEP_ORD(inst)								\
	)(inst, fn, sep, __VA_ARGS__)

#define INTC_DT_INST_INTL_HANDLER_SYMBOL(inst, intln) \
	INTC_DT_INTL_HANDLER_SYMBOL(DT_DRV_INST(inst), intln)

#define INTC_DT_INST_VECTOR_SYMBOL(inst, intln) \
	INTC_DT_VECTOR_SYMBOL(DT_DRV_INST(inst), intln)

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_INTL_H_ */
