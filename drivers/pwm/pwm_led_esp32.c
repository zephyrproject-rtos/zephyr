/*
 * Copyright (c) 2017 Vitor Massaru Iha <vitor@massaru.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_gpio

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <esp_intr_alloc.h>
#include <soc/dport_reg.h>
#include <rom/gpio.h>
#include <soc/gpio_sig_map.h>
#include <soc/ledc_reg.h>

#include <soc.h>
#include <errno.h>
#include <drivers/pwm.h>
#include <kernel.h>
#include <drivers/gpio.h>
#include <string.h>

#define PWM_ESP32_HSCH_HPOINT(i) (LEDC_HSCH0_HPOINT_REG + (0x14 * i))
#define PWM_ESP32_HSCH_DUTY(i) (LEDC_HSCH0_DUTY_REG + (0x14 * i))
#define PWM_ESP32_HSCH_CONF0(i) (LEDC_HSCH0_CONF0_REG + (0x14 * i))
#define PWM_ESP32_HSCH_CONF1(i) (LEDC_HSCH0_CONF1_REG + (0x14 * i))
#define PWM_ESP32_HSTIMER(i) (LEDC_HSTIMER0_CONF_REG + (0x8 * i))

#define PWM_ESP32_LSCH_HPOINT(i) (LEDC_LSCH0_HPOINT_REG + (0x14 * i))
#define PWM_ESP32_LSCH_DUTY(i) (LEDC_LSCH0_DUTY_REG + (0x14 * i))
#define PWM_ESP32_LSCH_CONF0(i) (LEDC_LSCH0_CONF0_REG + (0x14 * i))
#define PWM_ESP32_LSCH_CONF1(i) (LEDC_LSCH0_CONF1_REG + (0x14 * i))
#define PWM_ESP32_LSTIMER(i) (LEDC_LSTIMER0_CONF_REG + (0x8 * i))

enum {
	PWM_LED_ESP32_REF_TICK_FREQ,
	PWM_LED_ESP32_APB_CLK_FREQ,
};

enum {
	PWM_LED_ESP32_HIGH_SPEED,
	PWM_LED_ESP32_LOW_SPEED
};

struct pwm_led_esp32_timer {
	int freq;
	uint8_t bit_num;
} __attribute__ ((__packed__));

struct pwm_led_esp32_channel {
	uint8_t timer : 2;
	uint8_t gpio : 6;
};

union pwm_led_esp32_duty {
	struct {
		uint32_t start: 1;
		uint32_t direction: 1;
		uint32_t num: 3;
		uint32_t cycle: 3;
		uint32_t scale: 3;
	};
	uint32_t val;
};

struct pwm_led_esp32_config {
	/* Speed mode
	 * 0: High speed mode
	 * 1: Low speed mode
	 *
	 * Timers
	 * 0 - 3: 4 timers
	 */

	struct pwm_led_esp32_channel ch_cfg[16];

	struct pwm_led_esp32_timer timer_cfg[2][4];
};

/* TODO: Remove these functions after this PR:
 * https://github.com/zephyrproject-rtos/zephyr/pull/5113
 */
static inline void set_mask32(uint32_t v, uint32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) | v, mem_addr);
}

static inline void clear_mask32(uint32_t v, uint32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) & ~v, mem_addr);
}

static const char *esp32_get_gpio_for_pin(int pin)
{
	if (pin < 32) {
#if defined(CONFIG_GPIO_ESP32_0)
		return DT_INST_LABEL(0);
#else
		return NULL;
#endif /* CONFIG_GPIO_ESP32_0 */
	}

#if defined(CONFIG_GPIO_ESP32_1)
	return DT_INST_LABEL(1);
#else
	return NULL;
#endif /* CONFIG_GPIO_ESP32_1 */
}
/* end Remove after PR 5113 */

