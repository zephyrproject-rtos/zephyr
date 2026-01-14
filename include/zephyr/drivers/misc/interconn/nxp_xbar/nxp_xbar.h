/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the NXP XBAR driver
 * @ingroup nxp_xbar_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_XBAR_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_XBAR_H_

/**
 * @brief Interfaces for NXP Crossbar Switch (XBAR).
 * @defgroup nxp_xbar_interface NXP XBAR
 * @ingroup misc_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief XBAR active edge for detection
 */
enum nxp_xbar_active_edge {
	/** Edge detection status bit never asserts. */
	NXP_XBAR_EDGE_NONE = 0U,
	/** Edge detection status bit asserts on rising edges. */
	NXP_XBAR_EDGE_RISING = 1U,
	/** Edge detection status bit asserts on falling edges. */
	NXP_XBAR_EDGE_FALLING = 2U,
	/** Edge detection status bit asserts on rising and falling edges. */
	NXP_XBAR_EDGE_RISING_AND_FALLING = 3U
};

/**
 * @brief Defines the XBAR DMA and interrupt configurations.
 */
enum nxp_xbar_request {
	/** Interrupt and DMA are disabled. */
	NXP_XBAR_REQUEST_DISABLE = 0U,
	/** DMA enabled, interrupt disabled. */
	NXP_XBAR_REQUEST_DMA_ENABLE = 1U,
	/** Interrupt enabled, DMA disabled. */
	NXP_XBAR_REQUEST_INTERRUPT_ENABLE = 2U
};

/**
 * @brief Defines the configuration structure of the XBAR output control.
 *
 * This structure keeps the configuration of XBAR control register for one output.
 * Control registers are available only for a few outputs. Not every XBAR module has
 * control registers.
 */
struct nxp_xbar_control_config {
	/** Active edge to be detected. */
	enum nxp_xbar_active_edge active_edge;
	/** Selects DMA/Interrupt request. */
	enum nxp_xbar_request request_type;
};

/**
 * @brief Get the device pointer from the "xbars" property by element name.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the xbars-names property.
 *
 * @return Device pointer.
 */
#define NXP_XBAR_DEVICE_GET_BY_NAME(node_id, name)                                         \
	DEVICE_DT_GET(DT_PHANDLE_BY_NAME(node_id, xbars, name))

/**
 * @brief Get the device pointer from the "xbars" property by index.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the xbars property.
 *
 * @return Device pointer.
 */
#define NXP_XBAR_DEVICE_GET_BY_IDX(node_id, idx)                                           \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, xbars, idx))

/**
 * @brief Get the output cell value from the "xbars" property by element name.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the xbars-names property.
 *
 * @return Output cell value.
 */
#define NXP_XBAR_OUTPUT_GET_BY_NAME(node_id, name)                                         \
	DT_PHA_BY_NAME(node_id, xbars, name, output)

/**
 * @brief Get the output cell value from the "xbars" property by index.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the xbars property.
 *
 * @return Output cell value.
 */
#define NXP_XBAR_OUTPUT_GET_BY_IDX(node_id, idx)                                           \
	DT_PHA_BY_IDX(node_id, xbars, idx, output)

/**
 * @brief Get the input cell value from the "xbars" property by element name.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name as specified in the xbars-names property.
 *
 * @return Input cell value.
 */
#define NXP_XBAR_INPUT_GET_BY_NAME(node_id, name)                                          \
	DT_PHA_BY_NAME(node_id, xbars, name, input)

/**
 * @brief Get the input cell value from the "xbars" property by index.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into the xbars property.
 *
 * @return Input cell value.
 */
#define NXP_XBAR_INPUT_GET_BY_IDX(node_id, idx)                                            \
	DT_PHA_BY_IDX(node_id, xbars, idx, input)

/**
 * @brief Get the XBAR device pointer from the "xbars" property by index (instance version).
 *
 * @param inst Instance number of the node with the xbars property.
 * @param idx Logical index into the xbars property.
 *
 * @return Device pointer to the XBAR device.
 */
#define NXP_XBAR_DT_INST_DEVICE_GET_BY_IDX(inst, idx)                                     \
	DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(inst, xbars, idx))

/**
 * @brief Get the output cell value from the "xbars" property by index (instance version).
 *
 * @param inst Instance number of the node with the xbars property.
 * @param idx Logical index into the xbars property.
 *
 * @return Output cell value.
 */
#define NXP_XBAR_DT_INST_OUTPUT_GET_BY_IDX(inst, idx)                                     \
	DT_INST_PHA_BY_IDX(inst, xbars, idx, output)

/**
 * @brief Get the input cell value from the "xbars" property by index (instance version).
 *
 * @param inst Instance number of the node with the xbars property.
 * @param idx Logical index into the xbars property.
 *
 * @return Input cell value.
 */
#define NXP_XBAR_DT_INST_INPUT_GET_BY_IDX(inst, idx)                                      \
	DT_INST_PHA_BY_IDX(inst, xbars, idx, input)

