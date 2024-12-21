/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_ppc

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb_c/usbc_ppc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ppc_numaker, CONFIG_USBC_LOG_LEVEL);

#include <soc.h>
#include <NuMicro.h>

#include "../tcpc/ucpd_numaker.h"

/* Implementation notes on NuMaker TCPC/PPC/VBUS
 *
 * PPC and VBUS rely on TCPC/UTCPD and are just pseudo. They are completely
 * implemented in TCPC/UTCPD.
 */

/**
 * @brief Immutable device context
 */
struct numaker_ppc_config {
	const struct device *tcpc_dev;
};

/**
 * @brief Initializes the usb-c ppc driver
 *
 * @retval 0 on success
 * @retval -ENODEV if dependent TCPC device is not ready
 */
static int numaker_ppc_init(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	/* Rely on TCPC */
	if (!device_is_ready(tcpc_dev)) {
		LOG_ERR("TCPC device not ready");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Check if PPC is in the dead battery mode
 *
 * @retval 1 if PPC is in the dead battery mode
 * @retval 0 if PPC is not in the dead battery mode
 * @retval -EIO if on failure
 */
static int numaker_ppc_is_dead_battery_mode(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_is_dead_battery_mode(tcpc_dev);
}

/**
 * @brief Request the PPC to exit from the dead battery mode
 *
 * @retval 0 if request was successfully sent
 * @retval -EIO if on failure
 */
static int numaker_ppc_exit_dead_battery_mode(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_exit_dead_battery_mode(tcpc_dev);
}

/**
 * @brief Check if the PPC is sourcing the VBUS
 *
 * @retval 1 if the PPC is sourcing the VBUS
 * @retval 0 if the PPC is not sourcing the VBUS
 * @retval -EIO on failure
 */
static int numaker_ppc_is_vbus_source(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_is_vbus_source(tcpc_dev);
}

/**
 * @brief Check if the PPC is sinking the VBUS
 *
 * @retval 1 if the PPC is sinking the VBUS
 * @retval 0 if the PPC is not sinking the VBUS
 * @retval -EIO on failure
 */
static int numaker_ppc_is_vbus_sink(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_is_vbus_sink(tcpc_dev);
}

/**
 * @brief Set the state of VBUS sinking
 *
 * @retval 0 if success
 * @retval -EIO on failure
 */
static int numaker_ppc_set_snk_ctrl(const struct device *dev, bool enable)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_set_snk_ctrl(tcpc_dev, enable);
}

/**
 * @brief Set the state of VBUS sourcing
 *
 * @retval 0 if success
 * @retval -EIO on failure
 */
static int numaker_ppc_set_src_ctrl(const struct device *dev, bool enable)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_set_src_ctrl(tcpc_dev, enable);
}

/**
 * @brief Set the state of VBUS discharging
 *
 * @retval 0 if success
 * @retval -EIO on failure
 */
static int numaker_ppc_set_vbus_discharge(const struct device *dev, bool enable)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_set_vbus_discharge(tcpc_dev, enable);
}

/**
 * @brief Check if VBUS is present
 *
 * @retval 1 if VBUS voltage is present
 * @retval 0 if no VBUS voltage is detected
 * @retval -EIO on failure
 */
static int numaker_ppc_is_vbus_present(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_is_vbus_present(tcpc_dev);
}

/**
 * @brief Set the callback used to notify about PPC events
 *
 * @retval 0 if success
 */
static int numaker_ppc_set_event_handler(const struct device *dev, usbc_ppc_event_cb_t handler,
					 void *data)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_set_event_handler(tcpc_dev, handler, data);
}

/**
 * @brief Print the values or PPC registers
 *
 * @retval 0 if success
 * @retval -EIO on failure
 */
static int numaker_ppc_dump_regs(const struct device *dev)
{
	const struct numaker_ppc_config *const config = dev->config;
	const struct device *tcpc_dev = config->tcpc_dev;

	return numaker_tcpc_ppc_dump_regs(tcpc_dev);
}

static const struct usbc_ppc_driver_api numaker_ppc_driver_api = {
	.is_dead_battery_mode = numaker_ppc_is_dead_battery_mode,
	.exit_dead_battery_mode = numaker_ppc_exit_dead_battery_mode,
	.is_vbus_source = numaker_ppc_is_vbus_source,
	.is_vbus_sink = numaker_ppc_is_vbus_sink,
	.set_snk_ctrl = numaker_ppc_set_snk_ctrl,
	.set_src_ctrl = numaker_ppc_set_src_ctrl,
	.set_vbus_discharge = numaker_ppc_set_vbus_discharge,
	.is_vbus_present = numaker_ppc_is_vbus_present,
	.set_event_handler = numaker_ppc_set_event_handler,
	.dump_regs = numaker_ppc_dump_regs,
};

#define NUMAKER_TCPC(inst) DT_INST_PARENT(inst)

#define PPC_NUMAKER_INIT(inst)                                                                     \
	static const struct numaker_ppc_config numaker_ppc_config_##inst = {                       \
		.tcpc_dev = DEVICE_DT_GET(NUMAKER_TCPC(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, numaker_ppc_init, NULL, NULL, &numaker_ppc_config_##inst,      \
			      POST_KERNEL, CONFIG_USBC_PPC_INIT_PRIORITY,                          \
			      &numaker_ppc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PPC_NUMAKER_INIT);
