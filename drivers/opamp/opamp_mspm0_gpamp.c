/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_gpamp

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* PMUOPAMP register offset from the SYSCTL base, not protected by SYSCTL WRITELOCK */
#define MSPM0_GPAMP_PMUOPAMP_OFFSET 0x1380U

#define MSPM0_GPAMP_ENABLE_MASK    BIT(0)
#define MSPM0_GPAMP_PCHENABLE_MASK BIT(1)
#define MSPM0_GPAMP_NSEL_MASK      GENMASK(3, 2)
#define MSPM0_GPAMP_RRI_MASK       GENMASK(5, 4)
#define MSPM0_GPAMP_OUTENABLE_MASK BIT(6)
#define MSPM0_GPAMP_CHOPFREQ_MASK  GENMASK(9, 8)
#define MSPM0_GPAMP_CHOPMODE_MASK  GENMASK(11, 10)

/*
 * Inverting input select. Follower with the output pin uses the pad feedback (unity gain mode),
 * follower without the pin uses the internal output (ADC buffer mode), standalone uses GPAMP_IN-.
 */
#define MSPM0_GPAMP_NSEL_OUTPUT_PIN 0U
#define MSPM0_GPAMP_NSEL_IN_NEG     1U
#define MSPM0_GPAMP_NSEL_INTERNAL   2U

/*
 * Maximum enable time from the device data sheet. VBOOST needs MCLK and ULPCLK not sourced from
 * LFCLK while starting; application software is responsible for the clock tree.
 */
#define MSPM0_GPAMP_ENABLE_TIME_US 20U

struct mspm0_gpamp_config {
	const struct device *sysctl;
	const uint32_t pmuopamp;
	const bool follower;
};

static int mspm0_gpamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct mspm0_gpamp_config *config = dev->config;

	/*
	 * Gain is set by the external network. In standalone mode the call has no effect, per
	 * opamp_functional_mode. In follower mode gain is fixed at x1.
	 */
	if (config->follower && gain != OPAMP_GAIN_1) {
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(opamp, mspm0_gpamp_api) = {
	.set_gain = mspm0_gpamp_set_gain,
};

static int mspm0_gpamp_init(const struct device *dev)
{
	const struct mspm0_gpamp_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->sysctl)) {
		return -ENODEV;
	}

	ret = syscon_write_reg(config->sysctl, MSPM0_GPAMP_PMUOPAMP_OFFSET, config->pmuopamp);
	if (ret < 0) {
		return ret;
	}

	ret = syscon_write_reg(config->sysctl, MSPM0_GPAMP_PMUOPAMP_OFFSET,
			       config->pmuopamp | MSPM0_GPAMP_ENABLE_MASK);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(MSPM0_GPAMP_ENABLE_TIME_US);

	return 0;
}

#define MSPM0_GPAMP_IS_FOLLOWER(inst) DT_INST_ENUM_HAS_VALUE(inst, functional_mode, follower)

#define MSPM0_GPAMP_NSEL(inst)                                                                     \
	(!MSPM0_GPAMP_IS_FOLLOWER(inst)         ? MSPM0_GPAMP_NSEL_IN_NEG                          \
	 : DT_INST_PROP(inst, ti_outpin_enable) ? MSPM0_GPAMP_NSEL_OUTPUT_PIN                      \
						: MSPM0_GPAMP_NSEL_INTERNAL)

#define MSPM0_GPAMP_INIT(inst)                                                                     \
	BUILD_ASSERT(MSPM0_GPAMP_IS_FOLLOWER(inst) ||                                              \
			     DT_INST_ENUM_HAS_VALUE(inst, functional_mode, standalone),            \
		     "GPAMP supports only follower and standalone functional-mode");               \
	BUILD_ASSERT(MSPM0_GPAMP_IS_FOLLOWER(inst) || DT_INST_PROP(inst, ti_outpin_enable),        \
		     "GPAMP standalone mode needs ti,outpin-enable for the external feedback");    \
	static const struct mspm0_gpamp_config mspm0_gpamp_config_##inst = {                       \
		.sysctl = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.follower = MSPM0_GPAMP_IS_FOLLOWER(inst),                                         \
		.pmuopamp =                                                                        \
			MSPM0_GPAMP_PCHENABLE_MASK |                                               \
			FIELD_PREP(MSPM0_GPAMP_NSEL_MASK, MSPM0_GPAMP_NSEL(inst)) |                \
			FIELD_PREP(MSPM0_GPAMP_RRI_MASK, DT_INST_ENUM_IDX(inst, ti_rri_mode)) |    \
			FIELD_PREP(MSPM0_GPAMP_OUTENABLE_MASK,                                     \
				   DT_INST_PROP(inst, ti_outpin_enable)) |                         \
			FIELD_PREP(MSPM0_GPAMP_CHOPFREQ_MASK,                                      \
				   DT_INST_ENUM_IDX(inst, ti_chop_freq_hz)) |                      \
			FIELD_PREP(MSPM0_GPAMP_CHOPMODE_MASK,                                      \
				   DT_INST_ENUM_IDX(inst, ti_chop_mode)),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mspm0_gpamp_init, NULL, NULL, &mspm0_gpamp_config_##inst,      \
			      POST_KERNEL, CONFIG_OPAMP_INIT_PRIORITY, &mspm0_gpamp_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_GPAMP_INIT)
