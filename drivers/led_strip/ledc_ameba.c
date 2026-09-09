/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Realtek Ameba Led Controller
 */

#define DT_DRV_COMPAT realtek_ameba_ledc

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/led/led.h>

#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ledc_ameba);

#define RESULT_RUNNING  0
#define RESULT_COMPLETE 1
#define RESULT_ERR      2

/* Generous upper bound for a LEDC_MAX_LED_NUM transfer plus wait-data margin */
#define LEDC_TX_TIMEOUT K_MSEC(1000)

struct ameba_ledc_data_struct {
	LEDC_InitTypeDef ledc_init_struct;

	uint32_t *tx_data;     /* TX data handle */
	uint16_t tx_total_len; /* TX total length */
	uint16_t tx_len;       /* TX length already written to the FIFO */
	uint8_t irq_result;    /* TX status, published to the caller via tx_done_sem */
	struct k_sem tx_done_sem;
};

/* The LEDC timing registers count in 25 ns steps (40 MHz) */
#define LEDC_TICK_NS 25U

/* Bit timing fields are 8 bits wide and must not be 0 */
#define LEDC_T_MAX_NS (0xFFU * LEDC_TICK_NS)

/* The reset time is 25 ns * (N + 1) with a 14 bit N */
#define LEDC_RESET_MAX_US ((0x3FFFU + 1U) * LEDC_TICK_NS / 1000U)

/* Largest value of the 15 bit wait-data time field, 25 ns * (N + 1) */
#define LEDC_WAIT_DATA_MAX 0x7FFFU

struct ameba_ledc_cfg_struct {
	const struct pinctrl_dev_config *pinctrl_dev;
	const struct device *clock_dev;
	const uint8_t *color_mapping;
	const clock_control_subsys_t clock_subsys;

	/* Bit timings and reset time, already converted to LEDC register counts */
	uint32_t reset_cnt;
	uint32_t t0h_cnt;
	uint32_t t0l_cnt;
	uint32_t t1h_cnt;
	uint32_t t1l_cnt;

	uint16_t led_num_cfg; /* Max 1024 */

	uint8_t num_colors;
};

static void ameba_ledc_isr_handle(const struct device *dev)
{
	struct ameba_ledc_data_struct *pdata = dev->data;
	uint32_t intr_status;
	uint32_t ledc_fifothr;
	uint32_t *start_addr;

	LEDC_INTConfig(LEDC_DEV, LEDC_BIT_GLOBAL_INT_EN, DISABLE);

	intr_status = LEDC_GetINT(LEDC_DEV);

	if (intr_status & LEDC_BIT_FIFO_CPUREQ_INT) {
		LEDC_ClearINT(LEDC_DEV, LEDC_BIT_FIFO_CPUREQ_INT);

		ledc_fifothr = LEDC_GetFIFOLevel(LEDC_DEV);
		start_addr = pdata->tx_data + pdata->tx_len;

		if ((pdata->tx_total_len - pdata->tx_len) >= ledc_fifothr) {
			pdata->tx_len += LEDC_SendData(LEDC_DEV, start_addr, ledc_fifothr);
		} else {
			pdata->tx_len += LEDC_SendData(LEDC_DEV, start_addr,
						       pdata->tx_total_len - pdata->tx_len);
		}

		LEDC_INTConfig(LEDC_DEV, LEDC_BIT_GLOBAL_INT_EN, ENABLE);
		return;
	}

	if (intr_status & LEDC_BIT_LED_TRANS_FINISH_INT) {
		LEDC_ClearINT(LEDC_DEV, LEDC_BIT_LED_TRANS_FINISH_INT);

		pdata->irq_result = RESULT_COMPLETE;
		LEDC_SoftReset(LEDC_DEV);
	}

	if (intr_status & LEDC_BIT_WAITDATA_TIMEOUT_INT) {
		LEDC_ClearINT(LEDC_DEV, LEDC_BIT_WAITDATA_TIMEOUT_INT);

		pdata->irq_result = RESULT_ERR;
		LEDC_SoftReset(LEDC_DEV);
	}

	if (intr_status & LEDC_BIT_FIFO_OVERFLOW_INT) {
		LEDC_ClearINT(LEDC_DEV, LEDC_BIT_FIFO_OVERFLOW_INT);

		pdata->irq_result = RESULT_ERR;
		LEDC_SoftReset(LEDC_DEV);
	}

	if (pdata->irq_result != RESULT_RUNNING) {
		k_sem_give(&pdata->tx_done_sem);
	}

	LEDC_INTConfig(LEDC_DEV, LEDC_BIT_GLOBAL_INT_EN, ENABLE);
}

