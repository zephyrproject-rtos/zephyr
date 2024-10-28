/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_touch

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <esp_err.h>
#include <soc/soc_pins.h>
#include <soc/periph_defs.h>
#include <hal/touch_sensor_types.h>
#include <hal/touch_sensor_hal.h>
#include <driver/rtc_io.h>
#include <esp_intr_alloc.h>

LOG_MODULE_REGISTER(espressif_esp32_touch, CONFIG_INPUT_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_COUNTER_RTC_ESP32),
	     "Conflict detected: COUNTER_RTC_ESP32 enabled");

#define ESP32_SCAN_DONE_MAX_COUNT 5

#if defined(CONFIG_SOC_SERIES_ESP32)
#define ESP32_RTC_INTR_MSK RTC_CNTL_TOUCH_INT_ST_M
#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)

#define ESP32_RTC_INTR_MSK (RTC_CNTL_TOUCH_DONE_INT_ST_M |		\
			    RTC_CNTL_TOUCH_ACTIVE_INT_ST_M |		\
			    RTC_CNTL_TOUCH_INACTIVE_INT_ST_M |		\
			    RTC_CNTL_TOUCH_SCAN_DONE_INT_ST_M |		\
			    RTC_CNTL_TOUCH_TIMEOUT_INT_ST_M)

#define ESP32_TOUCH_PAD_INTR_MASK (TOUCH_PAD_INTR_MASK_ACTIVE |		\
				   TOUCH_PAD_INTR_MASK_INACTIVE |	\
				   TOUCH_PAD_INTR_MASK_TIMEOUT |	\
				   TOUCH_PAD_INTR_MASK_SCAN_DONE)

#endif /* defined(CONFIG_SOC_SERIES_ESP32) */

struct esp32_touch_sensor_channel_config {
	int32_t channel_num;
	int32_t channel_sens;
	uint32_t zephyr_code;
};

struct esp32_touch_sensor_config {
	uint32_t debounce_interval_ms;
	int num_channels;
	int href_microvolt_enum_idx;
	int lref_microvolt_enum_idx;
	int href_atten_microvolt_enum_idx;
	int filter_mode;
	int filter_debounce_cnt;
	int filter_noise_thr;
	int filter_jitter_step;
	int filter_smooth_level;
	const struct esp32_touch_sensor_channel_config *channel_cfg;
	struct esp32_touch_sensor_channel_data *channel_data;
};

struct esp32_touch_sensor_channel_data {
	const struct device *dev;
	struct k_work_delayable work;
	uint32_t status;
#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
	uint32_t last_status;
#endif /* defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3) */
};

struct esp32_touch_sensor_data {
	uint32_t rtc_intr_msk;
};

static void esp32_touch_sensor_interrupt_cb(void *arg)
{
	const struct device *dev = arg;
	const struct esp32_touch_sensor_config *dev_cfg = dev->config;
	const struct esp32_touch_sensor_channel_config *channel_cfg;
	const int num_channels = dev_cfg->num_channels;
	uint32_t pad_status;

#if defined(CONFIG_SOC_SERIES_ESP32)
	touch_hal_intr_clear();

#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
	static uint8_t scan_done_counter;

	touch_pad_intr_mask_t intr_mask = touch_hal_read_intr_status_mask();

	if (intr_mask & TOUCH_PAD_INTR_MASK_SCAN_DONE) {
		if (++scan_done_counter == ESP32_SCAN_DONE_MAX_COUNT) {
			touch_hal_intr_disable(TOUCH_PAD_INTR_MASK_SCAN_DONE);
			for (int i = 0; i < num_channels; i++) {
				channel_cfg = &dev_cfg->channel_cfg[i];

				/* Set interrupt threshold */
				uint32_t benchmark_value;

				touch_hal_read_benchmark(channel_cfg->channel_num,
					&benchmark_value);
				touch_hal_set_threshold(channel_cfg->channel_num,
					channel_cfg->channel_sens * benchmark_value / 100);
			}
		}
		return;
	}
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */

	touch_hal_read_trigger_status_mask(&pad_status);
#if defined(CONFIG_SOC_SERIES_ESP32)
	touch_hal_clear_trigger_status_mask();
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */
	for (int i = 0; i < num_channels; i++) {
		uint32_t channel_status;

		channel_cfg = &dev_cfg->channel_cfg[i];
		channel_status = (pad_status >> channel_cfg->channel_num) & 0x01;

#if defined(CONFIG_SOC_SERIES_ESP32)
		if (channel_status != 0) {
#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
		uint32_t channel_num = (uint32_t)touch_hal_get_current_meas_channel();

		if (channel_cfg->channel_num == channel_num) {
#endif /* CONFIG_SOC_SERIES_ESP32 */
			struct esp32_touch_sensor_channel_data
				*channel_data = &dev_cfg->channel_data[i];

			channel_data->status = channel_status;
			(void)k_work_reschedule(&channel_data->work,
						K_MSEC(dev_cfg->debounce_interval_ms));
		}
	}
}

static void esp32_rtc_isr(void *arg)
{
	uint32_t status = REG_READ(RTC_CNTL_INT_ST_REG);

	if (arg != NULL) {
		const struct device *dev = arg;
		struct esp32_touch_sensor_data *dev_data = dev->data;

		if (dev_data->rtc_intr_msk & status) {
			esp32_touch_sensor_interrupt_cb(arg);
		}
	}

	REG_WRITE(RTC_CNTL_INT_CLR_REG, status);
}

static esp_err_t esp32_rtc_isr_install(intr_handler_t intr_handler, const void *handler_arg)
{
	esp_err_t err;

	REG_WRITE(RTC_CNTL_INT_ENA_REG, 0);
	REG_WRITE(RTC_CNTL_INT_CLR_REG, UINT32_MAX);

	err = esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, irq),
			ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, priority)) |
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, flags)),
			intr_handler, (void *)handler_arg, NULL);

	return err;
}

