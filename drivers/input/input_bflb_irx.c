/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_irx

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_bflb_irx, CONFIG_INPUT_LOG_LEVEL);

#include <bflb_soc.h>
#include <hbn_reg.h>
#include <glb_reg.h>
#include <ir_reg.h>
#include <zephyr/dt-bindings/clock/bflb_clock_common.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

/* The default uses 2MHz input clock, however it can go up to 32 MHz */
#define BFLB_IRX_CLOCK MHZ(2)

#if defined(CONFIG_SOC_SERIES_BL60X)
#define IRX_MIN_PIN	11
#define IRX_MAX_PIN	13
#define IRX_OFFSET_PIN	10
#define IRX_PIN_OFFSET	GLB_LED_DRIVER_OFFSET
#define IRX_FIFO_OFFSET	IRRX_SWM_FIFO_CONFIG_0_OFFSET
#elif defined(CONFIG_SOC_SERIES_BL70X)
#define IRX_MIN_PIN	17
#define IRX_MAX_PIN	31
#define IRX_OFFSET_PIN	16
#define IRX_PIN_OFFSET	GLB_LED_DRIVER_OFFSET
#define IRX_FIFO_OFFSET	IRRX_SWM_FIFO_CONFIG_0_OFFSET
#elif defined(CONFIG_SOC_SERIES_BL61X)
#define IRX_MIN_PIN	9
#define IRX_MAX_PIN	23
#define IRX_OFFSET_PIN	8
#define IRX_PIN_OFFSET	GLB_IR_CFG1_OFFSET
#define IRX_FIFO_OFFSET	IR_FIFO_CONFIG_0_OFFSET
#define IRX_FIFO_THRES	1
#else
#error Unsupported Platform
#endif

#define IRX_US_TO_PW(rate, us) (((rate / USEC_PER_SEC) * us  - 1) & UINT16_MAX)
#define IRX_PW_TO_US(rate, pw) ((pw * USEC_PER_SEC) / rate)

#define IRX_WAIT_TIMEOUT_MS 1000

/* 1.7 ms (halfway between NEC 0 and NEC 1) */
#define IRX_NEC_DATA_THRESHOLD_US	1700
/* 4.5 ms, matches NEC spec*/
#define IRX_NEC_END_THRESHOLD_US	4500
/* 1.3 ms ? */
#define IRX_RC5_DATA_THRESHOLD_US	1300
/* 2.5 ms */
#define IRX_RC5_END_THRESHOLD_US	2500
/* Default to 4.5 ms end pulse for pulse width mode */
#define IRX_DEFAULT_PW_END_US		4500

enum bflb_irx_protocol {
	PROTOCOL_NEC = 0,
	PROTOCOL_RC5 = 1,
	PROTOCOL_PW = 2,
};

struct bflb_irx_data {
	struct device const *dev;
	uint32_t clock_rate;
	struct k_work_delayable fetch_work;
};

struct bflb_irx_config {
	struct gpio_dt_spec gpio;
	uintptr_t reg;
	void (*irq_config_func)(const struct device *dev);
	enum bflb_irx_protocol protocol;
	uint32_t pw_end_pulse_width;
	bool invert;
	uint16_t deglitch_cnt;
};

static uint32_t bflb_irx_get_set_clock(void)
{
	uint32_t ir_divider, set_divider;
	uint32_t uclk;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t main_clock = clock_bflb_get_root_clock();

	if (main_clock == BFLB_MAIN_CLOCK_RC32M || main_clock == BFLB_MAIN_CLOCK_PLL_RC32M) {
		uclk = BFLB_RC32M_FREQUENCY;
	} else {
		clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_CRYSTAL, &uclk);
	}

	/* Set divider so the output clock is BFLB_IRX_CLOCK */
	set_divider = uclk / BFLB_IRX_CLOCK - 1;
#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
	ir_divider = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	ir_divider &= GLB_IR_CLK_DIV_UMSK;
	ir_divider |= (set_divider << GLB_IR_CLK_DIV_POS) & GLB_IR_CLK_DIV_MSK;
	sys_write32(ir_divider, GLB_BASE + GLB_CLK_CFG2_OFFSET);
