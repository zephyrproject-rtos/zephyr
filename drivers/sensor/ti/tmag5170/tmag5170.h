/*
 * Copyright (c) 2023 Michal Morsisko
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_H_
#define ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#include "tmag5170_bus.h"

#define TMAG5170_REG_DEVICE_CONFIG     0x0U
#define TMAG5170_REG_SENSOR_CONFIG     0x1U
#define TMAG5170_REG_SYSTEM_CONFIG     0x2U
#define TMAG5170_REG_ALERT_CONFIG      0x3U
#define TMAG5170_REG_X_THRX_CONFIG     0x4U
#define TMAG5170_REG_Y_THRX_CONFIG     0x5U
#define TMAG5170_REG_Z_THRX_CONFIG     0x6U
#define TMAG5170_REG_T_THRX_CONFIG     0x7U
#define TMAG5170_REG_CONV_STATUS       0x8U
#define TMAG5170_REG_X_CH_RESULT       0x9U
#define TMAG5170_REG_Y_CH_RESULT       0xAU
#define TMAG5170_REG_Z_CH_RESULT       0xBU
#define TMAG5170_REG_TEMP_RESULT       0xCU
#define TMAG5170_REG_AFE_STATUS        0xDU
#define TMAG5170_REG_SYS_STATUS        0xEU
#define TMAG5170_REG_TEST_CONFIG       0xFU
#define TMAG5170_REG_OSC_MONITOR       0x10U
#define TMAG5170_REG_MAG_GAIN_CONFIG   0x11U
#define TMAG5170_REG_MAG_OFFSET_CONFIG 0x12U
#define TMAG5170_REG_ANGLE_RESULT      0x13U
#define TMAG5170_REG_MAGNITUDE_RESULT  0x14U

#define TMAG5170_CONV_AVG_POS        12U
#define TMAG5170_CONV_AVG_MASK       (BIT_MASK(3U) << TMAG5170_CONV_AVG_POS)
#define TMAG5170_CONV_AVG_SET(value) (((value) << TMAG5170_CONV_AVG_POS) & TMAG5170_CONV_AVG_MASK)

#define TMAG5170_MAG_TEMPCO_POS  8U
#define TMAG5170_MAG_TEMPCO_MASK (BIT_MASK(2U) << TMAG5170_MAG_TEMPCO_POS)
#define TMAG5170_MAG_TEMPCO_SET(value)                                                             \
	(((value) << TMAG5170_MAG_TEMPCO_POS) & TMAG5170_MAG_TEMPCO_MASK)

#define TMAG5170_OPERATING_MODE_POS  4U
#define TMAG5170_OPERATING_MODE_MASK (BIT_MASK(3U) << TMAG5170_OPERATING_MODE_POS)
#define TMAG5170_OPERATING_MODE_SET(value)                                                         \
	(((value) << TMAG5170_OPERATING_MODE_POS) & TMAG5170_OPERATING_MODE_MASK)

#define TMAG5170_T_CH_EN_POS        3U
#define TMAG5170_T_CH_EN_MASK       (BIT_MASK(1U) << TMAG5170_T_CH_EN_POS)
#define TMAG5170_T_CH_EN_SET(value) (((value) << TMAG5170_T_CH_EN_POS) & TMAG5170_T_CH_EN_MASK)

#define TMAG5170_T_RATE_POS        2U
#define TMAG5170_T_RATE_MASK       (BIT_MASK(1U) << TMAG5170_T_RATE_POS)
#define TMAG5170_T_RATE_SET(value) (((value) << TMAG5170_T_RATE_POS) & TMAG5170_T_RATE_MASK)

#define TMAG5170_ANGLE_EN_POS        14U
#define TMAG5170_ANGLE_EN_MASK       (BIT_MASK(2U) << TMAG5170_ANGLE_EN_POS)
#define TMAG5170_ANGLE_EN_SET(value) (((value) << TMAG5170_ANGLE_EN_POS) & TMAG5170_ANGLE_EN_MASK)

#define TMAG5170_SLEEPTIME_POS  10U
#define TMAG5170_SLEEPTIME_MASK (BIT_MASK(4U) << TMAG5170_SLEEPTIME_POS)
#define TMAG5170_SLEEPTIME_SET(value)                                                              \
	(((value) << TMAG5170_SLEEPTIME_POS) & TMAG5170_SLEEPTIME_MASK)

#define TMAG5170_MAG_CH_EN_POS  6U
#define TMAG5170_MAG_CH_EN_MASK (BIT_MASK(4U) << TMAG5170_MAG_CH_EN_POS)
#define TMAG5170_MAG_CH_EN_SET(value)                                                              \
	(((value) << TMAG5170_MAG_CH_EN_POS) & TMAG5170_MAG_CH_EN_MASK)

#define TMAG5170_Z_RANGE_POS        4U
#define TMAG5170_Z_RANGE_MASK       (BIT_MASK(2U) << TMAG5170_Z_RANGE_POS)
#define TMAG5170_Z_RANGE_SET(value) (((value) << TMAG5170_Z_RANGE_POS) & TMAG5170_Z_RANGE_MASK)

#define TMAG5170_Y_RANGE_POS        2U
#define TMAG5170_Y_RANGE_MASK       (BIT_MASK(2U) << TMAG5170_Y_RANGE_POS)
#define TMAG5170_Y_RANGE_SET(value) (((value) << TMAG5170_Y_RANGE_POS) & TMAG5170_Y_RANGE_MASK)

#define TMAG5170_X_RANGE_POS        0U
#define TMAG5170_X_RANGE_MASK       (BIT_MASK(2U) << TMAG5170_X_RANGE_POS)
#define TMAG5170_X_RANGE_SET(value) (((value) << TMAG5170_X_RANGE_POS) & TMAG5170_X_RANGE_MASK)

#define TMAG5170_RSLT_ALRT_POS  8U
#define TMAG5170_RSLT_ALRT_MASK (BIT_MASK(1U) << TMAG5170_RSLT_ALRT_POS)
#define TMAG5170_RSLT_ALRT_SET(value)                                                              \
	(((value) << TMAG5170_RSLT_ALRT_POS) & TMAG5170_RSLT_ALRT_MASK)

#define TMAG5170_VER_POS        4U
#define TMAG5170_VER_MASK       (BIT_MASK(2U) << TMAG5170_VER_POS)
#define TMAG5170_VER_GET(value) (((value) & TMAG5170_VER_MASK) >> TMAG5170_VER_POS)

#define TMAG5170_A1_REV 0x0U
#define TMAG5170_A2_REV 0x1U

#define TMAG5170_MAX_RANGE_50MT_IDX      0x0U
#define TMAG5170_MAX_RANGE_25MT_IDX      0x1U
#define TMAG5170_MAX_RANGE_100MT_IDX     0x2U
#define TMAG5170_MAX_RANGE_EXTEND_FACTOR 0x3U

#define TMAG5170_CONFIGURATION_MODE  0x0U
#define TMAG5170_STAND_BY_MODE       0x1U
#define TMAG5170_ACTIVE_TRIGGER_MODE 0x3U
#define TMAG5170_SLEEP_MODE          0x5U
#define TMAG5170_DEEP_SLEEP_MODE     0x6U

#define TMAG5170_MT_TO_GAUSS_RATIO 10U
#define TMAG5170_T_SENS_T0         25U
#define TMAG5170_T_ADC_T0          17522U
#define TMAG5170_T_ADC_RES         60U

/** Conversion time bridged in the trigger driven operating modes */
#define TMAG5170_CONVERSION_TIME_MS 5U