static uint8_t pwm_led_esp32_get_gpio_config(uint8_t pin,
		const struct pwm_led_esp32_channel *ch_cfg)
{
	uint8_t i;

	for (i = 0U; i < 16; i++) {
		if (ch_cfg[i].gpio == pin) {
			return i;
		}
	}
	return -EINVAL;
}

static void pwm_led_esp32_low_speed_update(int speed_mode, int channel)
{
	uint32_t reg_addr;

	if (speed_mode == PWM_LED_ESP32_LOW_SPEED) {
		reg_addr = PWM_ESP32_LSCH_CONF0(channel);
		sys_set_bit(reg_addr, LEDC_PARA_UP_LSCH0_S);
	}
}

static void pwm_led_esp32_update_duty(int speed_mode, int channel)
{
	uint32_t conf0_addr;
	uint32_t conf1_addr;

	if (speed_mode == PWM_LED_ESP32_HIGH_SPEED) {
		conf0_addr = PWM_ESP32_HSCH_CONF0(channel);
		conf1_addr = PWM_ESP32_HSCH_CONF1(channel);
	} else {
		conf0_addr = PWM_ESP32_LSCH_CONF0(channel);
		conf1_addr = PWM_ESP32_LSCH_CONF1(channel);
	}

	sys_set_bit(conf0_addr, LEDC_SIG_OUT_EN_LSCH0_S);
	sys_set_bit(conf1_addr, LEDC_DUTY_START_LSCH0_S);

	pwm_led_esp32_low_speed_update(speed_mode, channel);
}

static void pwm_led_esp32_duty_config(int speed_mode,
				      int channel,
				      int duty_val,
				      union pwm_led_esp32_duty duty)
{
	volatile uint32_t hpoint_addr;
	volatile uint32_t duty_addr;
	volatile uint32_t conf1_addr;
	volatile uint32_t conf1_val;

	if (speed_mode == PWM_LED_ESP32_HIGH_SPEED) {
		hpoint_addr = PWM_ESP32_HSCH_HPOINT(channel);
		duty_addr = PWM_ESP32_HSCH_DUTY(channel);
		conf1_addr = PWM_ESP32_HSCH_CONF1(channel);

	} else {
		hpoint_addr = PWM_ESP32_LSCH_HPOINT(channel);
		duty_addr = PWM_ESP32_LSCH_DUTY(channel);
		conf1_addr = PWM_ESP32_LSCH_CONF1(channel);
	}

	sys_write32(0, hpoint_addr);
	sys_write32(duty_val, duty_addr);
	sys_write32(duty.val, conf1_addr);

	pwm_led_esp32_low_speed_update(speed_mode, channel);
}

static void pwm_led_esp32_duty_set(int speed_mode, int channel, int duty_val)
{
	union pwm_led_esp32_duty duty;

	duty.start = 0U;
	duty.direction = 1U;
	duty.cycle = 1U;
	duty.scale = 0U;

	pwm_led_esp32_duty_config(speed_mode, channel, duty_val << 4, duty);
}

static void pwm_led_esp32_bind_channel_timer(int speed_mode,
					     int channel,
					     int timer)
{
	volatile uint32_t timer_addr;

	if (speed_mode == PWM_LED_ESP32_HIGH_SPEED) {
		timer_addr = PWM_ESP32_HSCH_CONF0(channel);
	} else {
		timer_addr = PWM_ESP32_LSCH_CONF0(channel);
	}

	set_mask32(timer, timer_addr);

	pwm_led_esp32_low_speed_update(speed_mode, channel);
}