/**
 * Handle debounced touch sensor touch state.
 */
static void esp32_touch_sensor_change_deferred(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esp32_touch_sensor_channel_data *channel_data =
			CONTAINER_OF(dwork, struct esp32_touch_sensor_channel_data, work);
	const struct device *dev = channel_data->dev;
	const struct esp32_touch_sensor_config *dev_cfg = dev->config;
	int key_index = channel_data - &dev_cfg->channel_data[0];
	const struct esp32_touch_sensor_channel_config
		*channel_cfg = &dev_cfg->channel_cfg[key_index];

#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
	if (channel_data->last_status != channel_data->status) {
#endif /* defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3) */
		input_report_key(dev, channel_cfg->zephyr_code,
				channel_data->status, true, K_FOREVER);
#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
		channel_data->last_status = channel_data->status;
	}
#endif /* defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3) */
}

static int esp32_touch_sensor_init(const struct device *dev)
{
	struct esp32_touch_sensor_data *dev_data = dev->data;
	const struct esp32_touch_sensor_config *dev_cfg = dev->config;
	const int num_channels = dev_cfg->num_channels;

	touch_hal_init();

#if defined(CONFIG_SOC_SERIES_ESP32)
	touch_hal_volt_t volt = {
		.refh = dev_cfg->href_microvolt_enum_idx,
		.refh = dev_cfg->href_microvolt_enum_idx,
		.atten = dev_cfg->href_atten_microvolt_enum_idx
	};

	touch_hal_set_voltage(&volt);
	touch_hal_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */

	for (int i = 0; i < num_channels; i++) {
		struct esp32_touch_sensor_channel_data *channel_data = &dev_cfg->channel_data[i];
		const struct esp32_touch_sensor_channel_config *channel_cfg =
			&dev_cfg->channel_cfg[i];

		if (!(channel_cfg->channel_num > 0 &&
			channel_cfg->channel_num < SOC_TOUCH_SENSOR_NUM)) {
			LOG_ERR("Touch %d configuration failed: "
				"Touch channel error", i);
			return -EINVAL;
		}

#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
		if (channel_cfg->channel_num == SOC_TOUCH_DENOISE_CHANNEL) {
			LOG_ERR("Touch %d configuration failed: "
				"TOUCH0 is internal denoise channel", i);
			return -EINVAL;
		}
#endif  /* defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3) */

		gpio_num_t gpio_num = touch_sensor_channel_io_map[channel_cfg->channel_num];

		rtc_gpio_init(gpio_num);
		rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_DISABLED);
		rtc_gpio_pulldown_dis(gpio_num);
		rtc_gpio_pullup_dis(gpio_num);

		touch_hal_config(channel_cfg->channel_num);
#if defined(CONFIG_SOC_SERIES_ESP32)
		touch_hal_set_threshold(channel_cfg->channel_num, 0);
		touch_hal_set_group_mask(BIT(channel_cfg->channel_num),
					 BIT(channel_cfg->channel_num));
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */
		touch_hal_set_channel_mask(BIT(channel_cfg->channel_num));

		channel_data->status = 0;
#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
		channel_data->last_status = 0;
#endif /* defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3) */
		channel_data->dev = dev;

		k_work_init_delayable(&channel_data->work, esp32_touch_sensor_change_deferred);
	}

