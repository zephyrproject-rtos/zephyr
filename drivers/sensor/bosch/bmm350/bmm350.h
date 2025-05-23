/*
 * Copyright (c) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMM350s accessed via I2C.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_H_
#define ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT bosch_bmm350

#define BMM350_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

struct bmm350_bus {
	struct i2c_dt_spec i2c;
};

typedef int (*bmm350_bus_check_fn)(const struct bmm350_bus *bus);
typedef int (*bmm350_reg_read_fn)(const struct bmm350_bus *bus, uint8_t start, uint8_t *buf,
				  int size);
typedef int (*bmm350_reg_write_fn)(const struct bmm350_bus *bus, uint8_t reg, uint8_t val);

struct bmm350_bus_io {
	bmm350_bus_check_fn check;
	bmm350_reg_read_fn read;
	bmm350_reg_write_fn write;
};

extern const struct bmm350_bus_io bmm350_bus_io_i2c;

#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/gpio.h>

#define DT_DRV_COMPAT  bosch_bmm350
#define BMM350_OK      (0)
#define BMM350_DISABLE UINT8_C(0x0)
#define BMM350_ENABLE  UINT8_C(0x1)

#define BMM350_REG_CHIP_ID           UINT8_C(0x00)
#define BMM350_REG_REV_ID            UINT8_C(0x01)
#define BMM350_REG_ERR_REG           UINT8_C(0x02)
#define BMM350_REG_PAD_CTRL          UINT8_C(0x03)
#define BMM350_REG_PMU_CMD_AGGR_SET  UINT8_C(0x04)
#define BMM350_REG_PMU_CMD_AXIS_EN   UINT8_C(0x05)
#define BMM350_REG_PMU_CMD           UINT8_C(0x06)
#define BMM350_REG_PMU_CMD_STATUS_0  UINT8_C(0x07)
#define BMM350_REG_PMU_CMD_STATUS_1  UINT8_C(0x08)
#define BMM350_REG_I3C_ERR           UINT8_C(0x09)
#define BMM350_REG_I2C_WDT_SET       UINT8_C(0x0A)
#define BMM350_REG_TRSDCR_REV_ID     UINT8_C(0x0D)
#define BMM350_REG_TC_SYNC_TU        UINT8_C(0x21)
#define BMM350_REG_TC_SYNC_ODR       UINT8_C(0x22)
#define BMM350_REG_TC_SYNC_TPH_1     UINT8_C(0x23)
#define BMM350_REG_TC_SYNC_TPH_2     UINT8_C(0x24)
#define BMM350_REG_TC_SYNC_DT        UINT8_C(0x25)
#define BMM350_REG_TC_SYNC_ST_0      UINT8_C(0x26)
#define BMM350_REG_TC_SYNC_ST_1      UINT8_C(0x27)
#define BMM350_REG_TC_SYNC_ST_2      UINT8_C(0x28)
#define BMM350_REG_TC_SYNC_STATUS    UINT8_C(0x29)
#define BMM350_REG_INT_CTRL          UINT8_C(0x2E)
#define BMM350_REG_INT_CTRL_IBI      UINT8_C(0x2F)
#define BMM350_REG_INT_STATUS        UINT8_C(0x30)
#define BMM350_REG_MAG_X_XLSB        UINT8_C(0x31)
#define BMM350_REG_MAG_X_LSB         UINT8_C(0x32)
#define BMM350_REG_MAG_X_MSB         UINT8_C(0x33)
#define BMM350_REG_MAG_Y_XLSB        UINT8_C(0x34)
#define BMM350_REG_MAG_Y_LSB         UINT8_C(0x35)
#define BMM350_REG_MAG_Y_MSB         UINT8_C(0x36)
#define BMM350_REG_MAG_Z_XLSB        UINT8_C(0x37)
#define BMM350_REG_MAG_Z_LSB         UINT8_C(0x38)
#define BMM350_REG_MAG_Z_MSB         UINT8_C(0x39)
#define BMM350_REG_TEMP_XLSB         UINT8_C(0x3A)
#define BMM350_REG_TEMP_LSB          UINT8_C(0x3B)
#define BMM350_REG_TEMP_MSB          UINT8_C(0x3C)
#define BMM350_REG_SENSORTIME_XLSB   UINT8_C(0x3D)
#define BMM350_REG_SENSORTIME_LSB    UINT8_C(0x3E)
#define BMM350_REG_SENSORTIME_MSB    UINT8_C(0x3F)
#define BMM350_REG_OTP_CMD_REG       UINT8_C(0x50)
#define BMM350_REG_OTP_DATA_MSB_REG  UINT8_C(0x52)
#define BMM350_REG_OTP_DATA_LSB_REG  UINT8_C(0x53)
#define BMM350_REG_OTP_STATUS_REG    UINT8_C(0x55)
#define BMM350_REG_TMR_SELFTEST_USER UINT8_C(0x60)
#define BMM350_REG_CTRL_USER         UINT8_C(0x61)
#define BMM350_REG_CMD               UINT8_C(0x7E)

/************************* Sensor Shuttle Variant **************************/
#define BMM350_LEGACY_SHUTTLE_VARIANT_ID  UINT8_C(0x10)
#define BMM350_CURRENT_SHUTTLE_VARIANT_ID UINT8_C(0x11)

