/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the Renesas ELC driver
 * @ingroup renesas_elc_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_RENESAS_ELC_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_RENESAS_ELC_H_

/**
 * @brief Interfaces for Renesas Event Link Controller (ELC).
 * @defgroup renesas_elc_interface Renesas ELC
 * @ingroup misc_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Container for Renesas ELC information specified in devicetree.
 *
 * This type contains a pointer to a Renesas ELC device, along with the
 * peripheral ID and event ID used to configure a link between peripherals
 * via the Event Link Controller.
 *
 * This structure is typically initialized using devicetree macros that parse
 * phandle-array properties referencing ELC instances.
 */
struct renesas_elc_dt_spec {
	/** Renesas ELC device instance. */
	const struct device *dev;
	/** Renesas ELC peripheral ID. */
	uint32_t peripheral;
	/** Renesas ELC event ID. */
	uint32_t event;
};

/**
 * @brief Get the device pointer from the "renesas-elcs" property by element name.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Device pointer.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_GET_BY_NAME(node_id, name)                                      \
	DEVICE_DT_GET(DT_PHANDLE_BY_NAME(node_id, renesas_elcs, name))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by index.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Device pointer.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_GET_BY_IDX(node_id, idx)                                        \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, renesas_elcs, idx))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by element name,
 *        or return NULL if the property does not exist.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Device pointer or NULL.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_GET_BY_NAME_OR_NULL(node_id, name)                              \
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_NAME(node_id, renesas_elcs, name))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by index,
 *        or return NULL if the property does not exist.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Device pointer or NULL.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_GET_BY_IDX_OR_NULL(node_id, idx)                                \
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_IDX(node_id, renesas_elcs, idx))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by element name for a
 * DT_DRV_COMPAT instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Device pointer.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_INST_GET_BY_NAME(inst, name)                                    \
	DEVICE_DT_GET(DT_PHANDLE_BY_NAME(DT_DRV_INST(inst), renesas_elcs, name))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by index for a DT_DRV_COMPAT
 * instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Device pointer.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_INST_GET_BY_IDX(inst, idx)                                      \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), renesas_elcs, idx))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by element name
 *        for a DT_DRV_COMPAT instance, or return NULL if the property does not exist.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Device pointer or NULL.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_INST_GET_BY_NAME_OR_NULL(inst, name)                            \
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_NAME(DT_DRV_INST(inst), renesas_elcs, name))

/**
 * @brief Get the device pointer from the "renesas-elcs" property by index
 *        for a DT_DRV_COMPAT instance, or return NULL if the property does not exist.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Device pointer or NULL.
 */
#define RENESAS_ELC_DT_SPEC_DEVICE_INST_GET_BY_IDX_OR_NULL(inst, idx)                              \
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), renesas_elcs, idx))

/**
 * @brief Get the peripheral cell value from the "renesas-elcs" property by element name.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Peripheral cell value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_NAME(node_id, name)                                  \
	DT_PHA_BY_NAME(node_id, renesas_elcs, name, peripheral)

/**
 * @brief Get the peripheral cell value from the "renesas-elcs" property by index.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Peripheral cell value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_IDX(node_id, idx)                                    \
	DT_PHA_BY_IDX(node_id, renesas_elcs, idx, peripheral)

/**
 * @brief Get the peripheral cell value from the "renesas-elcs" property by element name,
 *        or return a default value if the property does not exist.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Peripheral cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_NAME_OR(node_id, name, default_value)                \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, renesas_elcs),                                       \
		(RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_NAME(node_id, name)),                       \
		(default_value))

/**
 * @brief Get the peripheral cell value from the "renesas-elcs" property by index,
 *        or return a default value if the property does not exist.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the renesas-elcs property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Peripheral cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_IDX_OR(node_id, idx, default_value)                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, renesas_elcs),                                       \
		(RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_IDX(node_id, idx)),                         \
		(default_value))

/**
 * @brief Get the peripheral cell value by element name for a DT_DRV_COMPAT instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Peripheral cell value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_INST_GET_BY_NAME(inst, name)                                \
	RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get the peripheral cell value by index for a DT_DRV_COMPAT instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Peripheral cell value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_INST_GET_BY_IDX(inst, idx)                                  \
	RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get the peripheral cell value by element name for a DT_DRV_COMPAT instance,
 *        or return a default value if the property does not exist.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Peripheral cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_INST_GET_BY_NAME_OR(inst, name, default_value)              \
	RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Get the peripheral cell value by index for a DT_DRV_COMPAT instance,
 *        or return a default value if the property does not exist.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into the renesas-elcs property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Peripheral cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_PERIPHERAL_INST_GET_BY_IDX_OR(inst, idx, default_value)                \
	RENESAS_ELC_DT_SPEC_PERIPHERAL_GET_BY_IDX_OR(DT_DRV_INST(inst), idx, default_value)

/**
 * @brief Get the event cell value from the "renesas-elcs" property by element name.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Event cell value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_GET_BY_NAME(node_id, name)                                       \
	DT_PHA_BY_NAME(node_id, renesas_elcs, name, event)

/**
 * @brief Get the event cell value from the "renesas-elcs" property by index.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Event cell value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_GET_BY_IDX(node_id, idx)                                         \
	DT_PHA_BY_IDX(node_id, renesas_elcs, idx, event)

/**
 * @brief Get the event cell value from the "renesas-elcs" property by element name,
 *        or return a default value if the property does not exist.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Event cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_GET_BY_NAME_OR(node_id, name, default_value)                     \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, renesas_elcs),                                       \
		(RENESAS_ELC_DT_SPEC_EVENT_GET_BY_NAME(node_id, name)),                            \
		(default_value))

/**
 * @brief Get the event cell value from the "renesas-elcs" property by index,
 *        or return a default value if the property does not exist.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the renesas-elcs property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Event cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_GET_BY_IDX_OR(node_id, idx, default_value)                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, renesas_elcs),                                       \
		(RENESAS_ELC_DT_SPEC_EVENT_GET_BY_IDX(node_id, idx)),                              \
		(default_value))

/**
 * @brief Get the event cell value by element name for a DT_DRV_COMPAT instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 *
 * @return Event cell value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_INST_GET_BY_NAME(inst, name)                                     \
	RENESAS_ELC_DT_SPEC_EVENT_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get the event cell value by index for a DT_DRV_COMPAT instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into the renesas-elcs property.
 *
 * @return Event cell value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_INST_GET_BY_IDX(inst, idx)                                       \
	RENESAS_ELC_DT_SPEC_EVENT_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get the event cell value by element name for a DT_DRV_COMPAT instance,
 *        or return a default value if the property does not exist.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name as specified in the renesas-elcs-names property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Event cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_INST_GET_BY_NAME_OR(inst, name, default_value)                   \
	RENESAS_ELC_DT_SPEC_EVENT_GET_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Get the event cell value by index for a DT_DRV_COMPAT instance,
 *        or return a default value if the property does not exist.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into the renesas-elcs property.
 * @param default_value Value to return if the property is not present.
 *
 * @return Event cell value or default_value.
 */