#else
	ir_divider = sys_read32(GLB_BASE + GLB_IR_CFG0_OFFSET);
	ir_divider &= GLB_IR_CLK_DIV_UMSK;
	ir_divider |= (set_divider << GLB_IR_CLK_DIV_POS) & GLB_IR_CLK_DIV_MSK;
	sys_write32(ir_divider, GLB_BASE + GLB_IR_CFG0_OFFSET);
#endif
	ir_divider = (ir_divider & GLB_IR_CLK_DIV_MSK) >> GLB_IR_CLK_DIV_POS;

	return uclk / (ir_divider + 1);
}

static int bflb_irx_configure(struct device const *dev)
{
	struct bflb_irx_config const *cfg = dev->config;
	struct bflb_irx_data *data = dev->data;
	uint32_t tmp;
	uint16_t data_threshold, end_threshold;

	data->clock_rate = bflb_irx_get_set_clock();

	tmp = sys_read32(cfg->reg + IRRX_CONFIG_OFFSET);
	tmp &= ~IR_CR_IRRX_MODE_MASK;
	tmp |= cfg->protocol << IR_CR_IRRX_MODE_SHIFT;
	if (cfg->invert) {
		tmp |= IR_CR_IRRX_IN_INV;
	} else {
		tmp &= ~IR_CR_IRRX_IN_INV;
	}
	if (cfg->deglitch_cnt > 0) {
		tmp |= IR_CR_IRRX_DEG_EN;
		tmp &= ~IR_CR_IRRX_DEG_CNT_MASK;
		tmp |= (cfg->deglitch_cnt << IR_CR_IRRX_DEG_CNT_SHIFT) & IR_CR_IRRX_DEG_CNT_MASK;
	} else {
		tmp &= ~IR_CR_IRRX_DEG_EN;
	}
	sys_write32(tmp, cfg->reg + IRRX_CONFIG_OFFSET);

	if (cfg->protocol == PROTOCOL_NEC) {
		data_threshold = IRX_US_TO_PW(data->clock_rate, IRX_NEC_DATA_THRESHOLD_US);
		end_threshold = IRX_US_TO_PW(data->clock_rate, IRX_NEC_END_THRESHOLD_US);
	} else if (cfg->protocol == PROTOCOL_RC5) {
		data_threshold = IRX_US_TO_PW(data->clock_rate, IRX_RC5_DATA_THRESHOLD_US);
		end_threshold = IRX_US_TO_PW(data->clock_rate, IRX_RC5_END_THRESHOLD_US);
	} else {
		/* PW: doesn't care, have a value*/
		data_threshold = 0x1000;
		end_threshold = IRX_US_TO_PW(data->clock_rate, cfg->pw_end_pulse_width);
	}

	tmp = end_threshold << IR_CR_IRRX_END_TH_SHIFT | data_threshold;
	sys_write32(tmp, cfg->reg + IRRX_PW_CONFIG_OFFSET);

#if defined(CONFIG_SOC_SERIES_BL61X)
	tmp = sys_read32(cfg->reg + IR_FIFO_CONFIG_1_OFFSET);
	tmp &= ~IR_RX_FIFO_TH_MASK;
	tmp |= IRX_FIFO_THRES << IR_RX_FIFO_TH_SHIFT;
	sys_write32(tmp, cfg->reg + IR_FIFO_CONFIG_1_OFFSET);
#endif

	/* Setup Interrupts */
	tmp = sys_read32(cfg->reg + IRRX_INT_STS_OFFSET);
	tmp |= IR_CR_IRRX_END_EN;
	tmp |= IR_CR_IRRX_END_CLR;
#if defined(CONFIG_SOC_SERIES_BL61X)
	tmp |= IR_CR_IRRX_FRDY_EN | IR_CR_IRRX_FER_EN;
#endif
	tmp &= ~IR_CR_IRRX_END_MASK;
#if defined(CONFIG_SOC_SERIES_BL61X)
	if (cfg->protocol == PROTOCOL_PW) {
		tmp &= ~IR_CR_IRRX_FRDY_MASK;
	}
#endif
	sys_write32(tmp, cfg->reg + IRRX_INT_STS_OFFSET);

	return 0;
}

