/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL38X_ADXL38X_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL38X_ADXL38X_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/sensor/adxl380.h>
#include <zephyr/dt-bindings/sensor/adxl382.h>
#include <zephyr/drivers/sensor/adxl38x.h>

/* This driver supports SPI communication for both ADXL380 and ADXL382 */
#define ADXL38X_BUS_SPI

/**
 * @brief ADXL38X SPI read address byte
 */
#define ADXL38X_SPI_READ(x) (((x) << 1) | 0x01u)

/**
 * @brief ADXL38X SPI write address byte
 */
#define ADXL38X_SPI_WRITE(x) (((x) << 1) | 0x00u)

/*
 * ADXL38X register map
 */
#define ADXL38X_DEVID_AD         0x00u
#define ADXL38X_DEVID_MST        0x01u
#define ADXL38X_PART_ID          0x02u
#define ADXL38X_PART_ID_REV_ID   0x03u
#define ADXL38X_SERIAL_NUMBER_0  0x04u
#define ADXL38X_SERIAL_NUMBER_1  0x05u
#define ADXL38X_SERIAL_NUMBER_2  0x06u
#define ADXL38X_SERIAL_NUMBER_3  0x07u
#define ADXL38X_SERIAL_NUMBER_4  0x08u
#define ADXL38X_SERIAL_NUMBER_5  0x09u
#define ADXL38X_SERIAL_NUMBER_6  0x0Au
#define ADXL38X_DEV_DELTA_Q_X    0x0Bu
#define ADXL38X_DEV_DELTA_Q_Y    0x0Cu
#define ADXL38X_DEV_DELTA_Q_Z    0x0Du
#define ADXL38X_DEV_DELTA_F0_X   0x0Eu
#define ADXL38X_DEV_DELTA_F0_Y   0x0Fu
#define ADXL38X_DEV_DELTA_F0_Z   0x10u
#define ADXL38X_STATUS0          0x11u
#define ADXL38X_STATUS1          0x12u
#define ADXL38X_STATUS2          0x13u
#define ADXL38X_STATUS3          0x14u
#define ADXL38X_XDATA_H          0x15u
#define ADXL38X_XDATA_L          0x16u
#define ADXL38X_YDATA_H          0x17u
#define ADXL38X_YDATA_L          0x18u
#define ADXL38X_ZDATA_H          0x19u
#define ADXL38X_ZDATA_L          0x1Au
#define ADXL38X_TDATA_H          0x1Bu
#define ADXL38X_TDATA_L          0x1Cu
#define ADXL38X_FIFO_DATA        0x1Du
#define ADXL38X_FIFO_STATUS0     0x1Eu
#define ADXL38X_FIFO_STATUS1     0x1Fu
#define ADXL38X_MISC0            0x20u
#define ADXL38X_MISC1            0x21u
#define ADXL38X_SENS_DSM         0x24u
#define ADXL38X_CLK_CTRL         0x25u
#define ADXL38X_OP_MODE          0x26u
#define ADXL38X_DIG_EN           0x27u
#define ADXL38X_SAR_I2C          0x28u
#define ADXL38X_NVM_CTL          0x29u
#define ADXL38X_REG_RESET        0x2Au
#define ADXL38X_INT0_MAP0        0x2Bu
#define ADXL38X_INT0_MAP1        0x2Cu
#define ADXL38X_INT1_MAP0        0x2Du
#define ADXL38X_INT1_MAP1        0x2Eu
#define ADXL38X_FIFO_CFG0        0x30u
#define ADXL38X_FIFO_CFG1        0x31u
#define ADXL38X_SPT_CFG0         0x32u
#define ADXL38X_SPT_CFG1         0x33u
#define ADXL38X_SPT_CFG2         0x34u
#define ADXL38X_SYNC_CFG         0x35u
#define ADXL38X_PDM_CFG          0x36u
#define ADXL38X_ACT_INACT_CTL    0x37u
#define ADXL38X_SNSR_AXIS_EN     0x38u
#define ADXL38X_THRESH_ACT_H     0x39u
#define ADXL38X_THRESH_ACT_L     0x3Au
#define ADXL38X_TIME_ACT_H       0x3Bu
#define ADXL38X_TIME_ACT_M       0x3Cu
#define ADXL38X_TIME_ACT_L       0x3Du
#define ADXL38X_THRESH_INACT_H   0x3Eu
#define ADXL38X_THRESH_INACT_L   0x3Fu
#define ADXL38X_TIME_INACT_H     0x40u
#define ADXL38X_TIME_INACT_M     0x41u
#define ADXL38X_TIME_INACT_L     0x42u
#define ADXL38X_TAP_THRESH       0x43u
#define ADXL38X_TAP_DUR          0x44u
#define ADXL38X_TAP_LATENT       0x45u
#define ADXL38X_TAP_WINDOW       0x46u
#define ADXL38X_TAP_CFG          0x47u
#define ADXL38X_OR_CFG           0x48u
#define ADXL38X_TRIG_CFG         0x49u
#define ADXL38X_X_SAR_OFFSET     0x4Au
#define ADXL38X_Y_SAR_OFFSET     0x4Bu
#define ADXL38X_Z_SAR_OFFSET     0x4Cu
#define ADXL38X_X_DSM_OFFSET     0x4Du
#define ADXL38X_Y_DSM_OFFSET     0x4Eu
#define ADXL38X_Z_DSM_OFFSET     0x4Fu
#define ADXL38X_FILTER           0x50u
#define ADXL38X_USER_TEMP_SENS_0 0x55u
#define ADXL38X_USER_TEMP_SENS_1 0x56u
#define ADXL38X_MISO             0x58u
#define ADXL38X_SOUT0            0x59u
#define ADXL38X_MCLK             0x5Au
#define ADXL38X_BCLK             0x5Bu
#define ADXL38X_FSYNC            0x5Cu
#define ADXL38X_INT0             0x5Du
#define ADXL38X_INT1             0x5Eu

