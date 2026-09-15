/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_HOST_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_HOST_H_

/**
 * @file
 * @brief Control interface for the host managed by the BMC.
 */

#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC host control
 * @defgroup bmc_host BMC host control
 * @ingroup bmc_api
 * @{
 */

/**
 * @brief Operations used to control the managed host.
 *
 * The BMC core ships a GPIO backend (CONFIG_BMC_HOST_GPIO) that drives the
 * `power-gpio-1`, `power-gpio-2`, `reset-gpio-1` and `status-led` devicetree
 * aliases. Products whose host is controlled over some other transport, for
 * example a mailbox or a vendor sideband protocol, provide their own
 * implementation and register it instead.
 *
 * Unimplemented operations may be left NULL, in which case the corresponding
 * bmc_host_*() call returns -ENOTSUP.
 */
struct bmc_host_ops {
	/** Turn host power on (@p on true) or off. */
	int (*power_set)(bool on);
	/** Read back the current host power state. */
	int (*power_get)(bool *on);
	/** Pulse the host reset line. */
	int (*reset)(void);
	/** Drive the BMC status indicator. */
	int (*status_led_set)(bool on);
};

/**
 * @brief Register the host control backend.
 *
 * A later registration replaces an earlier one, so an application can override
 * the built-in GPIO backend by registering from a BMC_INIT_PHASE_APP component.
 * The @p ops structure must remain valid for as long as it is registered.
 *
 * @param ops Operations to install, or NULL to remove the current backend.
 *
 * @return 0 on success.
 */
int bmc_host_ops_register(const struct bmc_host_ops *ops);

/**
 * @brief Set the host power state.
 *
 * @param on true to power on, false to power off.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if no backend implements the operation.
 */
int bmc_host_power_set(bool on);

/**
 * @brief Get the host power state.
 *
 * @return true if the host is powered on. Also returns false when no backend
 *         is registered.
 */
bool bmc_host_power_get(void);

/**
 * @brief Pulse the host reset line.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if no backend implements the operation.
 */
int bmc_host_reset(void);

/**
 * @brief Drive the BMC status indicator.
 *
 * @param on true to light the indicator.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if no backend implements the operation.
 */
int bmc_host_status_led_set(bool on);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_HOST_H_ */
