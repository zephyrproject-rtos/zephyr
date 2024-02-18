/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_IRQ_H_
#define ZEPHYR_INCLUDE_SYS_IRQ_H_

#include <zephyr/devicetree.h>
#include <zephyr/drivers/intc/irq_flags.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/** Include generated IRQ numbers */
#include <sys_irq_generated.h>

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Get interrupt line identifier from devicetree IRQ by index
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 */
#define SYS_DT_INTL_ID_BY_IDX(node_id, idx)					\
	_CONCAT_3(								\
		DT_DEP_ORD(DT_IRQ_INTC_BY_IDX(node_id, idx)),			\
		_,								\
		DT_IRQ_BY_IDX(node_id, idx, irq)				\
	)

/**
 * @brief Get interrupt line identifier from devicetree IRQ by name
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 */
#define SYS_DT_INTL_ID_BY_NAME(node_id, name)					\
	_CONCAT_3(								\
		DT_DEP_ORD(DT_IRQ_INTC_BY_NAME(node_id, name)),			\
		_,								\
		DT_IRQ_BY_NAME(node_id, name, irq)				\
	)

/**
 * @brief Get IRQ identifier from devicetree IRQ by index
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 */
#define SYS_DT_IRQ_ID_BY_IDX(node_id, idx)					\
	_CONCAT_5(								\
		DT_DEP_ORD(DT_IRQ_INTC_BY_IDX(node_id, idx)),			\
		_,								\
		DT_IRQ_BY_IDX(node_id, idx, irq),				\
		_,								\
		DT_DEP_ORD(node_id)						\
	)

/**
 * @brief Get IRQ identifier from devicetree IRQ by name
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 */
#define SYS_DT_IRQ_ID_BY_NAME(node_id, name)					\
	_CONCAT_5(								\
		DT_DEP_ORD(DT_IRQ_INTC_BY_NAME(node_id, name)),			\
		_,								\
		DT_IRQ_BY_NAME(node_id, name, irq),				\
		_,								\
		DT_DEP_ORD(node_id)						\
	)

/**
 * @brief Get IRQ handler's symbol by index from devicetree
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 */
#define SYS_DT_IRQ_HANDLER_SYMBOL_BY_IDX(node_id, idx)				\
	_CONCAT(								\
		__sys_irq_handler_,						\
		SYS_DT_IRQ_ID_BY_IDX(node_id, idx)				\
	)

/**
 * @brief Get IRQ handler's symbol by name from devicetree
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 */
#define SYS_DT_IRQ_HANDLER_SYMBOL_BY_NAME(node_id, name)			\
	_CONCAT(								\
		__sys_irq_handler_,						\
		SYS_DT_IRQ_ID_BY_NAME(node_id, name)				\
	)

/** @endcond */

/**
 * @brief IRQ handler return values
 * @{
 */

#define SYS_IRQ_NONE    0
#define SYS_IRQ_HANDLED 1

/** @} */

/**
 * @brief Define IRQ handler from devicetree IRQ by index
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>, <2 2>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     static int foo_irq_0_handler(const void *data)
 *     {
 *             return 1;
 *     }
 *
 *     static int foo_irq_1_handler(const void *data)
 *     {
 *             return 1;
 *     }
 *
 *     SYS_DT_DEFINE_IRQ_HANDLER_BY_IDX(DT_NODELABEL(foo),
 *                                      0,
 *                                      foo_irq_0_handler,
 *                                      NULL)
 *
 *     SYS_DT_DEFINE_IRQ_HANDLER_BY_IDX(DT_NODELABEL(foo),
 *                                      1,
 *                                      foo_irq_1_handler,
 *                                      NULL)
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 * @param handler IRQ handler of type sys_irq_handler_t
 * @param data Data passed to IRQ handler
 */
#define SYS_DT_DEFINE_IRQ_HANDLER_BY_IDX(node_id, idx, handler, data)		\
	int SYS_DT_IRQ_HANDLER_SYMBOL_BY_IDX(node_id, idx)(void)		\
	{									\
		return handler(data);						\
	}

