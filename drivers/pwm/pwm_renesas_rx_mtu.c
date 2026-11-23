/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/rx_mtu_pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include "r_gpio_rx_if.h"

#define DT_DRV_COMPAT renesas_rx_mtu_pwm

LOG_MODULE_REGISTER(pwm_renesas_rx_mtu, CONFIG_PWM_LOG_LEVEL);

#define MAX_CHANNEL (4)

#define TMDR_MD_PWM_NORMAL_MODE (0)
#define TMDR_MD_PWM_MODE_1      (2)
#define TMDR_MD_PWM_MODE_2      (3)

#define TCIEV_BIT (4)
#define TCFD_BIT  (7)

#define INPUT_CAPTURE_AT_RISING_EDGE  0x8
#define INPUT_CAPTURE_AT_FALLING_EDGE 0x9
#define INPUT_CAPTURE_AT_BOTH_EDGE    0xA

#define INPUT_LOW  (0)
#define INPUT_HIGH (1)

#define CAPTURE_STOP  (0)
#define CAPTURE_START (1)

/* output always low (0% duty cycle). */
#define PWM_STATE_0         0x11
/* output switches (1% - 99% duty cycle).*/
#define PWM_STATE_SWITCHING 0x65
/* output always high (100% duty cycle). */
#define PWM_STATE_100       0x66

#ifdef CONFIG_PWM_CAPTURE
struct pwm_renesas_rx_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t period;
	uint32_t pulse;
	uint32_t capture;
	uint8_t mode;
	uint32_t overflows;
	bool is_busy;
	bool is_pulse_capture;
	bool continuous;
	uint8_t channel;
};
#endif /* CONFIG_PWM_CAPTURE */

struct tcr_reg {
	/* time prescaler select */
	uint8_t tpsc: 3;
	/* input clock edge select */
	uint8_t ckeg: 2;
	/* counter clear source select */
	uint8_t cclr: 3;
};

struct pwm_renesas_rx_data {
	uint32_t clk_rate;
	uint8_t capture_a_irqn;
	uint8_t cycle_end_irqn;
	gpio_port_pin_t port_pin;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_renesas_rx_capture_data capture;
	bool start_flag;
	uint8_t skip_irq;
	uint8_t start_source;
	uint8_t capture_source;
#endif /* CONFIG_PWM_CAPTURE */
};

struct pwm_renesas_rx_config {
	/* channel MTU */
	uint8_t channel;
	uint8_t bit_idx;
	/* supported number of channels (not necessarily number of used channels) */
	uint8_t max_num_channels;
	/* operate the device in synchronous mode ? */
	bool synchronous;
	/* prescaler setting for TCR */
	uint8_t prescaler;
	const struct device *clock;
	struct clock_control_rx_subsys_cfg clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	struct {
		/* timer control register */
		volatile struct tcr_reg *tcr;
		/* timer mode register */
		volatile uint8_t *tmdr;
		/* timer I/O control register (16 bit or 8 bit depending on number of channels) */
		volatile uint8_t *tior;
		/* Timer Interrupt Enable Register */
		volatile uint8_t *tier;
		/* Timer Status Register */
		volatile uint8_t *tsr;
		/* timer general registers */
		volatile uint16_t *tgr;
		/* timer counter register */
		volatile uint16_t *tcnt;
		/* timer start register */
		volatile uint8_t *tstr;
		/* timer synchronous register */
		volatile uint8_t *tsyr;
		/* timer noise filter */
		volatile uint8_t *nfcr;
	} reg;
#ifdef CONFIG_PWM_CAPTURE
	uint8_t tgi_irq[MAX_CHANNEL + 1];
#endif
};

