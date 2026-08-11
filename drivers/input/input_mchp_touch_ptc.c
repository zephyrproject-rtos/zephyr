/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <touch_api_ptc.h>
#include <zephyr/input/input_mchp_touch_api.h>

#define DT_DRV_COMPAT microchip_ptc_controller

LOG_MODULE_REGISTER(input_mchp_touch_ptc, CONFIG_INPUT_LOG_LEVEL);

#define DATASTREAMER_DATA_BUFFER_SIZE 19

#define STD_THREAD_OPTION 0u
#define IRQ_FLAG_IDX      0
#define ISR_PRIORITY_IDX  1

/* Touch Error Codes */
#define ACQ_MODULE_ERROR           0u
#define FREQ_HOP_AUTO_MODULE_ERROR 1u
#define KEY_MODULE_ERROR           2u
#define SCROLLER_MODULE_ERROR      3u

#define DEF_NUM_CHANNELS DT_INST_PROP(0, def_num_channels)
#define DEF_NUM_SENSORS  DT_INST_PROP(0, def_num_sensors)
#define NUM_FREQ_STEPS   DT_INST_PROP(0, num_freq_steps)

#define DEF_NUM_SCROLLERS DT_INST_PROP(0, def_num_scrollers)

BUILD_ASSERT(NUM_FREQ_STEPS == DT_INST_PROP_LEN(0, def_median_filter_frequencies),
	     "def-median-filter-frequencies does not match num-freq-steps length");

struct mchp_touch_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct mchp_touch_config {
	const struct mchp_touch_clock clock_config;
	const uint8_t run_in_standby;
	const struct pinctrl_dev_config *pcfg;
#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
	const struct device *uart_dev;
	const uint8_t datastreamer_data[DATASTREAMER_DATA_BUFFER_SIZE];
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */
};

struct mchp_touch_data {
	volatile uint8_t time_to_measure_touch_var;
	volatile uint8_t touch_postprocess_request;
	uint8_t module_error_code;
	bool is_touch;
	/* Acquistion Module */
	touch_acq_signal_t touch_acq_signals_raw[DEF_NUM_CHANNELS];
	qtm_acq_node_group_config_t ptc_qtlib_acq_gen1;
	qtm_acq_node_data_t ptc_qtlib_node_stat1[DEF_NUM_CHANNELS];
	qtm_acq_node_config_t ptc_seq_node_cfg1[DEF_NUM_CHANNELS];
	qtm_acquisition_control_t qtlib_acq_set1;

	/* Frequency Hop Auto tune Module */
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE)
	/* Buffer used with various noise filtering functions */
	uint16_t noise_filter_buffer[DEF_NUM_CHANNELS * NUM_FREQ_STEPS];
	uint8_t freq_hop_delay_selection[NUM_FREQ_STEPS];
	uint8_t freq_hop_autotune_counters[NUM_FREQ_STEPS];
	qtm_freq_hop_autotune_config_t qtm_freq_hop_autotune_config1;
	qtm_freq_hop_autotune_data_t qtm_freq_hop_autotune_data1;
	qtm_freq_hop_autotune_control_t qtm_freq_hop_autotune_control1;
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE */

	/* Keys Module */
	qtm_touch_key_group_config_t qtlib_key_grp_config_set1;
	qtm_touch_key_group_data_t qtlib_key_grp_data_set1;
	qtm_touch_key_data_t qtlib_key_data_set1[DEF_NUM_SENSORS];
	qtm_touch_key_config_t qtlib_key_configs_set1[DEF_NUM_SENSORS];
	qtm_touch_key_control_t qtlib_key_set1;

	/* Scroller Module */
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	qtm_scroller_data_t qtm_scroller_data1[DEF_NUM_SCROLLERS];
	qtm_scroller_group_data_t qtm_scroller_group_data1;
	qtm_scroller_group_config_t qtm_scroller_group_config1;
	qtm_scroller_config_t qtm_scroller_config1[DEF_NUM_SCROLLERS];
	qtm_scroller_control_t qtm_scroller_control1;
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */
};

static void touch_process(void *p1, void *p2, void *p3);
static void touch_timer_handler(struct k_timer *timer);
#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
static void datastreamer_output(const struct device *dev);
static int datastreamer_init(const struct device *dev);
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */

K_TIMER_DEFINE(my_timer, touch_timer_handler, NULL);
K_THREAD_STACK_DEFINE(touch_process_th_stack, CONFIG_INPUT_MCHP_TOUCH_THREAD_STACK_SIZE);

static struct k_thread touch_process_th_struct;
static k_tid_t touch_process_th_id;

static int conf_touch_per_clk(const struct device *dev)
{
	int ret;
	const struct mchp_touch_config *config = dev->config;

	if (config->clock_config.gclk_sys != NULL) {
		ret = clock_control_on(config->clock_config.clock_dev,
					  config->clock_config.gclk_sys);
		if (ret < 0) {
			LOG_ERR("Touch::Failed to enable gclock: %d", ret);
			return ret;
		}
	}

	if (config->clock_config.mclk_sys != NULL) {
		ret = clock_control_on(config->clock_config.clock_dev,
					  config->clock_config.mclk_sys);
		if (ret < 0) {
			LOG_ERR("Touch::Failed to enable mclock: %d", ret);
			return ret;
		}
	}

	return 0;
}

static void touch_timer_config(void)
{
	k_timer_start(&my_timer,
		K_NO_WAIT,
		K_MSEC(DT_INST_PROP(0, def_touch_measurement_period_ms)));
}

static touch_ret_t touch_sensors_config(const struct device *dev)
{
	uint16_t sensor_nodes;
	touch_ret_t ret;
	struct mchp_touch_data *data = dev->data;

	ret = qtm_ptc_init_acquisition_module(&data->qtlib_acq_set1);
	if (ret != TOUCH_SUCCESS) {
		LOG_ERR("Touch::Failed to init acquisition module: %u", (uint8_t)ret);
		return ret;
	}

	ret = qtm_ptc_qtlib_assign_signal_memory(&data->touch_acq_signals_raw[0]);
	if (ret != TOUCH_SUCCESS) {
		LOG_ERR("Touch::Failed to assign signal memory: %u", (uint8_t)ret);
		return ret;
	}

	for (sensor_nodes = 0u; sensor_nodes < (uint16_t)DEF_NUM_CHANNELS; sensor_nodes++) {
		ret = qtm_enable_sensor_node(&data->qtlib_acq_set1, sensor_nodes);
		if (ret != TOUCH_SUCCESS) {
			LOG_ERR("Touch::Failed to enable the %dth sensor node: %u", sensor_nodes,
				(uint8_t)ret);
			return ret;
		}

		ret = qtm_calibrate_sensor_node(&data->qtlib_acq_set1, sensor_nodes);
		if (ret != TOUCH_SUCCESS) {
			LOG_ERR("Touch::Failed to calibrate the %dth sensor node: %u", sensor_nodes,
				(uint8_t)ret);
			return ret;
		}

		ret = qtm_init_sensor_key(&data->qtlib_key_set1,
					  (uint8_t)sensor_nodes,
					  &data->ptc_qtlib_node_stat1[sensor_nodes]);
		if (ret != TOUCH_SUCCESS) {
			LOG_ERR("Touch::Failed to init the %dth sensor key: %u", sensor_nodes,
					(uint8_t)ret);
		}
	}
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	ret = qtm_init_scroller_module(&data->qtm_scroller_control1);
	if (ret != TOUCH_SUCCESS) {
		LOG_ERR("Touch::Failed to init the scroller: %u", (uint8_t)ret);
		return ret;
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */

	return TOUCH_SUCCESS;
}

/*
 * Callback function called after the completion of
 * measurement cycle. This function sets the post processing request
 * flag to trigger the post processing.
 */
static void qtm_measure_complete_callback(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct mchp_touch_data *data = dev->data;

	data->touch_postprocess_request = 1u;
}

/*
 * Callback function called after the completion of
 * post processing. This function is called only when there is error
 * Notes  : Derived Module_error_codes:
 *             Acquisition module error = 1
 *             post processing module1 error = 2
 *             post processing module2 error = 3
 *             and so on
 */
static void qtm_error_callback(const struct device *dev, uint8_t error)
{
	struct mchp_touch_data *data = dev->data;

	data->module_error_code = error + 1u;

#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
	datastreamer_output(dev);
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */

	LOG_ERR("Touch::Error Callback: %d", error);
}

static int touch_init(const struct device *dev)
{
	int ret;

	ret = (int)touch_sensors_config(dev);
	if (ret != 0) {
		LOG_ERR("Touch::Failed to configure touch sensor: %d", ret);
		return ret;
	}

	touch_timer_config();

	return 0;
}

static void check_key_status(const struct device *dev)
{
	struct mchp_touch_data *data = dev->data;

	if ((data->qtlib_key_set1.qtm_touch_key_group_data->qtm_keys_status &
		QTM_KEY_REBURST) == QTM_KEY_REBURST) {
		data->time_to_measure_touch_var = 1u;
		if ((data->qtlib_key_set1.qtm_touch_key_group_data->qtm_keys_status &
			QTM_KEY_DETECT) == QTM_KEY_DETECT) {
			/* Measurement is in Progress therefore
			 * report to callback after it completes
			 */
			data->is_touch = true;
		}
	} else if ((data->qtlib_key_set1.qtm_touch_key_group_data->qtm_keys_status &
			QTM_KEY_DETECT) == QTM_KEY_DETECT) {
		data->is_touch = false;
		input_report_key(dev,
				 INPUT_BTN_TOUCH,
				 0,
				 true,
				 K_FOREVER);
	} else {
		if (data->is_touch == true) {
			/* Measurement completed
			 * report Filterout to No_detect state
			 * to application
			 */
			data->is_touch = false;
			input_report_key(dev,
					 INPUT_BTN_TOUCH,
					 0,
					 true,
					 K_FOREVER);
		}
	}
}

static void post_process(const struct device *dev)
{
	touch_ret_t ret;
	struct mchp_touch_data *data = dev->data;

	ret = qtm_acquisition_process();
	if (ret != TOUCH_SUCCESS) {
		qtm_error_callback(dev, ACQ_MODULE_ERROR);
		return;
	}
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE)
	ret = qtm_freq_hop_autotune(
		&data->qtm_freq_hop_autotune_control1);
	if (ret != TOUCH_SUCCESS) {
		qtm_error_callback(dev, FREQ_HOP_AUTO_MODULE_ERROR);
		/* proceed to run further */
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE */
	ret = qtm_key_sensors_process(&data->qtlib_key_set1);
	if (ret != TOUCH_SUCCESS) {
		qtm_error_callback(dev, KEY_MODULE_ERROR);
		/* proceed to run further */
	}
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	ret = qtm_scroller_process(&data->qtm_scroller_control1);
	if (ret != TOUCH_SUCCESS) {
		qtm_error_callback(dev, SCROLLER_MODULE_ERROR);
		/* proceed to run further */
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */
	check_key_status(dev);
}

/*
 * Main processing function of touch library. This function initiates the
 * acquisition, calls post processing after the acquistion complete and
 * sets the flag for next measurement based on the sensor status.
 */
static void touch_process(void *p1, void *p2, void *p3)
{
	touch_ret_t touch_ret;
	const struct device *dev = p1;
	struct mchp_touch_data *data = dev->data;

	while (1) {
		/* check the time_to_measure_touch for Touch Acquisition */
		if (data->time_to_measure_touch_var == 1u) {
			/* Do the acquisition */
			touch_ret = qtm_ptc_start_measurement_seq(&data->qtlib_acq_set1,
								  qtm_measure_complete_callback);
			/* if the Acquistion request was successful then clear the request flag */
			if (TOUCH_SUCCESS == touch_ret) {
				/* Clear the Measure request flag */
				data->time_to_measure_touch_var = 0u;
			}
		}

		/* check the flag for node level post processing */
		if (data->touch_postprocess_request == 1u) {
			data->touch_postprocess_request = 0u;
			post_process(dev);
#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
			datastreamer_output(dev);
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */
		}

		k_thread_suspend(touch_process_th_id);
	}
}

static void touch_timer_handler(struct k_timer *timer)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct mchp_touch_data *data = dev->data;

	data->time_to_measure_touch_var = 1u;
	qtm_update_qtlib_timer(DT_INST_PROP(0, def_touch_measurement_period_ms));

	k_thread_resume(touch_process_th_id);
}

static void ptc_handler(void)
{
	qtm_ptc_clear_int();
	qtm_ptc_handler_eoc();

	k_thread_resume(touch_process_th_id);
}

static int touch_driver_init(const struct device *dev)
{
	int ret;

	const struct mchp_touch_config *config = dev->config;
	struct mchp_touch_data *data = dev->data;

	data->qtlib_acq_set1.qtm_acq_node_group_config = &data->ptc_qtlib_acq_gen1;
	data->qtlib_acq_set1.qtm_acq_node_config = &data->ptc_seq_node_cfg1[0];
	data->qtlib_acq_set1.qtm_acq_node_data = &data->ptc_qtlib_node_stat1[0];

	data->qtlib_key_set1.qtm_touch_key_group_data = &data->qtlib_key_grp_data_set1;
	data->qtlib_key_set1.qtm_touch_key_group_config = &data->qtlib_key_grp_config_set1;
	data->qtlib_key_set1.qtm_touch_key_data = &data->qtlib_key_data_set1[0];
	data->qtlib_key_set1.qtm_touch_key_config = &data->qtlib_key_configs_set1[0];

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE)
	data->qtm_freq_hop_autotune_config1.freq_option_select =
		&data->ptc_qtlib_acq_gen1.freq_option_select;
	data->qtm_freq_hop_autotune_config1.median_filter_freq = &data->freq_hop_delay_selection[0];
	data->qtm_freq_hop_autotune_data1.filter_buffer = &data->noise_filter_buffer[0];
	data->qtm_freq_hop_autotune_data1.qtm_acq_node_data = &data->ptc_qtlib_node_stat1[0];
	data->qtm_freq_hop_autotune_data1.freq_tune_count_ins =
		&data->freq_hop_autotune_counters[0];
	data->qtm_freq_hop_autotune_control1.qtm_freq_hop_autotune_data =
		&data->qtm_freq_hop_autotune_data1;
	data->qtm_freq_hop_autotune_control1.qtm_freq_hop_autotune_config =
		&data->qtm_freq_hop_autotune_config1;
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE */

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	data->qtm_scroller_group_config1.qtm_touch_key_data = &data->qtlib_key_data_set1[0];
	data->qtm_scroller_group_config1.num_scrollers = DEF_NUM_SCROLLERS;
	data->qtm_scroller_control1.qtm_scroller_group_data = &data->qtm_scroller_group_data1;
	data->qtm_scroller_control1.qtm_scroller_group_config = &data->qtm_scroller_group_config1;
	data->qtm_scroller_control1.qtm_scroller_data = &data->qtm_scroller_data1[0];
	data->qtm_scroller_control1.qtm_scroller_config = &data->qtm_scroller_config1[0];
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */

	ret = conf_touch_per_clk(dev);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Touch::Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	IRQ_CONNECT(DT_INST_PROP_BY_IDX(0, interrupts, IRQ_FLAG_IDX),     /* IRQ Line */
		    DT_INST_PROP_BY_IDX(0, interrupts, ISR_PRIORITY_IDX), /* interrupt priority */
		    ptc_handler,                                          /* Handler */
		    NULL,          /* Handler Parameter (optional) */
		    IRQ_FLAG_IDX); /* Flags */

	irq_enable(DT_INST_PROP_BY_IDX(0, interrupts, IRQ_FLAG_IDX)); /* IRQ Line */

	ret = touch_init(dev);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
	ret = datastreamer_init(dev);
	if (ret != 0) {
		return ret;
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */

	touch_process_th_id = k_thread_create(&touch_process_th_struct,
					      touch_process_th_stack,
					      K_THREAD_STACK_SIZEOF(touch_process_th_stack),
					      touch_process,
					      (void *)dev,
					      NULL,
					      NULL,
					      CONFIG_INPUT_MCHP_TOUCH_THREAD_PRIORITY,
					      STD_THREAD_OPTION,
					      K_NO_WAIT);

#if defined(CONFIG_THREAD_NAME)
	k_thread_name_set(touch_process_th_id, "touch_process");
#endif /* CONFIG_THREAD_NAME */

	k_thread_suspend(touch_process_th_id);

	return 0;
}

#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
static int datastreamer_init(const struct device *dev)
{
	int ret;
	const struct mchp_touch_config *config = dev->config;

	ret = device_is_ready(config->uart_dev);
	if (ret < 0) {
		LOG_ERR("Touch::Failed to open datastreamer port: %d", ret);
		return ret;
	}

	return 0;
}

static void datastreamer_transmit(const struct device *dev, uint8_t data_byte)
{
	const struct mchp_touch_config *config = dev->config;

	/* Write the data bye */
	uart_poll_out(config->uart_dev, data_byte);
}

static void datastreamer_output(const struct device *dev)
{
	int16_t temp_int_calc;
	static uint8_t sequence;
	uint16_t u16temp_output;
	uint8_t u8temp_output, send_header;
	uint16_t count_bytes_out;

	struct mchp_touch_data *touch_data = dev->data;
	const struct mchp_touch_config *touch_config = dev->config;

	send_header = sequence & 0x0fu;
	if (send_header == 0u) {
		for (count_bytes_out = 0u;
		     count_bytes_out < (uint16_t)sizeof(touch_config->datastreamer_data);
		     count_bytes_out++) {
			datastreamer_transmit(dev,
					      touch_config->datastreamer_data[count_bytes_out]);
		}
	}

	/* Start token */
	datastreamer_transmit(dev, 0x55u);

	/* Frame Start */
	datastreamer_transmit(dev, sequence);

	for (count_bytes_out = 0u; count_bytes_out < (uint16_t)DEF_NUM_SENSORS; count_bytes_out++) {
		/* Signals */
		u16temp_output = get_sensor_node_signal(dev, count_bytes_out);
		datastreamer_transmit(dev, (uint8_t)u16temp_output);
		datastreamer_transmit(dev, (uint8_t)(u16temp_output >> 8u));

		/* Reference */
		u16temp_output = get_sensor_node_reference(dev, count_bytes_out);
		datastreamer_transmit(dev, (uint8_t)u16temp_output);
		datastreamer_transmit(dev, (uint8_t)(u16temp_output >> 8u));

		/* Touch delta */
		temp_int_calc = (int16_t)get_sensor_node_signal(dev, count_bytes_out);
		temp_int_calc -= (int16_t)get_sensor_node_reference(dev, count_bytes_out);
		u16temp_output = (uint16_t)(temp_int_calc);
		datastreamer_transmit(dev, (uint8_t)u16temp_output);
		datastreamer_transmit(dev, (uint8_t)(u16temp_output >> 8u));

		/* Comp Caps */
		u16temp_output = get_sensor_cc_val(dev, count_bytes_out);
		datastreamer_transmit(dev, (uint8_t)u16temp_output);
		datastreamer_transmit(dev, (uint8_t)(u16temp_output >> 8u));

#if (ACQ_MODULE_AUTOTUNE_OUTPUT == 1)

#if (DEF_PTC_CAL_OPTION == CAL_AUTO_TUNE_CSD)
		/* CSD */
		u8temp_output = ptc_seq_node_cfg1[count_bytes_out].node_csd;
		datastreamer_transmit(dev, u8temp_output);
#else
		/* Prescalar */
		u8temp_output = NODE_PRSC(ptc_seq_node_cfg1[count_bytes_out].node_rsel_prsc);
		datastreamer_transmit(dev, u8temp_output);
#endif /* DEF_PTC_CAL_OPTION == CAL_AUTO_TUNE_CSD */

#endif /* (ACQ_MODULE_AUTOTUNE_OUTPUT == 1) */
		/* State */
		u8temp_output = get_sensor_state(dev, count_bytes_out);
		if (0u != (u8temp_output & 0x80u)) {
			datastreamer_transmit(dev, 0x01u);
		} else {
			datastreamer_transmit(dev, 0x00u);
		}

		/* Threshold */
		datastreamer_transmit(
			dev, touch_data->qtlib_key_configs_set1[count_bytes_out].channel_threshold);
	}

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	for (count_bytes_out = 0u; count_bytes_out < DEF_NUM_SCROLLERS; count_bytes_out++) {

		/* State */
		u8temp_output = touch_data->qtm_scroller_data1[count_bytes_out].scroller_status;
		if (0u != (u8temp_output & 0x01u)) {
			datastreamer_transmit(dev, 0x01u);
		} else {
			datastreamer_transmit(dev, 0x00u);
		}

		/* Delta */
		u16temp_output = touch_data->qtm_scroller_data1[count_bytes_out].contact_size;
		datastreamer_transmit(dev, (uint8_t)u16temp_output);
		datastreamer_transmit(dev, (uint8_t)(u16temp_output >> 8u));

		/* Threshold */
		u16temp_output =
			touch_data->qtm_scroller_config1[count_bytes_out].contact_min_threshold;
		datastreamer_transmit(dev, (uint8_t)u16temp_output);
		datastreamer_transmit(dev, (uint8_t)(u16temp_output >> 8u));

		/* filtered position */
		u16temp_output = touch_data->qtm_scroller_data1[count_bytes_out].position;
		datastreamer_transmit(dev, (uint8_t)(u16temp_output & 0x00FFu));
		datastreamer_transmit(dev, (uint8_t)((u16temp_output & 0xFF00u) >> 8u));
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE)
	/* Frequency selection - from acq module */
	datastreamer_transmit(dev, *touch_data->qtm_freq_hop_autotune_config1.freq_option_select);

	for (count_bytes_out = 0u; count_bytes_out < NUM_FREQ_STEPS; count_bytes_out++) {
		/* Frequencies */
		datastreamer_transmit(dev, touch_data->qtm_freq_hop_autotune_config1
						   .median_filter_freq[count_bytes_out]);
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE */

	/* Other Debug Parameters */
	datastreamer_transmit(dev, touch_data->module_error_code);

	/* Frame End */
	datastreamer_transmit(dev, sequence++);

	/* End token */
	datastreamer_transmit(dev, (uint8_t)~0x55);
}
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */

/*
 * Public Function definition
 * Note:
 *    This driver is abstracted through the input driver layer, as there are no
 *    direct function mappings available for the public APIs. Therefore, the
 *    following functions are exposed to the application through the
 *    Microchip-defined header file: "zephyr/input/input_mchp_touch_api.h"
 */
uint16_t get_sensor_node_signal(const struct device *dev, uint16_t sensor_node)
{
	uint16_t node_signal = 0u;
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		node_signal = data->ptc_qtlib_node_stat1[sensor_node].node_acq_signals;
	}

	return node_signal;
}

void update_sensor_node_signal(const struct device *dev, uint16_t sensor_node, uint16_t new_signal)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		data->ptc_qtlib_node_stat1[sensor_node].node_acq_signals = new_signal;
	}
}