/**
 * @brief Define IRQ handler from devicetree IRQ by name
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>, <2 2>;
 *             interrupt-names = "bar", "baz";
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     static void foo_irq_bar_handler(const void *data)
 *     {
 *             return 1;
 *     }
 *
 *     static void foo_irq_baz_handler(const void *data)
 *     {
 *             return 1;
 *     }
 *
 *     SYS_DT_DEFINE_IRQ_HANDLER_BY_NAME(DT_NODELABEL(foo),
 *                                       bar,
 *                                       foo_irq_bar_handler,
 *                                       NULL)
 *
 *     SYS_DT_DEFINE_IRQ_HANDLER_BY_NAME(DT_NODELABEL(foo),
 *                                       baz,
 *                                       foo_irq_baz_handler,
 *                                       NULL)
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 * @param handler IRQ handler of type sys_irq_handler_t
 * @param data Data passed to IRQ handler
 */
#define SYS_DT_DEFINE_IRQ_HANDLER_BY_NAME(node_id, name, handler, data)		\
	int SYS_DT_IRQ_HANDLER_SYMBOL_BY_NAME(node_id, name)(void)		\
	{									\
		return handler(data);						\
	}

/**
 * @brief Define IRQ handler from devicetree IRQ
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     static void foo_irq_handler(const void *data)
 *     {
 *             return 1;
 *     }
 *
 *     SYS_DT_DEFINE_IRQ_HANDLER_BY_IDX(DT_NODELABEL(foo),
 *                                      foo_irq_handler,
 *                                      NULL)
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param handler IRQ handler of type sys_irq_handler_t
 * @param data Data passed to IRQ handler
 */
#define SYS_DT_DEFINE_IRQ_HANDLER(node_id, handler, data) \
	SYS_DT_DEFINE_IRQ_HANDLER_BY_IDX(node_id, 0, handler, data)

/**
 * @brief Get a devicetree node's IRQ's number by index
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>, <2 2>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_IRQN_BY_IDX(DT_NODELABEL(foo), 0) // irqn of <1 1>
 *     SYS_DT_IRQN_BY_IDX(DT_NODELABEL(foo), 1) // irqn of <2 2>
 */
#define SYS_DT_IRQN_BY_IDX(node_id, idx) \
	_CONCAT(SYS_DT_IRQN_, SYS_DT_INTL_ID_BY_IDX(node_id, idx))

/**
 * @brief Get a devicetree node's IRQ's number by name
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>, <2 2>;
 *             interrupt-names = "bar", "baz";
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_IRQN_BY_NAME(DT_NODELABEL(foo), bar) // irqn of <1 1>
 *     SYS_DT_IRQN_BY_NAME(DT_NODELABEL(foo), baz) // irqn of <2 2>
 */
#define SYS_DT_IRQN_BY_NAME(node_id, name) \
	_CONCAT(SYS_DT_IRQN_, SYS_DT_INTL_ID_BY_NAME(node_id, name))

/**
 * @brief Get a devicetree node's first IRQ's number
 *
 * @param node_id Interrupt generating device's devicetree node
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_IRQN(DT_NODELABEL(foo)) // irqn of <1 1>
 */
#define SYS_DT_IRQN(node_id) \
	SYS_DT_IRQN_BY_IDX(node_id, 0)

/**
 * @brief Get a devicetree node's IRQ's flags by index
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param idx Index of IRQ into interrupt generating device's devicetree node
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1 3>, <2 2 4>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_IRQ_FLAGS_BY_IDX(DT_NODELABEL(foo), 0) // irq flags of <1 1>
 *     SYS_DT_IRQ_FLAGS_BY_IDX(DT_NODELABEL(foo), 1) // irq flags of <2 2>
 */
#define SYS_DT_IRQ_FLAGS_BY_IDX(node_id, idx) \
	INTC_DT_IRQ_FLAGS_BY_IDX(node_id, idx)

/**
 * @brief Get a devicetree node's IRQ's flags by name
 *
 * @param node_id Interrupt generating device's devicetree node
 * @param name Name of IRQ within interrupt generating device's devicetree node
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1 3>, <2 2 4>;
 *             interrupt-names = "bar", "baz";
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_IRQ_FLAGS_BY_NAME(DT_NODELABEL(foo), bar) // irq flags of <1 1>
 *     SYS_DT_IRQ_FLAGS_BY_NAME(DT_NODELABEL(foo), baz) // irq flags of <2 2>
 */
#define SYS_DT_IRQ_FLAGS_BY_NAME(node_id, name) \
	INTC_DT_IRQ_FLAGS_BY_NAME(node_id, name)

/**
 * @brief Get a devicetree node's first IRQ's number
 *
 * @param node_id Interrupt generating device's devicetree node
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupts = <1 1 3>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     SYS_DT_IRQ_FLAGS(DT_NODELABEL(foo)) // irq flags of <1 1>
 */