static inline void mtu_output_enable(const struct device *dev, int channel, uint8_t state)
{
	const struct pwm_renesas_rx_config *config = dev->config;

	switch (config->channel) {
	case 3:
		if (channel == RX_MTIOCxB) {
			MTU.TOER.BIT.OE3B = state;
		} else if (channel == RX_MTIOCxD) {
			MTU.TOER.BIT.OE3D = state;
		} else {
			/* Do nothing */
		}
		break;

	case 4:
		if (channel == RX_MTIOCxA) {
			MTU.TOER.BIT.OE4A = state;
		} else if (channel == RX_MTIOCxB) {
			MTU.TOER.BIT.OE4B = state;
		} else if (channel == RX_MTIOCxC) {
			MTU.TOER.BIT.OE4C = state;
		} else if (channel == RX_MTIOCxD) {
			MTU.TOER.BIT.OE4D = state;
		} else {
			/* Do nothing */
		}
		break;

	default:
		break;
	}
}

static inline int pwm_rx_set_counter_clear(const struct device *dev, int counter_clear_channel)
{
	const struct pwm_renesas_rx_config *config = dev->config;

	switch (counter_clear_channel) {
	case RX_MTIOCxA:
		config->reg.tcr->cclr = 1;
		break;
	case RX_MTIOCxB:
		config->reg.tcr->cclr = 2;
		break;
	case RX_MTIOCxC:
		if (config->max_num_channels > 2) {
			config->reg.tcr->cclr = 5;
		}
		break;
	case RX_MTIOCxD:
		if (config->max_num_channels > 2) {
			config->reg.tcr->cclr = 6;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int pwm_rx_set_period(const struct device *dev, int channel, uint16_t period_cycles)
{
	const struct pwm_renesas_rx_config *config = dev->config;
	int counter_clear_channel, ret;

	if (channel % 2 == 0) {
		counter_clear_channel = channel + 1;
	} else {
		counter_clear_channel = channel - 1;
	}

	ret = pwm_rx_set_counter_clear(dev, counter_clear_channel);
	if (ret) {
		return ret;
	}
	config->reg.tgr[counter_clear_channel] = period_cycles;

	/* For synchronous PWM, the device with the counter clear register has to be started
	 * so that all other synchronous PWMs can work. For non-synchronous PWMs, the clock
	 * has to be started anyway.
	 */

	return 0;
}

static inline void mtu_start_counter(const struct device *dev)
{
	const struct pwm_renesas_rx_config *config = dev->config;

	WRITE_BIT(*config->reg.tstr, config->bit_idx, 1);
}

static inline void mtu_stop_counter(const struct device *dev)
{
	const struct pwm_renesas_rx_config *config = dev->config;

	WRITE_BIT(*config->reg.tstr, config->bit_idx, 0);
}

static int pwm_renesas_rx_set_cycles(const struct device *dev, uint32_t channel,
				     uint32_t period_cycles, uint32_t pulse_cycles,
				     pwm_flags_t flags)
{
	const struct pwm_renesas_rx_config *config = dev->config;
	uint8_t pwm_state = PWM_STATE_SWITCHING;
	uint32_t pulse;

	if ((period_cycles > UINT16_MAX) || (pulse_cycles > UINT16_MAX)) {
		LOG_INF("Fail to set period: %d", period_cycles);
		return -EINVAL;
	}

	if (channel >= config->max_num_channels) {
		LOG_INF("Fail to set channel: %d", channel);
		return -EINVAL;
	}

	mtu_stop_counter(dev);

	if (pulse_cycles == period_cycles) {
		/* 100% duty cycle */
		if (flags & PWM_POLARITY_INVERTED) {
			pwm_state = PWM_STATE_0;
		} else {
			pwm_state = PWM_STATE_100;
		}

		/* The PWM device apparently does not change state if pulse_cycles == period_cycles,
		 * so we have to reduce pulse_cycles by one. Due to the value of pwm_state, the
		 * signal will remain constant at compare match
		 */
		pulse_cycles--;
	}

	if (pulse_cycles == 0) {
		/* 0% duty cycle */
		if (flags & PWM_POLARITY_INVERTED) {
			pwm_state = PWM_STATE_100;
		} else {
			pwm_state = PWM_STATE_0;
		}
	}

	/* Enable TOER output when outputting a waveform from the MTIOC pin of MTU3 and MTU4. */
	mtu_output_enable(dev, channel, 1);

	/* Set the timer I/O control register (TIOR) for a PWM, in this version using PWM mode 1*/
	config->reg.tior[channel] = pwm_state;
	pulse = (flags & PWM_POLARITY_INVERTED) ? period_cycles - pulse_cycles : pulse_cycles;
	config->reg.tgr[channel] = (uint16_t)pulse;
	pwm_rx_set_period(dev, channel, (uint16_t)period_cycles);
	*config->reg.tcnt = 0;

	mtu_start_counter(dev);

	return 0;
}

static int pwm_renesas_rx_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					     uint64_t *cycles)
{

	const struct pwm_renesas_rx_config *config = dev->config;

	uint32_t freq_hz;
	int ret = 0;

	ret = clock_control_get_rate(config->clock, (clock_control_subsys_t)&config->clock_subsys,
				     &freq_hz);

	switch (config->prescaler) {
	case RX_MTU_PWM_SOURCE_DIV_1:
		__fallthrough;
	case RX_MTU_PWM_SOURCE_DIV_4:
		__fallthrough;
	case RX_MTU_PWM_SOURCE_DIV_16:
		__fallthrough;
	case RX_MTU_PWM_SOURCE_DIV_64:
		*cycles = freq_hz >> (config->prescaler * 2);
		break;
	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int pwm_renesas_rx_configure_capture(const struct device *dev, uint32_t channel,
					    pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					    void *user_data)
{
	const struct pwm_renesas_rx_config *config = dev->config;
	struct pwm_renesas_rx_data *data = dev->data;
	uint8_t state;
	int ret;

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No PWM capture type specified");
		return -EINVAL;
	}
	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}
	if (data->capture.is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}

	/* Set normal mode */
	*config->reg.tmdr = TMDR_MD_PWM_NORMAL_MODE;

	/* Set clear source */
	ret = pwm_rx_set_counter_clear(dev, channel);
	if (ret) {
		return ret;
	}

	pinctrl_soc_pin_t pin = config->pcfg->states->pins[0];

	data->port_pin = (pin.port_num << PORT_POS) | pin.pin_num;

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture.is_pulse_capture = false;
		if (flags & PWM_POLARITY_INVERTED) {
			state = INPUT_CAPTURE_AT_RISING_EDGE;
		} else {
			state = INPUT_CAPTURE_AT_FALLING_EDGE;
		}

	} else {
		state = INPUT_CAPTURE_AT_BOTH_EDGE;
		data->capture.is_pulse_capture = true;
		if (flags & PWM_POLARITY_INVERTED) {
			data->start_source = INPUT_LOW;
			data->capture_source = INPUT_HIGH;
		} else {
			data->start_source = INPUT_HIGH;
			data->capture_source = INPUT_LOW;
		}
	}

	/* Set noise filter */
	WRITE_BIT(*config->reg.nfcr, config->bit_idx, 1);

	/* Set Timer I/O Control */
	if (channel % 2 == 0) {
		/* I/O settings for even numbered channels are encoded in the lower 4 bytes
		 * of the timer I/O control register
		 */
		config->reg.tior[channel / 2] = (config->reg.tior[channel / 2] & 0xf0) + state;
	} else {
		/* I/O settings for even numbered channels are encoded in the higher 4 bytes
		 * of the timer I/O control register
		 */
		config->reg.tior[channel / 2] =
			(config->reg.tior[channel / 2] & 0x0f) + (state << 4);
	}

	data->capture.channel = channel;
	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.continuous = (flags & PWM_CAPTURE_MODE_CONTINUOUS) ? true : false;

	return 0;
}

static int pwm_renesas_rx_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_renesas_rx_config *config = dev->config;
	struct pwm_renesas_rx_data *data = dev->data;

	if (channel >= config->max_num_channels) {
		return -EINVAL;
	}

	if (data->capture.is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}

	if (!data->capture.callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	data->capture.is_busy = true;

	data->capture_a_irqn = config->tgi_irq[channel];
	data->cycle_end_irqn = config->tgi_irq[MAX_CHANNEL];

	/* start counter */
	mtu_start_counter(dev);

	WRITE_BIT(*(config->reg.tier), channel, 1);
	WRITE_BIT(*(config->reg.tier), TCIEV_BIT, 1);

	irq_enable(data->capture_a_irqn);
	irq_enable(data->cycle_end_irqn);

	return 0;
}

static int pwm_renesas_rx_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_renesas_rx_config *config = dev->config;
	struct pwm_renesas_rx_data *data = dev->data;

	if (channel >= config->max_num_channels) {
		return -EINVAL;
	}

	data->capture.is_busy = false;

	/* Disable interruption */
	irq_disable(data->capture_a_irqn);
	irq_disable(data->cycle_end_irqn);

	/* Disable capture source */
	WRITE_BIT(*(config->reg.tier), channel, 0);
	WRITE_BIT(*(config->reg.tier), TCIEV_BIT, 0);

	/* Stop timer */
	mtu_stop_counter(dev);

	/* Clear timer */
	config->reg.tgr[channel] = 0;
	*config->reg.tcnt = 0;

	return 0;
}

