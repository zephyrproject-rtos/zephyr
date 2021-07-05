/* shared_irq - Shared interrupt driver */

/*
 * Copyright (c) 2015 Intel corporation
 * Copyright (c) 2021 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHARED_IRQ_H_
#define ZEPHYR_INCLUDE_SHARED_IRQ_H_

#include <autoconf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get a node identifier for a shared-irqs specifier at the index
 *
 * @param node_id Node identifier of a device that has a shared-irqs
 *                phandle-array property.
 * @param irq_idx logical index into the shared-irqs phandle-array
 * @return node identifier for the shared-irq node at that index
 */
#define SHARED_IRQ_BY_IDX(node_id, irq_idx)                                    \
	DT_PHANDLE_BY_IDX(node_id, shared_irqs, irq_idx);                      \

/**
 * @brief Get a node identifier within a shared-irqs specifier by name
 *
 * This can be used to get an individual shared-irq when a device generates more
 * than one, provided that the bindings give each shared-irq specifier a name.
 *
 * @param node_id Node identifier of a device that has a shared-irqs
 *                phandle-array property.
 * @param irq_name lowercase-and-underscores shared interrupt specifier name
 * @return node identifier for the shared-irq identified by given by the name
 */
#define SHARED_IRQ_BY_NAME(node_id, irq_name)                                  \
	DT_PHANDLE_BY_NAME(node_id, shared_irqs, irq_name)                     \

/**
 * @brief Does the node have a shared-irqs element at index?
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array element at index.
 * @param irq_idx logical index into the shared-irqs phandle-array
 * @return 1 if the shared-irqs property has an element at index, 0 otherwise
 */
#define SHARED_IRQ_HAS_IDX(node_id, irq_idx)                                   \
	DT_PROP_HAS_IDX(node_id, shared_irqs, irq_idx)                         \

/**
 * @brief Does the node have a shared-irqs element with a matching name?
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array element with the given name.
 * @param irq_name lowercase-and-underscores name of a shared-irqs element
 *             as defined by the node's shared-irq-names property
 * @return 1 if the shared-irqs property has the named element, 0 otherwise
 */
