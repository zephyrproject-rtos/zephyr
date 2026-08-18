/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 * Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WS2812-family addressable LED driver over the nRF PWM peripheral.
 *
 * Each WS2812 bit is encoded as one PWM period (one 16-bit EasyDMA word) in
 * COMMON decoder mode: COUNTERTOP is the strip's bit period (time-total)
 * converted to 16 MHz ticks, and the high-pulse width (time-t0h / time-t1h,
 * likewise in ticks) selects '0' vs '1'. One frame is a contiguous array of
 * such words streamed by EasyDMA in a single burst, so the CPU is
 * uninvolved while a frame is clocked out.
 */

#define DT_DRV_COMPAT worldsemi_ws2812_nrf_pwm

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <nrfx_pwm.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_nrf_pwm);

/* Polarity bit (bit 15): output starts high and falls at the compare match. */
#define WS2812_NRF_PWM_POL_BIT 0x8000U

/* Idle/reset word: compare 0 + polarity -> output low for the whole period. */
#define WS2812_NRF_PWM_RESET_WORD WS2812_NRF_PWM_POL_BIT

#define WS2812_NRF_PWM_BITS_PER_COLOR 8U

/* PWM base clock is 16 MHz -> 62.5 ns per tick, i.e. ns * 16 / 1000 ticks. */
#define WS2812_NRF_PWM_NS_TO_TICKS(ns) DIV_ROUND_CLOSEST((ns) * 16U, 1000U)

/* PWM COUNTERTOP for instance @p idx: the WS2812 bit period, in ticks. */
/* The PWM instance this strip claims, as a devicetree node id. */
#define WS2812_NRF_PWM_GEN(idx) DT_INST_PHANDLE(idx, generator)

#define WS2812_NRF_PWM_TOP(idx) WS2812_NRF_PWM_NS_TO_TICKS(DT_INST_PROP(idx, time_total))

/* Reset/latch words = ceil(reset-delay us / bit-period us). The bit period is
 * top / 16 us at the 16 MHz base clock, so words = ceil(us * 16 / top).
 */
#define WS2812_NRF_PWM_RESET_WORDS(idx)                                                            \
	DIV_ROUND_UP(DT_INST_PROP(idx, reset_delay) * 16U, WS2812_NRF_PWM_TOP(idx))

/* Words to encode one full frame for instance @p idx (no reset padding). */
#define WS2812_NRF_PWM_FRAME_WORDS(idx)                                                            \
	((size_t)DT_INST_PROP(idx, chain_length) * DT_INST_PROP_LEN(idx, color_mapping) *          \
	 WS2812_NRF_PWM_BITS_PER_COLOR)

/* Total words for one frame including the trailing reset padding. */
#define WS2812_NRF_PWM_FRAME_TOTAL_WORDS(idx)                                                      \
	(WS2812_NRF_PWM_FRAME_WORDS(idx) + WS2812_NRF_PWM_RESET_WORDS(idx))

struct ws2812_nrf_pwm_config {
	const struct pinctrl_dev_config *pcfg;
	const uint8_t *color_map;
	uint8_t num_colors;
	uint16_t chain_length;
	uint16_t reset_words;
	uint16_t top;       /* PWM COUNTERTOP = WS2812 bit period in 62.5 ns ticks */
	uint16_t word_zero; /* encoded '0' bit (T0H high pulse) */
	uint16_t word_one;  /* encoded '1' bit (T1H high pulse) */
	uint8_t irq_priority;
};

struct ws2812_nrf_pwm_data {
	nrfx_pwm_t pwm;
	struct k_mutex lock;
	struct k_sem done; /* signalled from the PWM ISR on FINISHED */
	uint16_t *frame_seq;
};

/* ---- nrfx PWM + EasyDMA ---------------------------------------------------- */

static void ws2812_nrf_pwm_event_handler(nrfx_pwm_event_type_t event, void *context)
{
	const struct device *dev = context;
	struct ws2812_nrf_pwm_data *data = dev->data;

	if (event == NRFX_PWM_EVENT_FINISHED) {
		k_sem_give(&data->done);
	}
}

static int ws2812_nrf_pwm_play_blocking(struct ws2812_nrf_pwm_data *data, size_t words,
					k_timeout_t timeout)
{
	const nrf_pwm_sequence_t seq = {
		.values = {.p_common = data->frame_seq},
		.length = (uint16_t)words,
		.repeats = 0,
		.end_delay = 0,
	};

	k_sem_reset(&data->done);

	/* Returns the task address to trigger when NRFX_PWM_FLAG_START_VIA_TASK
	 * is used, and 0 otherwise - not an error code, so there is nothing to
	 * check here. A burst that never starts shows up as the wait timing out.
	 */
	(void)nrfx_pwm_simple_playback(&data->pwm, &seq, 1, NRFX_PWM_FLAG_STOP);

	return k_sem_take(&data->done, timeout);
}

