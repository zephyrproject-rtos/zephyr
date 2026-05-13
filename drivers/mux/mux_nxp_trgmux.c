/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP TRGMUX-based MUX driver.
 * Cell layout matches the HAL signature one-to-one:
 *
 *   cells[0] = TRGMUX device index (trgmux_device_t value)
 *   cells[1] = trigger input slot (trgmux_trigger_input_t value, the bit
 *              shift position used by the SDK; 0..3 *8 on most SoCs)
 *   state    = trigger source (trgmux_source_t)
 */

#define DT_DRV_COMPAT nxp_trgmux

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_trgmux.h>

LOG_MODULE_REGISTER(mux_nxp_trgmux, CONFIG_MUX_LOG_LEVEL);

struct mux_nxp_trgmux_config {
	TRGMUX_Type *base;
};

static int mux_nxp_trgmux_set(const struct device *dev,
			      const struct mux_control *control,
			      uint32_t state)
{
	const struct mux_nxp_trgmux_config *cfg = dev->config;
	status_t status;

	/* control->len is fixed at 2 by the nxp,trgmux binding
	 * (#mux-control-cells = const: 2), enforced at DT validation.
	 */
	status = TRGMUX_SetTriggerSource(cfg->base,
					 control->cells[0],
					 (trgmux_trigger_input_t)control->cells[1],
					 state);
	if (status == kStatus_TRGMUX_Locked) {
		return -EACCES;
	}
	if (status != kStatus_Success) {
		return -EIO;
	}

	return 0;
}

static int mux_nxp_trgmux_lock(const struct device *dev,
			       const struct mux_control *control,
			       bool lock)
{
	const struct mux_nxp_trgmux_config *cfg = dev->config;

	if (!lock) {
		/* TRGMUX register lock is one-way until reset. */
		return -ENOTSUP;
	}

	TRGMUX_LockRegister(cfg->base, control->cells[0]);
	return 0;
}

static DEVICE_API(mux_control, mux_nxp_trgmux_driver_api) = {
	.set = mux_nxp_trgmux_set,
	.lock = mux_nxp_trgmux_lock,
};

#define MUX_NXP_TRGMUX_INIT(n)                                       \
	static const struct mux_nxp_trgmux_config                    \
		mux_nxp_trgmux_cfg_##n = {                           \
			.base = (TRGMUX_Type *)DT_INST_REG_ADDR(n),  \
		};                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,                         \
			      NULL, &mux_nxp_trgmux_cfg_##n,         \
			      POST_KERNEL, CONFIG_MUX_INIT_PRIORITY, \
			      &mux_nxp_trgmux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_NXP_TRGMUX_INIT)
