/*
 * Copyright (c) 2025, Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/max32664c.h>

#define MAX32664C_BIT_STATUS_NO_ERR   1
#define MAX32664C_BIT_STATUS_DATA_RDY 3
#define MAX32664C_BIT_STATUS_OUT_OVFL 4
#define MAX32664C_BIT_STATUS_IN_OVFL  5
#define MAX32664C_BIT_STATUS_BUSY     6

#define MAX32664C_DEFAULT_CMD_DELAY 10

/** @brief Output formats of the sensor hub.
 */
enum max32664c_output_format {
	MAX32664C_OUT_PAUSE,
	MAX32664C_OUT_SENSOR_ONLY,
	MAX32664C_OUT_ALGORITHM_ONLY,
	MAX32664C_OUT_ALGO_AND_SENSOR,
};

/** @brief Skin contact detection states.
 *  @note The SCD states are only available when the SCD only mode is enabled.
 */
enum max32664c_scd_states {
	MAX32664C_SCD_STATE_UNKNOWN,
	MAX32664C_SCD_STATE_OFF_SKIN,
	MAX32664C_SCD_STATE_ON_OBJECT,
	MAX32664C_SCD_STATE_ON_SKIN,
};

/** @brief LED current structure.
 */
struct max32664c_led_current_t {
	uint8_t adj_flag;
	uint16_t adj_val;
} __packed;

/** @brief SpO2 measurement result structure.
 */
struct max32664c_spo2_meas_t {
	uint8_t confidence;
	uint16_t value;
	uint8_t complete;
	uint8_t low_signal_quality;
	uint8_t motion;
	uint8_t low_pi;
	uint8_t unreliable_r;
	uint8_t state;
} __packed;

/** @brief Extended SpO2 measurement result structure.
 */
struct max32664c_ext_spo2_meas_t {
	uint8_t confidence;
	uint16_t value;
	uint8_t valid_percent;
	uint8_t low_signal_flag;
	uint8_t motion_flag;
	uint8_t low_pi_flag;
	uint8_t unreliable_r_flag;
	uint8_t state;
} __packed;

/** @brief Raw data structure, reported by the sensor hub.
 */
struct max32664c_raw_report_t {
	uint32_t PPG1: 24;
	uint32_t PPG2: 24;
	uint32_t PPG3: 24;
	uint32_t PPG4: 24;
	uint32_t PPG5: 24;
	uint32_t PPG6: 24;
	struct max32664c_acc_data_t acc;
} __packed;

/** @brief SCD only data structure, reported by the sensor hub.
 */
struct max32664c_scd_report_t {
	uint8_t scd_classifier;
} __packed;

/** @brief Algorithm data structure, reported by the sensor hub.
 */
struct max32664c_report_t {
	uint8_t op_mode;
	uint16_t hr;
	uint8_t hr_confidence;
	uint16_t rr;
	uint8_t rr_confidence;
	uint8_t activity_class;
	uint16_t r;
	struct max32664c_spo2_meas_t spo2_meas;
	uint8_t scd_state;
} __packed;

/** @brief Extended algorithm data structure, reported by the sensor hub.
 */
struct max32664c_ext_report_t {
	uint8_t op_mode;
	uint16_t hr;
	uint8_t hr_confidence;
	uint16_t rr;
	uint8_t rr_confidence;
	uint8_t activity_class;

	uint32_t total_walk_steps;
	uint32_t total_run_steps;
	uint32_t total_energy_kcal;
	uint32_t total_amr_kcal;

	struct max32664c_led_current_t led_current_adj1;
	struct max32664c_led_current_t led_current_adj2;
	struct max32664c_led_current_t led_current_adj3;

	uint8_t integration_time_adj_flag;
	uint8_t requested_integration_time;

	uint8_t sampling_rate_adj_flag;
	uint8_t requested_sampling_rate;
	uint8_t requested_sampling_average;

	uint8_t hrm_afe_ctrl_state;
	uint8_t is_high_motion_for_hrm;

	uint8_t scd_state;

	uint16_t r_value;
	struct max32664c_ext_spo2_meas_t spo2_meas;

	uint8_t ibi_offset;
	uint8_t unreliable_orientation_flag;

	uint8_t reserved[2];
} __packed;

/** @brief Device configuration structure.
 */
struct max32664c_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;

#ifdef CONFIG_MAX32664C_USE_INTERRUPT
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work interrupt_work;
#endif /* CONFIG_MAX32664C_USE_INTERRUPT */

	struct gpio_dt_spec mfio_gpio;

	int32_t spo2_calib[3];
	uint16_t motion_time;
	uint16_t motion_threshold;

	uint8_t hr_config[2];
	uint8_t spo2_config[2];
	uint8_t led_current[3];           /**< Initial LED current in mA */
	uint8_t min_integration_time_idx;
	uint8_t min_sampling_rate_idx;
	uint8_t max_integration_time_idx;
	uint8_t max_sampling_rate_idx;
	uint8_t report_period;            /*< Samples report period */

	bool use_max86141;
	bool use_max86161;
};

