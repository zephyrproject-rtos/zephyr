/*
 * SPDX-FileCopyrightText: 2026 NXP
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

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
#include <zephyr/drivers/clock_control.h>
#endif

#include <fsl_trgmux.h>

LOG_MODULE_REGISTER(mux_nxp_trgmux, CONFIG_MUX_LOG_LEVEL);

struct mux_nxp_trgmux_config {
	TRGMUX_Type *base;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#endif
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

static int mux_nxp_trgmux_get_state(const struct device *dev,
				    const struct mux_control *control,
				    uint32_t *state)
{
	const struct mux_nxp_trgmux_config *cfg = dev->config;

	/* control->len is fixed at 2 by the nxp,trgmux binding.
	 * cells[1] is the trigger-input slot, i.e. the bit shift of the SELn
	 * field inside TRGCFG[device]; SEL0_MASK gives the field width
	 * (all SELn fields share the same width).
	 */
	*state = (cfg->base->TRGCFG[control->cells[0]] >> control->cells[1]) &
		 TRGMUX_TRGCFG_SEL0_MASK;

	return 0;
}

static DEVICE_API(mux_control, mux_nxp_trgmux_driver_api) = {
	.set = mux_nxp_trgmux_set,
	.get_state = mux_nxp_trgmux_get_state,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
/* Some SoCs (e.g. MCXE31x) gate the TRGMUX register interface behind a
 * peripheral clock, and touching TRGCFG while it is gated raises a bus
 * fault. Where TRGMUX is ungated the node carries no clocks property and
 * clock_dev stays NULL, so this init is a no-op.
 */
static int mux_nxp_trgmux_init(const struct device *dev)
{
	const struct mux_nxp_trgmux_config *cfg = dev->config;

	if (cfg->clock_dev == NULL) {
		return 0;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->clock_dev);
		return -ENODEV;
	}

	return clock_control_on(cfg->clock_dev, cfg->clock_subsys);
}

#define MUX_NXP_TRGMUX_INIT_FN mux_nxp_trgmux_init

#define MUX_NXP_TRGMUX_CLOCK_CFG(n)                                            \
	.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),             \
		(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL)),              \
	.clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),          \
		((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name)), (0)),
#else
#define MUX_NXP_TRGMUX_INIT_FN     NULL
#define MUX_NXP_TRGMUX_CLOCK_CFG(n)
#endif

#define MUX_NXP_TRGMUX_INIT(n)                                       \
	static const struct mux_nxp_trgmux_config                    \
		mux_nxp_trgmux_cfg_##n = {                           \
			.base = (TRGMUX_Type *)DT_INST_REG_ADDR(n),  \
			MUX_NXP_TRGMUX_CLOCK_CFG(n)                  \
		};                                                   \
	DEVICE_DT_INST_DEFINE(n, MUX_NXP_TRGMUX_INIT_FN, NULL,       \
			      NULL, &mux_nxp_trgmux_cfg_##n,         \
			      POST_KERNEL, CONFIG_MUX_INIT_PRIORITY, \
			      &mux_nxp_trgmux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_NXP_TRGMUX_INIT)
