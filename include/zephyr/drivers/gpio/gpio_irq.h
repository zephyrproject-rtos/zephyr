/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_IRQ_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_IRQ_H_

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/interrupt-controller/irq.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO IRQ utilities
 * @defgroup gpio_irq GPIO IRQ
 * @ingroup gpio_interface
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Get GPIO interrupt controller device by idx
 * @note Validates interrupt controller is also a GPIO controller
 */
#define GPIO_IRQ_DT_INTC_BY_IDX(node_id, idx)					\
	IF_ENABLED(								\
		DT_NODE_HAS_PROP(						\
			DT_IRQ_INTC_BY_IDX(node_id, idx),			\
			gpio_controller						\
		),								\
		(DEVICE_DT_GET(DT_IRQ_INTC_BY_IDX(node_id, idx)))		\
	)

/**
 * @brief Get GPIO interrupt controller device by name
 * @note Validates interrupt controller is also a GPIO controller
 */
#define GPIO_IRQ_DT_INTC_BY_NAME(node_id, name)					\
	IF_ENABLED(								\
		DT_NODE_HAS_PROP(						\
			DT_IRQ_INTC_BY_NAME(node_id, name),			\
			gpio_controller						\
		),								\
		(DEVICE_DT_GET(DT_IRQ_INTC_BY_NAME(node_id, name)))		\
	)

/** @endcond */

/**
 * @brief Get IRQ devicetree specifier by node identifier and idx
 *
 * @code{.dts}
 *     #include <zephyr/dt-bindings/interrupt-controller/irq.h>
 *
 *     foo: foo {
 *             interrupts-extended =
 *                     <&gpio0 0 IRQ_TYPE_EDGE_BOTH>,
 *                     <&gpio1 1 (IRQ_TYPE_LEVEL_HIGH | GPIO_PULL_DOWN)>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     const struct gpio_irq_dt_spec spec0 =
 *             GPIO_IRQ_DT_SPEC_GET_BY_IDX(DT_NODELABEL(foo), 0);
 *
 *     const struct gpio_irq_dt_spec spec1 =
 *             GPIO_IRQ_DT_SPEC_GET_BY_IDX(DT_NODELABEL(foo), 1);
 *
 * Becomes:
 *
 *     const struct gpio_irq_dt_spec spec0 = {
 *             .controller = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
 *             .irq_pin = 0,
 *             .irq_flags = IRQ_TYPE_EDGE_BOTH,
 *     };
 *
 *     const struct gpio_irq_dt_spec spec1 = {
 *             .controller = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
 *             .irq_pin = 1,
 *             .irq_flags = (IRQ_TYPE_LEVEL_HIGH | GPIO_PULL_DOWN),
 *     };
 */
#define GPIO_IRQ_DT_SPEC_GET_BY_IDX(node_id, idx)				\
	{									\
		.controller = GPIO_IRQ_DT_INTC_BY_IDX(node_id, idx),		\
		.irq_pin = DT_IRQ_BY_IDX(node_id, idx, irq),			\
		.irq_flags = DT_IRQ_BY_IDX(node_id, idx, flags),		\
	}

/**
 * @brief Get IRQ devicetree specifier by node identifier and idx if IRQ exists
 * @note Identical to GPIO_IRQ_DT_SPEC_GET_BY_IDX(node_id, 0) if IRQ exists
 * @see GPIO_IRQ_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX(node_id, idx)			\
	COND_CODE_1(								\
		DT_IRQ_HAS_IDX(node_id, idx),					\
		(GPIO_IRQ_DT_SPEC_GET_BY_IDX(node_id, idx)),			\
		({})								\
	)

/**
 * @brief Get IRQ devicetree specifier by node identifier and name
 *
 * @code{.dts}
 *     foo: foo {
 *             interrupt-parent = <&gpio0>
 *             interrupts = <0 IRQ_TYPE_EDGE_BOTH>,
 *                          <1 (IRQ_TYPE_LEVEL_HIGH | GPIO_PULL_DOWN)>;
 *             interrupt-names = "bar", "baz";
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     const struct gpio_irq_dt_spec spec0 =
 *             GPIO_IRQ_DT_SPEC_GET_BY_NAME(DT_NODELABEL(foo), bar);
 *
 *     const struct gpio_irq_dt_spec spec1 =
 *             GPIO_IRQ_DT_SPEC_GET_BY_NAME(DT_NODELABEL(foo), baz);
 *
 * Becomes:
 *
 *     const struct gpio_irq_dt_spec spec0 = {
 *             .controller = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
 *             .irq_pin = 0,
 *             .irq_flags = IRQ_TYPE_EDGE_BOTH,
 *     };
 *
 *     const struct gpio_irq_dt_spec spec1 = {
 *             .controller = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
 *             .irq_pin = 1,
 *             .irq_flags = (IRQ_TYPE_LEVEL_HIGH | GPIO_PULL_DOWN),
 *     };
 */
#define GPIO_IRQ_DT_SPEC_GET_BY_NAME(node_id, name)				\
	{									\
		.controller = GPIO_IRQ_DT_INTC_BY_NAME(node_id, name),		\
		.irq_pin = DT_IRQ_BY_NAME(node_id, name, irq),			\
		.irq_flags = DT_IRQ_BY_NAME(node_id, name, flags),		\
	}

/**
 * @brief Get IRQ devicetree specifier by node identifier and name if IRQ exists
 * @note Identical to GPIO_IRQ_DT_SPEC_GET_BY_NAME(node_id, 0) if IRQ exists
 * @see GPIO_IRQ_DT_SPEC_GET_BY_NAME()
 */