/* Part ID and Revision ID */
#define ADXL38X_PART_ID_MSK GENMASK(7, 4)
#define ADXL38X_REV_ID_MSK  GENMASK(3, 0)

/* STATUS0 Register (FIFO / NVM status) */
#define ADXL38X_STATUS0_PARITY_ERR_STICKY_MSK        BIT(0)
#define ADXL38X_STATUS0_FIFO_FULL_MSK                BIT(1)
#define ADXL38X_STATUS0_FIFO_OVERRUN_MSK             BIT(2)
#define ADXL38X_STATUS0_FIFO_WATERMARK_MSK           BIT(3)
#define ADXL38X_STATUS0_FIFO_READY_MSK               BIT(4)
#define ADXL38X_STATUS0_EFUSE_BUSY_REGERR_STICKY_MSK BIT(5)
#define ADXL38X_STATUS0_NVM_DONE_MSK                 BIT(6)
#define ADXL38X_STATUS0_NVM_BUSY_STATUS_MSK          BIT(7)

/* STATUS1 Register (activity / tap / over-range) */
#define ADXL38X_STATUS1_SINGLE_TAP_MSK        BIT(0)
#define ADXL38X_STATUS1_DOUBLE_TAP_MSK        BIT(1)
#define ADXL38X_STATUS1_TRIPLE_TAP_MSK        BIT(2)
#define ADXL38X_STATUS1_OVER_RANGE_MSK        BIT(3)
#define ADXL38X_STATUS1_OVER_RANGE_STICKY_MSK BIT(4)
#define ADXL38X_STATUS1_ACT_MSK               BIT(5)
#define ADXL38X_STATUS1_INACT_MSK             BIT(6)
#define ADXL38X_STATUS1_NVM_IRQ_MSK           BIT(7)

/* STATUS2 Register */
#define ADXL38X_STATUS2_UV_FLAG_MSK        BIT(0)
#define ADXL38X_STATUS2_UV_FLAG_STICKY_MSK BIT(1)
#define ADXL38X_STATUS2_NVM_CRC_ERR_MSK    BIT(3)
#define ADXL38X_STATUS2_NVM_CRC_DONE_MSK   BIT(4)
#define ADXL38X_STATUS2_NVM_ECC_DET_MSK    BIT(5)
#define ADXL38X_STATUS2_NVM_ECC_DONE_MSK   BIT(7)

/* STATUS3 Register (data ready) */
#define ADXL38X_STATUS3_DATA_READY_MSK BIT(0)

/* Temperature Data (TDATA_L 0x1C) and X DSM sensitivity trim */
#define ADXL38X_TEMP_OUTPUT_TDATA_L_MSK   GENMASK(7, 4)
#define ADXL38X_TEMP_OUTPUT_XSENS_DSM_MSK GENMASK(3, 0)