/** Fixed q31 shifts of the channels with a constant value range */
#define TMAG5170_TEMP_SHIFT     10
#define TMAG5170_ROTATION_SHIFT 9

#define TMAG5170_EVENT_DATA_READY BIT(0)

/** Result registers which can be part of a single one-shot read */
enum tmag5170_result_idx {
	TMAG5170_RESULT_IDX_X,
	TMAG5170_RESULT_IDX_Y,
	TMAG5170_RESULT_IDX_Z,
	TMAG5170_RESULT_IDX_ANGLE,
	TMAG5170_RESULT_IDX_TEMP,
	TMAG5170_RESULT_IDX_COUNT,
};

#if defined(CONFIG_SENSOR_ASYNC_API)
/** Result registers indexed by enum tmag5170_result_idx. Defined once in
 * tmag5170.c to avoid a copy per translation unit.
 */
extern const uint8_t tmag5170_result_regs[TMAG5170_RESULT_IDX_COUNT];
#endif

#if defined(CONFIG_TMAG5170_STREAM)
#include "tmag5170_stream.h"
#endif

/** Raw SPI frames of the result registers, indexed by enum tmag5170_result_idx.
 *
 * The frames are the DMA destination of the RTIO submissions, hence the raw
 * register content is kept and only decoded on demand.
 */