/* ---- pixel encoding -------------------------------------------------------- */

static size_t ws2812_nrf_pwm_encode_pixel(const struct ws2812_nrf_pwm_config *cfg,
					  const struct led_rgb *px, uint16_t *dst)
{
	size_t w = 0;

	for (uint8_t c = 0; c < cfg->num_colors; c++) {
		uint8_t v;

		switch (cfg->color_map[c]) {
		case LED_COLOR_ID_RED:
			v = px->r;
			break;
		case LED_COLOR_ID_GREEN:
			v = px->g;
			break;
		case LED_COLOR_ID_BLUE:
			v = px->b;
			break;
		default:
			v = 0U; /* unsupported channel (e.g. white) -> off */
			break;
		}

		for (uint8_t b = 0; b < WS2812_NRF_PWM_BITS_PER_COLOR; b++) {
			dst[w++] = (v & BIT(7U - b)) ? cfg->word_one : cfg->word_zero;
		}
	}

	return w;
}

static size_t ws2812_nrf_pwm_encode_frame(const struct ws2812_nrf_pwm_config *cfg,
					  const struct led_rgb *pixels, size_t num_pixels,
					  uint16_t *dst)
{
	size_t w = 0;

	for (size_t i = 0; i < num_pixels; i++) {
		w += ws2812_nrf_pwm_encode_pixel(cfg, &pixels[i], &dst[w]);
	}

	for (uint16_t r = 0; r < cfg->reset_words; r++) {
		dst[w++] = WS2812_NRF_PWM_RESET_WORD;
	}

	return w;
}

/* ---- Zephyr led_strip API -------------------------------------------------- */

