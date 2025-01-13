/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_tach

/**
 * @file
 * @brief Nuvoton NPCM tachometer sensor module driver
 *
 * This file contains a driver for the tachometer sensor module. It used to
 * capture a counter value when the signal via external pin match the condition.
 * The following is block diagram of this module when it set to mode 5.
 *
 *                            |        +-----+-----+
 *           +-----------+    |        |  Capture  |
 * APB_CLK-->| Prescaler |--->|        +-----------+
 *           +-----------+    |              |         +-----------+  TAn Pin
 *                            |        +-----+-----+   |   _   _   |   |
 *                            |---+--->|  Counter  |<--| _| |_| |_ |<--+
 *                            |   |    +-----------+   +-----------+
 * LFCLK--------------------->| CLK_SEL                Edge Detection
 *                            |
 *                            |
 *
 *          (NPCM Tachometer Mode 5, Input Capture)
 *
 * This mode is used to measure the frequency via TAn pin that are slower than
 * TACH_CLK. A transition event (rising or falling edge) received on TAn pin
 * causes a transfer of Counter contents to Capture register and reload counter.
 * Based on this value, we can compute the current RPM of external signal from
 * encoders.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/npcm_tach.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tach_npcm, CONFIG_SENSOR_LOG_LEVEL);

/* Device config */
struct tach_npcm_config {
	/* tachometer controller base address */
	uintptr_t base;
	/* clock configuration */
	uint32_t clk_cfg;
	/* sampling clock frequency of tachometer */
	uint32_t sample_clk;
	/* tach channel */
	int tach_channel;
	/* selected pin of tachometer */
	uint8_t pin_select;
	/* only support static ta pin */
	bool pin_static;
	/* number of pulses (holes) per round of tachometer's input (encoder) */
	int pulses_per_round;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct tach_npcm_data {
	/* Input clock for tachometers */
	uint32_t input_clk;
	/* Captured counts of tachometer */
	uint32_t capture;
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev)                                                                          \
	((struct tach_reg *)((const struct tach_npcm_config *)(dev)->config)->base)

/* Maximum count of prescaler */
#define NPCM_TACHO_PRSC_MAX 0xff
/* Maximum count of counter */
#define NPCM_TACHO_CNT_MAX 0xffff
/* Operation mode used for tachometer */
#define NPCM_TACH_MDSEL 4
/* Clock selection for tachometer */
#define NPCM_CLKSEL_APBCLK 1
#define NPCM_CLKSEL_LFCLK 4
#define NPCM_TACH_LFCLK	32768

/* TACH inline local functions */
static inline void tach_npcm_start(const struct device *dev)
{
	struct tach_npcm_data *const data = dev->data;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	/* Set the default value of counter and capture register of timer */
	inst->TCNT1 = NPCM_TACHO_CNT_MAX;
	inst->TCRA  = NPCM_TACHO_CNT_MAX;

	/*
	 * Set the edge detection polarity to falling (high-to-low transition)
	 * and enable the functionality to capture TCNT1 into TCRA and preset
	 * TCNT1 when event is triggered.
	 */
	inst->TMCTRL |= BIT(NPCM_TMCTRL_TAEN);

	/* Enable input debounce logic into TA pin. */
	inst->TCFG |= BIT(NPCM_TCFG_TADBEN);

	/* Select clock source of timer 1 from no clock and start to count. */
	SET_FIELD(inst->TCKC, NPCM_TCKC_C1CSEL_FIELD, data->input_clk == NPCM_TACH_LFCLK
		  ? NPCM_CLKSEL_LFCLK : NPCM_CLKSEL_APBCLK);
}

static inline bool tach_npcm_is_underflow(const struct device *dev)
{
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	LOG_DBG("tach is underflow %d",
		IS_BIT_SET(inst->TECTRL, NPCM_TECTRL_TCPND));

	/*
	 * In mode 5, the flag TCPND indicates the TCNT1 is under
	 * underflow situation. (No edges are detected.)
	 */
	return IS_BIT_SET(inst->TECTRL, NPCM_TECTRL_TCPND);
}

static inline void tach_npcm_clear_underflow_flag(const struct device *dev)
{
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	inst->TECLR = BIT(NPCM_TECLR_TCCLR);
}

static inline bool tach_npcm_is_captured(const struct device *dev)
{
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	LOG_DBG("tach is captured %d", IS_BIT_SET(inst->TECTRL, NPCM_TECTRL_TAPND));

	/*
	 * In mode 5, the flag TAPND indicates a input captured on TAn transition.
	 */
	return IS_BIT_SET(inst->TECTRL, NPCM_TECTRL_TAPND);
}

static inline void tach_npcm_clear_captured_flag(const struct device *dev)
{
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	inst->TECLR = BIT(NPCM_TECLR_TACLR);
}

static inline uint16_t tach_npcm_get_captured_count(const struct device *dev)
{
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	return inst->TCRA;
}

/* TACH local functions */
static int tach_npcm_configure(const struct device *dev)
{
	const struct tach_npcm_config *const config = dev->config;
	struct tach_npcm_data *const data = dev->data;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	/* Set mode 5 to tachometer module */
	SET_FIELD(inst->TMCTRL, NPCM_TMCTRL_MDSEL_FIELD, NPCM_TACH_MDSEL);

	/* Configure clock module and its frequency of tachometer */
	if (config->sample_clk == 0) {
		return -EINVAL;
	} else if (data->input_clk == NPCM_TACH_LFCLK) {
		/* Enable low power mode */
		inst->TCKC |= BIT(NPCM_TCKC_LOW_PWR);
		if (config->sample_clk != data->input_clk) {
			LOG_ERR("%s operate freq is %d not fixed to 32kHz",
				dev->name, data->input_clk);
			return -EINVAL;
		}
	} else {
		/* Configure sampling freq by setting prescaler of APB1 */
		uint16_t prescaler = data->input_clk / config->sample_clk;

		if (config->sample_clk > data->input_clk) {
			LOG_ERR("%s operate freq exceeds APB1 clock",
				dev->name);
			return -EINVAL;
		}
		inst->TPRSC = MIN(NPCM_TACHO_PRSC_MAX, MAX(prescaler, 1));
	}

	if (config->pin_static ||
			config->pin_select == NPCM_TACH_PIN_SELECT_DEFAULT) {
		LOG_WRN("Tachometer %d select default pin", config->tach_channel);
		SET_FIELD(inst->TCFG, NPCM_TCFG_MFT_IN_SEL, config->tach_channel);
	} else {
		/* select pin to sample */
		SET_FIELD(inst->TCFG, NPCM_TCFG_MFT_IN_SEL, config->pin_select);
	}

	return 0;
}

/* TACH api functions */
int tach_npcm_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);
	struct tach_npcm_data *const data = dev->data;

	/* Check whether underflow flag of tachometer is occurred */
	if (tach_npcm_is_underflow(dev)) {
		/* Clear pending flags */
		tach_npcm_clear_underflow_flag(dev);
		/* Clear stale captured data */
		tach_npcm_clear_captured_flag(dev);
		data->capture = 0;

		return 0;
	}

	/* Check whether capture flag of tachometer is set */
	if (tach_npcm_is_captured(dev)) {
		/* Clear pending flags */
		tach_npcm_clear_captured_flag(dev);
		/* Save captured count */
		data->capture = NPCM_TACHO_CNT_MAX -
					tach_npcm_get_captured_count(dev);
	}

	return 0;
}

