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

#include <soc/soc_caps.h>
#include <esp_err.h>
#include <soc/periph_defs.h>
#include <hal/touch_sensor_ll.h>
#include <hal/touch_sensor_periph.h>
#include <esp_intr_alloc.h>

#if SOC_TOUCH_SENSOR_VERSION <= 2
#include <soc/rtc_cntl_reg.h>
#endif
#include <driver/rtc_io.h>

#include <hal/touch_sens_hal.h>
#if SOC_TOUCH_SENSOR_VERSION <= 2
#include <hal/touch_sensor_legacy_types.h>
#endif

#define TOUCH_DENOISE_CHANNEL 0

LOG_MODULE_REGISTER(espressif_esp32_touch, CONFIG_INPUT_LOG_LEVEL);

#define ESP32_SCAN_DONE_MAX_COUNT 5

#if SOC_TOUCH_SENSOR_VERSION == 1
#define ESP32_RTC_INTR_MSK RTC_CNTL_TOUCH_INT_ST_M
#elif SOC_TOUCH_SENSOR_VERSION == 2

#define ESP32_RTC_INTR_MSK                                                                         \
	(RTC_CNTL_TOUCH_DONE_INT_ST_M | RTC_CNTL_TOUCH_ACTIVE_INT_ST_M |                           \
	 RTC_CNTL_TOUCH_INACTIVE_INT_ST_M | RTC_CNTL_TOUCH_SCAN_DONE_INT_ST_M |                    \
	 RTC_CNTL_TOUCH_TIMEOUT_INT_ST_M)

#define ESP32_TOUCH_PAD_INTR_MASK                                                                  \
	(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT | \
	 TOUCH_PAD_INTR_MASK_SCAN_DONE)

#elif SOC_TOUCH_SENSOR_VERSION == 3

#define ESP32_TOUCH_INTR_MASK                                                                      \
	(TOUCH_LL_INTR_MASK_ACTIVE | TOUCH_LL_INTR_MASK_INACTIVE | TOUCH_LL_INTR_MASK_TIMEOUT |    \
	 TOUCH_LL_INTR_MASK_SCAN_DONE)

#endif /* SOC_TOUCH_SENSOR_VERSION */

/*
 * Maximum valid touch channel number.
 * v1/v2 define TOUCH_PAD_MAX via legacy types, v3 uses SOC caps directly.
 */
#if SOC_TOUCH_SENSOR_VERSION <= 2
#define TOUCH_CHAN_MAX TOUCH_PAD_MAX
#else
#define TOUCH_CHAN_MAX (SOC_TOUCH_MAX_CHAN_ID + 1)
#endif

struct esp32_touch_sensor_channel_config {
	int32_t channel_num;
	int32_t channel_sens;
	uint32_t zephyr_code;
};

struct esp32_touch_sensor_config {
	uint32_t debounce_interval_ms;
	int num_channels;
#if SOC_TOUCH_SENSOR_VERSION <= 2
	int href_microvolt_enum_idx;
	int lref_microvolt_enum_idx;
	int href_atten_microvolt_enum_idx;
#endif
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
#if SOC_TOUCH_SENSOR_VERSION >= 2
	uint32_t last_status;
#endif
};

struct esp32_touch_sensor_data {
};