static int pwm_led_esp32_channel_set(int pin, bool speed_mode, int channel,
				     int duty, int timer)
{
	const int pin_mode = GPIO_OUTPUT;

	const char *device_name;
	struct device *gpio;
	int ret;
	uint32_t sig_out_idx;

	/* Set duty cycle */
	pwm_led_esp32_duty_set(speed_mode, channel, duty);

	pwm_led_esp32_update_duty(speed_mode, channel);

	pwm_led_esp32_bind_channel_timer(speed_mode, channel, timer);

	/* Set pin */
	device_name = esp32_get_gpio_for_pin(pin);
	if (!device_name) {
		return -EINVAL;
	}

	gpio = device_get_binding(device_name);
	if (!gpio) {
		return -EINVAL;
	}

	ret = gpio_pin_configure(gpio, pin, pin_mode);
	if (ret < 0) {
		return ret;
	}

	if (speed_mode == PWM_LED_ESP32_HIGH_SPEED) {
		sig_out_idx = LEDC_HS_SIG_OUT0_IDX + channel;
	} else {
		sig_out_idx = LEDC_LS_SIG_OUT0_IDX + channel;
	}
	esp32_rom_gpio_matrix_out(pin, sig_out_idx, 0, 0);

	return 0;
}

static int pwm_led_esp32_timer_set(int speed_mode, int timer,
				   int bit_num, int frequency)
{
	uint32_t timer_addr;
	uint64_t div_num;
	int tick_sel = PWM_LED_ESP32_APB_CLK_FREQ;
	uint32_t precision = (0x1 << bit_num);

	assert(frequency > 0);

	/* This expression comes from ESP32 Espressif's Technical Reference
	 * Manual chapter 13.2.2 Timers.
	 * div_num is a fixed point value (Q10.8).
	 */
	div_num = ((uint64_t) APB_CLK_FREQ << 8) / frequency / precision;

	if (div_num < 0x100) {
		/* Since Q10.8 is a fixed point value, then div_num < 0x100
		 *  means divisor is too low.
		 */
		return -EINVAL;
	}

	if (div_num > 0x3FFFF) {
		/* Since Q10.8 is a fixed point value, then div_num > 0x3FFFF
		 * means divisor is too high. We can try to use the REF_TICK.
		 */
		div_num = ((uint64_t) 1000000 << 8) / frequency / precision;
		if (div_num < 0x100 || div_num > 0x3FFFF) {
			return -EINVAL;
		}
		tick_sel = PWM_LED_ESP32_REF_TICK_FREQ;
	} else {
		if (speed_mode) {
			sys_set_bit(LEDC_CONF_REG, LEDC_APB_CLK_SEL_S);
		}
	}

	if (speed_mode == PWM_LED_ESP32_HIGH_SPEED) {
		timer_addr = PWM_ESP32_HSTIMER(timer);
	} else {
		timer_addr = PWM_ESP32_LSTIMER(timer);
	}

	set_mask32(div_num << LEDC_DIV_NUM_LSTIMER0_S, timer_addr);
	set_mask32(tick_sel << LEDC_TICK_SEL_LSTIMER0_S, timer_addr);
	set_mask32(bit_num & LEDC_LSTIMER0_LIM_M, timer_addr);

	if (speed_mode) {
		/* update div_num and bit_num */
		sys_set_bit(timer_addr, LEDC_LSTIMER0_PARA_UP_S);
	}

	/* reset low speed timer */
	sys_set_bit(timer_addr, LEDC_LSTIMER0_RST_S);
	sys_clear_bit(timer_addr, LEDC_LSTIMER0_RST_S);

	return 0;
}