#define SHARED_IRQ_HAS_NAME(node_id, irq_name)                                 \
	IS_ENABLED(DT_CAT(node_id, _P_shared_irqs_NAME_##irq_name##_EXISTS))

/**
 * @brief Does the node have an shared-irqs element at index with status okay?
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array element at the given index.
 * @param irq_idx logical index into the shared-irqs phandle-array
 * @return 1 if the shared-irqs element at index has status okay,
 *         0 otherwise
 */
#define SHARED_IRQ_BY_IDX_HAS_STATUS_OKAY(node_id, irq_idx)                    \
	COND_CODE_1(SHARED_IRQ_HAS_IDX(node_id, irq_idx),                      \
		   (COND_CODE_1(DT_NODE_HAS_STATUS(                            \
				  SHARED_IRQ_BY_IDX(node_id, irq_idx), okay),  \
				(1),                                           \
				(0)),                                          \
		    (0)))

/**
 * @brief Does the node have an shared-irqs element with a matching name and
 *        status okay?
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array element with the given name.
 * @param irq_name lowercase-and-underscores name of a shared-irqs element
 *             as defined by the node's shared-irq-names property
 * @return 1 if the shared-irqs property has the named element and status okay,
 *         0 otherwise
 */
#define SHARED_IRQ_BY_NAME_HAS_STATUS_OKAY(node_id, irq_name)                  \
	COND_CODE_1(SHARED_IRQ_HAS_NAME(node_id, irq_name),                    \
		   (COND_CODE_1(DT_NODE_HAS_STATUS(                            \
				  SHARED_IRQ_BY_NAME(node_id, irq_name), okay),\
				(1),                                           \
				(0))),                                         \
		    (0))

/**
 * @brief Connect to a devicetree nodes shared-irq at index.
 *
 * The IRQ must be subsequently enabled before the interrupt handler
 * begins servicing interrupts.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device that has a shared-irqs
 *                phandle-array property.
 * @param irq_idx logical index into the shared-irqs phandle-array
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine. Should be a
 *                    pointer to the device that will service the interrupt.
 */
#define SHARED_IRQ_CONNECT_BY_IDX(node_id, irq_idx, isr_p, isr_param_p)        \
	__ASSERT(device_is_ready(                                              \
			DEVICE_DT_GET(SHARED_IRQ_BY_IDX(node_id, irq_idx))),   \
		 "shared irq ##irq_idx## not ready");                          \
	shared_irq_isr_register(                                               \
			DEVICE_DT_GET(SHARED_IRQ_BY_IDX(node_id, irq_idx)),    \
				(isr_t)isr_p, isr_param_p);                    \

/**
 * @brief Connect to a devicetree nodes shared-irq by name.
 *
 * This routine connects to a shared-irqs element, provided the given name
 * finds a match.
 *
 * The IRQ must be subsequently enabled before the interrupt handler
 * begins servicing interrupts.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device that has a shared-irqs
 *                phandle-array property.
 * @param irq_name lowercase-and-underscores shared-irq specifier name
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine. Should be a
 *                    pointer to the device that will service the interrupt.
 */
#define SHARED_IRQ_CONNECT_BY_NAME(node_id, irq_name, isr_p, isr_param_p)      \
	__ASSERT(device_is_ready(                                              \
			DEVICE_DT_GET(SHARED_IRQ_BY_NAME(node_id, irq_name))), \
		 "shared-irq ##irq_name## not ready");                         \
	shared_irq_isr_register(                                               \
			DEVICE_DT_GET(SHARED_IRQ_BY_NAME(node_id, irq_name)),  \
				(isr_t)isr_p, isr_param_p)                     \

/**
 * @brief Connect to a devicetree nodes shared-irqs element at idx if it
 * exists, else connect to the IRQ in the interrupts array at the same idx.
 *
 * This routine registers a device isr with the shared-irq matching the given
 * index if a shared-irq with index idx exists.
 * Otherwise, it initializes an interrupt handler for the irq in the interrupts
 * array at the same idx.
 * The IRQ must be subsequently enabled before the interrupt handler
 * begins servicing interrupts.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device that has either a shared-irqs
 *                phandle-array or a interrupts property.
 * @param irq_idx logical index into the shared-irqs phandle-array if it exists,
 *                index into the interrupts array otherwise.
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine. Should be a
 *                    pointer to the device that will service the interrupt.
 * @param flags_p Architecture-specific IRQ configuration flags (not used for
 *        shared-irqs).
 *
 * @return N/A
 */
#define SHARED_IRQ_CONNECT_IRQ_BY_IDX_COND(node_id, irq_idx, isr_p,            \
					   isr_param_p, flags_p)               \
	COND_CODE_1(SHARED_IRQ_BY_IDX_HAS_STATUS_OKAY(node_id, irq_idx),       \
		    (SHARED_IRQ_CONNECT_BY_IDX(node_id, irq_idx, isr_p,        \
					       isr_param_p)),                  \
		    (IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, irq_idx, irq),         \
				 DT_IRQ_BY_IDX(node_id, irq_idx, priority),    \
				 isr_p, isr_param_p, flags_p)))

/**
 * @brief Connect to a devicetree nodes (only) shared-irqs element if it
 * exists, else connect to the single irq in the interrupts array.
 *
 * The IRQ must be subsequently enabled before the interrupt handler
 * begins servicing interrupts.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device that has either a shared-irqs
 *                phandle-array or a interrupts array.
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine. Should be a
 *                    pointer to the device that will service the interrupt.
 * @param flags_p Architecture-specific IRQ configuration flags (not used for
 *        shared-irqs).
 *
 * @return N/A
 */
#define SHARED_IRQ_CONNECT_IRQ_COND(node_id, isr_p, isr_param_p, flags_p)      \
	SHARED_IRQ_CONNECT_IRQ_BY_IDX_COND(node_id, 0, isr_p, isr_param_p,     \
					   flags_p)                            \