static void esp32_touch_handle_active(const struct device *dev)
{
	const struct esp32_touch_sensor_config *dev_cfg = dev->config;
	const struct esp32_touch_sensor_channel_config *channel_cfg;
	const int num_channels = dev_cfg->num_channels;
	uint32_t pad_status;

	touch_ll_get_active_channel_mask(&pad_status);
#if SOC_TOUCH_SENSOR_VERSION == 1
	touch_ll_clear_active_channel_status();
#endif

	for (int i = 0; i < num_channels; i++) {
		uint32_t channel_status;

		channel_cfg = &dev_cfg->channel_cfg[i];
		channel_status = (pad_status >> channel_cfg->channel_num) & 0x01;

#if SOC_TOUCH_SENSOR_VERSION == 1
		if (channel_status != 0) {
#elif SOC_TOUCH_SENSOR_VERSION >= 2
		uint32_t channel_num = (uint32_t)touch_ll_get_current_meas_channel();

		if (channel_cfg->channel_num == channel_num) {
#endif
			struct esp32_touch_sensor_channel_data *channel_data =
				&dev_cfg->channel_data[i];

			channel_data->status = channel_status;
			(void)k_work_reschedule(&channel_data->work,
						K_MSEC(dev_cfg->debounce_interval_ms));
		}
	}
}

static void esp32_touch_sensor_interrupt_cb(void *arg)
{
	const struct device *dev = arg;
	const struct esp32_touch_sensor_config *dev_cfg = dev->config;
	const struct esp32_touch_sensor_channel_config *channel_cfg;
	const int num_channels = dev_cfg->num_channels;

#if SOC_TOUCH_SENSOR_VERSION == 1
	touch_ll_interrupt_clear(TOUCH_LL_INTR_MASK_ALL);

#elif SOC_TOUCH_SENSOR_VERSION >= 2
	static uint8_t scan_done_counter;

#if SOC_TOUCH_SENSOR_VERSION == 2
	touch_pad_intr_mask_t intr_mask = touch_ll_read_intr_status_mask();

	if (!(intr_mask & TOUCH_PAD_INTR_MASK_SCAN_DONE)) {
		esp32_touch_handle_active(dev);
		return;
	}

	if (++scan_done_counter == ESP32_SCAN_DONE_MAX_COUNT) {
		touch_ll_intr_disable(TOUCH_PAD_INTR_MASK_SCAN_DONE);
		for (int i = 0; i < num_channels; i++) {
			channel_cfg = &dev_cfg->channel_cfg[i];

			uint32_t benchmark_value;

			touch_ll_read_benchmark(channel_cfg->channel_num, &benchmark_value);
			touch_ll_set_threshold(channel_cfg->channel_num,
					       channel_cfg->channel_sens * benchmark_value / 100);
		}
	}
	return;

#elif SOC_TOUCH_SENSOR_VERSION == 3
	uint32_t intr_mask = touch_ll_get_intr_status_mask();

	touch_ll_interrupt_clear(intr_mask);

	if (!(intr_mask & TOUCH_LL_INTR_MASK_SCAN_DONE)) {
		esp32_touch_handle_active(dev);
		return;
	}

	if (++scan_done_counter == ESP32_SCAN_DONE_MAX_COUNT) {
		touch_ll_interrupt_disable(TOUCH_LL_INTR_MASK_SCAN_DONE);
		for (int i = 0; i < num_channels; i++) {
			channel_cfg = &dev_cfg->channel_cfg[i];

			uint32_t benchmark_value;

			touch_ll_read_chan_data(channel_cfg->channel_num, 0,
						TOUCH_LL_READ_BENCHMARK, &benchmark_value);
			touch_ll_set_chan_active_threshold(channel_cfg->channel_num, 0,
							   channel_cfg->channel_sens *
								   benchmark_value / 100);
		}
	}
	return;
#endif /* SOC_TOUCH_SENSOR_VERSION == 3 */
#endif /* SOC_TOUCH_SENSOR_VERSION >= 2 */

	esp32_touch_handle_active(dev);
}

#if SOC_TOUCH_SENSOR_VERSION <= 2
static void IRAM_ATTR esp32_touch_rtc_isr(void *arg)
{
	uint32_t status = REG_READ(RTC_CNTL_INT_ST_REG);

	if (!(status & ESP32_RTC_INTR_MSK)) {
		return;
	}

	esp32_touch_sensor_interrupt_cb(arg);
	REG_WRITE(RTC_CNTL_INT_CLR_REG, status);
}
#endif

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
	const struct esp32_touch_sensor_channel_config *channel_cfg =
		&dev_cfg->channel_cfg[key_index];

#if SOC_TOUCH_SENSOR_VERSION >= 2
	if (channel_data->last_status != channel_data->status) {
#endif
		input_report_key(dev, channel_cfg->zephyr_code, channel_data->status, true,
				 K_FOREVER);
#if SOC_TOUCH_SENSOR_VERSION >= 2
		channel_data->last_status = channel_data->status;
	}
#endif
}