/** @brief Device runtime data structure.
 */
struct max32664c_data {
	struct max32664c_raw_report_t raw;
	struct max32664c_scd_report_t scd;
	struct max32664c_report_t report;
	struct max32664c_ext_report_t ext;

	enum max32664c_device_mode op_mode; /**< Current device mode */

	uint8_t motion_time;              /**< Motion time in milliseconds */
	uint8_t motion_threshold;         /**< Motion threshold in milli-g */
	uint8_t led_current[3];           /**< LED current in mA */
	uint8_t min_integration_time_idx;
	uint8_t min_sampling_rate_idx;
	uint8_t max_integration_time_idx;
	uint8_t max_sampling_rate_idx;
	uint8_t report_period;            /*< Samples report period */
	uint8_t afe_id;
	uint8_t accel_id;
	uint8_t hub_ver[3];

	/* Internal */
	struct k_thread thread;
	k_tid_t thread_id;
	bool is_thread_running;

#ifdef CONFIG_MAX32664C_USE_STATIC_MEMORY
	/** @brief This buffer is used to read all available messages from the sensor hub plus the
	 * status byte. The buffer size is defined by the CONFIG_MAX32664C_SAMPLE_BUFFER_SIZE
	 * Kconfig and the largest possible message. The buffer must contain enough space to store
	 * all available messages at every time because it is not possible to read a single message
	 * from the sensor hub.
	 */
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	uint8_t max32664_i2c_buffer[(CONFIG_MAX32664C_SAMPLE_BUFFER_SIZE *
				     (sizeof(struct max32664c_raw_report_t) +
				      sizeof(struct max32664c_ext_report_t))) +
				    1];
#else
	uint8_t max32664_i2c_buffer[(CONFIG_MAX32664C_SAMPLE_BUFFER_SIZE *
				     (sizeof(struct max32664c_raw_report_t) +
				      sizeof(struct max32664c_report_t))) +
				    1];
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
#else
	uint8_t *max32664_i2c_buffer;
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MAX32664C_THREAD_STACK_SIZE);

	struct k_msgq raw_report_queue;
	struct k_msgq scd_report_queue;

#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	struct k_msgq ext_report_queue;
#else
	struct k_msgq report_queue;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */

#ifdef CONFIG_MAX32664C_USE_STATIC_MEMORY
	uint8_t raw_report_queue_buffer[CONFIG_MAX32664C_QUEUE_SIZE *
					sizeof(struct max32664c_raw_report_t)];
	uint8_t scd_report_queue_buffer[CONFIG_MAX32664C_QUEUE_SIZE *
					sizeof(struct max32664c_scd_report_t)];

#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	uint8_t ext_report_queue_buffer[CONFIG_MAX32664C_QUEUE_SIZE *
					(sizeof(struct max32664c_raw_report_t) +
					 sizeof(struct max32664c_ext_report_t))];
#else
	uint8_t report_queue_buffer[CONFIG_MAX32664C_QUEUE_SIZE *
				    (sizeof(struct max32664c_raw_report_t) +
				     sizeof(struct max32664c_report_t))];
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY*/
};

/** @brief          Enable / Disable the accelerometer.
 *                  NOTE: This code is untested and may not work as expected.
 *  @param dev      Pointer to device
 *  @param enable   Enable / Disable
 *  @return         0 when successful
 */
int max32664c_acc_enable(const struct device *dev, bool enable);

/** @brief      Background worker for reading the sensor hub.
 *  @param dev  Pointer to device
 */
void max32664c_worker(const struct device *dev);

/** @brief          Read / write data from / to the sensor hub.
 *  @param dev      Pointer to device
 *  @param tx_buf   Pointer to transmit buffer
 *  @param tx_len   Length of transmit buffer
 *  @param rx_buf   Pointer to receive buffer
 *                  NOTE: The buffer must be large enough to store the response and the status byte!
 *  @param rx_len   Length of the receive buffer
 *  @param delay    Command delay (milliseconds)
 *  @return         0 when successful
 */
int max32664c_i2c_transmit(const struct device *dev, uint8_t *tx_buf, uint8_t tx_len,
			   uint8_t *rx_buf, uint32_t rx_len, uint16_t delay);

/** @brief      Run a basic initialization on the sensor hub.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
int max32664c_init_hub(const struct device *dev);

#if CONFIG_MAX32664C_USE_INTERRUPT
/** @brief      Initialize the interrupt support for the sensor hub.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
int max32664c_init_interrupt(const struct device *dev);
#endif /* CONFIG_MAX32664C_USE_INTERRUPT */