static void bflb_irx_isr_handle_prot(const struct device *dev)
{
	const struct bflb_irx_config *cfg = dev->config;
	uint32_t data_count, data;
	int ret;

	data_count = sys_read32(cfg->reg + IRRX_DATA_COUNT_OFFSET) & IR_STS_IRRX_DATA_CNT_MASK;

	data = sys_read32(cfg->reg + IRRX_DATA_WORD0_OFFSET);
	ret = input_report(dev, INPUT_EV_MSC, INPUT_MSC_SCAN, data, true, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Message failed to be enqueued: %d", ret);
	}
	if (data_count <= 32) {
		return;
	}
	data = sys_read32(cfg->reg + IRRX_DATA_WORD1_OFFSET);
	if (data == 0) {
		return;
	}
	ret = input_report(dev, INPUT_EV_MSC, INPUT_MSC_SCAN, data, true, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Message failed to be enqueued: %d", ret);
	}
}

#if defined(CONFIG_SOC_SERIES_BL61X)

static void bflb_irx_isr_handle_pw(const struct device *dev)
{
	const struct bflb_irx_config *cfg = dev->config;
	struct bflb_irx_data *data = dev->data;
	volatile uint32_t tmp;
	volatile uint32_t x;
	int ret;
	k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(IRX_WAIT_TIMEOUT_MS));

	while ((sys_read32(cfg->reg + IR_FIFO_CONFIG_1_OFFSET) & IR_RX_FIFO_CNT_MASK
		|| !(sys_read32(cfg->reg + IRRX_INT_STS_OFFSET) & IRRX_END_INT))
		&& !sys_timepoint_expired(end_timeout)) {
		if ((sys_read32(cfg->reg + IR_FIFO_CONFIG_1_OFFSET) & IR_RX_FIFO_CNT_MASK) == 0) {
			continue;
		}
		x = sys_read32(cfg->reg + IR_FIFO_RDATA_OFFSET);
		ret = input_report(dev, INPUT_EV_MSC, INPUT_MSC_SCAN,
				   IRX_PW_TO_US(data->clock_rate, x), true, K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Message failed to be enqueued: %d", ret);
			break;
		}
	}

	if (sys_timepoint_expired(end_timeout)) {
		LOG_ERR("Timed out");
	}

	tmp = sys_read32(cfg->reg + IRRX_CONFIG_OFFSET);
	tmp &= ~IR_CR_IRRX_EN;
	sys_write32(tmp, cfg->reg + IRRX_CONFIG_OFFSET);

	tmp = sys_read32(cfg->reg + IRX_FIFO_OFFSET);
	if (tmp & IR_RX_FIFO_OVERFLOW) {
		LOG_ERR("Too many pulses, FIFO overflow!");
	}
	tmp |= IR_RX_FIFO_CLR;
	sys_write32(tmp, cfg->reg + IRX_FIFO_OFFSET);

	tmp = sys_read32(cfg->reg + IRRX_INT_STS_OFFSET);
	tmp |= IR_CR_IRRX_END_CLR;
	tmp &= ~(IR_CR_IRRX_FRDY_MASK | IR_CR_IRRX_END_MASK);
	sys_write32(tmp, cfg->reg + IRRX_INT_STS_OFFSET);
}

#else

static void bflb_irx_isr_handle_pw(const struct device *dev)
{
	const struct bflb_irx_config *cfg = dev->config;
	struct bflb_irx_data *data = dev->data;
	volatile uint32_t tmp;
	volatile uint32_t x;
	int ret;

	while (sys_read32(cfg->reg + IRRX_SWM_FIFO_CONFIG_0_OFFSET) & IR_RX_FIFO_CNT_MASK) {
		x = sys_read32(cfg->reg + IRRX_SWM_FIFO_RDATA_OFFSET);
		ret = input_report(dev, INPUT_EV_MSC, INPUT_MSC_SCAN,
				   IRX_PW_TO_US(data->clock_rate, x), true, K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Message failed to be enqueued: %d", ret);
			break;
		}
	}

	tmp = sys_read32(cfg->reg + IRX_FIFO_OFFSET);
	if (tmp & IR_RX_FIFO_OVERFLOW) {
		LOG_ERR("Too many pulses, FIFO overflow!");
	}
	tmp |= IR_RX_FIFO_CLR;
	sys_write32(tmp, cfg->reg + IRX_FIFO_OFFSET);
}