/* period_cycles is not used, set frequency on menuconfig instead. */
static int pwm_led_esp32_pin_set_cycles(struct device *dev,
					uint32_t pwm, uint32_t period_cycles,
					uint32_t pulse_cycles, pwm_flags_t flags)
{
	int speed_mode;
	int channel;
	int timer;
	int ret;
	const struct pwm_led_esp32_config * const config =
		(const struct pwm_led_esp32_config *) dev->config;

	ARG_UNUSED(period_cycles);

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	channel = pwm_led_esp32_get_gpio_config(pwm, config->ch_cfg);
	if (channel < 0) {
		return -EINVAL;
	}
	speed_mode = channel < 8 ? PWM_LED_ESP32_HIGH_SPEED :
				   PWM_LED_ESP32_LOW_SPEED;

	timer = config->ch_cfg[channel].timer;
	/* Now we know which speed_mode and timer is set, then we will convert
	 * the channel number from (0 - 15) to (0 - 7).
	 */
	channel %= 8;

	/* Enable peripheral */
	set_mask32(DPORT_LEDC_CLK_EN, DPORT_PERIP_CLK_EN_REG);
	clear_mask32(DPORT_LEDC_RST, DPORT_PERIP_RST_EN_REG);

	/* Set timer */
	ret = pwm_led_esp32_timer_set(speed_mode, timer,
			config->timer_cfg[speed_mode][timer].bit_num,
			config->timer_cfg[speed_mode][timer].freq);
	if (ret < 0) {
		return ret;
	}

	/* Set channel */
	ret = pwm_led_esp32_channel_set(pwm, speed_mode, channel, 0, timer);
	if (ret < 0) {
		return ret;
	}
	pwm_led_esp32_duty_set(speed_mode, channel, pulse_cycles);
	pwm_led_esp32_update_duty(speed_mode, channel);

	return ret;
}

static int pwm_led_esp32_get_cycles_per_sec(struct device *dev, uint32_t pwm,
					    uint64_t *cycles)
{
	const struct pwm_led_esp32_config *config;
	int channel;
	int timer;
	int speed_mode;

	config = (const struct pwm_led_esp32_config *) dev->config;

	channel = pwm_led_esp32_get_gpio_config(pwm, config->ch_cfg);
	if (channel < 0) {
		return -EINVAL;
	}
	speed_mode = channel < 8 ? PWM_LED_ESP32_HIGH_SPEED :
				   PWM_LED_ESP32_LOW_SPEED;

	timer = config->ch_cfg[channel].timer;

	*cycles = config->timer_cfg[speed_mode][timer].freq;

	return 0;
}

static const struct pwm_driver_api pwm_led_esp32_api = {
	.pin_set = pwm_led_esp32_pin_set_cycles,
	.get_cycles_per_sec = pwm_led_esp32_get_cycles_per_sec,
};

int pwm_led_esp32_init(struct device *dev)
{
	return 0;
}

/* Initialization for PWM_LED_ESP32 */
#include <device.h>
#include <init.h>

DEVICE_DECLARE(pwm_led_esp32_0);

#define CH_HS_TIMER(i) ((CONFIG_PWM_LED_ESP32_HS_CH ## i ## _TIMER) & 0x2)
#define CH_HS_GPIO(i) ((CONFIG_PWM_LED_ESP32_HS_CH ## i ## _GPIO) & 0xfff)
#define CH_LS_TIMER(i) ((CONFIG_PWM_LED_ESP32_LS_CH ## i ## _TIMER) & 0x2)
#define CH_LS_GPIO(i) ((CONFIG_PWM_LED_ESP32_LS_CH ## i ## _GPIO) & 0xfff)

#define TIMER_HS_FREQ(i) (CONFIG_PWM_LED_ESP32_HS_TIMER ## i ## _FREQ)
#define TIMER_LS_FREQ(i) (CONFIG_PWM_LED_ESP32_LS_TIMER ## i ## _FREQ)
#define TIMER_HS_BIT_NUM(i) (CONFIG_PWM_LED_ESP32_HS_TIMER ## i ## _BIT_NUM)
#define TIMER_LS_BIT_NUM(i) (CONFIG_PWM_LED_ESP32_LS_TIMER ## i ## _BIT_NUM)