uint16_t get_sensor_node_reference(const struct device *dev, uint16_t sensor_node)
{
	uint16_t reference = 0u;
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		reference = data->qtlib_key_data_set1[sensor_node].channel_reference;
	}

	return reference;
}

void update_sensor_node_reference(const struct device *dev, uint16_t sensor_node,
				  uint16_t new_reference)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		data->qtlib_key_data_set1[sensor_node].channel_reference = new_reference;
	}
}

uint16_t get_sensor_cc_val(const struct device *dev, uint16_t sensor_node)
{
	uint16_t CC = 0u; /* CC - Compensation Capacitance */
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		CC = data->ptc_qtlib_node_stat1[sensor_node].node_comp_caps;
	}

	return CC;
}

void update_sensor_cc_val(const struct device *dev, uint16_t sensor_node, uint16_t new_cc_value)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		data->ptc_qtlib_node_stat1[sensor_node].node_comp_caps = new_cc_value;
	}
}

uint8_t get_sensor_state(const struct device *dev, uint16_t sensor_node)
{
	uint8_t sensor_state = QTM_KEY_STATE_DISABLE;
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		sensor_state = data->qtlib_key_set1.qtm_touch_key_data[sensor_node].sensor_state;
	}

	return sensor_state;
}

void update_sensor_state(const struct device *dev, uint16_t sensor_node, uint8_t new_state)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		data->qtlib_key_set1.qtm_touch_key_data[sensor_node].sensor_state = new_state;
	}
}