static int ws2812_nrf_pwm_update_rgb(const struct device *dev, struct led_rgb *pixels,
				     size_t num_pixels)
{
	const struct ws2812_nrf_pwm_config *cfg = dev->config;
	struct ws2812_nrf_pwm_data *data = dev->data;

	if (pixels == NULL) {
		return -EINVAL;
	}
	if (num_pixels > cfg->chain_length) {
		return -ERANGE;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Stop any in-flight burst BEFORE reusing the shared encode buffer, so
	 * EasyDMA never reads frame_seq while we overwrite it.
	 */
	(void)nrfx_pwm_stop(&data->pwm, true);

	size_t words = ws2812_nrf_pwm_encode_frame(cfg, pixels, num_pixels, data->frame_seq);

	if (words == 0U) {
		/* Nothing to clock out: no pixels and no reset padding. */
		k_mutex_unlock(&data->lock);
		return 0;
	}

	/* One word is one bit period, i.e. top ticks of the 16 MHz base clock.
	 * Wait for the frame the burst actually carries, plus a millisecond of
	 * slack, so a long chain does not time out while the wire is still busy.
	 */
	const uint32_t frame_us = (uint32_t)words * cfg->top / 16U;
	int rc = ws2812_nrf_pwm_play_blocking(data, words, K_USEC(frame_us + 1000U));

	k_mutex_unlock(&data->lock);

	if (rc != 0) {
		LOG_ERR("update_rgb playback did not finish: %d", rc);
		return -ETIMEDOUT;
	}
	return 0;
}

static size_t ws2812_nrf_pwm_length(const struct device *dev)
{
	const struct ws2812_nrf_pwm_config *cfg = dev->config;

	return cfg->chain_length;
}

static DEVICE_API(led_strip, ws2812_nrf_pwm_api) = {
	.update_rgb = ws2812_nrf_pwm_update_rgb,
	.length = ws2812_nrf_pwm_length,
};

/* ---- init ------------------------------------------------------------------ */

static int ws2812_nrf_pwm_init(const struct device *dev)
{
	const struct ws2812_nrf_pwm_config *cfg = dev->config;
	struct ws2812_nrf_pwm_data *data = dev->data;

	int err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (err < 0) {
		LOG_ERR("pinctrl apply failed: %d", err);
		return err;
	}

	k_mutex_init(&data->lock);
	k_sem_init(&data->done, 0, 1);

	const nrfx_pwm_config_t pwm_cfg = {
		.output_pins = {NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED,
				NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED},
		.pin_inverted = {false, false, false, false},
		.irq_priority = cfg->irq_priority,
		.base_clock = NRF_PWM_CLK_16MHz,
		.count_mode = NRF_PWM_MODE_UP,
		.top_value = cfg->top,
		.load_mode = NRF_PWM_LOAD_COMMON,
		.step_mode = NRF_PWM_STEP_AUTO,
		.skip_gpio_cfg = true, /* pinctrl owns the GPIO */
		.skip_psel_cfg = true, /* pinctrl owns the PSEL */
	};

	/* nrfx 4.0: nrfx_pwm_init returns an errno-style int (0 on success). */
	int nerr = nrfx_pwm_init(&data->pwm, &pwm_cfg, ws2812_nrf_pwm_event_handler, (void *)dev);

	if (nerr != 0) {
		LOG_ERR("nrfx_pwm_init failed: %d", nerr);
		return nerr;
	}

	return 0;
}

#define WS2812_NRF_PWM_DEVICE(idx)                                                                 \
	BUILD_ASSERT(WS2812_NRF_PWM_TOP(idx) < WS2812_NRF_PWM_POL_BIT,                             \
		     "time-total is too long for the 15-bit COUNTERTOP field");                    \
	BUILD_ASSERT(WS2812_NRF_PWM_NS_TO_TICKS(DT_INST_PROP(idx, time_t0h)) <                     \
			     WS2812_NRF_PWM_TOP(idx),                                              \
		     "time-t0h must be shorter than time-total");                                  \
	BUILD_ASSERT(WS2812_NRF_PWM_NS_TO_TICKS(DT_INST_PROP(idx, time_t1h)) <                     \
			     WS2812_NRF_PWM_TOP(idx),                                              \
		     "time-t1h must be shorter than time-total");                                  \
                                                                                                   \
	PINCTRL_DT_DEFINE(WS2812_NRF_PWM_GEN(idx));                                                \
                                                                                                   \
	static const uint8_t ws2812_nrf_pwm_color_map_##idx[] = DT_INST_PROP(idx, color_mapping);  \
                                                                                                   \
	static uint16_t ws2812_nrf_pwm_frame_seq_##idx[WS2812_NRF_PWM_FRAME_TOTAL_WORDS(idx)]      \
		__aligned(4);                                                                      \
                                                                                                   \
	static struct ws2812_nrf_pwm_data ws2812_nrf_pwm_data_##idx = {                            \
		.pwm = NRFX_PWM_INSTANCE(DT_REG_ADDR(WS2812_NRF_PWM_GEN(idx))),                    \
		.frame_seq = ws2812_nrf_pwm_frame_seq_##idx,                                       \
	};                                                                                         \
                                                                                                   \
	static const struct ws2812_nrf_pwm_config ws2812_nrf_pwm_config_##idx = {                  \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(WS2812_NRF_PWM_GEN(idx)),                        \
		.color_map = ws2812_nrf_pwm_color_map_##idx,                                       \
		.num_colors = DT_INST_PROP_LEN(idx, color_mapping),                                \
		.chain_length = DT_INST_PROP(idx, chain_length),                                   \
		.reset_words = WS2812_NRF_PWM_RESET_WORDS(idx),                                    \
		.top = WS2812_NRF_PWM_TOP(idx),                                                    \
		.word_zero = (WS2812_NRF_PWM_POL_BIT |                                             \
			      WS2812_NRF_PWM_NS_TO_TICKS(DT_INST_PROP(idx, time_t0h))),            \
		.word_one = (WS2812_NRF_PWM_POL_BIT |                                              \
			     WS2812_NRF_PWM_NS_TO_TICKS(DT_INST_PROP(idx, time_t1h))),             \
		.irq_priority = DT_IRQ(WS2812_NRF_PWM_GEN(idx), priority),                         \
	};                                                                                         \
                                                                                                   \
	static int ws2812_nrf_pwm_init_##idx(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(WS2812_NRF_PWM_GEN(idx)),                                      \
			    DT_IRQ(WS2812_NRF_PWM_GEN(idx), priority), nrfx_pwm_irq_handler,       \
			    &ws2812_nrf_pwm_data_##idx.pwm, 0);                                    \
		return ws2812_nrf_pwm_init(dev);                                                   \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, ws2812_nrf_pwm_init_##idx, NULL, &ws2812_nrf_pwm_data_##idx,    \
			      &ws2812_nrf_pwm_config_##idx, POST_KERNEL,                           \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_nrf_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_NRF_PWM_DEVICE)