/**
 * @brief Connect to a devicetree nodes shared-irqs element by name if it
 * exists, else connect to a IRQ using the same given name.
 *
 * This routine registers a device isr with the shared-irq matching the given
 * name if such a shared-irq exists.
 * Otherwise, it initializes an interrupt handler for an IRQ with the given name.
 * The IRQ must be subsequently enabled before the interrupt handler
 * begins servicing interrupts.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device that has either a shared-irqs
 *                phandle-array or a interrupts property.
 * @param irq_name lowercase-and-underscores device shared-irq-name if it
 *                 exists, interrupt-name otherwise.
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine. Should be a
 *                    pointer to the device that will service the interrupt.
 * @param flags_p Architecture-specific IRQ configuration flags (not used for
 *        shared-irqs).
 *
 * @return N/A
 */

#define SHARED_IRQ_CONNECT_IRQ_BY_NAME_COND(node_id, irq_name, isr_p,          \
					    isr_param_p, flags_p)              \
	COND_CODE_1(SHARED_IRQ_BY_NAME_HAS_STATUS_OKAY(node_id, irq_name),     \
		    (SHARED_IRQ_CONNECT_BY_NAME(node_id, irq_name,             \
						isr_p, isr_param_p)),          \
		    (IRQ_CONNECT(DT_IRQ_BY_NAME(node_id, irq_name, irq),       \
				 DT_IRQ_BY_NAME(node_id, irq_name, priority),  \
				 isr_p, isr_param_p, flags_p)))

/**
 * @brief Enable ISR for a devicetree node's shared interrupt at index if it
 *         exists, or enable interrupts from irq at index otherwise.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array property.
 * @param irq_idx logical index into the shared-irqs phandle-array if it exists,
 *                index into the interrupts array otherwise.
 *
 * @return N/A
 */
#define SHARED_IRQ_ENABLE_BY_IDX_COND(node_id, irq_idx)                        \
	COND_CODE_1(SHARED_IRQ_BY_IDX_HAS_STATUS_OKAY(node_id, irq_idx),       \
		    (shared_irq_enable(                                        \
			DEVICE_DT_GET(SHARED_IRQ_BY_IDX(node_id, irq_idx)),    \
			DEVICE_DT_GET(node_id))),                              \
		    (irq_enable(DT_IRQ_BY_IDX(node_id, irq_idx, irq))))        \

/**
 * @brief Enable ISR for a devicetree node's (only) shared-irqs interrupt if it
 *        exists, or enable interrupts from the single IRQ otherwise.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array property.
 *
 * @return N/A
 */
#define SHARED_IRQ_ENABLE_COND(node_id)                                        \
	COND_CODE_1(SHARED_IRQ_BY_IDX_HAS_STATUS_OKAY(node_id, 0),             \
		    (shared_irq_enable(                                        \
			DEVICE_DT_GET(SHARED_IRQ_BY_IDX(node_id, 0)),          \
			DEVICE_DT_GET(node_id))),                              \
		    (irq_enable(DT_IRQ(node_id, irq))))                        \

/**
 * @brief Enable ISR for a devicetree node's shared interrupt by name if it
 *         exists, or enable interrupts from irq by name otherwise.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array property.
 * @param irq_name lowercase-and-underscores device shared-irqs name if it
 *                 exists, irq name otherwise.
 *
 * @return N/A
 */
#define SHARED_IRQ_ENABLE_BY_NAME_COND(node_id, irq_name)                      \
	COND_CODE_1(SHARED_IRQ_BY_NAME_HAS_STATUS_OKAY(node_id, irq_name),     \
		    (shared_irq_enable(                                        \
			DEVICE_DT_GET(SHARED_IRQ_BY_NAME(node_id, irq_name)),  \
			DEVICE_DT_GET(node_id))),                              \
		    (irq_enable(DT_IRQ_BY_NAME(node_id, irq_name, irq))))      \