#define RENESAS_ELC_DT_SPEC_EVENT_INST_GET_BY_IDX_OR(inst, idx, default_value)                     \
	RENESAS_ELC_DT_SPEC_EVENT_GET_BY_IDX_OR(DT_DRV_INST(inst), idx, default_value)

/**
 * @cond INTERNAL_HIDDEN
 *
 * Renesas ELC driver API definition and system call entry points
 *
 * (Internal use only.)
 */
__subsystem struct renesas_elc_driver_api {
	int (*software_event_generate)(const struct device *dev, uint32_t event);
	int (*link_set)(const struct device *dev, uint32_t peripheral, uint32_t signal);
	int (*link_break)(const struct device *dev, uint32_t peripheral);
	int (*enable)(const struct device *dev);
	int (*disable)(const struct device *dev);
};

/**
 * @endcond
 */

/**
 * @brief Generate a software event in the Event Link Controller.
 *
 * This function requests the Renesas ELC to generate a software event
 * identified by @p event.
 *
 * @param dev The Event Link Controller device.
 * @param event Software event ID to generate.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_elc_software_event_generate(const struct device *dev, uint32_t event);

static inline int z_impl_renesas_elc_software_event_generate(const struct device *dev,
							     uint32_t event)
{
	return DEVICE_API_GET(renesas_elc, dev)->software_event_generate(dev, event);
}

/**
 * @brief Create a single event link.
 *
 * This function configures an event link by associating a peripheral with
 * a specific event signal.
 *
 * @param dev Event Link Controller device.
 * @param peripheral Peripheral ID to be linked to the event signal.
 * @param event Event signal ID to be associated with the peripheral.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_elc_link_set(const struct device *dev, uint32_t peripheral, uint32_t event);

static inline int z_impl_renesas_elc_link_set(const struct device *dev, uint32_t peripheral,
					      uint32_t event)
{
	return DEVICE_API_GET(renesas_elc, dev)->link_set(dev, peripheral, event);
}

/**
 * @brief Break an event link.
 *
 * This function breaks an existing event link for the given peripheral.
 *
 * @param dev Event Link Controller device.
 * @param peripheral Peripheral ID whose link is to be broken.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_elc_link_break(const struct device *dev, uint32_t peripheral);

static inline int z_impl_renesas_elc_link_break(const struct device *dev, uint32_t peripheral)
{
	return DEVICE_API_GET(renesas_elc, dev)->link_break(dev, peripheral);
}

/**
 * @brief Enable the operation of the Event Link Controller.
 *
 * This function enables the ELC so that it can process events.
 *
 * @param dev Event Link Controller device.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_elc_enable(const struct device *dev);

static inline int z_impl_renesas_elc_enable(const struct device *dev)
{
	return DEVICE_API_GET(renesas_elc, dev)->enable(dev);
}

/**
 * @brief Disable the operation of the Event Link Controller.
 *
 * This function disables the ELC, stopping event processing.
 *
 * @param dev Event Link Controller device.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int renesas_elc_disable(const struct device *dev);

static inline int z_impl_renesas_elc_disable(const struct device *dev)
{
	return DEVICE_API_GET(renesas_elc, dev)->disable(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/renesas_elc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_RENESAS_ELC_H_ */