static int esp32_touch_sensor_init(const struct device *dev)
{
	esp_err_t err;
	int flags;

	const struct esp32_touch_sensor_config *dev_cfg = dev->config;
	const int num_channels = dev_cfg->num_channels;

#if SOC_TOUCH_SENSOR_VERSION == 1
	touch_ll_stop_fsm_repeated_timer();
	touch_ll_interrupt_clear(TOUCH_LL_INTR_MASK_ALL);

	touch_volt_lim_h_t high_lim =
		(touch_volt_lim_h_t)((dev_cfg->href_atten_microvolt_enum_idx << 2) |
				     dev_cfg->href_microvolt_enum_idx);

	touch_hal_sample_config_t sample_cfg = {
		.charge_duration_ticks = 0x7fff,
		.charge_volt_lim_h = high_lim,
		.charge_volt_lim_l = (touch_volt_lim_l_t)dev_cfg->lref_microvolt_enum_idx,
	};
	touch_hal_config_t hal_cfg = {
		.power_on_wait_ticks = 0xFF,
		.meas_interval_ticks = 0x1000,
		.intr_trig_mode = TOUCH_INTR_TRIG_ON_BELOW_THRESH,
		.intr_trig_group = TOUCH_INTR_TRIG_GROUP_BOTH,
		.sample_cfg_num = 1,
		.sample_cfg = &sample_cfg,
	};
	touch_hal_config_controller(&hal_cfg);
	touch_ll_reset_trigger_groups();

#elif SOC_TOUCH_SENSOR_VERSION == 2
	touch_ll_enable_clock_gate(true);
	touch_ll_reset_module();
	touch_ll_reset_chan_benchmark(TOUCH_LL_FULL_CHANNEL_MASK);
	touch_ll_sleep_reset_benchmark();

	touch_hal_sample_config_t sample_cfg = {
		.charge_times = 500,
		.charge_volt_lim_h = TOUCH_VOLT_LIM_H_1V2,
		.charge_volt_lim_l = TOUCH_VOLT_LIM_L_0V5,
		.idle_conn = TOUCH_IDLE_CONN_GND,
		.bias_type = TOUCH_BIAS_TYPE_SELF,
	};
	touch_hal_config_t hal_cfg = {
		.power_on_wait_ticks = 0xFF,
		.meas_interval_ticks = 0xF,
		.timeout_ticks = 0,
		.sample_cfg_num = 1,
		.sample_cfg = &sample_cfg,
	};
	touch_hal_config_controller(&hal_cfg);

#elif SOC_TOUCH_SENSOR_VERSION == 3
	touch_ll_enable_module_clock(true);
	touch_ll_reset_module();
	touch_ll_reset_chan_benchmark(TOUCH_LL_FULL_CHANNEL_MASK);
	touch_ll_sleep_reset_benchmark();

	touch_hal_sample_config_t sample_cfg = {
		.div_num = 1,
		.charge_times = 500,
		.rc_filter_res = 1,
		.rc_filter_cap = 1,
		.low_drv = 1,
		.high_drv = 1,
		.bias_volt = 5,
		.bypass_shield_output = false,
	};
	touch_hal_config_t hal_cfg = {
		.power_on_wait_ticks = 0xFF,
		.meas_interval_ticks = 0xF,
		.timeout_ticks = 0,
		.sample_cfg_num = 1,
		.trigger_rise_cnt = 1,
		.output_mode = TOUCH_PAD_OUT_AS_CLOCK,
		.sample_cfg = &sample_cfg,
	};
	touch_hal_config_controller(&hal_cfg);
#endif /* SOC_TOUCH_SENSOR_VERSION */

	uint16_t channel_mask = 0;

	for (int i = 0; i < num_channels; i++) {
		struct esp32_touch_sensor_channel_data *channel_data = &dev_cfg->channel_data[i];
		const struct esp32_touch_sensor_channel_config *channel_cfg =
			&dev_cfg->channel_cfg[i];

		if (!(channel_cfg->channel_num > 0 && channel_cfg->channel_num < TOUCH_CHAN_MAX)) {
			LOG_ERR("Touch %d configuration failed: "
				"Touch channel error",
				i);
			return -EINVAL;
		}

#if SOC_TOUCH_SENSOR_VERSION >= 2
		if (channel_cfg->channel_num == TOUCH_DENOISE_CHANNEL) {
			LOG_ERR("Touch %d configuration failed: "
				"TOUCH0 is internal denoise channel",
				i);
			return -EINVAL;
		}
#endif

		gpio_num_t gpio_num = touch_sensor_channel_io_map[channel_cfg->channel_num];

		rtc_gpio_init(gpio_num);
		rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_DISABLED);
		rtc_gpio_pulldown_dis(gpio_num);
		rtc_gpio_pullup_dis(gpio_num);

#if SOC_TOUCH_SENSOR_VERSION == 1
		touch_ll_set_chan_active_threshold(channel_cfg->channel_num, 0);
		touch_ll_set_charge_speed(channel_cfg->channel_num, TOUCH_CHARGE_SPEED_7);
		touch_ll_set_init_charge_voltage(channel_cfg->channel_num,
						 TOUCH_INIT_CHARGE_VOLT_LOW);
		touch_ll_config_trigger_group1(channel_cfg->channel_num, true);
		touch_ll_config_trigger_group2(channel_cfg->channel_num, true);
		channel_mask |= BIT(channel_cfg->channel_num);
#elif SOC_TOUCH_SENSOR_VERSION == 2
		touch_ll_set_chan_active_threshold(channel_cfg->channel_num,
						   TOUCH_PAD_THRESHOLD_MAX);
		touch_ll_set_charge_speed(channel_cfg->channel_num, TOUCH_CHARGE_SPEED_7);
		touch_ll_set_init_charge_voltage(channel_cfg->channel_num,
						 TOUCH_INIT_CHARGE_VOLT_LOW);
		channel_mask |= BIT(channel_cfg->channel_num);
#elif SOC_TOUCH_SENSOR_VERSION == 3
		touch_ll_set_chan_active_threshold(channel_cfg->channel_num, 0,
						   TOUCH_LL_ACTIVE_THRESH_MAX);
		channel_mask |= BIT(channel_cfg->channel_num);
#endif

		channel_data->status = 0;
#if SOC_TOUCH_SENSOR_VERSION >= 2
		channel_data->last_status = 0;
#endif
		channel_data->dev = dev;

		k_work_init_delayable(&channel_data->work, esp32_touch_sensor_change_deferred);
	}