/**
 * @brief Disable ISR for a devicetree node's shared interrupt at index if it
 *         exists, or disable interrupts from irq at index otherwise.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array property.
 * @param irq_idx logical index into the shared-irqs phandle-array if it exists,
 *                index into the interrupts array otherwise.
 *
 * @return N/A
 */
#define SHARED_IRQ_DISABLE_BY_IDX_COND(node_id, irq_idx)                       \
	COND_CODE_1(SHARED_IRQ_BY_IDX_HAS_STATUS_OKAY(node_id, irq_idx),       \
		    (shared_irq_disable(                                       \
			DEVICE_DT_GET(SHARED_IRQ_BY_IDX(node_id, irq_idx)),    \
			DEVICE_DT_GET(node_id))),                              \
		    (irq_disable(DT_IRQ_BY_IDX(node_id, irq_idx, irq))))       \

/**
 * @brief Disable ISR for a devicetree node's (only) shared-irqs interrupt if it
 *        exists, or disable interrupts from the single IRQ otherwise.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array property.
 *
 * @return N/A
 */
#define SHARED_IRQ_DISABLE_COND(node_id)                                       \
	COND_CODE_1(SHARED_IRQ_BY_IDX_HAS_STATUS_OKAY(node_id, 0),             \
		    (shared_irq_disable(                                       \
			DEVICE_DT_GET(SHARED_IRQ_BY_IDX(node_id, 0)),          \
			DEVICE_DT_GET(node_id))),                              \
		    (irq_disable(DT_IRQ(node_id, irq))))                       \

/**
 * @brief Disable ISR for a devicetree node's shared interrupt by name if it
 *         exists, or disable interrupts from irq by name otherwise.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param node_id Node identifier of a device which may have a shared-irqs
 *                phandle-array property.
 * @param irq_name lowercase-and-underscores device shared-irqs name if it
 *                 exists, irq name otherwise.
 *
 * @return N/A
 */
#define SHARED_IRQ_DISABLE_BY_NAME_COND(node_id, irq_name)                     \
	COND_CODE_1(SHARED_IRQ_BY_NAME_HAS_STATUS_OKAY(node_id, irq_name),     \
		    (shared_irq_disable(                                       \
			DEVICE_DT_GET(SHARED_IRQ_BY_NAME(node_id, irq_name)),  \
			DEVICE_DT_GET(node_id))),                              \
		    (irq_disable(DT_IRQ_BY_NAME(node_id, irq_name, irq))))     \

typedef int (*isr_t)(const struct device *dev);

/* driver API definition */
typedef int (*shared_irq_register_t)(const struct device *dev,
				     isr_t isr_func,
				     const struct device *isr_dev);
typedef int (*shared_irq_enable_t)(const struct device *dev,
				   const struct device *isr_dev);
typedef int (*shared_irq_disable_t)(const struct device *dev,
				    const struct device *isr_dev);

struct shared_irq_driver_api {
	shared_irq_register_t isr_register;
	shared_irq_enable_t enable;
	shared_irq_disable_t disable;
};

/**
 *  @brief Register a device ISR
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_func Pointer to the ISR function for the device.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_isr_register(const struct device *dev,
					  isr_t isr_func,
					  const struct device *isr_dev)
{
	const struct shared_irq_driver_api *api =
		(const struct shared_irq_driver_api *)dev->api;

	return api->isr_register(dev, isr_func, isr_dev);
}

/**
 *  @brief Enable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_enable(const struct device *dev,
				    const struct device *isr_dev)
{
	const struct shared_irq_driver_api *api =
		(const struct shared_irq_driver_api *)dev->api;

	return api->enable(dev, isr_dev);
}

/**
 *  @brief Disable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_disable(const struct device *dev,
				     const struct device *isr_dev)
{
	const struct shared_irq_driver_api *api =
		(const struct shared_irq_driver_api *)dev->api;

	return api->disable(dev, isr_dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHARED_IRQ_H_ */