static int ameba_ledc_update_rgb(const struct device *dev, struct led_rgb *pixels,
				 size_t num_pixels)
{
	const struct ameba_ledc_cfg_struct *cfg = dev->config;
	struct ameba_ledc_data_struct *pdata = dev->data;
	uint16_t data_len;

	if (num_pixels == 0) {
		return 0;
	}

	/* LEDC_MAX_DATA_LENGTH 0x2000 */
	if (!IS_LEDC_DATA_LENGTH(num_pixels)) {
		LOG_ERR("Total data length %zu exceeds max %d", num_pixels, LEDC_MAX_DATA_LENGTH);
		return -EINVAL;
	}

	data_len = (uint16_t)num_pixels;

	pdata->tx_len = 0;
	pdata->tx_data = (uint32_t *)pixels;
	pdata->tx_total_len = data_len;
	pdata->irq_result = RESULT_RUNNING;
	k_sem_reset(&pdata->tx_done_sem);

	pdata->ledc_init_struct.data_length = data_len;
	LEDC_SetTotalLength(LEDC_DEV, pdata->ledc_init_struct.data_length);

	LOG_DBG("Write %d data/0x%08x cnt %d", pdata->ledc_init_struct.data_length,
		pdata->tx_data[0], cfg->num_colors);

	/* Convert from RGB to on-wire format (e.g. GRB, GRBW, RGB, etc).
	 *
	 * color_mapping[] lists the colors in on-wire order, i.e. color_mapping[0]
	 * is the first byte clocked out to the LED chain (matching the mainline
	 * ws2812 drivers). The LEDC transmits the packed 24-bit word starting from
	 * its most-significant color byte (bits [23:16]), so color_mapping[0] must
	 * land in that top byte and the rest fill downward. Packing from the low
	 * byte instead would emit the colors in reverse order on the wire.
	 */
	for (size_t i = 0; i < num_pixels; i++) {
		uint8_t j;
		struct led_rgb pixel_tmp = {0, 0, 0, 0};
		uint8_t *ptr = (uint8_t *)&pixel_tmp + (cfg->num_colors - 1);

		for (j = 0; j < cfg->num_colors; j++) {
			switch (cfg->color_mapping[j]) {
			case LED_COLOR_ID_RED:
				*ptr-- = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				*ptr-- = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				*ptr-- = pixels[i].b;
				break;
			default:
				return -EINVAL;
			}
		}

		memcpy(pixels + i, &pixel_tmp, sizeof(struct led_rgb));
	}
	LOG_DBG("Write %d data 0x%08x", num_pixels, pdata->tx_data[0]);

	LEDC_Cmd(LEDC_DEV, ENABLE);

	if (k_sem_take(&pdata->tx_done_sem, LEDC_TX_TIMEOUT) != 0) {
		LOG_WRN("Ledc TX timeout");
		/* Stop the transfer so the ISR no longer reads the caller's buffer */
		LEDC_INTConfig(LEDC_DEV, LEDC_BIT_GLOBAL_INT_EN, DISABLE);
		LEDC_Cmd(LEDC_DEV, DISABLE);
		LEDC_SoftReset(LEDC_DEV);
		LEDC_INTConfig(LEDC_DEV, LEDC_BIT_GLOBAL_INT_EN, ENABLE);
		return -ETIMEDOUT;
	}

	if (pdata->irq_result == RESULT_COMPLETE) {
		LOG_DBG("Ledc TX done!");
		return 0;
	}

	LOG_WRN("Ledc exit %d", pdata->irq_result);
	return -EIO;
}

static size_t ameba_ledc_length(const struct device *dev)
{
	const struct ameba_ledc_cfg_struct *cfg = dev->config;

	return cfg->led_num_cfg;
}

static DEVICE_API(led_strip, ameba_ledc_api) = {
	.update_rgb = ameba_ledc_update_rgb,
	.length = ameba_ledc_length,
};

