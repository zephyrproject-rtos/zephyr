/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of VEML60xx sensor family
 * @ingroup veml60xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML60XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML60XX_H_

/**
 * @defgroup veml60xx_interface VEML60XX
 * @ingroup sensor_interface_ext
 * @brief Vishay VEML60xx sensor family common attributes
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VEML60xx integration time options for ambient light measurements.
 *
 * Possible values for @ref SENSOR_ATTR_VEML6031_IT and
 * @ref SENSOR_ATTR_VEML6046_IT custom attribute.
 */
enum veml60xx_it {
	VEML60XX_IT_3_125, /**< 3.125 ms */
	VEML60XX_IT_6_25,  /**< 6.25 ms */
	VEML60XX_IT_12_5,  /**< 12.5 ms */
	VEML60XX_IT_25,    /**< 25 ms */
	VEML60XX_IT_50,    /**< 50 ms */
	VEML60XX_IT_100,   /**< 100 ms */
	VEML60XX_IT_200,   /**< 200 ms */
	VEML60XX_IT_400,   /**< 400 ms */
	/** @cond INTERNAL_HIDDEN */
	VEML60XX_IT_COUNT,
	/** @endcond */
};

/*
 * @brief VEML60xx integration time struct.
 */
struct veml60xx_it_data {
	enum veml60xx_it num;
	uint8_t val;
	int us;
};

/*
 * @brief VEML60xx integration time setting values.
 *
 * The enumerators of enum veml60xx_it provide indices into this array to get
 * the related value for the ALS_IT configuration bits.
 */
static const struct veml60xx_it_data veml60xx_it_values[VEML60XX_IT_COUNT] = {
	{VEML60XX_IT_3_125, 0x00, 3125}, /*   3.125 - 0b0000 */
	{VEML60XX_IT_6_25, 0x01, 6250},  /*   6.25  - 0b0001 */
	{VEML60XX_IT_12_5, 0x02, 12500}, /*  12.5   - 0b0010 */
	{VEML60XX_IT_25, 0x03, 25000},   /*  25     - 0b0011 */
	{VEML60XX_IT_50, 0x04, 50000},   /*  50     - 0b0100 */
	{VEML60XX_IT_100, 0x05, 100000}, /* 100     - 0b0101 */
	{VEML60XX_IT_200, 0x06, 200000}, /* 200     - 0b0110 */
	{VEML60XX_IT_400, 0x07, 400000}, /* 400     - 0b0111 */
};
/**
 * @brief VEML60xx gain options for ambient light measurements.
 */
enum veml60xx_gain {
	VEML60XX_GAIN_1 = 0x00,    /**< 1x gain */
	VEML60XX_GAIN_2 = 0x01,    /**< 2x gain */
	VEML60XX_GAIN_0_66 = 0x02, /**< 0.66x gain */
	VEML60XX_GAIN_0_5 = 0x03,  /**< 0.5x gain */
	/** @cond INTERNAL_HIDDEN */
	VEML60XX_GAIN_COUNT = 4,
	/** @endcond */
};

/**
 * @brief VEML60xx ALS interrupt persistence protect number options.
 *
 * Possible values for @ref SENSOR_ATTR_VEML6031_PERS and
 * @ref SENSOR_ATTR_VEML6046_PERS custom attribute.
 */
enum veml60xx_pers {
	VEML60XX_PERS_1 = 0x00, /**< 1 measurement */
	VEML60XX_PERS_2 = 0x01, /**< 2 measurements */
	VEML60XX_PERS_4 = 0x02, /**< 4 measurements */
	VEML60XX_PERS_8 = 0x03, /**< 8 measurements */
};


static inline bool veml60xx_gain_in_range(int32_t gain)
{
	return (gain >= VEML60XX_GAIN_1) && (gain <= VEML60XX_GAIN_0_5);
}

static inline bool veml60xx_it_in_range(int32_t it)
{
	return (it >= VEML60XX_IT_3_125) && (it <= VEML60XX_IT_400);
}

static inline bool veml60xx_pers_in_range(int32_t pers)
{
	return (pers >= VEML60XX_PERS_1) && (pers <= VEML60XX_PERS_8);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEML60XX_H_ */