static int tach_npcm_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct tach_npcm_config *const config = dev->config;
	struct tach_npcm_data *const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	if (data->capture > 0) {
		/*
		 * RPM = (f * 60) / (n * TACH)
		 *   n = Pulses per round
		 *   f = Tachometer operation frequency (Hz)
		 *   TACH = Captured counts of tachometer
		 */
		val->val1 = (config->sample_clk * 60) /
			(config->pulses_per_round * data->capture);
	} else {
		val->val1 = 0U;
	}

	val->val2 = 0U;

	return 0;
}


/* TACH driver registration */
static int tach_npcm_init(const struct device *dev)
{
	const struct tach_npcm_config *const config = dev->config;
	struct tach_npcm_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)
							config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on tachometer clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)
					config->clk_cfg, &data->input_clk);
	if (ret < 0) {
		LOG_ERR("Get tachometer clock rate error %d", ret);
		return ret;
	}

	/* Configure pin-mux for tachometer device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Tacho pinctrl setup failed (%d)", ret);
		return ret;
	}


	/* Configure tachometer and its operate freq. */
	ret = tach_npcm_configure(dev);
	if (ret < 0) {
		LOG_ERR("Config tachometer failed");
		return ret;
	}

	/* Start tachometer sensor */
	tach_npcm_start(dev);

	return 0;
}

static const struct sensor_driver_api tach_npcm_driver_api = {
	.sample_fetch = tach_npcm_sample_fetch,
	.channel_get = tach_npcm_channel_get,
};

#define NPCM_TACH_INIT(inst)						                       \
	PINCTRL_DT_INST_DEFINE(inst);					                       \
									                       \
	static const struct tach_npcm_config tach_cfg_##inst = {                               \
		.base = DT_INST_REG_ADDR(inst),                                                \
		.clk_cfg = DT_INST_PHA(inst, clocks, clk_cfg),                                 \
		.sample_clk = DT_INST_PROP(inst, sample_clk),                                  \
		.tach_channel = DT_INST_PROP(inst, tach_channel),                              \
		.pin_select = DT_INST_PROP_OR(inst, pin_select, NPCM_TACH_PIN_SELECT_DEFAULT), \
		.pin_static = DT_INST_PROP_OR(inst, pin_static, false),                        \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                  \
	};                                                                                     \
									                       \
	static struct tach_npcm_data tach_data_##inst;                                         \
									                       \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                                     \
			      tach_npcm_init,                                                  \
			      NULL,                                                            \
			      &tach_data_##inst,                                               \
			      &tach_cfg_##inst,                                                \
			      POST_KERNEL,                                                     \
			      CONFIG_SENSOR_INIT_PRIORITY,                                     \
			      &tach_npcm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPCM_TACH_INIT)