#if SOC_TOUCH_SENSOR_VERSION == 1
	touch_ll_enable_channel_mask(channel_mask);
	touch_ll_enable_fsm_timer(true);
	touch_ll_start_fsm_repeated_timer();

	for (int i = 0; i < num_channels; i++) {
		const struct esp32_touch_sensor_channel_config *channel_cfg =
			&dev_cfg->channel_cfg[i];
		uint32_t ref_time;

		ref_time = k_uptime_get_32();
		while (!touch_ll_is_measure_done()) {
			if (k_uptime_get_32() - ref_time > 500) {
				return -ETIMEDOUT;
			}
			k_busy_wait(1000);
		}
		uint32_t touch_value;

		touch_ll_read_chan_data(channel_cfg->channel_num, TOUCH_LL_READ_RAW, &touch_value);
		touch_ll_set_chan_active_threshold(channel_cfg->channel_num,
						   touch_value * (100 - channel_cfg->channel_sens) /
							   100);
	}

#elif SOC_TOUCH_SENSOR_VERSION >= 2
	touch_ll_filter_set_filter_mode(dev_cfg->filter_mode);
	touch_ll_filter_set_debounce(dev_cfg->filter_debounce_cnt);
	touch_ll_filter_set_denoise_level(dev_cfg->filter_noise_thr);
	touch_ll_filter_set_jitter_step(dev_cfg->filter_jitter_step);
	touch_ll_filter_set_smooth_mode(dev_cfg->filter_smooth_level);
	touch_ll_filter_enable(true);
#endif /* SOC_TOUCH_SENSOR_VERSION */

	flags = ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, priority)) |
		ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, flags));

#if SOC_TOUCH_SENSOR_VERSION <= 2
	flags |= ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM;
	err = esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, irq), flags, esp32_touch_rtc_isr,
			     (void *)dev, NULL);
#elif SOC_TOUCH_SENSOR_VERSION == 3
	err = esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(touch), 0, irq), flags,
			     esp32_touch_sensor_interrupt_cb, (void *)dev, NULL);