static void mtu_rx_tgi_isr(const struct device *dev)
{
	struct pwm_renesas_rx_data *data = dev->data;
	const struct pwm_renesas_rx_config *config = dev->config;
	uint16_t counter = config->reg.tgr[data->capture.channel];
	uint32_t period = UINT16_MAX + 1U;
	uint8_t source = R_GPIO_PinRead(data->port_pin);

	if (data->capture.is_pulse_capture) {
		if (source == data->start_source) {
			data->capture.overflows = 0U;     /* Clear the overflow counter */
			data->start_flag = CAPTURE_START; /* Start pulse width measurement */
		} else if (source == data->capture_source) {
			data->capture.pulse = (data->capture.overflows * period) + counter;
			data->start_flag = CAPTURE_STOP; /* Measurement Invalid */
			if (data->capture.callback != NULL) {
				data->capture.callback(dev, data->capture.channel, 0,
						       data->capture.pulse, 0,
						       data->capture.user_data);
			}

			/* Disable capture in single mode */
			if (data->capture.continuous == false) {
				pwm_renesas_rx_disable_capture(dev, data->capture.channel);
			}
		} else {
			/* Do nothing */
		}
	} else {
		if (data->start_flag == CAPTURE_STOP) {
			data->start_flag = CAPTURE_START; /* Start measurement */
			data->capture.overflows = 0U;     /* Clear the overflow counter */
		} else {
			data->capture.period = (data->capture.overflows * period) + counter;
			data->start_flag = CAPTURE_STOP;
			if (data->capture.callback != NULL) {
				data->capture.callback(dev, data->capture.channel,
						       data->capture.period, 0, 0,
						       data->capture.user_data);
			}

			/* Disable capture in single mode */
			if (data->capture.continuous == false) {
				pwm_renesas_rx_disable_capture(dev, data->capture.channel);
			}
		}
	}
}

