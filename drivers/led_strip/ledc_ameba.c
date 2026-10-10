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

	uint32_t *tx_data;     /* tx data handle */
	uint16_t tx_total_len; /* tx total length */
	uint16_t tx_len;       /* tx len that has been wrote to the FIFO */
	uint8_t irq_result;    /* tx status, published to the caller via tx_done_sem */
	struct k_sem tx_done_sem;
};

struct ameba_ledc_cfg_struct {
	const struct pinctrl_dev_config *pinctrl_dev;
	const struct device *clock_dev;
	const uint8_t *color_mapping;
	const clock_control_subsys_t clock_subsys;

	uint32_t wait_data_time_ns;
	uint32_t reset_ns;

	uint32_t t0h_ns;
	uint32_t t0l_ns;
	uint32_t t1h_ns;
	uint32_t t1l_ns;

	uint16_t led_num_cfg; /* max 1024 */

	uint8_t num_colors;
	uint8_t output_rgb_mode;
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
	uint16_t data_len = (uint16_t)num_pixels;
	uint8_t i;

	/* LEDC_MAX_DATA_LENGTH 0x2000 */
	if (!IS_LEDC_DATA_LENGTH(num_pixels)) {
		LOG_WRN("Total data length too long, force to Max %d", LEDC_MAX_DATA_LENGTH);
		data_len = LEDC_MAX_DATA_LENGTH;
	}

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
	for (i = 0; i < num_pixels; i++) {
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
		return -ETIMEDOUT;
	}

	if (pdata->irq_result == RESULT_COMPLETE) {
		LOG_DBG("Ledc TX done!");
		return 0;
	}

	LOG_WRN("Ledc exit %d", pdata->irq_result);
	return -EFAULT;
}

static int ameba_ledc_update_channels(const struct device *dev, uint8_t *channels,
				      size_t num_channels)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channels);
	ARG_UNUSED(num_channels);
	LOG_WRN("Update_channels not support");
	return -ENOTSUP;
}

static DEVICE_API(led_strip, ameba_ledc_api) = {
	.update_rgb = ameba_ledc_update_rgb,
	.update_channels = ameba_ledc_update_channels,
};

static int ameba_ledc_init(const struct device *dev)
{
	const struct ameba_ledc_cfg_struct *cfg = dev->config;
	struct ameba_ledc_data_struct *data = dev->data;
	LEDC_InitTypeDef *pledc_init_struct = &(data->ledc_init_struct);
	uint16_t led_num;
	int err = 0;

	k_sem_init(&data->tx_done_sem, 0, 1);

	/* enable clock */
	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("Enable clk %d err %d", (uint32_t)cfg->clock_subsys, err);
		return err;
	}

	/* enable pinctrl */
	if (pinctrl_apply_state(cfg->pinctrl_dev, PINCTRL_STATE_DEFAULT)) {
		LOG_ERR("Pinctrl device not ready");
		return -ENODEV;
	}

	/* check the dts config valid
	 * LEDC_MAX_LED_NUM 1024
	 */
	if (!IS_LEDC_LED_NUM(cfg->led_num_cfg)) {
		LOG_ERR("Illegal parameter: LED cnt %d, force to Max %d", cfg->led_num_cfg,
			LEDC_MAX_LED_NUM);
		led_num = LEDC_MAX_LED_NUM;
	} else {
		led_num = cfg->led_num_cfg;
	}

	/* ledc init */
	LEDC_StructInit(pledc_init_struct);

	pledc_init_struct->led_count = led_num;
	pledc_init_struct->ledc_trans_mode = LEDC_CPU_MODE;
	pledc_init_struct->t1h_ns = cfg->t1h_ns;
	pledc_init_struct->t1l_ns = cfg->t1l_ns;
	pledc_init_struct->t0h_ns = cfg->t0h_ns;
	pledc_init_struct->t0l_ns = cfg->t0l_ns;
	pledc_init_struct->reset_ns = cfg->reset_ns;
	pledc_init_struct->wait_data_time_ns = cfg->wait_data_time_ns;
	pledc_init_struct->output_RGB_mode = cfg->output_rgb_mode;
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
	.output_rgb_mode = DT_INST_PROP(0, output_rgb_mode),
	.wait_data_time_ns = DT_INST_PROP(0, wait_data_timeout),
	.t0h_ns = DT_INST_PROP(0, data_tx_time0h),
	.t0l_ns = DT_INST_PROP(0, data_tx_time0l),
	.t1h_ns = DT_INST_PROP(0, data_tx_time1h),
	.t1l_ns = DT_INST_PROP(0, data_tx_time1l),
	.reset_ns = DT_INST_PROP(0, refresh_time),
};

DEVICE_DT_INST_DEFINE(0, &ameba_ledc_init, NULL, &ameba_ledc_data, &ameba_ledc_cfg, POST_KERNEL,
		      CONFIG_LED_STRIP_INIT_PRIORITY, &ameba_ledc_api);