struct tmag5170_encoded_payload {
	uint8_t frames[TMAG5170_RESULT_IDX_COUNT][TMAG5170_SPI_BUFFER_LEN];
};

struct tmag5170_encoded_header {
	uint64_t timestamp;
	uint8_t chip_revision;
	uint8_t x_range: 2;
	uint8_t y_range: 2;
	uint8_t z_range: 2;
	/** Bitmask of enum tmag5170_result_idx entries held by the payload */
	uint8_t channels;
	uint8_t events;
};

struct tmag5170_encoded_data {
	struct tmag5170_encoded_header header;
	struct tmag5170_encoded_payload payload;
};

struct tmag5170_dev_config {
	uint8_t magnetic_channels;
	uint8_t x_range;
	uint8_t y_range;
	uint8_t z_range;
	uint8_t oversampling;
	bool temperature_measurement;
	uint8_t magnet_type;
	uint8_t angle_measurement;
	bool disable_temperature_oversampling;
	uint8_t sleep_time;
	uint8_t operating_mode;
#if defined(CONFIG_TMAG5170_TRIGGER) || defined(CONFIG_TMAG5170_STREAM)
	struct gpio_dt_spec int_gpio;
#endif
};

struct tmag5170_data {
	struct tmag5170_bus bus;
	uint8_t chip_revision;
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint16_t temperature;
	uint16_t angle;
#if defined(CONFIG_SENSOR_ASYNC_API)
	/** Frames shifted out by the one-shot submission. They must be
	 * persistent, because the SQEs referencing them are processed
	 * asynchronously.
	 */
	uint8_t tx_frames[TMAG5170_RESULT_IDX_COUNT][TMAG5170_SPI_BUFFER_LEN];
	uint8_t tx_trigger_frame[TMAG5170_SPI_BUFFER_LEN];
	uint8_t rx_trigger_frame[TMAG5170_SPI_BUFFER_LEN];
#endif
#if defined(CONFIG_TMAG5170_STREAM)
	struct tmag5170_stream_data stream;
#elif defined(CONFIG_TMAG5170_TRIGGER)
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy;
	const struct sensor_trigger *trigger_drdy;
	const struct device *dev;
#endif

#if defined(CONFIG_TMAG5170_TRIGGER_OWN_THREAD)
	struct k_sem sem;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TMAG5170_THREAD_STACK_SIZE);
#elif defined(CONFIG_TMAG5170_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
};

/**
 * @brief Get the full scale span of a magnetic channel in Gauss.
 *
 * @param chan_range Range index as configured in the devicetree
 * @param chip_revision Revision read back from the TEST_CONFIG register
 * @param full_scale_gauss Destination of the full scale span
 *
 * @retval 0 on success
 * @retval -ENOTSUP if the range index is not supported
 */