/* Signed 12-bit temperature, low 4 bits in TDATA_L[7:4] */
#define ADXL38X_TEMP_RESOLUTION 12U
#define ADXL38X_TEMP_LSB_BITS   4U
#define ADXL38X_TEMP_SIGN_BIT   (ADXL38X_TEMP_RESOLUTION - 1U)

/* Miscellaneous 0 */
#define ADXL38X_MISC0_GAIN_SCALER_XY_MSK GENMASK(4, 0)
#define ADXL38X_MISC0_PARITY_ERR_MSK     BIT(5)
#define ADXL38X_MISC0_PARITY_REG_MSK     BIT(6)
#define ADXL38X_MISC0_XL382_MSK          BIT(7)

/* Miscellaneous 1 */
#define ADXL38X_MISC1_GAIN_SCALER_Z_MSK GENMASK(4, 0)

/* Sensor DSM Register */
#define ADXL38X_SENSOR_DSM_ZSENS_DSM_MSK GENMASK(3, 0)
#define ADXL38X_SENSOR_DSM_YSENS_DSM_MSK GENMASK(7, 4)

/* CLK_CTRL Register */
#define ADXL38X_CLK_CTRL_CLK_SRC_MSK      GENMASK(1, 0)
#define ADXL38X_CLK_CTRL_EXT_CLK_RATE_MSK GENMASK(7, 4)

/* OP_MODE Register */
#define ADXL38X_OP_MODE_RANGE_MSK GENMASK(7, 6)
#define ADXL38X_OP_MODE_PDM_MSK   BIT(5)
#define ADXL38X_OP_MODE_AUDIO_MSK BIT(4)
#define ADXL38X_OP_MODE_MSK       GENMASK(3, 0)

/* DIG_EN Register */
#define ADXL38X_DIG_EN_CHANNEL_X_MSK    BIT(4)
#define ADXL38X_DIG_EN_CHANNEL_Y_MSK    BIT(5)
#define ADXL38X_DIG_EN_CHANNEL_Z_MSK    BIT(6)
#define ADXL38X_DIG_EN_CHANNEL_TEMP_MSK BIT(7)
#define ADXL38X_DIG_EN_CHANNEL_XYZ_MSK  GENMASK(6, 4)
#define ADXL38X_DIG_EN_FIFO_EN_MSK      BIT(3)
#define ADXL38X_DIG_EN_DOUBLE_SPEED_MSK BIT(2)
#define ADXL38X_DIG_EN_INT01_EVENT_MSK  BIT(1)
#define ADXL38X_DIG_EN_PARITY_EN_MSK    BIT(0)

/* TRIG_CFG Register (decimation controls that set ODR) */
#define ADXL38X_TRIG_CFG_DEC_2X_BYPASS_MSK BIT(7)
#define ADXL38X_TRIG_CFG_SINC_RATE_MSK     BIT(6)

/* FILTER Register */
#define ADXL38X_FILTER_HPF_CORNER_MSK GENMASK(2, 0)
#define ADXL38X_FILTER_HPF_PATH_MSK   BIT(3)
#define ADXL38X_FILTER_LPF_MODE_MSK   GENMASK(5, 4)
#define ADXL38X_FILTER_EQ_BYPASS_MSK  BIT(6)
#define ADXL38X_FILTER_DCF_BYPASS_MSK BIT(7)

/* FIFO_CFG0 Register */
#define ADXL38X_FIFO_CFG0_SAMPLES_8_MSK  BIT(0)
#define ADXL38X_FIFO_CFG0_EXT_TRIG_MSK   BIT(3)
#define ADXL38X_FIFO_CFG0_MODE_MSK       GENMASK(5, 4)
#define ADXL38X_FIFO_CFG0_CH_ID_MSK      BIT(6)
#define ADXL38X_FIFO_CFG0_READ_RESET_MSK BIT(7)

/* FIFO_CFG1 Register */
#define ADXL38X_FIFO_CFG1_SAMPLES_LOW_MSK GENMASK(7, 0)

/* FIFO_STATUS1 Register (bit 8 of the 9-bit entry count) */
#define ADXL38X_FIFO_STATUS1_ENTRIES_8_MSK BIT(0)

