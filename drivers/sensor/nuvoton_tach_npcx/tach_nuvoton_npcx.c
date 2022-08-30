/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_tach

/**
 * @file
 * @brief Nuvoton NPCX tachometer sensor module driver
 *
 * This file contains a driver for the tachometer sensor module which contains
 * two independent timers (counter 1 and 2). They are used to capture a counter
 * value when the signals via external pins match the condition. The following
 * is block diagram of this module when it set to mode 5.
 *
 *                            |          Capture A
 *                            |              |         +-----------+  TA Pin
 *           +-----------+    |        +-----+-----+   |   _   _   |   |
 * APB_CLK-->| Prescaler |--->|---+--->| Counter 1 |<--| _| |_| |_ |<--+
 *           +-----------+    |   |    +-----------+   +-----------+
 *                            | CLK_SEL                Edge Detection
 *                            |          Capture B
 * LFCLK--------------------->|              |         +-----------+  TB Pin
 *                            |        +-----+-----+   |   _   _   |   |
 *                            |---+--->| Counter 2 |<--| _| |_| |_ |<--+
 *                            |   |    +-----------+   +-----------+
 *                            | CLK_SEL                Edge Detection
 *                            |
 *                            | TACH_CLK
 *                            +----------
 *          (NPCX Tachometer Mode 5, Dual-Independent Input Capture)
 *
 * This mode is used to measure either the frequency of two external clocks
 * (via TA or TB pins) that are slower than TACH_CLK. A transition event (rising
 * or falling edge) received on TAn/TBn pin causes a transfer of timer 1/2
 * contents to Capture register and reload counter. Based on this value, we can
 * compute the current RPM of external signal from encoders.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/npcx_tach.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tach_npcx, CONFIG_SENSOR_LOG_LEVEL);

/* Device config */
struct tach_npcx_config {
	/* tachometer controller base address */
	uintptr_t base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* sampling clock frequency of tachometer */
	uint32_t sample_clk;
	/* selected port of tachometer */
	int port;
	/* number of pulses (holes) per round of tachometer's input (encoder) */
	int pulses_per_round;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct tach_npcx_data {
	/* Input clock for tachometers */
	uint32_t input_clk;
	/* Captured counts of tachometer */
	uint32_t capture;
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev)                                                                          \
	((struct tach_reg *)((const struct tach_npcx_config *)(dev)->config)->base)

/* Maximum count of prescaler */
#define NPCX_TACHO_PRSC_MAX 0xff
/* Maximum count of counter */
#define NPCX_TACHO_CNT_MAX 0xffff
/* Operation mode used for tachometer */
#define NPCX_TACH_MDSEL 4
/* Clock selection for tachometer */
#define NPCX_CLKSEL_APBCLK 1
#define NPCX_CLKSEL_LFCLK 4

/* TACH inline local functions */
static inline void tach_npcx_start_port_a(const struct device *dev)
{
	struct tach_npcx_data *const data = dev->data;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	/* Set the default value of counter and capture register of timer 1. */
	inst->TCNT1 = NPCX_TACHO_CNT_MAX;
	inst->TCRA  = NPCX_TACHO_CNT_MAX;

	/*
	 * Set the edge detection polarity of port A to falling (high-to-low
	 * transition) and enable the functionality to capture TCNT1 into TCRA
	 * and preset TCNT1 when event is triggered.
	 */
	inst->TMCTRL |= BIT(NPCX_TMCTRL_TAEN);

	/* Enable input debounce logic into TA pin. */
	inst->TCFG |= BIT(NPCX_TCFG_TADBEN);

	/* Select clock source of timer 1 from no clock and start to count. */
	SET_FIELD(inst->TCKC, NPCX_TCKC_C1CSEL_FIELD, data->input_clk == LFCLK
		  ? NPCX_CLKSEL_LFCLK : NPCX_CLKSEL_APBCLK);
}

static inline void tach_npcx_start_port_b(const struct device *dev)
{
	struct tach_reg *const inst = HAL_INSTANCE(dev);
	struct tach_npcx_data *const data = dev->data;

	/* Set the default value of counter and capture register of timer 2. */
	inst->TCNT2 = NPCX_TACHO_CNT_MAX;
	inst->TCRB  = NPCX_TACHO_CNT_MAX;

	/*
	 * Set the edge detection polarity of port B to falling (high-to-low
	 * transition) and enable the functionality to capture TCNT2 into TCRB
	 * and preset TCNT2 when event is triggered.
	 */
	inst->TMCTRL |= BIT(NPCX_TMCTRL_TBEN);

	/* Enable input debounce logic into TB pin. */
	inst->TCFG |= BIT(NPCX_TCFG_TBDBEN);

	/* Select clock source of timer 2 from no clock and start to count. */
	SET_FIELD(inst->TCKC, NPCX_TCKC_C2CSEL_FIELD, data->input_clk == LFCLK
		  ? NPCX_CLKSEL_LFCLK : NPCX_CLKSEL_APBCLK);
}

static inline bool tach_npcx_is_underflow(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	LOG_DBG("port A is underflow %d, port b is underflow %d",
		IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TCPND),
		IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TDPND));

	/*
	 * In mode 5, the flag TCPND or TDPND indicates the TCNT1 or TCNT2
	 * is under underflow situation. (No edges are detected.)
	 */
	if (config->port == NPCX_TACH_PORT_A) {
		return IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TCPND);
	} else {
		return IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TDPND);
	}
}