static void mtu_rx_tgiv_isr(const struct device *dev)
{
	struct pwm_renesas_rx_data *data = dev->data;

	/* During the pulse width measurement */
	if (data->start_flag != CAPTURE_STOP) {
		data->capture.overflows++;
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static DEVICE_API(pwm, pwm_renesas_rx_driver_api) = {
	.set_cycles = pwm_renesas_rx_set_cycles,
	.get_cycles_per_sec = pwm_renesas_rx_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_renesas_rx_configure_capture,
	.enable_capture = pwm_renesas_rx_enable_capture,
	.disable_capture = pwm_renesas_rx_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

static int pwm_renesas_rx_init(const struct device *dev)
{
	const struct pwm_renesas_rx_config *config = dev->config;
	struct pwm_renesas_rx_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clock, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock, (clock_control_subsys_t)&config->clock_subsys,
				     &data->clk_rate);

	if (ret < 0) {
		return ret;
	}

	/* for the functionality provided by the zephyr PWM API, PWM mode 2 is sufficient,
	 * but some MTUs only support PWM mode 1. So that, we using PWM mode 1 in this version.
	 */
	*config->reg.tmdr = TMDR_MD_PWM_MODE_1;

	config->reg.tcr->tpsc = config->prescaler;

	/* internal input clock default setting (falling edge) */
	config->reg.tcr->ckeg = 0;

	/* setting count-up */
	WRITE_BIT(*config->reg.tsr, TCFD_BIT, true);

	/* synchronize not set*/
	WRITE_BIT(*config->reg.tsyr, config->bit_idx, false);

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
#define IS_HAVE_4_CHANNEL(index) ((DT_REG_SIZE_BY_NAME(DT_INST_PARENT(index), TIOR) * 2) > 2)
#define IRQ_PWM_INIT(index)                                                                        \
	.tgi_irq[RX_MTIOCxA] = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgia, irq),                   \
	.tgi_irq[RX_MTIOCxB] = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgib, irq),                   \
	.tgi_irq[MAX_CHANNEL] = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgiv, irq),                  \
	COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(index), tgic),	                           \
		(.tgi_irq[RX_MTIOCxC] =                                                            \
			DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgic, irq),), ())     \
			 COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(index), tgid),	           \
		(.tgi_irq[RX_MTIOCxD] = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgid, irq),), ())