/* Interrupt map (INT0_MAP0 0x2B / INT1_MAP0 0x2D) */
#define ADXL38X_INT_MAP0_DATA_RDY_MSK       BIT(0)
#define ADXL38X_INT_MAP0_FIFO_FULL_MSK      BIT(1)
#define ADXL38X_INT_MAP0_FIFO_OVERRUN_MSK   BIT(2)
#define ADXL38X_INT_MAP0_FIFO_WATERMARK_MSK BIT(3)
#define ADXL38X_INT_MAP0_ACT_MSK            BIT(5)
#define ADXL38X_INT_MAP0_INACT_MSK          BIT(6)
#define ADXL38X_INT_MAP0_NVM_BUSY_MSK       BIT(7)

/* Interrupt map (INT0_MAP1 0x2C / INT1_MAP1 0x2E) */
#define ADXL38X_INT_MAP1_NVM_DONE_MSK BIT(7)

/* Interrupt pad config (INT0 0x5D / INT1 0x5E) */
#define ADXL38X_INT_DRV_MSK BIT(0)
#define ADXL38X_INT_PD_MSK  BIT(1)
#define ADXL38X_INT_POL_MSK BIT(7)

/* ACT_INACT_CTL Register */
#define ADXL38X_ACT_INACT_CTL_ACT_EN_MSK   GENMASK(1, 0)
#define ADXL38X_ACT_INACT_CTL_INACT_EN_MSK GENMASK(3, 2)
#define ADXL38X_ACT_INACT_CTL_LINKLOOP_MSK GENMASK(5, 4)
#define ADXL38X_ACT_INACT_CTL_HPF_MSK      BIT(6)
#define ADXL38X_ACT_INACT_CTL_PDM_POL_MSK  BIT(7)

/* Activity / inactivity enable codes (absolute mode) */
#define ADXL38X_ACT_INACT_DISABLE  0x00u
#define ADXL38X_ACT_INACT_ABSOLUTE 0x01u

/* SNSR_AXIS_EN Register */
#define ADXL38X_SNSR_AXIS_EN_MSK GENMASK(2, 0)

/* THRESH_ACT_H / THRESH_INACT_H (bits [10:8]) */
#define ADXL38X_THRESH_H_MSK GENMASK(2, 0)

/* RESET Register */
#define ADXL38X_REG_RESET_SOFT_RESET_MSK BIT(1)
#define ADXL38X_RESET_CODE               0x52u

/* EFUSE refresh after POR/soft reset takes ~2.2 ms; wait with margin before use. */
#define ADXL38X_RESET_SETTLE_MS 5U

/* The device may not answer the first SPI access right after power-on; retry the
 * device-ID read a few times before declaring the device absent.
 */
#define ADXL38X_ID_READ_RETRIES  5U
#define ADXL38X_ID_READ_RETRY_MS 2U

/* Device identification */
#define ADXL38X_DEVID_AD_VAL  0xADu
#define ADXL38X_DEVID_MST_VAL 0x1Du
#define ADXL38X_PART_ID_VAL   0x17u

/* FIFO modes (FIFO_CFG0 bits [5:4]) */
#define ADXL38X_FIFO_DISABLED 0x00u
#define ADXL38X_FIFO_NORMAL   0x01u
#define ADXL38X_FIFO_STREAM   0x02u
#define ADXL38X_FIFO_TRIGGER  0x03u

/* FIFO geometry */
#define ADXL38X_FIFO_MAX_SAMPLES 320u
#define ADXL38X_SAMPLE_SIZE      2u /* bytes per axis word */
#define ADXL38X_FIFO_CHANNELS    4u /* X, Y, Z, T stored per sample set */

/* FIFO_SAMPLES (watermark) is 9 bits: [7:0] in FIFO_CFG1, bit [8] in FIFO_CFG0. */
#define ADXL38X_FIFO_SAMPLES_LOW_BITS 8u

/* ADXL380 Sensitivity values (LSB/g, 16-bit DSM) */
#define ADXL380_SENSITIVITY_4G  7500
#define ADXL380_SENSITIVITY_8G  3750
#define ADXL380_SENSITIVITY_16G 1875

/* ADXL382 Sensitivity values (LSB/g, 16-bit DSM) */
#define ADXL382_SENSITIVITY_15G 2000
#define ADXL382_SENSITIVITY_30G 1000
#define ADXL382_SENSITIVITY_60G 500

/* Full-scale measurement range, in g, for each DT range setting */
#define ADXL380_RANGE_4G_FS  4
#define ADXL380_RANGE_8G_FS  8
#define ADXL380_RANGE_16G_FS 16
#define ADXL382_RANGE_15G_FS 15
#define ADXL382_RANGE_30G_FS 30
#define ADXL382_RANGE_60G_FS 60

