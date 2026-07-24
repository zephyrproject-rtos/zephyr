/* Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Architecture notes:
 * - AON manages root references shared by HP and ULP domains (HP_REF, ULP_REF,
 *   UULP_HF_REF, UULP_LF_REF, SYSRTC).
 * - Device tree routes (`silabs,siwx91x-clock-mux`) are applied at init so
 *   peripherals start with deterministic parent clocks.
 * - This driver only routes and gates AON-visible roots; rate programming for
 *   derived clocks is delegated to HP/ULP managers.
 */

#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs_siwx91x.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "clock_control_silabs_siwx91x_common.h"

#include "si91x_device.h"

#define DT_DRV_COMPAT silabs_siwx91x_cmu_aon

LOG_MODULE_REGISTER(siwx91x_aon_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct siwx91x_aon_clock_config {
	MCU_FSM_Type *fsm_reg;
	MCU_AON_Type *aon_reg;
	struct silabs_siwx91x_clock_control_config *aon_clk_mux;
	size_t aon_clk_mux_count;
};

static const struct {
	uint32_t clkid;
	uint32_t ref_clkid;
	uint32_t reg_value;
} clk_reg_map[] = {
	/*clockid             ref_clkid             reg_value                                 */
	/*   |                    |                        |                                  */
	{ SIWX91X_CLK_REF_HP,     0,                       0 },
	{ SIWX91X_CLK_REF_HP,     SIWX91X_CLK_RC_MHZ,      1 }, /* ULP_MHZ_RC_BYP_CLK         */
	{ SIWX91X_CLK_REF_HP,     SIWX91X_CLK_RC_MHZ,      2 }, /* ULP_MHZ_RC_CLK             */
	{ SIWX91X_CLK_REF_HP,     SIWX91X_CLK_XTAL_MHZ,    3 }, /* EXT_40MHZ_CLK              */
	{ SIWX91X_CLK_REF_ULP,    0,                       0 },
	{ SIWX91X_CLK_REF_ULP,    SIWX91X_CLK_RC_MHZ,      1 }, /* ULP_REF_ULP_MHZ_RC_BYP_CLK */
	{ SIWX91X_CLK_REF_ULP,    SIWX91X_CLK_RC_MHZ,      2 }, /* ULP_REF_ULP_MHZ_RC_CLK     */
	{ SIWX91X_CLK_REF_ULP,    SIWX91X_CLK_XTAL_MHZ,    3 }, /* ULPSS_40MHZ_CLK            */
	{ SIWX91X_CLK_AON_REF_HF, 0,                       0 }, /* FSM_NO_CLOCK               */
	{ SIWX91X_CLK_AON_REF_HF, SIWX91X_CLK_RC_MHZ,      2 }, /* FSM_MHZ_RC                 */
	{ SIWX91X_CLK_AON_REF_LF, SIWX91X_CLK_RC_KHZ,      2 }, /* KHZ_RC_CLK_SEL             */
	{ SIWX91X_CLK_AON_REF_LF, SIWX91X_CLK_XTAL_KHZ,    4 }, /* KHZ_XTAL_CLK_SEL           */
	{ SIWX91X_CLK_SYSRTC,     SIWX91X_CLK_RC_KHZ,      4 }, /* KHZ_RC_CLK_SEL             */
	{ SIWX91X_CLK_SYSRTC,     SIWX91X_CLK_XTAL_KHZ,    8 }, /* KHZ_XTAL_CLK_SEL           */
};

static uint32_t siwx91x_aon_clock_get_reg(uint32_t clockid, uint32_t ref_clkid)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_reg_map); i++) {
		if (clockid == clk_reg_map[i].clkid && ref_clkid == clk_reg_map[i].ref_clkid) {
			return clk_reg_map[i].reg_value;
		}
	}

	return UINT32_MAX;
}

static uint32_t siwx91x_aon_clock_get_ref_clock(uint32_t clockid, uint32_t reg_value)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_reg_map); i++) {
		if (clockid == clk_reg_map[i].clkid && reg_value == clk_reg_map[i].reg_value) {
			return clk_reg_map[i].ref_clkid;
		}
	}
	return UINT32_MAX;
}

static bool siwx91x_aon_clk_valid(uint32_t clockid, uint32_t ref_clkid)
{
	return siwx91x_aon_clock_get_reg(clockid, ref_clkid) != UINT32_MAX;
}