/**
 * @cond INTERNAL_HIDDEN
 *
 * NXP XBAR driver API definition and system call entry points
 *
 * @note @kconfig{CONFIG_NXP_XBAR_WRITE_PROTECT} must be selected for this function to be
 * available.
 *
 * (Internal use only.)
 */
__subsystem struct nxp_xbar_driver_api {
	int (*set_connection)(const struct device *dev, uint32_t output, uint32_t input);
	int (*set_output_config)(const struct device *dev, uint32_t output,
				const struct nxp_xbar_control_config *config);
	int (*get_status_flag)(const struct device *dev, uint32_t output, bool *flag);
	int (*clear_status_flag)(const struct device *dev, uint32_t output);
#if defined(CONFIG_NXP_XBAR_WRITE_PROTECT)
	int (*lock_sel_reg)(const struct device *dev, uint32_t output);
	int (*lock_ctrl_reg)(const struct device *dev, uint32_t output);
#endif
};

/**
 * @endcond
 */

/**
 * @brief Set a connection between the selected XBAR input and output signal.
 *
 * This function connects the XBAR input to the selected XBAR output.
 *
 * @param dev XBAR device.
 * @param output XBAR output signal index.
 * @param input XBAR input signal index.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int nxp_xbar_set_connection(const struct device *dev, uint32_t output, uint32_t input);

static inline int z_impl_nxp_xbar_set_connection(const struct device *dev, uint32_t output,
						 uint32_t input)
{
	return DEVICE_API_GET(nxp_xbar, dev)->set_connection(dev, output, input);
}

/**
 * @brief Configure the XBAR output signal control register.
 *
 * This function configures an XBAR output control register. The active edge detection
 * and the DMA/IRQ function on the corresponding XBAR output can be set.
 *
 * @param dev XBAR device.
 * @param output XBAR output signal index.
 * @param config Pointer to structure that keeps configuration of control register.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int nxp_xbar_set_output_config(const struct device *dev, uint32_t output,
					const struct nxp_xbar_control_config *config);

static inline int z_impl_nxp_xbar_set_output_config(const struct device *dev, uint32_t output,
						    const struct nxp_xbar_control_config *config)
{
	return DEVICE_API_GET(nxp_xbar, dev)->set_output_config(dev, output, config);
}

/**
 * @brief Get the active edge detection status.
 *
 * This function gets the active edge detect status of the specified XBAR output.
 * If the active edge occurs, the flag is asserted.
 *
 * @param dev XBAR device.
 * @param output XBAR output signal index.
 * @param flag Pointer to store the XBAR output status flag.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int nxp_xbar_get_status_flag(const struct device *dev, uint32_t output, bool *flag);

static inline int z_impl_nxp_xbar_get_status_flag(const struct device *dev, uint32_t output,
						  bool *flag)
{
	return DEVICE_API_GET(nxp_xbar, dev)->get_status_flag(dev, output, flag);
}

/**
 * @brief Clear the edge detection status flag.
 *
 * @param dev XBAR device.
 * @param output XBAR output signal index.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int nxp_xbar_clear_status_flag(const struct device *dev, uint32_t output);

static inline int z_impl_nxp_xbar_clear_status_flag(const struct device *dev, uint32_t output)
{
	return DEVICE_API_GET(nxp_xbar, dev)->clear_status_flag(dev, output);
}

#if defined(CONFIG_NXP_XBAR_WRITE_PROTECT) || defined(__DOXYGEN__)
/**
 * @brief Lock the XBAR SEL register
 *
 * When locked, the register can't be written until the XBAR module is reset.
 *
 * @note @kconfig{CONFIG_NXP_XBAR_WRITE_PROTECT} must be selected for this function to be
 * available.
 *
 * @param dev XBAR device.
 * @param output XBAR output signal index.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int nxp_xbar_lock_sel_reg(const struct device *dev, uint32_t output);

static inline int z_impl_nxp_xbar_lock_sel_reg(const struct device *dev, uint32_t output)
{
	return DEVICE_API_GET(nxp_xbar, dev)->lock_sel_reg(dev, output);
}

/**
 * @brief Lock the XBAR CTRL register
 *
 * When locked, the register can't be written until the XBAR module is reset.
 *
 * @note @kconfig{CONFIG_NXP_XBAR_WRITE_PROTECT} must be selected for this function to be
 * available.
 * @param dev XBAR device.
 * @param output XBAR output signal index.
 *
 * @return 0 if successful.
 * @return A negative errno code on failure.
 */
__syscall int nxp_xbar_lock_ctrl_reg(const struct device *dev, uint32_t output);

static inline int z_impl_nxp_xbar_lock_ctrl_reg(const struct device *dev, uint32_t output)
{
	return DEVICE_API_GET(nxp_xbar, dev)->lock_ctrl_reg(dev, output);
}
#endif /* CONFIG_NXP_XBAR_WRITE_PROTECT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/nxp_xbar.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_XBAR_H_ */
