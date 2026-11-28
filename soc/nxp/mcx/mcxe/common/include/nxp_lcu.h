/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP Logic Control Unit (LCU) device interface.
 *
 * SoC-private interface for the LCU instances of the MCXE family. It is not
 * part of the public Zephyr API and carries no API stability guarantee; only
 * SoC-internal consumers are expected to use it.
 */

#ifndef NXP_LCU_H_
#define NXP_LCU_H_

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup nxp_lcu_interface NXP Logic Control Unit
 * @{
 *
 * Supervisor-only interface. Each logic cell has one owner, identified by a
 * non-NULL stable pointer. Cell-local masks use bits 0..3. Configuration leaves
 * outputs disabled. Force, synchronized overrides and runtime PM are not supported.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Number of inputs and outputs per logic cell. */
#define NXP_LCU_CELL_IO_COUNT 4U
/** Select constant zero as an input source. */
#define NXP_LCU_SOURCE_ZERO 0U
/** Select an instance input, indexed 0..11. */
#define NXP_LCU_SOURCE_INPUT(n) (1U + (n))
/** Select registered output feedback, indexed 0..11 within the instance. */
#define NXP_LCU_SOURCE_OUTPUT(n) (13U + (n))

/** Configuration of one output. Filter thresholds are LCU clock ticks; zero bypasses. */
struct nxp_lcu_output_config {
	uint16_t lut;         /**< LUT bit indexed by I3:I2:I1:I0. */
	uint16_t rise_filter; /**< Consecutive high samples before asserting. */
	uint16_t fall_filter; /**< Consecutive low samples before deasserting. */
	bool inverted;       /**< Invert the pin after output gating. */
};

/** Configuration of a whole cell. */
struct nxp_lcu_cell_config {
	uint8_t sources[NXP_LCU_CELL_IO_COUNT]; /**< NXP_LCU_SOURCE_* selectors. */
	struct nxp_lcu_output_config outputs[NXP_LCU_CELL_IO_COUNT]; /**< Output functions. */
	uint8_t software_inputs; /**< Inputs replaced by software, initially zero. */
};

/** Cell-local state, before final pin polarity where applicable. */
struct nxp_lcu_state {
	uint8_t inputs;        /**< Selected hardware inputs. */
	uint8_t overridden;    /**< Inputs presented to the LUT. */
	uint8_t outputs;       /**< Filtered LUT results, before output gating. */
	uint8_t enabled;       /**< Output enable bits, not pin levels. */
};

/** LCU driver operations, dispatched through the bound device instance. */
struct nxp_lcu_driver_api {
	int (*request_cell)(const struct device *dev, uint8_t cell, const void *owner);
	int (*release)(const struct device *dev, uint8_t cell, const void *owner);
	int (*configure)(const struct device *dev, uint8_t cell, const void *owner,
			 const struct nxp_lcu_cell_config *config);
	int (*set_inputs)(const struct device *dev, uint8_t cell, const void *owner,
			  uint8_t values);
	int (*set_outputs)(const struct device *dev, uint8_t cell, const void *owner,
			   uint8_t enabled);
	int (*get_state)(const struct device *dev, uint8_t cell, const void *owner,
			 struct nxp_lcu_state *state);
	int (*get_rate)(const struct device *dev, uint32_t *rate);
};

/*
 * The API struct lives outside include/zephyr, so DEVICE_API_GET() cannot be
 * used here: its DEVICE_API_IS() type check needs the linker section that the
 * build only generates for headers under include/. Dispatch stays identical.
 */
static inline const struct nxp_lcu_driver_api *nxp_lcu_get_api(const struct device *dev)
{
	return (const struct nxp_lcu_driver_api *)dev->api;
}

/**
 * @brief Reserve a whole logic cell.
 * @param dev LCU controller.
 * @param cell Cell index.
 * @param owner Stable non-NULL owner identifier.
 * @retval 0 Cell reserved.
 * @retval -EINVAL Invalid cell or owner.
 * @retval -EBUSY Cell already reserved.
 * @retval -EACCES Configuration write-protected.
 */
static inline int nxp_lcu_request_cell(const struct device *dev, uint8_t cell, const void *owner)
{
	return nxp_lcu_get_api(dev)->request_cell(dev, cell, owner);
}

/**
 * @brief Disable and release a cell.
 * @param dev LCU controller.
 * @param cell Cell index.
 * @param owner Identifier used to reserve the cell.
 * @retval 0 Cell disabled and released.
 * @retval -EINVAL Invalid cell or owner.
 * @retval -EPERM Caller does not own the cell.
 * @retval -EACCES Configuration write-protected; ownership retained.
 */
static inline int nxp_lcu_release(const struct device *dev, uint8_t cell, const void *owner)
{
	return nxp_lcu_get_api(dev)->release(dev, cell, owner);
}

/**
 * @brief Configure a reserved, disabled cell.
 * @param dev LCU controller.
 * @param cell Cell index.
 * @param owner Identifier used to reserve the cell.
 * @param config Complete cell configuration.
 * @retval 0 Configuration applied with outputs disabled.
 * @retval -EINVAL Invalid arguments or source selector.
 * @retval -EPERM Caller does not own the cell.
 * @retval -EBUSY Cell outputs are enabled.
 * @retval -EACCES Configuration write-protected.
 */
static inline int nxp_lcu_configure(const struct device *dev, uint8_t cell, const void *owner,
				    const struct nxp_lcu_cell_config *config)
{
	return nxp_lcu_get_api(dev)->configure(dev, cell, owner, config);
}

/**
 * @brief Update immediate software inputs.
 *
 * Only inputs selected by software_inputs are changed. Multi-bit transitions
 * are not guaranteed glitch-free.
 *
 * @param dev LCU controller.
 * @param cell Cell index.
 * @param owner Identifier used to reserve the cell.
 * @param values Cell-local input values, bits 0..3.
 * @retval 0 Values written.
 * @retval -EINVAL Invalid arguments or cell not configured.
 * @retval -EPERM Caller does not own the cell.
 */
static inline int nxp_lcu_set_inputs(const struct device *dev, uint8_t cell, const void *owner,
				     uint8_t values)
{
	return nxp_lcu_get_api(dev)->set_inputs(dev, cell, owner, values);
}

/**
 * @brief Set all four output enables in one register update.
 *
 * Disabled outputs drive the inactive level selected by output polarity, not
 * necessarily logic low. Callable from interrupt context.
 *
 * @param dev LCU controller.
 * @param cell Cell index.
 * @param owner Identifier used to reserve the cell.
 * @param enabled Cell-local output enable mask, bits 0..3.
 * @retval 0 Enables updated.
 * @retval -EINVAL Invalid arguments or cell not configured.
 * @retval -EPERM Caller does not own the cell.
 * @retval -EACCES Configuration write-protected.
 */
static inline int nxp_lcu_set_outputs(const struct device *dev, uint8_t cell, const void *owner,
				      uint8_t enabled)
{
	return nxp_lcu_get_api(dev)->set_outputs(dev, cell, owner, enabled);
}

/**
 * @brief Read cell-local state.
 * @param dev LCU controller.
 * @param cell Cell index.
 * @param owner Identifier used to reserve the cell.
 * @param state Destination.
 * @retval 0 State read.
 * @retval -EINVAL Invalid arguments.
 * @retval -EPERM Caller does not own the cell.
 */
static inline int nxp_lcu_get_state(const struct device *dev, uint8_t cell, const void *owner,
				    struct nxp_lcu_state *state)
{
	return nxp_lcu_get_api(dev)->get_state(dev, cell, owner, state);
}

/**
 * @brief Read the LCU functional clock frequency.
 * @param dev LCU controller.
 * @param rate Destination frequency in Hz.
 * @retval 0 Frequency read.
 * @retval -EINVAL Destination is NULL.
 * @return Negative errno from the clock controller on failure.
 */
static inline int nxp_lcu_get_rate(const struct device *dev, uint32_t *rate)
{
	return nxp_lcu_get_api(dev)->get_rate(dev, rate);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NXP_LCU_H_ */