#define SYS_DT_IRQ_FLAGS(node_id) \
	SYS_DT_IRQ_FLAGS_BY_IDX(node_id, 0)

/**
 * @brief Device driver instance variants of SYS_DT_IRQ macros
 * @{
 */

#define SYS_DT_INST_DEFINE_IRQ_HANDLER_BY_IDX(inst, idx, handler, data) \
	SYS_DT_DEFINE_IRQ_HANDLER_BY_IDX(DT_DRV_INST(inst), idx, handler, data)

#define SYS_DT_INST_DEFINE_IRQ_HANDLER_BY_NAME(inst, name, handler, data) \
	SYS_DT_DEFINE_IRQ_HANDLER_BY_NAME(DT_DRV_INST(inst), name, handler, data)

#define SYS_DT_INST_DEFINE_IRQ_HANDLER(inst, handler, data) \
	SYS_DT_DEFINE_IRQ_HANDLER(DT_DRV_INST(inst), handler, data)

#define SYS_DT_INST_IRQN_BY_IDX(inst, idx) \
	SYS_DT_IRQN_BY_IDX(DT_DRV_INST(inst), idx)

#define SYS_DT_INST_IRQN_BY_NAME(inst, name) \
	SYS_DT_IRQN_BY_NAME(DT_DRV_INST(inst), name)

#define SYS_DT_INST_IRQN(inst) \
	SYS_DT_IRQN(DT_DRV_INST(inst))

#define SYS_DT_INST_IRQ_FLAGS_BY_IDX(inst, idx) \
	SYS_DT_IRQ_FLAGS_BY_IDX(DT_DRV_INST(inst), idx)

#define SYS_DT_INST_IRQ_FLAGS_BY_NAME(inst, name) \
	SYS_DT_IRQ_FLAGS_BY_NAME(DT_DRV_INST(inst), name)

#define SYS_DT_INST_IRQ_FLAGS(inst) \
	SYS_DT_IRQ_FLAGS(DT_DRV_INST(inst))

/** @} */

/**
 * @brief Configure IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 * @param flags The IRQ configuration flags obtained using SYS_DT_IRQ_FLAGS()
 *
 * @note This function exits with the IRQ disabled and cleared
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int sys_irq_configure(uint16_t irqn, uint32_t flags);

/**
 * @brief Enable IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 *
 * @note The IRQ is not cleared when enabled
 * @note Invoking from ISR is not permitted if shared IRQs are enabled
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int sys_irq_enable(uint16_t irqn);

/**
 * @brief Disable IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 *
 * @retval 1 if interrupt was previously enabled
 * @retval 0 if interrupt was already disabled
 * @retval -errno code if failure
 */
int sys_irq_disable(uint16_t irqn);

/**
 * @brief Trigger IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int sys_irq_trigger(uint16_t irqn);

/**
 * @brief Clear IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 *
 * @retval 1 if interrupt was previously triggered
 * @retval 0 if interrupt was already cleared
 * @retval -errno code if failure
 */
int sys_irq_clear(uint16_t irqn);

/**
 * @brief Dynamic IRQ handler template
 *
 * @param data Data provided to sys_irq_request()
 *
 * @retval SYS_IRQ_NONE IRQ did not originate from this device
 * @retval SYS_IRQ_HANDLED IRQ was handled
 */
typedef int (*sys_irq_handler_t)(const void *data);

/** Dynamic IRQ structure */
struct sys_irq {
	/** List node */
	sys_snode_t node;
	/** IRQ handler */
	sys_irq_handler_t handler;
	/** Data passed to IRQ handler */
	const void *data;
};

/**
 * @brief Request dynamic IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 * @param irq Dynamic IRQ structure
 * @param handler IRQ handler
 * @param data Data passed to IRQ handler
 *
 * @note The dynamic IRQ structure must remain valid until released
 * @note Invoking from ISR is not permitted
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int sys_irq_request(uint16_t irqn, struct sys_irq *irq, sys_irq_handler_t handler,
		    const void *data);

/**
 * @brief Release dynamic IRQ
 *
 * @param irqn System IRQ number obtained using SYS_DT_IRQN()
 * @param irq Dynamic IRQ structure
 *
 * @note The dynamic IRQ structure must have been previously requested
 * @note Invoking from ISR is not permitted
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int sys_irq_release(uint16_t irqn, struct sys_irq *irq);

#endif /* ZEPHYR_INCLUDE_SYS_IRQ_H_ */