/********************* Sensor interface success code **********************/
#define BMM350_INTF_RET_SUCCESS INT8_C(0)

/* default value */
#define BMM350_CHIP_ID                 UINT8_C(0x33)
#define BMM350_REV_ID                  UINT8_C(0x00)
#define BMM350_OTP_CMD_DIR_READ        UINT8_C(0x20)
#define BMM350_OTP_WORD_ADDR_MSK       UINT8_C(0x1F)
#define BMM350_OTP_STATUS_ERROR_MSK    UINT8_C(0xE0)
#define BMM350_OTP_STATUS_ERROR(val)   (val & BMM350_OTP_STATUS_ERROR_MSK)
#define BMM350_OTP_STATUS_NO_ERROR     UINT8_C(0x00)
#define BMM350_OTP_STATUS_BOOT_ERR     (0x20)
#define BMM350_OTP_STATUS_PAGE_RD_ERR  (0x40)
#define BMM350_OTP_STATUS_PAGE_PRG_ERR (0x60)
#define BMM350_OTP_STATUS_SIGN_ERR     (0x80)
#define BMM350_OTP_STATUS_INV_CMD_ERR  (0xA0)
#define BMM350_OTP_STATUS_CMD_DONE     UINT8_C(0x01)
#define BMM350_CMD_SOFTRESET           UINT8_C(0xB6)

/****************************** OTP indices ***************************/
#define BMM350_TEMP_OFF_SENS UINT8_C(0x0D)
#define BMM350_MAG_OFFSET_X  UINT8_C(0x0E)
#define BMM350_MAG_OFFSET_Y  UINT8_C(0x0F)
#define BMM350_MAG_OFFSET_Z  UINT8_C(0x10)

#define BMM350_MAG_SENS_X UINT8_C(0x10)
#define BMM350_MAG_SENS_Y UINT8_C(0x11)
#define BMM350_MAG_SENS_Z UINT8_C(0x11)

#define BMM350_MAG_TCO_X UINT8_C(0x12)
#define BMM350_MAG_TCO_Y UINT8_C(0x13)
#define BMM350_MAG_TCO_Z UINT8_C(0x14)

#define BMM350_MAG_TCS_X UINT8_C(0x12)
#define BMM350_MAG_TCS_Y UINT8_C(0x13)
#define BMM350_MAG_TCS_Z UINT8_C(0x14)

#define BMM350_MAG_DUT_T_0 UINT8_C(0x18)