#define IRQ_PWM_CONFIG_INIT(index)                                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgia, irq),                      \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgia, priority), mtu_rx_tgi_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgib, irq),                      \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgib, priority), mtu_rx_tgi_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgiv, irq),                      \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgiv, priority),                 \
			    mtu_rx_tgiv_isr, DEVICE_DT_INST_GET(index), 0);                        \
		COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(index), tgic),	\
			    (IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgic, irq),	\
					 DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgic, priority),    \
					 mtu_rx_tgi_isr, DEVICE_DT_INST_GET(index), 0);),	\
			    ())                        \
		COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(index), tgid),	\
			    (IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgid, irq),	\
					 DT_IRQ_BY_NAME(DT_INST_PARENT(index), tgid, priority),    \
					 mtu_rx_tgi_isr, DEVICE_DT_INST_GET(index), 0);),	\
			    ())                        \
	} while (0)
#else
#define IRQ_PWM_INIT(index)
#define IRQ_PWM_CONFIG_INIT(index)
#endif

#define PWM_DEVICE_INIT(index)                                                                     \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
	static const struct pwm_renesas_rx_config pwm_rx_cfg##index = {                            \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.channel = DT_PROP(DT_INST_PARENT(index), channel),                                \
		.prescaler = DT_INST_PROP(index, prescaler),                                       \
		.reg =                                                                             \
			{                                                                          \
				.tcr = (struct tcr_reg *)DT_REG_ADDR_BY_NAME(                      \
					DT_INST_PARENT(index), TCR),                               \
				.tmdr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index),      \
								       TMDR),                      \
				.tior = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index),      \
								       TIOR),                      \
				.tier = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index),      \
								       TIER),                      \
				.tsr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index), TSR), \
				.tgr = (uint16_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index),      \
								       TGR),                       \
				.tcnt = (uint16_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index),     \
									TCNT),                     \
				.nfcr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(index),      \
								       NFCR),                      \
				.tstr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_GPARENT(index),     \
								       TSTR),                      \
				.tsyr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_GPARENT(index),     \
								       TSYR),                      \
			},                                                                         \
		.bit_idx = DT_PROP(DT_INST_PARENT(index), bit_idx),                                \
		.max_num_channels = DT_REG_SIZE_BY_NAME(DT_INST_PARENT(index), TIOR) * 2,          \
		.clock = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(index))),                     \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL(DT_INST_PARENT(index), mstp),               \
				.stop_bit = DT_CLOCKS_CELL(DT_INST_PARENT(index), stop_bit),       \
			},                                                                         \
		IRQ_PWM_INIT(index)};                                                              \
	static struct pwm_renesas_rx_data pwm_renesas_rx_data##index;                              \
	static int pwm_renesas_rx_init_##index(const struct device *dev)                           \
	{                                                                                          \
		IRQ_PWM_CONFIG_INIT(index);                                                        \
		int err = pwm_renesas_rx_init(dev);                                                \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, pwm_renesas_rx_init_##index, NULL,                            \
			      &pwm_renesas_rx_data##index, &pwm_rx_cfg##index, POST_KERNEL,        \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_renesas_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