void calibrate_node(const struct device *dev, uint16_t sensor_node)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;

		/* Calibrate Node */
		if (qtm_calibrate_sensor_node(&data->qtlib_acq_set1, sensor_node) !=
		    TOUCH_SUCCESS) {
			LOG_ERR("Acquisition Calibration Failure::sensor_node = %u", sensor_node);
		}

		/* Initialize key */
		if (qtm_init_sensor_key(&data->qtlib_key_set1, (uint8_t)sensor_node,
					&data->ptc_qtlib_node_stat1[sensor_node]) !=
		    TOUCH_SUCCESS) {
			LOG_ERR("Button Calibration Failure::sensor_node = %u", sensor_node);
		}
	}
}

void suspend_sensor(const struct device *dev, uint16_t sensor_node)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		if (qtm_key_suspend(sensor_node, &data->qtlib_key_set1) != TOUCH_SUCCESS) {
			LOG_ERR("Failed to suspend the sensor[%u]", sensor_node);
		}
	}
}

void resume_sensor(const struct device *dev, uint16_t sensor_node)
{
	struct mchp_touch_data *data = NULL;

	if ((dev != NULL) && (sensor_node < DEF_NUM_CHANNELS)) {
		data = dev->data;
		if (qtm_key_resume(sensor_node, &data->qtlib_key_set1) != TOUCH_SUCCESS) {
			LOG_ERR("Failed to resume the suspended sensor[%u]", sensor_node);
		}
	}
}