#define BMM350_CROSS_X_Y UINT8_C(0x15)
#define BMM350_CROSS_Y_X UINT8_C(0x15)
#define BMM350_CROSS_Z_X UINT8_C(0x16)
#define BMM350_CROSS_Z_Y UINT8_C(0x16)

/**************************** Signed bit macros **********************/
enum bmm350_signed_bit {
	BMM350_SIGNED_8_BIT = 8,
	BMM350_SIGNED_12_BIT = 12,
	BMM350_SIGNED_16_BIT = 16,
	BMM350_SIGNED_21_BIT = 21,
	BMM350_SIGNED_24_BIT = 24
};

/********************* Power modes *************************/
#define BMM350_PMU_CMD_SUS        0x00
#define BMM350_PMU_CMD_NM         0x01
#define BMM350_PMU_CMD_UPD_OAE    UINT8_C(0x02)
#define BMM350_PMU_CMD_FM         0x03
#define BMM350_PMU_CMD_FM_FAST    0x04
#define BMM350_PMU_CMD_FGR        UINT8_C(0x05)
#define BMM350_PMU_CMD_FGR_FAST   UINT8_C(0x06)
#define BMM350_PMU_CMD_BR         UINT8_C(0x07)
#define BMM350_PMU_CMD_BR_FAST    UINT8_C(0x08)
#define BMM350_PMU_CMD_ENABLE_XYZ UINT8_C(0x70)
#define BMM350_PMU_STATUS_0       UINT8_C(0x00)

/**************************** PMU command status 0 macros **********************/
#define BMM350_PMU_CMD_STATUS_0_SUS      UINT8_C(0x00)
#define BMM350_PMU_CMD_STATUS_0_NM       UINT8_C(0x01)
#define BMM350_PMU_CMD_STATUS_0_UPD_OAE  UINT8_C(0x02)
#define BMM350_PMU_CMD_STATUS_0_FM       UINT8_C(0x03)
#define BMM350_PMU_CMD_STATUS_0_FM_FAST  UINT8_C(0x04)
#define BMM350_PMU_CMD_STATUS_0_FGR      UINT8_C(0x05)
#define BMM350_PMU_CMD_STATUS_0_FGR_FAST UINT8_C(0x06)
#define BMM350_PMU_CMD_STATUS_0_BR       UINT8_C(0x07)
#define BMM350_PMU_CMD_STATUS_0_BR_FAST  UINT8_C(0x07)

/*********************** Macros for bit masking ***************************/
#define BMM350_AVG_MSK                (0x30)
#define BMM350_AVG_POS                UINT8_C(0x04)
#define BMM350_PMU_CMD_BUSY_MSK       UINT8_C(0x01)
#define BMM350_PMU_CMD_BUSY_POS       UINT8_C(0x00)
#define BMM350_ODR_OVWR_MSK           UINT8_C(0x02)
#define BMM350_ODR_OVWR_POS           UINT8_C(0x01)
#define BMM350_AVG_OVWR_MSK           UINT8_C(0x04)
#define BMM350_AVG_OVWR_POS           UINT8_C(0x02)
#define BMM350_PWR_MODE_IS_NORMAL_MSK UINT8_C(0x08)
#define BMM350_PWR_MODE_IS_NORMAL_POS UINT8_C(0x03)
#define BMM350_CMD_IS_ILLEGAL_MSK     UINT8_C(0x10)
#define BMM350_CMD_IS_ILLEGAL_POS     UINT8_C(0x04)
#define BMM350_PMU_CMD_VALUE_MSK      UINT8_C(0xE0)
#define BMM350_PMU_CMD_VALUE_POS      UINT8_C(0x05)

/**************************** Self-test macros **********************/
#define BMM350_SELF_TEST_DISABLE UINT8_C(0x00)
#define BMM350_SELF_TEST_POS_X   UINT8_C(0x0D)
#define BMM350_SELF_TEST_NEG_X   UINT8_C(0x0B)
#define BMM350_SELF_TEST_POS_Y   UINT8_C(0x15)
#define BMM350_SELF_TEST_NEG_Y   UINT8_C(0x13)