/* Activity threshold: signed SAR hits full scale (range_g) at 2^11 codes; field is 11-bit */
#define ADXL38X_SAR_FULL_SCALE_CODES 2048
#define ADXL38X_THRESH_MAX           GENMASK(10, 0)

/* sensor_value.val2 is expressed in micro-units (one-millionths). */
#define ADXL38X_MICRO_PER_UNIT 1000000

/* Temperature conversion: T(degC) = 25 + (raw - 550) / 10.2 */
#define ADXL38X_TEMP_REF_DEG_C     25 /* Reference temperature at the intercept */
#define ADXL38X_TEMP_25C_INTERCEPT 550
#define ADXL38X_TEMP_SENS_X10      102 /* 10.2 LSB/degC scaled by 10 */
#define ADXL38X_TEMP_SENS_SCALE    10  /* The x10 scaling baked into *_SENS_X10 */

/* Activity timer: 1 LSB = 500 us, 24-bit field (TIME_ACT/TIME_INACT across 3 registers) */
#define ADXL38X_TIME_ACT_LSB_US 500
#define ADXL38X_TIME_ACT_MAX    GENMASK(23, 0)

/* DSM-mode ODR multipliers applied on top of the per-mode base rate. */
#define ADXL38X_ODR_MULT_SINC_RATE    4U /* 16x sinc filter (dec2x implicitly bypassed) */
#define ADXL38X_ODR_MULT_DEC2X_BYPASS 2U /* 2x decimation bypassed */
#define ADXL38X_ODR_MULT_DOUBLE_SPEED 2U /* 1024 kHz system clock (ADXL380, external clock) */

/* ADXL38X base output data rates (Hz) */
#define ADXL38X_ODR_50_HZ    50U
#define ADXL38X_ODR_400_HZ   400U
#define ADXL38X_ODR_500_HZ   500U
#define ADXL38X_ODR_2000_HZ  2000U
#define ADXL38X_ODR_4000_HZ  4000U
#define ADXL38X_ODR_8000_HZ  8000U
#define ADXL38X_ODR_16000_HZ 16000U

/**
 * @brief ADXL38X bus configuration
 */
union adxl38x_bus {
#if defined(ADXL38X_BUS_SPI)
	struct spi_dt_spec spi;
#endif
	uint8_t dummy;
};

typedef int (*adxl38x_bus_is_ready_fn)(const union adxl38x_bus *bus);

typedef int (*adxl38x_reg_access_fn)(const struct device *dev, bool read, uint8_t reg_addr,
				     uint8_t *data, size_t length);

/**
 * @brief ADXL38X operating modes (OP_MODE bits [3:0])
 */
enum adxl38x_op_mode {
	ADXL38X_STANDBY_MODE = 0,
	ADXL38X_HEART_SOUND_MODE = 1,
	ADXL38X_ULTRA_LP_MODE = 2,
	ADXL38X_VERY_LP_MODE = 3,
	ADXL38X_LP_MODE = 4,
	ADXL38X_LP_ULP_MODE = 6,
	ADXL38X_LP_VLP_MODE = 7,
	ADXL38X_RBW_MODE = 8,
	ADXL38X_RBW_ULP_MODE = 10,
	ADXL38X_RBW_VLP_MODE = 11,
	ADXL38X_HP_MODE = 12,
	ADXL38X_HP_ULP_MODE = 14,
	ADXL38X_HP_VLP_MODE = 15,
};

/**
 * @brief Logical measurement channels
 */
enum adxl38x_channel {
	ADXL38X_X_AXIS = 0,
	ADXL38X_Y_AXIS = 1,
	ADXL38X_Z_AXIS = 2,
	ADXL38X_TEMP_CHANNEL = 3,
};

/**
 * @brief Supported chip variants
 */
enum adxl38x_chip_type {
	ADXL38X_CHIP_ADXL380 = 0,
	ADXL38X_CHIP_ADXL382 = 1,
};

/**
 * @brief ADXL38X single-sample data
 */
struct adxl38x_sample {
	uint8_t is_fifo: 1;
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t temp;
	uint8_t range;
	enum adxl38x_chip_type chip_type;
};

/**
 * @brief ADXL38X FIFO frame header (prepended to a streamed buffer)
 */