static int siwx91x_aon_clk_config(const struct device *dev,
			   struct silabs_siwx91x_clock_control_config *mux)
{
	const struct siwx91x_aon_clock_config *cfg = dev->config;
	uint32_t reg_value;
	int ret = 0;

	reg_value = siwx91x_aon_clock_get_reg(mux->clkid, mux->ref_clkid);

	if (mux->ref_clkid == SIWX91X_CLK_XTAL_MHZ) {
		siwx91x_clock_request_xtal_to_nwp();
	}

	switch (mux->clkid) {
	case SIWX91X_CLK_REF_HP:
		cfg->fsm_reg->MCU_FSM_REF_CLK_REG_b.M4SS_REF_CLK_SEL = reg_value;

		/* In Wiseconnect, waiting for ulp ref clock change */
		/* M4CLK in Hardcoded because we cannot call HP clock driver since
		 * it is not initialized yet.
		 */
		ret = WAIT_FOR(M4CLK->PLL_STAT_REG_b.M4_SOC_CLK_SWITCHED, 1000, NULL);

		break;
	case SIWX91X_CLK_REF_ULP:
		cfg->fsm_reg->MCU_FSM_REF_CLK_REG_b.ULPSS_REF_CLK_SEL_b = reg_value;

		/* Not done in Wiseconnect */
		/* M4CLK in Hardcoded because we cannot call HP clock driver since
		 * it is not initialized yet.
		 */
		ret = WAIT_FOR(M4CLK->PLL_STAT_REG_b.ULP_REF_CLK_SWITCHED, 1000, NULL);

		break;
	case SIWX91X_CLK_AON_REF_HF:
		cfg->fsm_reg->MCU_FSM_CLKS_REG_b.HF_FSM_CLK_SELECT = reg_value;

		ret = WAIT_FOR(cfg->fsm_reg->MCU_FSM_CLKS_REG_b.HF_FSM_CLK_SWITCHED_SYNC, 1000,
			       NULL);

		break;
	case SIWX91X_CLK_AON_REF_LF:
		cfg->aon_reg->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b.AON_KHZ_CLK_SEL = reg_value;

		ret = WAIT_FOR(cfg->aon_reg->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b
			       .AON_KHZ_CLK_SEL_CLOCK_SWITCHED, 1000, NULL);

		break;
	case SIWX91X_CLK_SYSRTC:
		/* SYSRTC is an exception and is completely managed by sleeptimer
		 * TODO: Introduce this code when sleeptimer is fully integrated with zephyr
		 * for now, we just break.
		 *
		 * cfg->aon_reg->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b.AON_KHZ_CLK_SEL_SYSRTC =
		 * reg_value; while (cfg->aon_reg->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b
		 *       .AON_KHZ_CLK_SEL_CLOCK_SWITCHED_SYSRTC != 1) {
		 * }
		 */
		ret = 1;
		break;
	default:
		return -EINVAL;
	}

	if(!ret) {
		return -EIO;
	}

	return 0;
}