/************************* Sensor delay time settings in microseconds **************************/
#define BMM350_SOFT_RESET_DELAY       UINT32_C(24000)
#define BMM350_MAGNETIC_RESET_DELAY   UINT32_C(40000)
#define BMM350_START_UP_TIME_FROM_POR UINT32_C(3000)

#define BMM350_GOTO_SUSPEND_DELAY      UINT32_C(6000)
#define BMM350_SUSPEND_TO_NORMAL_DELAY UINT32_C(38000)

#define BMM350_SUS_TO_FORCEDMODE_NO_AVG_DELAY (15000)
#define BMM350_SUS_TO_FORCEDMODE_AVG_2_DELAY  (17000)
#define BMM350_SUS_TO_FORCEDMODE_AVG_4_DELAY  (20000)
#define BMM350_SUS_TO_FORCEDMODE_AVG_8_DELAY  (28000)

#define BMM350_SUS_TO_FORCEDMODE_FAST_NO_AVG_DELAY (4000)
#define BMM350_SUS_TO_FORCEDMODE_FAST_AVG_2_DELAY  (5000)
#define BMM350_SUS_TO_FORCEDMODE_FAST_AVG_4_DELAY  (9000)
#define BMM350_SUS_TO_FORCEDMODE_FAST_AVG_8_DELAY  (16000)

#define BMM350_PMU_CMD_NM_TC       UINT8_C(0x09)
#define BMM350_OTP_DATA_LENGTH     UINT8_C(32)
#define BMM350_READ_BUFFER_LENGTH  UINT8_C(127)
#define BMM350_MAG_TEMP_DATA_LEN   UINT8_C(12)
#define BMM350_OTP_CMD_PWR_OFF_OTP UINT8_C(0x80)
#define BMM350_UPD_OAE_DELAY       UINT16_C(1000)

#define BMM350_BR_DELAY         UINT16_C(14000)
#define BMM350_FGR_DELAY        UINT16_C(18000)
#define BMM350_SOFT_RESET_DELAY UINT32_C(24000)

#define BMM350_LSB_MASK UINT16_C(0x00FF)
#define BMM350_MSB_MASK UINT16_C(0xFF00)

#define BMM350_LSB_TO_UT_XY_COEFF     71
#define BMM350_LSB_TO_UT_Z_COEFF      72
#define BMM350_LSB_TO_UT_TEMP_COEFF   10
#define BMM350_LSB_TO_UT_COEFF_DIV    10000
#define BMM350_MAG_COMP_COEFF_SCALING 1000

#define BMM350_SENS_CORR_Y 1
#define BMM350_TCS_CORR_Z  1

#define BMM350_EN_X_MSK            UINT8_C(0x01)
#define BMM350_EN_X_POS            UINT8_C(0x0)
#define BMM350_EN_Y_MSK            UINT8_C(0x02)
#define BMM350_EN_Y_POS            UINT8_C(0x1)
#define BMM350_EN_Z_MSK            UINT8_C(0x04)
#define BMM350_EN_Z_POS            UINT8_C(0x2)
#define BMM350_EN_XYZ_MSK          UINT8_C(0x7)
#define BMM350_EN_XYZ_POS          UINT8_C(0x0)
/************************ Averaging macros **********************/
#define BMM350_AVG_NO_AVG          0x0
#define BMM350_AVG_2               0x1
#define BMM350_AVG_4               0x2
#define BMM350_AVG_8               0x3
/******************************* ODR **************************/
#define BMM350_ODR_400HZ           UINT8_C(0x2)
#define BMM350_ODR_200HZ           UINT8_C(0x3)
#define BMM350_ODR_100HZ           UINT8_C(0x4)
#define BMM350_ODR_50HZ            UINT8_C(0x5)
#define BMM350_ODR_25HZ            UINT8_C(0x6)
#define BMM350_ODR_12_5HZ          UINT8_C(0x7)
#define BMM350_ODR_6_25HZ          UINT8_C(0x8)
#define BMM350_ODR_3_125HZ         UINT8_C(0x9)
#define BMM350_ODR_1_5625HZ        UINT8_C(0xA)
#define BMM350_ODR_MSK             UINT8_C(0xf)
#define BMM350_ODR_POS             UINT8_C(0x0)
#define BMM350_DATA_READY_INT_CTRL UINT8_C(0x8e)