struct adxl38x_fifo_data {
	uint8_t is_fifo: 1;
	uint64_t timestamp;
	uint8_t status0;
	uint8_t range;
	enum adxl38x_chip_type chip_type;
	uint32_t accel_odr;
	uint16_t fifo_samples;
	uint16_t fifo_byte_count;
	uint8_t sample_set_size;
} __packed;

/**
 * @brief ADXL38X device configuration (compile-time / devicetree)
 */
struct adxl38x_dev_config {
	const union adxl38x_bus bus;
	adxl38x_bus_is_ready_fn bus_is_ready;
	adxl38x_reg_access_fn reg_access;
	enum adxl38x_chip_type chip_type;
	enum adxl38x_op_mode op_mode;
	uint16_t fifo_watermark;
	uint8_t range;
	uint8_t hpf_corner: 3;
	uint8_t int_pol: 1;
	uint8_t double_speed: 1;
#ifdef CONFIG_ADXL38X_TRIGGER
	struct gpio_dt_spec interrupt_gpio;
	bool use_int1;
#endif
};

/**
 * @brief ADXL38X runtime data
 */
struct adxl38x_data {
	struct adxl38x_sample samples;
	enum adxl38x_op_mode op_mode;
	uint32_t accel_odr;
	uint16_t fifo_watermark;
	int16_t temp_val;
	uint8_t range;
	uint8_t hpf_corner: 3;
	uint8_t int_pol: 1;
	uint8_t sinc_rate: 1;
	uint8_t dec2x_bypass: 1;
	uint8_t double_speed: 1;
#ifdef CONFIG_ADXL38X_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	bool use_int1;

	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t act_handler;
	const struct sensor_trigger *act_trigger;
#if defined(CONFIG_ADXL38X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADXL38X_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ADXL38X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ADXL38X_TRIGGER */
#ifdef CONFIG_ADXL38X_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint8_t status0;
	uint8_t fifo_ent[2];
	uint64_t timestamp;
	uint8_t fifo_watermark_irq;
#endif
};

/**
 * @brief Read one X/Y/Z acceleration sample set
 *
 * Sets x/y/z, range and chip_type; leaves is_fifo untouched.
 *
 * @param dev Device pointer
 * @param sample Destination sample
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_read_accel_data(const struct device *dev, struct adxl38x_sample *sample);

/**
 * @brief Read the sign-extended raw die-temperature sample
 *
 * @param dev Device pointer
 * @param raw Destination for the raw temperature code
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_read_temp_raw(const struct device *dev, int16_t *raw);

/**
 * @brief ADXL38X register read
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Destination buffer
 * @param length Number of bytes to read
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length);

/**
 * @brief ADXL38X register write
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Source buffer
 * @param length Number of bytes to write
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length);

/**
 * @brief ADXL38X read-modify-write of a masked register field
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param mask Field bit mask
 * @param value Value to write into the masked field
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);

/**
 * @brief Set the ADXL38X operating mode
 *
 * @param dev Device pointer
 * @param op_mode Operating mode to set
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_set_op_mode(const struct device *dev, enum adxl38x_op_mode op_mode);

#ifdef CONFIG_ADXL38X_TRIGGER
/**
 * @brief Initialize the ADXL38X interrupt / GPIO plumbing
 *
 * @param dev Device pointer
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_init_interrupt(const struct device *dev);

/**
 * @brief Configure an ADXL38X trigger
 *
 * @param dev Device pointer
 * @param trig Trigger specification
 * @param handler Trigger handler callback
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
#endif /* CONFIG_ADXL38X_TRIGGER */

#ifdef CONFIG_ADXL38X_STREAM
/**
 * @brief ADXL38X streaming IRQ handler (kicks the RTIO chain)
 *
 * @param dev Device pointer
 */
void adxl38x_stream_irq_handler(const struct device *dev);
#endif

#ifdef CONFIG_SENSOR_ASYNC_API
/**
 * @brief Submit an ADXL38X RTIO read request
 *
 * @param dev Device pointer
 * @param iodev_sqe RTIO submission queue entry
 */
void adxl38x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Submit an ADXL38X RTIO streaming request
 *
 * @param dev Device pointer
 * @param iodev_sqe RTIO submission queue entry
 */
void adxl38x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Get the ADXL38X sensor data decoder
 *
 * @param dev Device pointer
 * @param decoder Set to the decoder API on success
 * @return 0 on success, negative errno code on failure.
 */
int adxl38x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif /* CONFIG_SENSOR_ASYNC_API */

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL38X_ADXL38X_H_ */