static int ameba_ledc_init(const struct device *dev)
{
	const struct ameba_ledc_cfg_struct *cfg = dev->config;
	struct ameba_ledc_data_struct *data = dev->data;
	LEDC_InitTypeDef *pledc_init_struct = &(data->ledc_init_struct);
	int err = 0;

	k_sem_init(&data->tx_done_sem, 0, 1);

	/* Enable clock */
	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("Enable clock failed %d", err);
		return err;
	}

	/* Enable pinctrl */
	err = pinctrl_apply_state(cfg->pinctrl_dev, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Pinctrl setup failed %d", err);
		return err;
	}

	/* LEDC init */
	LEDC_StructInit(pledc_init_struct);

	pledc_init_struct->led_count = cfg->led_num_cfg;
	pledc_init_struct->ledc_trans_mode = LEDC_CPU_MODE;
	/* The HAL fields are named *_ns but take register counts */
	pledc_init_struct->t1h_ns = cfg->t1h_cnt;
	pledc_init_struct->t1l_ns = cfg->t1l_cnt;
	pledc_init_struct->t0h_ns = cfg->t0h_cnt;
	pledc_init_struct->t0l_ns = cfg->t0l_cnt;
	pledc_init_struct->reset_ns = cfg->reset_cnt;
	/* The FIFO underrun limit is min(reset, wait-data), so use the maximum for reset-delay */
	pledc_init_struct->wait_data_time_ns = LEDC_WAIT_DATA_MAX;
	/* Mode 0 keeps the byte order, the pixels are already packed in on-wire order */
	pledc_init_struct->output_RGB_mode = LEDC_OUTPUT_GRB;
	pledc_init_struct->data_length = LEDC_DEFAULT_LED_NUM;
	pledc_init_struct->ledc_fifo_level = 0xF;
	pledc_init_struct->ledc_polarity = LEDC_IDLE_POLARITY_LOW;
	pledc_init_struct->wait_time0_en = ENABLE;
	pledc_init_struct->wait_time1_en = ENABLE;
	pledc_init_struct->wait_time0_ns = 0xEF;      /* 6us */
	pledc_init_struct->wait_time1_ns = 0x2625A00; /* 1000000000ns */

	LEDC_Init(LEDC_DEV, pledc_init_struct);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), ameba_ledc_isr_handle,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	LOG_DBG("Ledc init finish");

	return 0;
}

BUILD_ASSERT(DT_INST_PROP_LEN(0, color_mapping) <= sizeof(struct led_rgb),
	     "color-mapping has more entries than struct led_rgb can hold");
BUILD_ASSERT(IS_LEDC_LED_NUM(DT_INST_PROP(0, chain_length)),
	     "chain-length must be 1 to LEDC_MAX_LED_NUM");

/* Convert a DT time in nanoseconds to LEDC register counts */
#define LEDC_NS_TO_CNT(prop) (DT_INST_PROP(0, prop) / LEDC_TICK_NS)

#define LEDC_CHECK_T(prop, name)                                                                   \
	BUILD_ASSERT(DT_INST_PROP(0, prop) % LEDC_TICK_NS == 0,                                    \
		     name " must be a multiple of 25 ns");                                         \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, prop), LEDC_TICK_NS, LEDC_T_MAX_NS),                 \
		     name " must be 25 to 6375 ns")

LEDC_CHECK_T(time_t0h, "time-t0h");
LEDC_CHECK_T(time_t0l, "time-t0l");
LEDC_CHECK_T(time_t1h, "time-t1h");
LEDC_CHECK_T(time_t1l, "time-t1l");

BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, reset_delay), 1, LEDC_RESET_MAX_US),
	     "reset-delay must be 1 to 409 us");

PINCTRL_DT_INST_DEFINE(0);
static const uint8_t ameba_ledc_color_mapping[] = DT_INST_PROP(0, color_mapping);
static struct ameba_ledc_data_struct ameba_ledc_data;
static const struct ameba_ledc_cfg_struct ameba_ledc_cfg = {
	.pinctrl_dev = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, idx),
	.num_colors = DT_INST_PROP_LEN(0, color_mapping),
	.color_mapping = ameba_ledc_color_mapping,

	.led_num_cfg = DT_INST_PROP(0, chain_length),
	.t0h_cnt = LEDC_NS_TO_CNT(time_t0h),
	.t0l_cnt = LEDC_NS_TO_CNT(time_t0l),
	.t1h_cnt = LEDC_NS_TO_CNT(time_t1h),
	.t1l_cnt = LEDC_NS_TO_CNT(time_t1l),
	/* Register value is 25 ns * (N + 1) */
	.reset_cnt = DT_INST_PROP(0, reset_delay) * 1000U / LEDC_TICK_NS - 1U,
};

DEVICE_DT_INST_DEFINE(0, &ameba_ledc_init, NULL, &ameba_ledc_data, &ameba_ledc_cfg, POST_KERNEL,
		      CONFIG_LED_STRIP_INIT_PRIORITY, &ameba_ledc_api);