#endif
	if (err) {
		LOG_ERR("Failed to register ISR\n");
		return -EFAULT;
	}

#if SOC_TOUCH_SENSOR_VERSION == 1
	touch_ll_interrupt_clear(TOUCH_LL_INTR_MASK_ALL);
	touch_ll_interrupt_enable(TOUCH_LL_INTR_MASK_ALL);
#elif SOC_TOUCH_SENSOR_VERSION == 2
	touch_ll_enable_channel_mask(channel_mask);
	touch_ll_intr_enable(ESP32_TOUCH_PAD_INTR_MASK);
	touch_ll_enable_fsm_timer(true);
	touch_ll_start_fsm_repeated_timer();
#elif SOC_TOUCH_SENSOR_VERSION == 3
	touch_ll_enable_out_gate(true);
	touch_ll_enable_channel_mask(channel_mask);
	touch_ll_interrupt_clear(TOUCH_LL_INTR_MASK_ALL);
	touch_ll_interrupt_enable(ESP32_TOUCH_INTR_MASK);
	touch_ll_enable_fsm_timer(true);
	touch_ll_start_fsm_repeated_timer();
#endif

	return 0;
}

/* clang-format off */

#define ESP32_TOUCH_SENSOR_CHANNEL_CFG_INIT(node_id)                                               \
	{                                                                                          \
		.channel_num = DT_PROP(node_id, channel_num),                                      \
		.channel_sens = DT_PROP(node_id, channel_sens),                                    \
		.zephyr_code = DT_PROP(node_id, zephyr_code),                                      \
	}

#if SOC_TOUCH_SENSOR_VERSION <= 2
#define ESP32_TOUCH_VOLTAGE_CFG(inst)                                                              \
	.href_microvolt_enum_idx = DT_INST_ENUM_IDX(inst, href_microvolt),                         \
	.lref_microvolt_enum_idx = DT_INST_ENUM_IDX(inst, lref_microvolt),                         \
	.href_atten_microvolt_enum_idx = DT_INST_ENUM_IDX(inst, href_atten_microvolt),
#else
#define ESP32_TOUCH_VOLTAGE_CFG(inst)
#endif

#define ESP32_TOUCH_SENSOR_INIT(inst)                                                              \
	static const struct esp32_touch_sensor_channel_config                                      \
		esp32_touch_sensor_channel_config_##inst[] = {                                     \
			DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(                                     \
				inst, ESP32_TOUCH_SENSOR_CHANNEL_CFG_INIT, (,))};                  \
                                                                                                   \
	static struct esp32_touch_sensor_channel_data                                              \
		esp32_touch_sensor_channel_data_##inst[ARRAY_SIZE(                                 \
			esp32_touch_sensor_channel_config_##inst)];                                \
                                                                                                   \
	static const struct esp32_touch_sensor_config esp32_touch_sensor_config_##inst = {         \
		.debounce_interval_ms = DT_INST_PROP(inst, debounce_interval_ms),                  \
		.num_channels = ARRAY_SIZE(esp32_touch_sensor_channel_config_##inst),              \
		ESP32_TOUCH_VOLTAGE_CFG(inst).filter_mode = DT_INST_PROP(inst, filter_mode),       \
		.filter_debounce_cnt = DT_INST_PROP(inst, filter_debounce_cnt),                    \
		.filter_noise_thr = DT_INST_PROP(inst, filter_noise_thr),                          \
		.filter_jitter_step = DT_INST_PROP(inst, filter_jitter_step),                      \
		.filter_smooth_level = DT_INST_PROP(inst, filter_smooth_level),                    \
		.channel_cfg = esp32_touch_sensor_channel_config_##inst,                           \
		.channel_data = esp32_touch_sensor_channel_data_##inst,                            \
	};                                                                                         \
                                                                                                   \
	static struct esp32_touch_sensor_data esp32_touch_sensor_data_##inst;                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &esp32_touch_sensor_init, NULL,                                \
			      &esp32_touch_sensor_data_##inst, &esp32_touch_sensor_config_##inst,  \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ESP32_TOUCH_SENSOR_INIT)
