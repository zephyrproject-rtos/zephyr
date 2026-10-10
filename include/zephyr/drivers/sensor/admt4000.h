/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADMT4000_H_
#define ZEPHYR_DRIVERS_SENSOR_ADMT4000_H_

/**
 * @file
 * @brief ADMT4000 True Power-On Multiturn Sensor public definitions.
 *
 * Extended sensor channels and attributes for the Analog Devices ADMT4000
 * magnetic multiturn sensor, used together with the standard sensor API.
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/rtio/rtio.h>
#endif

/** @cond INTERNAL_HIDDEN */

/*ADMT4000 Registers*/
#define ADMT4000_REG_RST 0x00

/*ADMT4000 Agnostic Page Registers*/
#define ADMT4000_AGP_REG_CNVPAGE  0x01
#define ADMT4000_AGP_REG_ABSANGLE 0x03
#define ADMT4000_AGP_REG_DGIO     0x04
#define ADMT4000_AGP_REG_ANGLE    0x05
#define ADMT4000_AGP_REG_FAULT    0x06
#define ADMT4000_AGP_REG_ANGLESEC 0x08

/*ADMT 0x00 Page Registers*/
#define ADMT4000_RAW_ANGLE_REG(x) (0x10 + (x))
#define ADMT4000_00_REG_SINE      0x10
#define ADMT4000_00_REG_COSINE    0x11
#define ADMT4000_00_REG_RADIUS    0x18
#define ADMT4000_00_REG_DIAG1     0x1D
#define ADMT4000_00_REG_DIAG2     0x1E
#define ADMT4000_00_REG_TMP0      0x20
#define ADMT4000_00_REG_TMP1      0x23

/*ADMT 0x02 Page Registers*/
#define ADMT4000_02_REG_GENERAL 0x10
#define ADMT4000_02_REG_DIGIOEN 0x12
#define ADMT4000_02_REG_ANGLECK 0x13
#define ADMT4000_02_REG_CNVCNT  0x14
#define ADMT4000_02_REG_H1MAG   0x15
#define ADMT4000_02_REG_H1PH    0x16
#define ADMT4000_02_REG_H2MAG   0x17
#define ADMT4000_02_REG_H2PH    0x18
#define ADMT4000_02_REG_H3MAG   0x19
#define ADMT4000_02_REG_H3PH    0x1A
#define ADMT4000_02_REG_H8MAG   0x1B
#define ADMT4000_02_REG_H8PH    0x1C
#define ADMT4000_02_REG_ECCEDC  0x1D
#define ADMT4000_02_REG_UNIQD0  0x1E
#define ADMT4000_02_REG_UNIQD1  0x1F
#define ADMT4000_02_REG_UNIQD2  0x20
#define ADMT4000_02_REG_UNIQD3  0x21
#define ADMT4000_02_REG_ECCDIS  0x23

/* ADMT4000 Other Macros, Masks etc. */
#define ADMT4000_RW_MASK    GENMASK(5, 0)
#define ADMT4000_WR_EN      BIT(6)
#define ADMT4000_FAULT_MASK BIT(7)
#define ADMT4000_LIFE_CTR   GENMASK(6, 5)
#define ADMT4000_RCV_CRC    GENMASK(4, 0)

#define ADMT4000_MIN_PAGE 0x00
#define ADMT4000_MAX_PAGE 0x02

#define ADMT4000_DIGIO3FNC_MASK BIT(3)
#define ADMT4000_DIGIO0FNC_MASK BIT(0)

/* Upper and Lower Byte Masking */
#define ADMT4000_LOW_BYTE GENMASK(7, 0)
#define ADMT4000_HI_BYTE  GENMASK(15, 8)

/* Register 01 */
#define ADMT4000_CNV_EDGE_MASK GENMASK(15, 14)
#define ADMT4000_PAGE_MASK     GENMASK(4, 0)
#define ADMT4000_FALLING_EDGE  0x00
#define ADMT4000_RISING_EDGE   0x3