uint8_t get_def_no_of_sensors(const struct device *dev)
{
	uint8_t no_sensors = 0u;
	struct mchp_touch_data *data = NULL;

	if (dev != NULL) {
		data = dev->data;
		no_sensors = data->ptc_qtlib_acq_gen1.num_sensor_nodes;
	}

	return no_sensors;
}

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
uint8_t get_scroller_state(const struct device *dev, uint16_t sensor_node)
{
	struct mchp_touch_data *data = dev->data;

	return data->qtm_scroller_control1.qtm_scroller_data[sensor_node].scroller_status;
}

uint16_t get_scroller_position(const struct device *dev, uint16_t sensor_node)
{
	struct mchp_touch_data *data = dev->data;

	return data->qtm_scroller_control1.qtm_scroller_data[sensor_node].position;
}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */

#define GET_X_OR(node_id, prop, idx) X((DT_PROP_BY_IDX(node_id, prop, idx)))
#define GET_Y_OR(node_id, prop, idx) Y((DT_PROP_BY_IDX(node_id, prop, idx)))

#define GET_NODE_PARAM(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, x_mask), (                                           \
		{                                                                                  \
			.node_xmask = DT_FOREACH_PROP_ELEM_SEP(node_id, x_mask, GET_X_OR, (|)),    \
			.node_ymask = DT_FOREACH_PROP_ELEM_SEP(node_id, y_mask, GET_Y_OR, (|)),    \
			.node_csd = DT_PROP(node_id, csd),                                         \
			.node_rsel_prsc = ((DT_ENUM_IDX(node_id, rselect) << 4u) |                 \
					    DT_ENUM_IDX(node_id, prescaler)),                      \
			.node_gain = ((DT_ENUM_IDX(node_id, digital_gain) << 4u) |                 \
				       DT_ENUM_IDX(node_id, analog_gain)),                         \
			.node_oversampling = DT_ENUM_IDX(node_id, filter_level)                    \
		},                                                                                 \
	), (/* do nothing... */))

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
#define GET_SCROLLER_PARAM(node_id)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, scroller_type), (                                    \
		{                                                                                  \
			.type = DT_ENUM_IDX(node_id, scroller_type),                               \
			.start_key = DT_PROP(node_id, start_key),                                  \
			.number_of_keys = DT_PROP(node_id, number_of_keys),                        \
			.resol_deadband = SCROLLER_RESOL_DEADBAND(                                 \
				DT_ENUM_IDX(node_id, scroller_resolution),                         \
				DT_ENUM_IDX(node_id, scr_db_percent)),                             \
			.position_hysteresis = DT_PROP(node_id, position_hysteresis),              \
			.contact_min_threshold = DT_PROP(node_id, contact_min_threshold)           \
		},                                                                                 \
	), (/* do nothing... */))
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */

#define GET_KEY_PARAM(node_id)                                                             \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, threshold), (                                \
		{                                                                          \
			.channel_threshold = DT_PROP(node_id, threshold),                  \
			.channel_hysteresis = DT_ENUM_IDX(node_id, hysteresis),            \
			.channel_aks_group = DT_ENUM_IDX(node_id, aks_group),              \
		},                                                                         \
	), (/* do nothing... */))

/* Macro to get the corresponding clock if available, otherwise NULL */
#define TOUCH_CLK_SYS(inst, clk)                                                           \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(inst, clk),                                    \
		((void *)DT_INST_CLOCKS_CELL_BY_NAME(inst, clk, subsystem)),               \
		(NULL))

#define TOUCH_ACQ_GEN_INIT(inst) {                                                         \
	.num_sensor_nodes = DEF_NUM_CHANNELS,                                              \
	.acq_sensor_type = DT_INST_PROP(inst, def_sensor_type),                            \
	.calib_option_select =                                                             \
		((DT_INST_ENUM_IDX(inst, def_ptc_tau_target) << CAL_CHRG_TIME_POS) |       \
		DT_INST_ENUM_IDX(inst, def_ptc_cal_option)),                               \
	.freq_option_select = DT_INST_ENUM_IDX(inst, def_sel_freq_init),                   \
	.ptc_interrupt_priority = DT_INST_PROP_BY_IDX(inst, interrupts, ISR_PRIORITY_IDX)  \
}