/* Macro to SET and GET BITS of a register*/
#define BMM350_SET_BITS(reg_data, bitname, data)                                                   \
	((reg_data & ~(bitname##_MSK)) | ((data << bitname##_POS) & bitname##_MSK))

#define BMM350_GET_BITS(reg_data, bitname) ((reg_data & (bitname##_MSK)) >> (bitname##_POS))

#define BMM350_GET_BITS_POS_0(reg_data, bitname) (reg_data & (bitname##_MSK))

#define BMM350_SET_BITS_POS_0(reg_data, bitname, data)                                             \
	((reg_data & ~(bitname##_MSK)) | (data & bitname##_MSK))

enum bmm350_power_modes {
	BMM350_SUSPEND_MODE = BMM350_PMU_CMD_SUS,
	BMM350_NORMAL_MODE = BMM350_PMU_CMD_NM,
	BMM350_FORCED_MODE = BMM350_PMU_CMD_FM,
	BMM350_FORCED_MODE_FAST = BMM350_PMU_CMD_FM_FAST
};

enum bmm350_data_rates {
	BMM350_DATA_RATE_400HZ = 2,    /* BMM350_ODR_400HZ */
	BMM350_DATA_RATE_200HZ = 3,    /* BMM350_ODR_200HZ */
	BMM350_DATA_RATE_100HZ = 4,    /* BMM350_ODR_100HZ */
	BMM350_DATA_RATE_50HZ = 5,     /* BMM350_ODR_50HZ */
	BMM350_DATA_RATE_25HZ = 6,     /* BMM350_ODR_25HZ */
	BMM350_DATA_RATE_12_5HZ = 7,   /* BMM350_ODR_12_5HZ */
	BMM350_DATA_RATE_6_25HZ = 8,   /* BMM350_ODR_6_25HZ */
	BMM350_DATA_RATE_3_125HZ = 9,  /* BMM350_ODR_3_125HZ */
	BMM350_DATA_RATE_1_5625HZ = 10 /* BMM350_ODR_1_5625HZ */
};
enum bmm350_performance_parameters {
	BMM350_NO_AVERAGING = BMM350_AVG_NO_AVG,
	BMM350_AVERAGING_2 = BMM350_AVG_2,
	BMM350_AVERAGING_4 = BMM350_AVG_4,
	BMM350_AVERAGING_8 = BMM350_AVG_8,
	BMM350_ULTRALOWNOISE = BMM350_AVG_8,
	BMM350_LOWNOISE = BMM350_AVG_4,
	BMM350_REGULARPOWER = BMM350_AVG_2,
	BMM350_LOWPOWER = BMM350_AVG_NO_AVG
};

/*!
 * @brief bmm350 compensated magnetometer data and temperature data
 */
struct bmm350_mag_temp_data {
	/*! Compensated mag X data */
	int32_t x;

	/*! Compensated mag Y data */
	int32_t y;

	/*! Compensated mag Z data */
	int32_t z;

	/*! Temperature */
	int32_t temperature;
};

/*!
 * @brief bmm350 magnetometer dut offset coefficient structure
 */
struct bmm350_dut_offset_coef {
	/*! Temperature offset */
	int32_t t_offs;

	/*! Offset x-axis */
	int32_t offset_x;

	/*! Offset y-axis */
	int32_t offset_y;

	/*! Offset z-axis */
	int32_t offset_z;
};

/*!
 * @brief bmm350 magnetometer dut sensitivity coefficient structure
 */
struct bmm350_dut_sensit_coef {
	/*! Temperature sensitivity */
	int32_t t_sens;

	/*! Sensitivity x-axis */
	int32_t sens_x;

	/*! Sensitivity y-axis */
	int32_t sens_y;

	/*! Sensitivity z-axis */
	int32_t sens_z;
};

/*!
 * @brief bmm350 magnetometer dut tco structure
 */
struct bmm350_dut_tco {
	int32_t tco_x;
	int32_t tco_y;
	int32_t tco_z;
};

/*!
 * @brief bmm350 magnetometer dut tcs structure
 */
struct bmm350_dut_tcs {
	int32_t tcs_x;
	int32_t tcs_y;
	int32_t tcs_z;
};

/*!
 * @brief bmm350 magnetometer cross axis compensation structure
 */
struct bmm350_cross_axis {
	int32_t cross_x_y;
	int32_t cross_y_x;
	int32_t cross_z_x;
	int32_t cross_z_y;
};
struct mag_compensate {
	/*! Structure to store dut offset coefficient */
	struct bmm350_dut_offset_coef dut_offset_coef;

	/*! Structure to store dut sensitivity coefficient */
	struct bmm350_dut_sensit_coef dut_sensit_coef;

	/*! Structure to store dut tco */
	struct bmm350_dut_tco dut_tco;

	/*! Structure to store dut tcs */
	struct bmm350_dut_tcs dut_tcs;

	/*! Initialize T0_reading parameter */
	int32_t dut_t0;

	/*! Structure to define cross axis compensation */
	struct bmm350_cross_axis cross_axis;
};

struct bmm350_pmu_cmd_status_0 {
	/*! The previous PMU CMD is still in processing */
	uint8_t pmu_cmd_busy;

	/*! The previous PMU_CMD_AGGR_SET.odr has been overwritten */
	uint8_t odr_ovwr;

	/*! The previous PMU_CMD_AGGR_SET.avg has been overwritten */
	uint8_t avr_ovwr;

	/*! The chip is in normal power mode */
	uint8_t pwr_mode_is_normal;

	/*! CMD value is not allowed */
	uint8_t cmd_is_illegal;

	/*! Stores the latest PMU_CMD code processed */
	uint8_t pmu_cmd_value;
};

/*!
 * @brief bmm350 un-compensated (raw) magnetometer data, signed integer
 */
struct bmm350_raw_mag_data {
	/*! Raw mag X data */
	int32_t raw_xdata;

	/*! Raw mag Y data */
	int32_t raw_ydata;

	/*! Raw mag Z data */
	int32_t raw_zdata;

	/*! Raw mag temperature value */
	int32_t raw_data_temp;
};

struct bmm350_config {
	struct bmm350_bus bus;
	const struct bmm350_bus_io *bus_io;
	struct gpio_dt_spec drdy_int;
};

struct bmm350_data {

	/*! Variable to store status of axes enabled */
	uint8_t axis_en;
	struct mag_compensate mag_comp;
	/*! Array to store OTP data */
	uint16_t otp_data[BMM350_OTP_DATA_LENGTH];
	/*! Variant ID */
	uint8_t var_id;
	/*! Variable to enable/disable xy bit reset */
	uint8_t enable_auto_br;
	struct bmm350_mag_temp_data mag_temp_data;

#ifdef CONFIG_BMM350_TRIGGER
	struct gpio_callback gpio_cb;
#endif

#ifdef CONFIG_BMM350_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_BMM350_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif

#if defined(CONFIG_BMM350_TRIGGER_GLOBAL_THREAD)
	const struct device *dev;
#endif

#ifdef CONFIG_BMM350_TRIGGER
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t drdy_handler;
#endif /* CONFIG_BMM350_TRIGGER */
};

int bmm350_trigger_mode_init(const struct device *dev);

int bmm350_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int bmm350_reg_write(const struct device *dev, uint8_t reg, uint8_t val);
#endif /* __SENSOR_BMM350_H__ */