#endif

static void bflb_irx_fetch_work_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct bflb_irx_data *data = CONTAINER_OF(dwork, struct bflb_irx_data, fetch_work);
	struct bflb_irx_config const *cfg = data->dev->config;
	uint32_t tmp;

	if (cfg->protocol == PROTOCOL_PW) {
		bflb_irx_isr_handle_pw(data->dev);
	} else {
		bflb_irx_isr_handle_prot(data->dev);
	}

	tmp = sys_read32(cfg->reg + IRRX_CONFIG_OFFSET);
	tmp |= IR_CR_IRRX_EN;
	sys_write32(tmp, cfg->reg + IRRX_CONFIG_OFFSET);
}

static int bflb_irx_init(struct device const *dev)
{
	struct bflb_irx_config const *config = dev->config;
	struct gpio_dt_spec const *gpio = &config->gpio;
	struct bflb_irx_data *data = dev->data;
	int ret;
	uint32_t tmp;

	data->dev = dev;

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR("GPIO input pin is not ready");
		return -ENODEV;
	}

	/* IRX is a special case where the GPIO mode is SWGPIO input */
	gpio_pin_configure_dt(gpio, GPIO_INPUT);

	/* Select GPIO */
	tmp = sys_read32(GLB_BASE + IRX_PIN_OFFSET);
	tmp &= GLB_IR_RX_GPIO_SEL_UMSK;
	tmp |= ((gpio->pin - IRX_OFFSET_PIN) << GLB_IR_RX_GPIO_SEL_POS) & GLB_IR_RX_GPIO_SEL_MSK;
	sys_write32(tmp, GLB_BASE + IRX_PIN_OFFSET);

	ret = bflb_irx_configure(dev);
	if (ret < 0) {
		return ret;
	}

	config->irq_config_func(dev);

	k_work_init_delayable(&data->fetch_work, bflb_irx_fetch_work_handler);

	tmp = sys_read32(config->reg + IRX_FIFO_OFFSET);
	tmp |= IR_RX_FIFO_CLR;
	sys_write32(tmp, config->reg + IRX_FIFO_OFFSET);

	sys_write32(0, config->reg + IRRX_DATA_WORD0_OFFSET);
	sys_write32(0, config->reg + IRRX_DATA_WORD1_OFFSET);

	tmp = sys_read32(config->reg + IRRX_CONFIG_OFFSET);
	tmp |= IR_CR_IRRX_EN;
	sys_write32(tmp, config->reg + IRRX_CONFIG_OFFSET);

	return 0;
}