static int siwx91x_aon_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct siwx91x_aon_clock_config *cfg = dev->config;
	uint32_t clockid = POINTER_TO_UINT(sys);

	switch (clockid) {
	case SIWX91X_CLK_XTAL_MHZ:
		siwx91x_clock_request_xtal_to_nwp();
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_40MHZ_CLK_EN_b = 1;
		break;
	case SIWX91X_CLK_XTAL_KHZ:
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_32KHZ_XTAL_CLK_EN_b = 1;
		break;
	case SIWX91X_CLK_RC_MHZ:
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_MHZ_RC_CLK_EN_b = 1;
		break;
	case SIWX91X_CLK_RC_KHZ:
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_32KHZ_RC_CLK_EN_b = 1;
		break;
	case SIWX91X_CLK_REF_HP:
	case SIWX91X_CLK_REF_ULP:
	case SIWX91X_CLK_AON_REF_HF:
	case SIWX91X_CLK_AON_REF_LF:
	case SIWX91X_CLK_SYSRTC:
	case SIWX91X_CLK_WATCHDOG:
	case SIWX91X_CLK_RTC:
		return -EALREADY;
	default:
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_aon_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct siwx91x_aon_clock_config *cfg = dev->config;
	uint32_t clockid = POINTER_TO_UINT(sys);

	ARG_UNUSED(dev);


	switch (clockid) {
	case SIWX91X_CLK_XTAL_MHZ:
		siwx91x_clock_release_xtal_to_nwp();
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_40MHZ_CLK_EN_b = 0;
		break;
	case SIWX91X_CLK_XTAL_KHZ:
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_32KHZ_XTAL_CLK_EN_b = 0;
		break;
	case SIWX91X_CLK_RC_MHZ:
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_MHZ_RC_CLK_EN_b = 0;
		break;
	case SIWX91X_CLK_RC_KHZ:
		cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_32KHZ_RC_CLK_EN_b = 0;
		break;
	case SIWX91X_CLK_REF_HP:
	case SIWX91X_CLK_REF_ULP:
	case SIWX91X_CLK_AON_REF_HF:
	case SIWX91X_CLK_AON_REF_LF:
	case SIWX91X_CLK_SYSRTC: /* SYSRTC is an exception: managed by sleeptimer */
	case SIWX91X_CLK_WATCHDOG:
	case SIWX91X_CLK_RTC:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_aon_clock_get_rate(const struct device *dev, clock_control_subsys_t clk_cfg,
				      uint32_t *rate)
{
	const struct siwx91x_aon_clock_config *cfg = dev->config;
	uint32_t clockid = POINTER_TO_UINT(clk_cfg);
	uint32_t ref_clkid = UINT32_MAX;
	uint32_t reg = 0;
	int ret = 0;

	if (rate == NULL) {
		return -EINVAL;
	}

	*rate = 0;

	switch (clockid) {
	case SIWX91X_CLK_XTAL_MHZ:
		*rate = DT_PROP(DT_NODELABEL(xtal_mhz), clock_frequency);
		break;
	case SIWX91X_CLK_XTAL_KHZ:
		*rate = DT_PROP(DT_NODELABEL(xtal_khz), clock_frequency);
		break;
	case SIWX91X_CLK_RC_MHZ:
		*rate = DT_PROP(DT_NODELABEL(rc_mhz), clock_frequency);
		break;
	case SIWX91X_CLK_RC_KHZ:
		*rate = DT_PROP(DT_NODELABEL(rc_khz), clock_frequency);
		break;
	case SIWX91X_CLK_REF_HP:
		reg = cfg->fsm_reg->MCU_FSM_REF_CLK_REG_b.M4SS_REF_CLK_SEL;
		break;
	case SIWX91X_CLK_REF_ULP:
		reg = cfg->fsm_reg->MCU_FSM_REF_CLK_REG_b.ULPSS_REF_CLK_SEL_b;
		break;
	case SIWX91X_CLK_AON_REF_HF:
		reg = cfg->fsm_reg->MCU_FSM_CLKS_REG_b.HF_FSM_CLK_SELECT;
		break;
	case SIWX91X_CLK_AON_REF_LF:
		reg = cfg->aon_reg->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b.AON_KHZ_CLK_SEL;
		break;
	case SIWX91X_CLK_SYSRTC:
		reg = cfg->aon_reg->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS_b.AON_KHZ_CLK_SEL_SYSRTC;
		break;
	case SIWX91X_CLK_WATCHDOG:
	case SIWX91X_CLK_RTC:
	case SIWX91X_CLK_UULP_GPIO:
		/* RTC and WATCHDOG are linked to UULP_LF_REF */
		ref_clkid = SIWX91X_CLK_AON_REF_LF;
		break;
	default:
		return -EINVAL;
	}

	if (*rate == 0) {
		/* No fixed ref clock, get the ref clock from the register */
		if (ref_clkid == UINT32_MAX) {
			ref_clkid = siwx91x_aon_clock_get_ref_clock(clockid, reg);
			if (ref_clkid == UINT32_MAX) {
				return -EINVAL;
			}
		}

		ret = siwx91x_aon_clock_get_rate(dev, (clock_control_subsys_t)ref_clkid,
						 rate);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int siwx91x_aon_clock_set_rate(const struct device *dev, clock_control_subsys_t sys,
				      clock_control_subsys_rate_t raw_rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);
	ARG_UNUSED(raw_rate);
	return -ENOTSUP;
}

static int siwx91x_aon_clock_configure(const struct device *dev, clock_control_subsys_t sys,
				       void *data)
{
	struct silabs_siwx91x_clock_control_config *cfg = data;
	uint32_t clockid = POINTER_TO_UINT(sys);

	if (cfg->clkid != clockid) {
		return -EINVAL;
	}

	if (!siwx91x_aon_clk_valid(cfg->clkid, cfg->ref_clkid)) {
		return -EINVAL;
	}

	return siwx91x_aon_clk_config(dev, cfg);
}

static enum clock_control_status siwx91x_aon_clock_get_status(const struct device *dev,
							      clock_control_subsys_t clk_cfg)
{
	const struct siwx91x_aon_clock_config *cfg = dev->config;
	uint32_t clkid = POINTER_TO_UINT(clk_cfg);
	bool val;

	switch (clkid) {
	case SIWX91X_CLK_XTAL_MHZ:
		val = cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_40MHZ_CLK_EN_b;
		break;
	case SIWX91X_CLK_XTAL_KHZ:
		val = cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_32KHZ_XTAL_CLK_EN_b;
		break;
	case SIWX91X_CLK_RC_MHZ:
		val = cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_MHZ_RC_CLK_EN_b;
		break;
	case SIWX91X_CLK_RC_KHZ:
		val = cfg->fsm_reg->MCU_FSM_CLK_ENS_AND_FIRST_BOOTUP_b.MCU_ULP_32KHZ_RC_CLK_EN_b;
		break;
	case SIWX91X_CLK_REF_HP:
	case SIWX91X_CLK_REF_ULP:
	case SIWX91X_CLK_AON_REF_HF:
	case SIWX91X_CLK_AON_REF_LF:
	case SIWX91X_CLK_SYSRTC:
	case SIWX91X_CLK_WATCHDOG:
	case SIWX91X_CLK_RTC:
	case SIWX91X_CLK_UULP_GPIO:
		val = 1;
		break;
	default:
		return -EINVAL;
	}

	return val ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int siwx91x_aon_clock_init(const struct device *dev)
{
	const struct siwx91x_aon_clock_config *cfg = dev->config;
	int ret;

	/* The KHz crystal (Xtal) is the only clock that is not enabled out of reset. The register
	 * reports it as off, but in reality, peripherals under this clock are working. This may be
	 * because the NWP driver enables this clock with SL_SI91X_EXT_FEAT_XTAL_CLK. However, even
	 * without this NWP configuration, the peripherals still seem to work. There is still some
	 * mystery to be resolved around this clock, but we enable it here to reflect the actual
	 * situation.
	 */
	clock_control_on(dev, (clock_control_subsys_t)SIWX91X_CLK_XTAL_KHZ);

	for (size_t i = 0; i < cfg->aon_clk_mux_count; i++) {
		struct silabs_siwx91x_clock_control_config *mux = &cfg->aon_clk_mux[i];

		ret = siwx91x_aon_clk_valid(mux->clkid, mux->ref_clkid);
		if (!ret) {
			LOG_ERR("Invalid AON route clkid %u ref %u", mux->clkid, mux->ref_clkid);
			continue;
		}

		ret = siwx91x_aon_clk_config(dev, mux);
		if (ret) {
			LOG_ERR("AON route %zu (clkid %u) failed", i, mux->clkid);
		}
	}

	return 0;
}

static DEVICE_API(clock_control, siwx91x_aon_clock_api) = {
	.on = siwx91x_aon_clock_on,
	.off = siwx91x_aon_clock_off,
	.get_rate = siwx91x_aon_clock_get_rate,
	.set_rate = siwx91x_aon_clock_set_rate,
	.configure = siwx91x_aon_clock_configure,
	.get_status = siwx91x_aon_clock_get_status,
};

#define SIWX91X_AON_CLOCK_INIT(inst)                                                               \
	static struct silabs_siwx91x_clock_control_config aon_clk_mux_##inst[] = {                 \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SIWX91X_CLOCK_MANAGER_CHILD_INIT)};        \
	static const struct siwx91x_aon_clock_config siwx91x_aon_clock_config_##inst = {           \
		.fsm_reg = (MCU_FSM_Type *)DT_INST_REG_ADDR_BY_NAME(inst, mcu_fsm),                \
		.aon_reg = (MCU_AON_Type *)DT_INST_REG_ADDR_BY_NAME(inst, mcu_aon),                \
		.aon_clk_mux = aon_clk_mux_##inst,                                                 \
		.aon_clk_mux_count = ARRAY_SIZE(aon_clk_mux_##inst)                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_aon_clock_init, NULL, NULL,                            \
			      &siwx91x_aon_clock_config_##inst, PRE_KERNEL_1,                      \
			      CONFIG_SIWX91X_AON_CLOCK_INIT_PRIORITY, &siwx91x_aon_clock_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_AON_CLOCK_INIT)