/* Register 03 */
#define ADMT4000_ABS_ANGLE_MASK       GENMASK(15, 0)
#define ADMT4000_TURN_CNT_MASK        GENMASK(15, 10)
#define ADMT4000_ABS_ANGLE_ANGLE_MASK GENMASK(9, 0)

#define ADMT4000_INVALID_TURN 0x53

/* Register 04*/
#define ADMT4000_MAX_GPIO_INDEX 5
#define ADMT4000_GPIO_LOGIC(x)  BIT(x)

/* Register 05 and 08 */
#define ADMT4000_ANGLE_MASK GENMASK(15, 4)

/* Register 06 */
#define ADMT4000_ALL_FAULTS         GENMASK(15, 0)
#define ADMT4000_AGP_INDIV_FAULT(x) BIT((x))

/* Register 0x08/10/11/12/13/18, page 0 */
#define ADMT4000_ANGLE_STAT_MASK BIT(0)

/* Register 0x10/11/12/13, page 0 */
#define ADMT4000_RAW_ANGLE_MASK  GENMASK(15, 2)
#define ADMT4000_RAW_COSINE_MASK GENMASK(15, 2)
#define ADMT4000_RAW_SINE_MASK   GENMASK(15, 2)

/* Register 18, page 0 */
#define ADMT4000_RADIUS_MASK GENMASK(15, 1)

/* Register 0x1D, page 0 */
#define ADMT4000_MTDIAG1_MASK  GENMASK(15, 8)
#define ADMT4000_AFEDIAG2_MASK GENMASK(7, 0)

/* Register 0x1E, page 0 */
#define ADMT4000_AFEDIAG1_MASK GENMASK(15, 8)
#define ADMT4000_REF_RES(x)    BIT(8 + (x))
#define ADMT4000_AFEDIAG0_MASK GENMASK(7, 0)

/* Register 0x20, page 0 */
#define ADMT4000_TEMP_MASK GENMASK(15, 4)

/* Register 0x10, page 2 */
#define ADMT4000_STORAGE_MSB_XTRACT   BIT(7)
#define ADMT4000_STORAGE_BIT6_XTRACT  BIT(6)
#define ADMT4000_STORAGE_MASK1_XTRACT GENMASK(5, 3)
#define ADMT4000_STORAGE_MASK0_XTRACT GENMASK(2, 0)
#define ADMT4000_STORAGE_MSB          BIT(15)
#define ADMT4000_STORAGE_BIT6         BIT(11)
#define ADMT4000_STORAGE_MASK1        GENMASK(8, 6)
#define ADMT4000_STORAGE_MASK0        GENMASK(3, 1)
#define ADMT4000_STORAGE_MASK_FULL                                                                 \
	ADMT4000_STORAGE_MSB | ADMT4000_STORAGE_BIT6 | ADMT4000_STORAGE_MASK1 |                    \
		ADMT4000_STORAGE_MASK0

#define ADMT4000_CONV_SYNC_MODE_MASK GENMASK(14, 13)
#define ADMT4000_ANGL_FILT_MASK      BIT(12)
#define ADMT4000_H8_CTRL_MASK        BIT(10)
#define ADMT4000_CNV_MODE_MASK       BIT(0)

/* Register 0x12, page 2 */
#define ADMT4000_DIG_IO_EN(x) BIT(8 + (x))
#define ADMT4000_GPIO_FUNC(x) BIT(x)

/* Register 0x13, page 2 */
#define ADMT4000_ANGL_CHK_MASK GENMASK(9, 0)

/* Register 0x13, page 2 */
#define ADMT4000_CNV_CTR_MASK GENMASK(7, 0)

/* Register 0x15 / 0x18, page 2 */
#define ADMT4000_H_11BIT_MAG_MASK GENMASK(10, 0)
#define ADMT4000_11BIT_MAX        2047
#define ADMT4000_H_12BIT_PHA_MASK GENMASK(11, 0)
#define ADMT4000_12BIT_MAX        4095