#if defined(CONFIG_SOC_SERIES_BL61X)
static void bflb_irx_isr(const struct device *dev)
{
	const struct bflb_irx_config *cfg = dev->config;
	struct bflb_irx_data *data = dev->data;
	volatile uint32_t tmp;
	bool has_data = sys_read32(cfg->reg + IR_FIFO_CONFIG_1_OFFSET) & IR_RX_FIFO_CNT_MASK
		|| sys_read32(cfg->reg + IRRX_INT_STS_OFFSET) & IRRX_FRDY_INT;

	if (cfg->protocol != PROTOCOL_PW || !has_data) {
		tmp = sys_read32(cfg->reg + IRRX_CONFIG_OFFSET);
		tmp &= ~IR_CR_IRRX_EN;
		sys_write32(tmp, cfg->reg + IRRX_CONFIG_OFFSET);

		tmp = sys_read32(cfg->reg + IRRX_INT_STS_OFFSET);
		tmp |= IR_CR_IRRX_END_CLR;
		sys_write32(tmp, cfg->reg + IRRX_INT_STS_OFFSET);
	}

	if (cfg->protocol == PROTOCOL_PW) {
		has_data = sys_read32(cfg->reg + IR_FIFO_CONFIG_1_OFFSET) & IR_RX_FIFO_CNT_MASK
			|| sys_read32(cfg->reg + IRRX_INT_STS_OFFSET) & IRRX_FRDY_INT;
		if (has_data) {
			tmp = sys_read32(cfg->reg + IRRX_INT_STS_OFFSET);
			tmp |= IR_CR_IRRX_FRDY_MASK | IR_CR_IRRX_END_MASK;
			sys_write32(tmp, cfg->reg + IRRX_INT_STS_OFFSET);
			k_work_schedule(&data->fetch_work, K_NO_WAIT);
		} else {
			tmp = sys_read32(cfg->reg + IRRX_CONFIG_OFFSET);
			tmp |= IR_CR_IRRX_EN;
			sys_write32(tmp, cfg->reg + IRRX_CONFIG_OFFSET);
		}
	} else {
		k_work_schedule(&data->fetch_work, K_NO_WAIT);
	}
}
#else

static void bflb_irx_isr(const struct device *dev)
{
	const struct bflb_irx_config *cfg = dev->config;
	struct bflb_irx_data *data = dev->data;
	volatile uint32_t tmp;

	tmp = sys_read32(cfg->reg + IRRX_CONFIG_OFFSET);
	tmp &= ~IR_CR_IRRX_EN;
	sys_write32(tmp, cfg->reg + IRRX_CONFIG_OFFSET);

	tmp = sys_read32(cfg->reg + IRRX_INT_STS_OFFSET);
	tmp |= IR_CR_IRRX_END_CLR;
	sys_write32(tmp, cfg->reg + IRRX_INT_STS_OFFSET);

	/* Don't do processing in ISR */
	k_work_schedule(&data->fetch_work, K_NO_WAIT);
}

#endif

#define IRX_BFLB_IRQ_HANDLER_DECL(n)					\
	static void bflb_irx_config_func_##n(const struct device *dev);
#define IRX_BFLB_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = bflb_irx_config_func_##n
#define IRX_BFLB_IRQ_HANDLER(n)						\
	static void bflb_irx_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    bflb_irx_isr,				\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
		irq_enable(DT_INST_IRQN(n));				\
	}

#define BFLB_IRX_DEFINE(inst)									\
	IRX_BFLB_IRQ_HANDLER_DECL(inst)								\
	static struct bflb_irx_data bflb_irx_data_##inst;					\
	static struct bflb_irx_config const bflb_irx_config_##inst = {				\
		.gpio = GPIO_DT_SPEC_INST_GET(inst, ir_gpios),					\
		.reg = DT_INST_REG_ADDR(inst),							\
		.protocol = DT_INST_ENUM_IDX(inst, protocol),					\
		.pw_end_pulse_width =								\
			DT_INST_PROP_OR(inst, pw_end_pulse_width, IRX_DEFAULT_PW_END_US),	\
		.invert = DT_INST_PROP(inst, invert),						\
		.deglitch_cnt = DT_INST_PROP(inst, deglitch_cnt),				\
		IRX_BFLB_IRQ_HANDLER_FUNC(inst)							\
	};											\
	DEVICE_DT_INST_DEFINE(inst, bflb_irx_init, NULL, &bflb_irx_data_##inst,			\
			      &bflb_irx_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,	\
			      NULL);								\
	IRX_BFLB_IRQ_HANDLER(inst)								\
	BUILD_ASSERT(DT_INST_GPIO_PIN(inst, ir_gpios) <= IRX_MAX_PIN,				\
		     "Pin is invalid for IRX, must be at most " STRINGIFY(IRX_MAX_PIN));	\
	BUILD_ASSERT(DT_INST_GPIO_PIN(inst, ir_gpios) >= IRX_MIN_PIN,				\
		     "Pin is invalid for IRX, must be at least " STRINGIFY(IRX_MIN_PIN));

DT_INST_FOREACH_STATUS_OKAY(BFLB_IRX_DEFINE)