const static struct pwm_led_esp32_config pwm_led_esp32_config = {
	.timer_cfg[PWM_LED_ESP32_HIGH_SPEED][0] = {
		.bit_num = TIMER_HS_BIT_NUM(0),
		.freq = TIMER_HS_FREQ(0),
	},
	.timer_cfg[PWM_LED_ESP32_HIGH_SPEED][1] = {
		.bit_num = TIMER_HS_BIT_NUM(1),
		.freq = TIMER_HS_FREQ(1),
	},
	.timer_cfg[PWM_LED_ESP32_HIGH_SPEED][2] = {
		.bit_num = TIMER_HS_BIT_NUM(2),
		.freq = TIMER_HS_FREQ(2),
	},
	.timer_cfg[PWM_LED_ESP32_HIGH_SPEED][2] = {
		.bit_num = TIMER_HS_BIT_NUM(2),
		.freq = TIMER_HS_FREQ(2),
	},
	.timer_cfg[PWM_LED_ESP32_HIGH_SPEED][3] = {
		.bit_num = TIMER_HS_BIT_NUM(3),
		.freq = TIMER_HS_FREQ(3),
	},
	.timer_cfg[PWM_LED_ESP32_LOW_SPEED][0] = {
		.bit_num = TIMER_LS_BIT_NUM(0),
		.freq = TIMER_LS_FREQ(0),
	},
	.timer_cfg[PWM_LED_ESP32_LOW_SPEED][1] = {
		.bit_num = TIMER_LS_BIT_NUM(1),
		.freq = TIMER_LS_FREQ(1),
	},
	.timer_cfg[PWM_LED_ESP32_LOW_SPEED][2] = {
		.bit_num = TIMER_LS_BIT_NUM(2),
		.freq = TIMER_LS_FREQ(2),
	},
	.timer_cfg[PWM_LED_ESP32_LOW_SPEED][3] = {
		.bit_num = TIMER_LS_BIT_NUM(3),
		.freq = TIMER_LS_FREQ(3),
	},
	.ch_cfg[0] = {
		.timer = CH_HS_TIMER(0),
		.gpio = CH_HS_GPIO(0),
	},
	.ch_cfg[1] = {
		.timer = CH_HS_TIMER(1),
		.gpio = CH_HS_GPIO(1),
	},
	.ch_cfg[2] = {
		.timer = CH_HS_TIMER(2),
		.gpio = CH_HS_GPIO(2),
	},
	.ch_cfg[3] = {
		.timer = CH_HS_TIMER(3),
		.gpio = CH_HS_GPIO(3),
	},
	.ch_cfg[4] = {
		.timer = CH_HS_TIMER(4),
		.gpio = CH_HS_GPIO(4),
	},
	.ch_cfg[5] = {
		.timer = CH_HS_TIMER(5),
		.gpio = CH_HS_GPIO(5),
	},
	.ch_cfg[6] = {
		.timer = CH_HS_TIMER(6),
		.gpio = CH_HS_GPIO(6),
	},
	.ch_cfg[7] = {
		.timer = CH_HS_TIMER(7),
		.gpio = CH_HS_GPIO(7),
	},
	.ch_cfg[8] = {
		.timer = CH_LS_TIMER(0),
		.gpio = CH_LS_GPIO(0),
	},
	.ch_cfg[9] = {
		.timer = CH_LS_TIMER(1),
		.gpio = CH_LS_GPIO(1),
	},
	.ch_cfg[10] = {
		.timer = CH_LS_TIMER(2),
		.gpio = CH_LS_GPIO(2),
	},
	.ch_cfg[11] = {
		.timer = CH_LS_TIMER(3),
		.gpio = CH_LS_GPIO(3),
	},
	.ch_cfg[12] = {
		.timer = CH_LS_TIMER(4),
		.gpio = CH_LS_GPIO(4),
	},
	.ch_cfg[13] = {
		.timer = CH_LS_TIMER(5),
		.gpio = CH_LS_GPIO(5),
	},
	.ch_cfg[14] = {
		.timer = CH_LS_TIMER(6),
		.gpio = CH_LS_GPIO(6),
	},
	.ch_cfg[15] = {
		.timer = CH_LS_TIMER(7),
		.gpio = CH_LS_GPIO(7),
	},
};

DEVICE_AND_API_INIT(pwm_led_esp32_0, CONFIG_PWM_LED_ESP32_DEV_NAME_0,
		    pwm_led_esp32_init, NULL, &pwm_led_esp32_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_led_esp32_api);