static inline int tmag5170_magn_full_scale_gauss(uint8_t chan_range, uint8_t chip_revision,
						 uint32_t *full_scale_gauss)
{
	uint16_t max_range_mt;

	if (chan_range == TMAG5170_MAX_RANGE_50MT_IDX) {
		max_range_mt = 50U;
	} else if (chan_range == TMAG5170_MAX_RANGE_25MT_IDX) {
		max_range_mt = 25U;
	} else if (chan_range == TMAG5170_MAX_RANGE_100MT_IDX) {
		max_range_mt = 100U;
	} else {
		return -ENOTSUP;
	}

	if (chip_revision == TMAG5170_A2_REV) {
		max_range_mt *= TMAG5170_MAX_RANGE_EXTEND_FACTOR;
	}

	max_range_mt *= 2U;

	/* The sensor returns data in mT, we need to convert it to Gauss */
	*full_scale_gauss = (uint32_t)max_range_mt * TMAG5170_MT_TO_GAUSS_RATIO;

	return 0;
}

/**
 * @brief Convert a raw magnetic channel reading into micro-Gauss.
 */
static inline int tmag5170_magn_reading_to_micro_gauss(uint16_t chan_reading, uint8_t chan_range,
						       uint8_t chip_revision, int64_t *output)
{
	uint32_t full_scale_gauss;
	int ret = tmag5170_magn_full_scale_gauss(chan_range, chip_revision, &full_scale_gauss);

	if (ret != 0) {
		return ret;
	}

	/* Convert from 2's complementary system, as it is shown in datasheet.
	 * Since the DATA_TYPE register is not written (default=0x0),
	 * the formula for 16-bit sensor data must be applied.
	 */
	int64_t result = chan_reading - ((chan_reading & 0x8000) << 1);

	result *= full_scale_gauss;

	/* Scale to micro-units */
	result *= 1000000LL;

	/* Divide as it is shown in datasheet */
	result /= 65536LL;

	*output = result;

	return 0;
}

/**
 * @brief Get the q31 shift of a magnetic channel.
 */
static inline int tmag5170_magn_shift(uint8_t chan_range, uint8_t chip_revision, int8_t *shift)
{
	uint32_t full_scale_gauss;
	int ret = tmag5170_magn_full_scale_gauss(chan_range, chip_revision, &full_scale_gauss);

	if (ret != 0) {
		return ret;
	}

	/* Readings span half of the full scale in each direction */
	uint32_t magnitude = full_scale_gauss / 2U;
	int8_t bits = 0;

	while ((1U << bits) < magnitude) {
		bits++;
	}

	*shift = bits;

	return 0;
}

/**
 * @brief Convert a raw temperature reading into micro-degrees Celsius.
 */
static inline void tmag5170_temp_reading_to_micro_celsius(uint16_t chan_reading, int64_t *output)
{
	/* Apply value conversion as shown in the datasheet */
	int64_t result = chan_reading - TMAG5170_T_ADC_T0;

	*output = (TMAG5170_T_SENS_T0 * 1000000LL) +
		  (1000000LL * result / (int64_t)TMAG5170_T_ADC_RES);
}

/**
 * @brief Convert a raw angle reading into micro-degrees.
 */
static inline void tmag5170_angle_reading_to_micro_degrees(uint16_t chan_reading, int64_t *output)
{
	/* Apply value conversion as shown in the datasheet.
	 * 12 MSBs store the integer part of the result,
	 * 4 LSBs store the fractional part of the result
	 */
	*output = (chan_reading >> 4) * 1000000LL + ((chan_reading & 0xF) * 1000000LL) / 16LL;
}

#if defined(CONFIG_TMAG5170_TRIGGER)
int tmag5170_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int tmag5170_trigger_init(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_H_ */