#define GPIO_IRQ_DT_SPEC_GET_OPT_BY_NAME(node_id, name)			\
	COND_CODE_1(								\
		DT_IRQ_HAS_NAME(node_id, name),					\
		(GPIO_IRQ_DT_SPEC_GET_BY_NAME(node_id, name)),			\
		({})								\
	)

/**
 * @brief Get the first IRQ devicetree specifier by node identifier
 * @note Identical to GPIO_IRQ_DT_SPEC_GET_BY_IDX(node_id, 0)
 * @see GPIO_IRQ_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_IRQ_DT_SPEC_GET(node_id) \
	GPIO_IRQ_DT_SPEC_GET_BY_IDX(node_id, 0)

/**
 * @brief Get first IRQ devicetree specifier by node identifier if IRQ exists
 * @note Identical to GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX(node_id, 0)
 * @see GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX()
 */
#define GPIO_IRQ_DT_SPEC_GET_OPT(node_id) \
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX(node_id, 0)

/**
 * @brief Get IRQ devicetree specifier by DT_DRV_COMPAT instance and idx
 * @see GPIO_IRQ_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_IRQ_DT_INST_SPEC_GET_BY_IDX(inst, idx) \
	GPIO_IRQ_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get IRQ devicetree specifier by DT_DRV_COMPAT instance and idx if IRQ exists
 * @see GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX()
 */
#define GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_IDX(inst, idx) \
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get IRQ devicetree specifier by DT_DRV_COMPAT instance and name
 * @see GPIO_IRQ_DT_SPEC_GET_BY_NAME()
 */
#define GPIO_IRQ_DT_INST_SPEC_GET_BY_NAME(inst, name) \
	GPIO_IRQ_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get IRQ devicetree specifier by DT_DRV_COMPAT instance and name if IRQ exists
 * @see GPIO_IRQ_DT_SPEC_GET_OPT_BY_NAME()
 */
#define GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_NAME(inst, name) \
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get the first IRQ devicetree specifier by DT_DRV_COMPAT instance
 * @see GPIO_IRQ_DT_SPEC_GET()
 */
#define GPIO_IRQ_DT_INST_SPEC_GET(inst) \
	GPIO_IRQ_DT_SPEC_GET(DT_DRV_INST(inst))

/**
 * @brief Get first IRQ devicetree specifier by DT_DRV_COMPAT instance if IRQ exists
 * @see GPIO_IRQ_DT_SPEC_GET_OPT()
 */
#define GPIO_IRQ_DT_INST_SPEC_GET_OPT(inst) \
	GPIO_IRQ_DT_SPEC_GET_OPT(DT_DRV_INST(inst))

/** @cond INTERNAL_HIDDEN */

/** GPIO IRQ devicetree specifier */
struct gpio_irq_dt_spec {
	/** GPIO port which acts as interrupt controller */
	const struct device *controller;
	/** GPIO pin which will trigger interrupt request */
	uint16_t irq_pin;
	/**
	 * IRQ and GPIO flags. The lowest 4 bits are the IRQ_TYPE_ trigger
	 * flags, the remaining 12 bits are GPIO flags.
	 */
	uint16_t irq_flags;
};

struct gpio_irq;

/** IRQ callback handler */
typedef void (*gpio_irq_callback_handler_t)(struct gpio_irq *irq);

/** GPIO IRQ structure */
struct gpio_irq {
	/** GPIO port which acts as interrupt controller */
	const struct device *controller;
	/** The nested callback structure used by the GPIO controller */
	struct gpio_callback nested_callback;
	/** The IRQ callback handler */
	gpio_irq_callback_handler_t handler;
	/** GPIO pin which will trigger IRQ */
	gpio_pin_t irq_pin;
	/** IRQ and GPIO flags */
	gpio_dt_flags_t irq_flags;
};

/** @endcond */

/**
 * @brief Request and enable GPIO IRQ
 * @param controller GPIO port which the GPIO pin belongs to
 * @param irq_pin GPIO pin which will trigger interrupt request
 * @param irq_flags IRQ_TYPE_* and GPIO_* configuration flags
 * @param irq Destination for GPIO IRQ data
 * @param handler handler which is called on interrupt request
 * @retval 0 if successful
 * @retval -errno code if failure
 * @note The irq data structure must be valid until released
 */
int gpio_irq_request(const struct device *controller,
		     gpio_pin_t irq_pin,
		     gpio_dt_flags_t irq_flags,
		     struct gpio_irq *irq,
		     gpio_irq_callback_handler_t handler);

/**
 * @brief Test if a GPIO IRQ DT spec exists
 * @param spec GPIO IRQ devicetree specifier
 * @retval true if set
 */
bool gpio_irq_dt_spec_exists(const struct gpio_irq_dt_spec *spec);

/**
 * @brief Request and enable GPIO IRQ from DT GPIO IRQ specifier
 * @param spec GPIO IRQ devicetree specifier
 * @param irq Destination for GPIO IRQ data
 * @param handler handler which is called on interrupt request
 * @retval 0 if successful
 * @retval -errno code if failure
 * @note The irq data structure must be valid until released
 */
int gpio_irq_request_dt(const struct gpio_irq_dt_spec *spec,
			struct gpio_irq *irq,
			gpio_irq_callback_handler_t handler);

/**
 * @brief Disable and release GPIO IRQ
 * @param irq GPIO IRQ to release
 * @retval 0 if successful
 * @retval -errno code if failure
 * @note The irq must have been previously requested
 * @see gpio_irq_request()
 */
int gpio_irq_release(struct gpio_irq *irq);

/**
 * @brief Enable GPIO IRQ
 * @param irq GPIO IRQ to enable
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int gpio_irq_enable(struct gpio_irq *irq);

/**
 * @brief Disable GPIO IRQ
 * @param irq GPIO IRQ to disable
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int gpio_irq_disable(struct gpio_irq *irq);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_IRQ_H_ */