/* Register 0x1A / 0x1B, page 2 */
#define ADMT4000_H_8BIT_MAG_MASK GENMASK(7, 0)
/*
 * H3MAG/H8MAG are unsigned 8-bit magnitude fields, so the valid range is
 * 0..255 (2^8 - 1), matching the GENMASK(7, 0) field width and the unsigned
 * getter path. Some other implementations cap this at 127; that appears to be
 * an undocumented restriction and is not backed by the datasheet.
 */
#define ADMT4000_8BIT_MAX        255

/* Register 0x1D, page 2 */
#define ADMT4000_ECC_CFG1 GENMASK(15, 8)
#define ADMT4000_ECC_CFG0 GENMASK(7, 0)

/* Register 0x1E 0x1F 0x20 0x21 , page 2 */
#define ADMT4000_ID0_MASK       GENMASK(14, 0)
#define ADMT4000_ID_PROD_MASK   GENMASK(10, 8)
#define ADMT4000_ID_SUPPLY_MASK GENMASK(7, 6)
#define ADMT4000_ID_ASIL_MASK   GENMASK(5, 3)
#define ADMT4000_ID_SIL_REV_MAS GENMASK(2, 0)

/* Register 0x23, page 2 */
#define ADMT4000_ECC_EN_COMM  0x0000
#define ADMT4000_ECC_DIS_COMM 0x4D54

/* Scaling factor */
#define ADMT4000_SF 10000000

/* Angle to degree conversion */
#define ADMT4000_ABS_ANGLE_RES 3515625
#define ADMT4000_ANGLE_RES     878906

/* Turn Count Conversion */
#define ADMT4000_TURN_CNT_THRES 0x35
#define ADMT4000_TURN_CNT_TWOS  64

/*
 * Quarter-turn count decoding: the 8-bit raw turn field is a signed
 * quarter-turn count. Values greater than ADMT4000_QUARTER_TURNS_MAX
 * represent negative counts (raw - RES). Full turns = quarter_turns / 4.
 */
#define ADMT4000_QUARTER_TURNS_MAX 215
#define ADMT4000_QUARTER_TURNS_RES 256

/* Conversion Factors */
/*
 * Radius resolution: 0.000924 mV/V per LSB, expressed in 10^-7 mV/V units
 * (9240 * 10^-7 = 9.24e-4).
 */
#define ADMT4000_RADIUS_RES          9240
#define ADMT4000_FIXED_VOLT_3P3V_RES 32220
#define ADMT4000_FIXED_VOLT_5V_RES   48828
#define ADMT4000_8BIT_THRES          0x7f
#define ADMT4000_8BIT_TWOS           0x100
#define ADMT4000_DIAG_RESISTOR_RES   7812500
#define ADMT4000_CORDIC_SCALER       6072000
#define ADMT4000_HMAG_RES            54930
#define ADMT4000_HPHA_RES            878910

/* Device configuration structure */
struct admt4000_dev_config {
	struct spi_dt_spec spi;
	uint8_t vdd_variant;
	struct gpio_dt_spec gpio_busy;
	struct gpio_dt_spec gpio_acalc;
	struct gpio_dt_spec gpio_coil_rs;
	struct gpio_dt_spec gpio_cnv;
};

/* Forward declarations */
struct admt4000_dev;

struct admt4000_gpio {
	bool logic_state;
	bool is_alt_pin;
};

enum admt4000_faults {
	ADMT4000_FAULT_VDD_UV = 0,
	ADMT4000_FAULT_VDD_OV,
	ADMT4000_FAULT_VDRIVE_UV,
	ADMT4000_FAULT_VDRIVE_OV,
	ADMT4000_FAULT_NVM_CRC = 5,
	ADMT4000_FAULT_ECC_DOUBLE = 7,
	ADMT4000_FAULT_GMR_REF_RES = 9,
	ADMT4000_FAULT_TURN_CTRS = 13,
	ADMT4000_FAULT_AMR_RAD_AMP,
};

enum admt4000_vdd {
	ADMT4000_3P3V,
	ADMT4000_5V,
};

enum admt4000_conv_sync_mode {
	ADMT4000_SEQ_CTRL = 0,
	ADMT4000_START_EDGE = 3,
};

enum admt4000_harmonic_corr_src {
	ADMT4000_FACTORY = 0,
	ADMT4000_USER = 1,
};