/* this pin control define should be at top of the touch_config structure definition */
PINCTRL_DT_INST_DEFINE(0);

static struct mchp_touch_config touch_config = {
	.clock_config = {
		 .clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),
		 .mclk_sys = TOUCH_CLK_SYS(0, mclk_ptc),
		 .gclk_sys = TOUCH_CLK_SYS(0, gclk_ptc)
	},
	.run_in_standby = DT_INST_PROP(0, run_in_standby_en),
#if defined(CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR)
	.uart_dev = DEVICE_DT_GET(DT_INST_PROP(0, data_streamer_port)),
	.datastreamer_data = {0x5F, 0xB4, 0x00, 0x86, 0x4A, 0x03, 0xEB, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0xAA, 0x55, 0x01, 0x6E, 0xA0},
#endif /* CONFIG_INPUT_MCHP_TOUCH_DATASTREAMER_UNI_DIR */
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0)
};

static struct mchp_touch_data touch_data = {
	.is_touch = false,
	.time_to_measure_touch_var = 0u,
	.touch_postprocess_request = 0u,
	.module_error_code = 0u,
	.ptc_qtlib_acq_gen1 = TOUCH_ACQ_GEN_INIT(0),
	.ptc_seq_node_cfg1 = {DT_INST_FOREACH_CHILD(0, GET_NODE_PARAM)},
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE)
	.freq_hop_delay_selection = {DT_INST_ENUM_IDX(0, def_median_filter_frequencies)},
	.qtm_freq_hop_autotune_config1 = {
		 .num_sensors = DEF_NUM_CHANNELS,
		 .num_freqs = NUM_FREQ_STEPS,
		 .enable_freq_autotune = DT_INST_PROP(0, def_freq_autotune_enable),
		 .max_variance_limit = DT_INST_PROP(0, freq_autotune_max_variance),
		 .autotune_count_in_limit = DT_INST_PROP(0, freq_autotune_count_in)
	},
	.qtm_freq_hop_autotune_data1 = {.module_status = 0u, .current_freq = 0u},
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE */
	.qtlib_key_grp_config_set1 = {
		DEF_NUM_SENSORS,
		DT_INST_PROP(0, def_touch_det_int),
		DT_INST_PROP(0, def_max_on_duration),
		DT_INST_PROP(0, def_anti_tch_det_int),
		DT_INST_PROP(0, def_tch_drift_rate),
		DT_INST_PROP(0, def_anti_tch_drift_rate),
		DT_INST_PROP(0, def_drift_hold_time),
		DT_INST_ENUM_IDX(0, def_anti_tch_recal_thrshld),
		DT_INST_ENUM_IDX(0, def_reburst_mode)
	},
	.qtlib_key_configs_set1 = {DT_INST_FOREACH_CHILD(0, GET_KEY_PARAM)},
#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	.qtm_scroller_config1 = {DT_INST_FOREACH_CHILD(0, GET_SCROLLER_PARAM)}
#endif
};

DEVICE_DT_INST_DEFINE(0, touch_driver_init, NULL, &touch_data, &touch_config, POST_KERNEL,
		      CONFIG_INPUT_INIT_PRIORITY, NULL);