#if defined(CONFIG_SOC_SERIES_ESP32)
	for (int i = 0; i < num_channels; i++) {
		const struct esp32_touch_sensor_channel_config *channel_cfg =
			&dev_cfg->channel_cfg[i];
		uint32_t ref_time;

		ref_time = k_uptime_get_32();
		while (!touch_hal_meas_is_done()) {
			if (k_uptime_get_32() - ref_time > 500) {
				return -ETIMEDOUT;
			}
			k_busy_wait(1000);
		}
		uint16_t touch_value = touch_hal_read_raw_data(channel_cfg->channel_num);

		touch_hal_set_threshold(channel_cfg->channel_num,
					touch_value * (100 - channel_cfg->channel_num) / 100);
	}

#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
	touch_filter_config_t filter_info = {
		.mode = dev_cfg->filter_mode,
		.debounce_cnt = dev_cfg->filter_debounce_cnt,
		.noise_thr = dev_cfg->filter_noise_thr,
		.jitter_step = dev_cfg->filter_jitter_step,
		.smh_lvl = dev_cfg->filter_smooth_level,
	};
	touch_hal_filter_set_config(&filter_info);
	touch_hal_filter_enable();

	touch_hal_timeout_enable();
	touch_hal_timeout_set_threshold(SOC_TOUCH_PAD_THRESHOLD_MAX);
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */

	dev_data->rtc_intr_msk = ESP32_RTC_INTR_MSK;
	esp32_rtc_isr_install(&esp32_rtc_isr, dev);
#if defined(CONFIG_SOC_SERIES_ESP32)
	touch_hal_intr_enable();
#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
	touch_hal_intr_enable(ESP32_TOUCH_PAD_INTR_MASK);
	touch_hal_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */

	touch_hal_start_fsm();

	return 0;
}

#define ESP32_TOUCH_SENSOR_CHANNEL_CFG_INIT(node_id)						\
	{											\
		.channel_num  = DT_PROP(node_id, channel_num),					\
		.channel_sens = DT_PROP(node_id, channel_sens),					\
		.zephyr_code  = DT_PROP(node_id, zephyr_code),					\
	}

#define ESP32_TOUCH_SENSOR_INIT(inst)								\
	static const struct esp32_touch_sensor_channel_config					\
		esp32_touch_sensor_channel_config_##inst[] = {					\
			DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst,				\
				ESP32_TOUCH_SENSOR_CHANNEL_CFG_INIT, (,))			\
		};										\
												\
	static struct esp32_touch_sensor_channel_data esp32_touch_sensor_channel_data_##inst	\
				[ARRAY_SIZE(esp32_touch_sensor_channel_config_##inst)];		\
												\
	static const struct esp32_touch_sensor_config esp32_touch_sensor_config_##inst = {	\
		.debounce_interval_ms = DT_INST_PROP(inst, debounce_interval_ms),		\
		.num_channels = ARRAY_SIZE(esp32_touch_sensor_channel_config_##inst),		\
		.href_microvolt_enum_idx = DT_INST_ENUM_IDX(inst, href_microvolt),		\
		.lref_microvolt_enum_idx = DT_INST_ENUM_IDX(inst, lref_microvolt),		\
		.href_atten_microvolt_enum_idx = DT_INST_ENUM_IDX(inst, href_atten_microvolt),	\
		.filter_mode = DT_INST_PROP(inst, filter_mode),					\
		.filter_debounce_cnt = DT_INST_PROP(inst, filter_debounce_cnt),			\
		.filter_noise_thr = DT_INST_PROP(inst, filter_noise_thr),			\
		.filter_jitter_step = DT_INST_PROP(inst, filter_jitter_step),			\
		.filter_smooth_level = DT_INST_PROP(inst, filter_smooth_level),			\
		.channel_cfg = esp32_touch_sensor_channel_config_##inst,			\
		.channel_data = esp32_touch_sensor_channel_data_##inst,				\
	};											\
												\
	static struct esp32_touch_sensor_data esp32_touch_sensor_data_##inst;			\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &esp32_touch_sensor_init,						\
			      NULL,								\
			      &esp32_touch_sensor_data_##inst,					\
			      &esp32_touch_sensor_config_##inst,				\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,				\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(ESP32_TOUCH_SENSOR_INIT)