enum admt4000_angle_type {
	ADMT4000_RAW_SINE,
	ADMT4000_RAW_COSINE,
	ADMT4000_RAW_SECANGLI,
	ADMT4000_RAW_SECANGLEQ,
};

enum admt4000_angle_eck_type {
	ADMT4000_ALWAYS_FLAG,
	ADMT4000_VALID,
	ADMT4000_DISABLE_CHECK,
};

struct admt4000_data {
	const struct device *dev;

	/* Last fetched sensor data */
	uint8_t turns;
	int16_t quarter_turns;
	uint16_t angle[2];
	uint16_t temp;
	int16_t cos_val;
	int16_t sin_val;
	int32_t converted_temp;
	uint32_t converted_radius;

	/* Configuration cache */
	uint8_t conv_mode;
	uint8_t conv_sync_mode;
	bool angle_filter_enabled;
	uint8_t h8_corr_src;
	enum admt4000_angle_eck_type eck_type;
	int16_t hmc[3];
	int16_t hpc[3];
	int16_t hmc8;
	int16_t hpc8;

	/* Page and conversion state */
	bool is_page_zero;
	bool is_rising_edge;
	bool is_one_shot;

	/* VDD variant */
	enum admt4000_vdd vdd_variant;

	/* GPIOs */
	struct admt4000_gpio gpios[ADMT4000_MAX_GPIO_INDEX + 1];
	struct gpio_dt_spec gpio_busy;
	struct gpio_dt_spec gpio_coil_rs;
	struct gpio_dt_spec gpio_cnv;
	struct gpio_dt_spec gpio_acalc;

	/* Conversion factor */
	uint32_t fixed_conv_factor_mv;

	/* Trigger support (used for SENSOR_TRIG_DATA_READY via GPIO_ACALC) */
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;
};

/** @endcond */

/** ADMT4000-specific sensor channels. */
enum {
	/** Absolute mechanical angle, in degrees. */
	SENSOR_CHAN_ADMT4000_ANGLE = SENSOR_CHAN_PRIV_START,
	/** CORDIC cosine component, normalized to [-1, 1). */
	SENSOR_CHAN_ADMT4000_COS,
	/** CORDIC sine component, normalized to [-1, 1). */
	SENSOR_CHAN_ADMT4000_SIN,
	/** Signal radius (vector amplitude), diagnostic. */
	SENSOR_CHAN_ADMT4000_RADIUS,
};

/** ADMT4000-specific sensor attributes. */
enum {
	/** Conversion mode (one-shot or continuous). */
	SENSOR_ATTR_ADMT4000_CONV_MODE = SENSOR_ATTR_PRIV_START,
	/** Conversion synchronization mode. */
	SENSOR_ATTR_ADMT4000_SYNC_MODE,
	/** Enable or disable the angle output filter. */
	SENSOR_ATTR_ADMT4000_FILTER_ENABLE,
	/** Angle error-correction type. */
	SENSOR_ATTR_ADMT4000_ECK_TYPE,
	/** 8th-harmonic correction source. */
	SENSOR_ATTR_ADMT4000_H8_CORR_SRC,

	/** 1st-harmonic magnitude correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_MAG_1,
	/** 1st-harmonic phase correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_PH_1,
	/** 2nd-harmonic magnitude correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_MAG_2,
	/** 2nd-harmonic phase correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_PH_2,
	/** 3rd-harmonic magnitude correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_MAG_3,
	/** 3rd-harmonic phase correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_PH_3,
	/** 8th-harmonic magnitude correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_MAG_8,
	/** 8th-harmonic phase correction. */
	SENSOR_ATTR_ADMT4000_HARMONIC_PH_8,

	/** Trigger a GMR coil reset pulse (write-only, value ignored). */
	SENSOR_ATTR_ADMT4000_COIL_RESET,
};

/** @cond INTERNAL_HIDDEN */

