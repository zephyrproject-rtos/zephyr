/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP INPUTMUX-based MUX driver.
 * Cell layout matches the HAL signature one-to-one:
 *
 *   cells[0] = "index"      -- serial number of the destination register
 *                              within its group (uint16_t).
 *   state    = "connection" -- inputmux_connection_t value carrying both
 *                              the destination register-group offset and
 *                              the source/function ID; the SDK splits it
 *                              into pmux_id and output_id internally.
 *
 */

#define DT_DRV_COMPAT nxp_inputmux

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_inputmux.h>

LOG_MODULE_REGISTER(mux_nxp_inputmux, CONFIG_MUX_LOG_LEVEL);

struct mux_nxp_inputmux_config {
	void *base;
};

static int mux_nxp_inputmux_set(const struct device *dev,
				const struct mux_control *control,
				uint32_t state)
{
	const struct mux_nxp_inputmux_config *cfg = dev->config;
	uint16_t index;

	/* control->len is fixed at 1 by the nxp,inputmux binding
	 * (#mux-control-cells = const: 1), enforced at DT validation.
	 */
	if (control->cells[0] > UINT16_MAX) {
		LOG_ERR("inputmux index 0x%x exceeds uint16_t", control->cells[0]);
		return -EINVAL;
	}

	index = (uint16_t)control->cells[0];

	INPUTMUX_Init(cfg->base);
	INPUTMUX_AttachSignal(cfg->base, index, (inputmux_connection_t)state);
	INPUTMUX_Deinit(cfg->base);

	return 0;
}

static DEVICE_API(mux_control, mux_nxp_inputmux_driver_api) = {
	.set = mux_nxp_inputmux_set,
};

#define MUX_NXP_INPUTMUX_INIT(n)                                                  \
	static const struct mux_nxp_inputmux_config                               \
		mux_nxp_inputmux_cfg_##n = {                                      \
			.base = (void *)DT_INST_REG_ADDR(n),                      \
		};                                                                \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,                                      \
			      NULL, &mux_nxp_inputmux_cfg_##n,                    \
			      POST_KERNEL, CONFIG_MUX_INIT_PRIORITY,              \
			      &mux_nxp_inputmux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_NXP_INPUTMUX_INIT)