static inline void tach_npcx_clear_underflow_flag(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	if (config->port == NPCX_TACH_PORT_A) {
		inst->TECLR = BIT(NPCX_TECLR_TCCLR);
	} else {
		inst->TECLR = BIT(NPCX_TECLR_TDCLR);
	}
}

static inline bool tach_npcx_is_captured(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	LOG_DBG("port A is captured %d, port b is captured %d",
		IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TAPND),
		IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TAPND));

	/*
	 * In mode 5, the flag TAPND or TBPND indicates a input captured on
	 * TAn or TBn transition.
	 */
	if (config->port == NPCX_TACH_PORT_A) {
		return IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TAPND);
	} else {
		return IS_BIT_SET(inst->TECTRL, NPCX_TECTRL_TBPND);
	}
}

static inline void tach_npcx_clear_captured_flag(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	if (config->port == NPCX_TACH_PORT_A) {
		inst->TECLR = BIT(NPCX_TECLR_TACLR);
	} else {
		inst->TECLR = BIT(NPCX_TECLR_TBCLR);
	}
}

static inline uint16_t tach_npcx_get_captured_count(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	if (config->port == NPCX_TACH_PORT_A) {
		return inst->TCRA;
	} else {
		return inst->TCRB;
	}
}

/* TACH local functions */
static int tach_npcx_configure(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_npcx_data *const data = dev->data;
	struct tach_reg *const inst = HAL_INSTANCE(dev);

	/* Set mode 5 to tachometer module */
	SET_FIELD(inst->TMCTRL, NPCX_TMCTRL_MDSEL_FIELD, NPCX_TACH_MDSEL);

	/* Configure clock module and its frequency of tachometer */
	if (config->sample_clk == 0) {
		return -EINVAL;
	} else if (data->input_clk == LFCLK) {
		/* Enable low power mode */
		inst->TCKC |= BIT(NPCX_TCKC_LOW_PWR);
		if (config->sample_clk != data->input_clk) {
			LOG_ERR("%s operate freq is %d not fixed to 32kHz",
				dev->name, data->input_clk);
			return -EINVAL;
		}
	} else {
		/* Configure sampling freq by setting prescaler of APB1 */
		uint16_t prescaler = data->input_clk / config->sample_clk;

		if (data->input_clk > config->sample_clk) {
			LOG_ERR("%s operate freq exceeds APB1 clock",
				dev->name);
			return -EINVAL;
		}
		inst->TPRSC = MIN(NPCX_TACHO_PRSC_MAX, MAX(prescaler, 1));
	}

	return 0;
}

/* TACH api functions */
int tach_npcx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);
	struct tach_npcx_data *const data = dev->data;

	/* Check whether underflow flag of tachometer is occurred */
	if (tach_npcx_is_underflow(dev)) {
		/* Clear pending flags */
		tach_npcx_clear_underflow_flag(dev);
		data->capture = 0;

		return 0;
	}

	/* Check whether capture flag of tachometer is set */
	if (tach_npcx_is_captured(dev)) {
		/* Clear pending flags */
		tach_npcx_clear_captured_flag(dev);
		/* Save captured count */
		data->capture = NPCX_TACHO_CNT_MAX -
					tach_npcx_get_captured_count(dev);
	}

	return 0;
}

static int tach_npcx_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_npcx_data *const data = dev->data;

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
static int tach_npcx_init(const struct device *dev)
{
	const struct tach_npcx_config *const config = dev->config;
	struct tach_npcx_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)
							&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on tachometer clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)
					&config->clk_cfg, &data->input_clk);
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
	ret = tach_npcx_configure(dev);
	if (ret < 0) {
		LOG_ERR("Config tachometer port %d failed", config->port);
		return ret;
	}

	/* Start tachometer sensor */
	if (config->port == NPCX_TACH_PORT_A) {
		tach_npcx_start_port_a(dev);
	} else if (config->port == NPCX_TACH_PORT_B) {
		tach_npcx_start_port_b(dev);
	} else {
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api tach_npcx_driver_api = {
	.sample_fetch = tach_npcx_sample_fetch,
	.channel_get = tach_npcx_channel_get,
};

#define NPCX_TACH_INIT(inst)                                                   \
	PINCTRL_DT_INST_DEFINE(inst);					       \
									       \
	static const struct tach_npcx_config tach_cfg_##inst = {               \
		.base = DT_INST_REG_ADDR(inst),                                \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(inst),                         \
		.sample_clk = DT_INST_PROP(inst, sample_clk),                  \
		.port = DT_INST_PROP(inst, port),                              \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                  \
	};                                                                     \
									       \
	static struct tach_npcx_data tach_data_##inst;                         \
									       \
	DEVICE_DT_INST_DEFINE(inst,                                            \
			      tach_npcx_init,                                  \
			      NULL,                                            \
			      &tach_data_##inst,                               \
			      &tach_cfg_##inst,                                \
			      POST_KERNEL,                                     \
			      CONFIG_SENSOR_INIT_PRIORITY,                     \
			      &tach_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPCX_TACH_INIT)