/*
 * Q31 conversion constants for RTIO/async decoder.
 *
 * Angle: 12-bit raw (0-4095) -> 0-360 degrees, Q9.22 format
 *   q31 = raw * ADMT4000_ANGLE_CONV_Q9_22
 *   shift = 9, range [-512, 512)
 *
 * Temperature: converted_temp is in 10^-5 degrees C units
 *   q31 = (int64_t)converted_temp * (1 << 24) / 100000
 *   shift = 7, range [-128, 128)
 *
 * Turns: signed quarter-turn count converted to full turns (quarter / 4)
 *   q31 = quarter_turns * ADMT4000_TURNS_CONV_Q6_25
 *   shift = 6, range [-64, 64)
 *
 * Cos/Sin: int16_t from CORDIC, normalized to [-1, 1)
 *   q31 = raw * ADMT4000_CORDIC_CONV_Q0_31
 *   shift = 0, range [-1, 1)
 *
 * Radius: converted_radius is in 10^-7 mV/V units
 *   q31 = (int64_t)converted_radius * (1 << 25) / 10000000
 *   shift = 6, range [-64, 64)
 */
#define ADMT4000_ANGLE_CONV_Q9_22 368640
#define ADMT4000_ANGLE_Q31_SHIFT  9

#define ADMT4000_TEMP_Q31_SHIFT 7

#define ADMT4000_TURNS_CONV_Q6_25 8388608
#define ADMT4000_TURNS_Q31_SHIFT  6

#define ADMT4000_CORDIC_CONV_Q0_31 262144
#define ADMT4000_CORDIC_Q31_SHIFT  0

/*
 * CORDIC sine/cosine full-scale value. The raw fields are 14-bit two's
 * complement (sign_extend to bit 13), so the magnitude at full scale is
 * 2^13 = 8192. Dividing a raw count by this yields the normalized [-1, 1)
 * value reported on the SIN/COS channels.
 */
#define ADMT4000_CORDIC_FULL_SCALE 8192

#define ADMT4000_RADIUS_Q31_SHIFT 6

/** @endcond */

#ifdef CONFIG_SENSOR_ASYNC_API

/** @cond INTERNAL_HIDDEN */

struct admt4000_decoder_header {
	uint64_t timestamp;
} __attribute__((__packed__));

/*
 * Compact sample payload carried in the RTIO encoded buffer. Only the decoded
 * sample values are stored here (not the full device state), keeping each RTIO
 * frame small.
 */
struct admt4000_sample {
	int16_t quarter_turns;
	uint16_t angle;
	int16_t cos_val;
	int16_t sin_val;
	int32_t converted_temp;
	uint32_t converted_radius;
};

struct admt4000_encoded_data {
	struct admt4000_decoder_header header;
	struct {
		uint8_t has_angle: 1;
		uint8_t has_turns: 1;
		uint8_t has_temp: 1;
		uint8_t has_cos: 1;
		uint8_t has_sin: 1;
		uint8_t has_radius: 1;
	} __attribute__((__packed__));
	struct admt4000_sample data;
};

/** @endcond */

/**
 * @brief Get the RTIO decoder API for an ADMT4000 device.
 *
 * @param dev ADMT4000 device instance.
 * @param decoder Set to the device's sensor decoder API on success.
 *
 * @retval 0 Success.
 * @retval -ENOSYS Decoding is not supported.
 */
int admt4000_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

/**
 * @brief Submit an RTIO read request to an ADMT4000 device.
 *
 * @param dev ADMT4000 device instance.
 * @param iodev_sqe RTIO submission queue entry describing the read.
 */
void admt4000_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

#endif /* CONFIG_SENSOR_ASYNC_API */

/**
 * @brief Fetch a sample from an ADMT4000 device into a caller-provided buffer.
 *
 * @param dev ADMT4000 device instance.
 * @param chan Channel to fetch, or SENSOR_CHAN_ALL for all channels.
 * @param data Destination for the fetched sample data.
 *
 * @retval 0 Success.
 * @retval -EINVAL The requested channel is not supported.
 * @retval -EIO Communication with the device failed.
 */
int admt4000_sample_fetch_helper(const struct device *dev, enum sensor_channel chan,
				 struct admt4000_data *data);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADMT4000_H_ */
