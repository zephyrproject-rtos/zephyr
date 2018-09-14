/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL372_ADXL372_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL372_ADXL372_H_

#include <zephyr/types.h>
#include <device.h>
#include <gpio.h>
#include <spi.h>
#include <i2c.h>

#define GENMASK(h, l) (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (31 - (h))))

/*
 * ADXL372 registers definition
 */
#define ADXL372_DEVID		0x00u  /* Analog Devices accelerometer ID */
#define ADXL372_DEVID_MST	0x01u  /* Analog Devices MEMS device ID */
#define ADXL372_PARTID		0x02u  /* Device ID */
#define ADXL372_REVID		0x03u  /* product revision ID*/
#define ADXL372_STATUS_1	0x04u  /* Status register 1 */
#define ADXL372_STATUS_2	0x05u  /* Status register 2 */
#define ADXL372_FIFO_ENTRIES_2	0x06u  /* Valid data samples in the FIFO */
#define ADXL372_FIFO_ENTRIES_1	0x07u  /* Valid data samples in the FIFO */
#define ADXL372_X_DATA_H	0x08u  /* X-axis acceleration data [11:4] */
#define ADXL372_X_DATA_L	0x09u  /* X-axis acceleration data [3:0] */
#define ADXL372_Y_DATA_H	0x0Au  /* Y-axis acceleration data [11:4] */
#define ADXL372_Y_DATA_L	0x0Bu  /* Y-axis acceleration data [3:0] */
#define ADXL372_Z_DATA_H	0x0Cu  /* Z-axis acceleration data [11:4] */
#define ADXL372_Z_DATA_L	0x0Du  /* Z-axis acceleration data [3:0] */
#define ADXL372_X_MAXPEAK_H	0x15u  /* X-axis MaxPeak acceleration data */
#define ADXL372_X_MAXPEAK_L	0x16u  /* X-axis MaxPeak acceleration data */
#define ADXL372_Y_MAXPEAK_H	0x17u  /* Y-axis MaxPeak acceleration data */
#define ADXL372_Y_MAXPEAK_L	0x18u  /* Y-axis MaxPeak acceleration data */
#define ADXL372_Z_MAXPEAK_H	0x19u  /* Z-axis MaxPeak acceleration data */
#define ADXL372_Z_MAXPEAK_L	0x1Au  /* Z-axis MaxPeak acceleration data */
#define ADXL372_OFFSET_X	0x20u  /* X axis offset */
#define ADXL372_OFFSET_Y	0x21u  /* Y axis offset */
#define ADXL372_OFFSET_Z	0x22u  /* Z axis offset */
#define ADXL372_X_THRESH_ACT_H	0x23u  /* X axis Activity Threshold [15:8] */
#define ADXL372_X_THRESH_ACT_L	0x24u  /* X axis Activity Threshold [7:0] */
#define ADXL372_Y_THRESH_ACT_H	0x25u  /* Y axis Activity Threshold [15:8] */
#define ADXL372_Y_THRESH_ACT_L	0x26u  /* Y axis Activity Threshold [7:0] */
#define ADXL372_Z_THRESH_ACT_H	0x27u  /* Z axis Activity Threshold [15:8] */
#define ADXL372_Z_THRESH_ACT_L	0x28u  /* Z axis Activity Threshold [7:0] */
#define ADXL372_TIME_ACT	0x29u  /* Activity Time */
#define ADXL372_X_THRESH_INACT_H	0x2Au  /* X axis Inactivity Threshold */
#define ADXL372_X_THRESH_INACT_L	0x2Bu  /* X axis Inactivity Threshold */
#define ADXL372_Y_THRESH_INACT_H	0x2Cu  /* Y axis Inactivity Threshold */
#define ADXL372_Y_THRESH_INACT_L	0x2Du  /* Y axis Inactivity Threshold */
#define ADXL372_Z_THRESH_INACT_H	0x2Eu  /* Z axis Inactivity Threshold */
#define ADXL372_Z_THRESH_INACT_L	0x2Fu  /* Z axis Inactivity Threshold */
#define ADXL372_TIME_INACT_H	0x30u  /* Inactivity Time [15:8] */
#define ADXL372_TIME_INACT_L	0x31u  /* Inactivity Time [7:0] */
#define ADXL372_X_THRESH_ACT2_H	0x32u  /* X axis Activity2 Threshold [15:8] */
#define ADXL372_X_THRESH_ACT2_L	0x33u  /* X axis Activity2 Threshold [7:0] */
#define ADXL372_Y_THRESH_ACT2_H	0x34u  /* Y axis Activity2 Threshold [15:8] */
#define ADXL372_Y_THRESH_ACT2_L	0x35u  /* Y axis Activity2 Threshold [7:0] */
#define ADXL372_Z_THRESH_ACT2_H	0x36u  /* Z axis Activity2 Threshold [15:8] */
#define ADXL372_Z_THRESH_ACT2_L	0x37u  /* Z axis Activity2 Threshold [7:0] */
#define ADXL372_HPF		0x38u  /* High Pass Filter */
#define ADXL372_FIFO_SAMPLES	0x39u  /* FIFO Samples */
#define ADXL372_FIFO_CTL	0x3Au  /* FIFO Control */
#define ADXL372_INT1_MAP	0x3Bu  /* Interrupt 1 mapping control */
#define ADXL372_INT2_MAP        0x3Cu  /* Interrupt 2 mapping control */
#define ADXL372_TIMING		0x3Du  /* Timing */
#define ADXL372_MEASURE		0x3Eu  /* Measure */
#define ADXL372_POWER_CTL	0x3Fu  /* Power control */
#define ADXL372_SELF_TEST	0x40u  /* Self Test */
#define ADXL372_RESET		0x41u  /* Reset */
#define ADXL372_FIFO_DATA	0x42u  /* FIFO Data */

#define ADXL372_DEVID_VAL	0xADu  /* Analog Devices accelerometer ID */
#define ADXL372_MST_DEVID_VAL	0x1Du  /* Analog Devices MEMS device ID */
#define ADXL372_PARTID_VAL	0xFAu  /* Device ID */
#define ADXL372_REVID_VAL	0x02u  /* product revision ID*/
#define ADXL372_RESET_CODE	0x52u  /* Writing code 0x52 resets the device */

#define ADXL372_READ		0x01u
#define ADXL372_REG_READ(x)	(((x & 0xFF) << 1) | ADXL372_READ)
#define ADXL372_REG_WRITE(x)	((x & 0xFF) << 1)
#define ADXL372_TO_I2C_REG(x)	((x) >> 1)

/* ADXL372_POWER_CTL */
#define ADXL372_POWER_CTL_INSTANT_ON_TH_MSK	BIT(5)
#define ADXL372_POWER_CTL_INSTANT_ON_TH_MODE(x)	(((x) & 0x1) << 5)
#define ADXL372_POWER_CTL_FIL_SETTLE_MSK	BIT(4)
#define ADXL372_POWER_CTL_FIL_SETTLE_MODE(x)	(((x) & 0x1) << 4)
#define ADXL372_POWER_CTL_LPF_DIS_MSK		BIT(3)
#define ADXL372_POWER_CTL_LPF_DIS_MODE(x)	(((x) & 0x1) << 3)
#define ADXL372_POWER_CTL_HPF_DIS_MSK		BIT(2)
#define ADXL372_POWER_CTL_HPF_DIS_MODE(x)	(((x) & 0x1) << 2)
#define ADXL372_POWER_CTL_MODE_MSK		GENMASK(1, 0)
#define ADXL372_POWER_CTL_MODE(x)		(((x) & 0x3) << 0)

/* ADXL372_MEASURE */
#define ADXL372_MEASURE_AUTOSLEEP_MSK		BIT(6)
#define ADXL372_MEASURE_AUTOSLEEP_MODE(x)	(((x) & 0x1) << 6)
#define ADXL372_MEASURE_LINKLOOP_MSK		GENMASK(5, 4)
#define ADXL372_MEASURE_LINKLOOP_MODE(x)	(((x) & 0x3) << 4)
#define ADXL372_MEASURE_LOW_NOISE_MSK		BIT(3)
#define ADXL372_MEASURE_LOW_NOISE_MODE(x)	(((x) & 0x1) << 3)
#define ADXL372_MEASURE_BANDWIDTH_MSK		GENMASK(2, 0)
#define ADXL372_MEASURE_BANDWIDTH_MODE(x)	(((x) & 0x7) << 0)

/* ADXL372_TIMING */
#define ADXL372_TIMING_ODR_MSK			GENMASK(7, 5)
#define ADXL372_TIMING_ODR_MODE(x)		(((x) & 0x7) << 5)
#define ADXL372_TIMING_WAKE_UP_RATE_MSK		GENMASK(4, 2)
#define ADXL372_TIMING_WAKE_UP_RATE_MODE(x)	(((x) & 0x7) << 2)
#define ADXL372_TIMING_EXT_CLK_MSK		BIT(1)
#define ADXL372_TIMING_EXT_CLK_MODE(x)		(((x) & 0x1) << 1)
#define ADXL372_TIMING_EXT_SYNC_MSK		BIT(0)
#define ADXL372_TIMING_EXT_SYNC_MODE(x)		(((x) & 0x1) << 0)

/* ADXL372_FIFO_CTL */
#define ADXL372_FIFO_CTL_FORMAT_MSK		GENMASK(5, 3)
#define ADXL372_FIFO_CTL_FORMAT_MODE(x)		(((x) & 0x7) << 3)
#define ADXL372_FIFO_CTL_MODE_MSK		GENMASK(2, 1)
#define ADXL372_FIFO_CTL_MODE_MODE(x)		(((x) & 0x3) << 1)
#define ADXL372_FIFO_CTL_SAMPLES_MSK		BIT(0)
#define ADXL372_FIFO_CTL_SAMPLES_MODE(x)	(((x) > 0xFF) ? 1 : 0)

/* ADXL372_STATUS_1 */
#define ADXL372_STATUS_1_DATA_RDY(x)		(((x) >> 0) & 0x1)
#define ADXL372_STATUS_1_FIFO_RDY(x)		(((x) >> 1) & 0x1)
#define ADXL372_STATUS_1_FIFO_FULL(x)		(((x) >> 2) & 0x1)
#define ADXL372_STATUS_1_FIFO_OVR(x)		(((x) >> 3) & 0x1)
#define ADXL372_STATUS_1_USR_NVM_BUSY(x)	(((x) >> 5) & 0x1)
#define ADXL372_STATUS_1_AWAKE(x)		(((x) >> 6) & 0x1)
#define ADXL372_STATUS_1_ERR_USR_REGS(x)	(((x) >> 7) & 0x1)

/* ADXL372_STATUS_2 */
#define ADXL372_STATUS_2_INACT(x)		(((x) >> 4) & 0x1)
#define ADXL372_STATUS_2_ACTIVITY(x)		(((x) >> 5) & 0x1)
#define ADXL372_STATUS_2_ACTIVITY2(x)		(((x) >> 6) & 0x1)

/* ADXL372_INT1_MAP */
#define ADXL372_INT1_MAP_DATA_RDY_MSK		BIT(0)
#define ADXL372_INT1_MAP_DATA_RDY_MODE(x)	(((x) & 0x1) << 0)
#define ADXL372_INT1_MAP_FIFO_RDY_MSK		BIT(1)
#define ADXL372_INT1_MAP_FIFO_RDY_MODE(x)	(((x) & 0x1) << 1)
#define ADXL372_INT1_MAP_FIFO_FULL_MSK		BIT(2)
#define ADXL372_INT1_MAP_FIFO_FULL_MODE(x)	(((x) & 0x1) << 2)
#define ADXL372_INT1_MAP_FIFO_OVR_MSK		BIT(3)
#define ADXL372_INT1_MAP_FIFO_OVR_MODE(x)	(((x) & 0x1) << 3)
#define ADXL372_INT1_MAP_INACT_MSK		BIT(4)
#define ADXL372_INT1_MAP_INACT_MODE(x)		(((x) & 0x1) << 4)
#define ADXL372_INT1_MAP_ACT_MSK		BIT(5)
#define ADXL372_INT1_MAP_ACT_MODE(x)		(((x) & 0x1) << 5)
#define ADXL372_INT1_MAP_AWAKE_MSK		BIT(6)
#define ADXL372_INT1_MAP_AWAKE_MODE(x)		(((x) & 0x1) << 6)
#define ADXL372_INT1_MAP_LOW_MSK		BIT(7)
#define ADXL372_INT1_MAP_LOW_MODE(x)		(((x) & 0x1) << 7)

/* ADXL372_INT2_MAP */
#define ADXL372_INT2_MAP_DATA_RDY_MSK		BIT(0)
#define ADXL372_INT2_MAP_DATA_RDY_MODE(x)	(((x) & 0x1) << 0)
#define ADXL372_INT2_MAP_FIFO_RDY_MSK		BIT(1)
#define ADXL372_INT2_MAP_FIFO_RDY_MODE(x)	(((x) & 0x1) << 1)
#define ADXL372_INT2_MAP_FIFO_FULL_MSK		BIT(2)
#define ADXL372_INT2_MAP_FIFO_FULL_MODE(x)	(((x) & 0x1) << 2)
#define ADXL372_INT2_MAP_FIFO_OVR_MSK		BIT(3)
#define ADXL372_INT2_MAP_FIFO_OVR_MODE(x)	(((x) & 0x1) << 3)
#define ADXL372_INT2_MAP_INACT_MSK		BIT(4)
#define ADXL372_INT2_MAP_INACT_MODE(x)		(((x) & 0x1) << 4)
#define ADXL372_INT2_MAP_ACT_MSK		BIT(5)
#define ADXL372_INT2_MAP_ACT_MODE(x)		(((x) & 0x1) << 5)
#define ADXL372_INT2_MAP_AWAKE_MSK		BIT(6)
#define ADXL372_INT2_MAP_AWAKE_MODE(x)		(((x) & 0x1) << 6)
#define ADXL372_INT2_MAP_LOW_MSK		BIT(7)
#define ADXL372_INT2_MAP_LOW_MODE(x)		(((x) & 0x1) << 7)

/* ADXL372_HPF */
#define ADXL372_HPF_CORNER(x)			(((x) & 0x3) << 0)

enum adxl372_axis {
	ADXL372_X_AXIS,
	ADXL372_Y_AXIS,
	ADXL372_Z_AXIS
};

enum adxl372_op_mode {
	ADXL372_STANDBY,
	ADXL372_WAKE_UP,
	ADXL372_INSTANT_ON,
	ADXL372_FULL_BW_MEASUREMENT
};

enum adxl372_bandwidth {
	ADXL372_BW_200HZ,
	ADXL372_BW_400HZ,
	ADXL372_BW_800HZ,
	ADXL372_BW_1600HZ,
	ADXL372_BW_3200HZ,
	ADXL372_BW_LPF_DISABLED = 0xC,
};

enum adxl372_hpf_corner {
	ADXL372_HPF_CORNER_0,
	ADXL372_HPF_CORNER_1,
	ADXL372_HPF_CORNER_2,
	ADXL372_HPF_CORNER_3,
	ADXL372_HPF_DISABLED,
};

enum adxl372_act_proc_mode {
	ADXL372_DEFAULT,
	ADXL372_LINKED,
	ADXL372_LOOPED
};

enum adxl372_odr {
	ADXL372_ODR_400HZ,
	ADXL372_ODR_800HZ,
	ADXL372_ODR_1600HZ,
	ADXL372_ODR_3200HZ,
	ADXL372_ODR_6400HZ
};

enum adxl372_instant_on_th_mode {
	ADXL372_INSTANT_ON_LOW_TH,
	ADXL372_INSTANT_ON_HIGH_TH
};

enum adxl372_wakeup_rate {
	ADXL372_WUR_52ms,
	ADXL372_WUR_104ms,
	ADXL372_WUR_208ms,
	ADXL372_WUR_512ms,
	ADXL372_WUR_2048ms,
	ADXL372_WUR_4096ms,
	ADXL372_WUR_8192ms,
	ADXL372_WUR_24576ms
};

enum adxl372_filter_settle {
	ADXL372_FILTER_SETTLE_370,
	ADXL372_FILTER_SETTLE_16
};

enum adxl372_fifo_format {
	ADXL372_XYZ_FIFO,
	ADXL372_X_FIFO,
	ADXL372_Y_FIFO,
	ADXL372_XY_FIFO,
	ADXL372_Z_FIFO,
	ADXL372_XZ_FIFO,
	ADXL372_YZ_FIFO,
	ADXL372_XYZ_PEAK_FIFO,
};

enum adxl372_fifo_mode {
	ADXL372_FIFO_BYPASSED,
	ADXL372_FIFO_STREAMED,
	ADXL372_FIFO_TRIGGERED,
	ADXL372_FIFO_OLD_SAVED
};

struct adxl372_fifo_config {
	enum adxl372_fifo_mode fifo_mode;
	enum adxl372_fifo_format fifo_format;
	u16_t fifo_samples;
};

struct adxl372_activity_threshold {
	u16_t thresh;
	bool referenced;
	bool enable;
};

struct adxl372_xyz_accel_data {
	s16_t x;
	s16_t y;
	s16_t z;
};

struct adxl372_data {
	struct device *bus;
#ifdef CONFIG_ADXL372_SPI
	struct spi_config spi_cfg;
#endif
#if defined(CONFIG_ADXL372_SPI_GPIO_CS)
	struct spi_cs_control adxl372_cs_ctrl;
#endif

	struct adxl372_xyz_accel_data sample;
	struct adxl372_fifo_config fifo_config;

#ifdef CONFIG_ADXL372_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	struct sensor_trigger th_trigger;
	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_ADXL372_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADXL372_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif
#endif /* CONFIG_ADXL372_TRIGGER */
};

struct adxl372_dev_config {
#ifdef CONFIG_ADXL372_I2C
	const char *i2c_port;
	u16_t i2c_addr;
#endif
#ifdef CONFIG_ADXL372_SPI
	const char *spi_port;
	u16_t spi_slave;
	u32_t spi_max_frequency;
#ifdef CONFIG_ADXL372_SPI_GPIO_CS
	const char *gpio_cs_port;
	u8_t cs_gpio;
#endif
#endif /* CONFIG_ADXL372_SPI */
#ifdef CONFIG_ADXL372_TRIGGER
	const char *gpio_port;
	u8_t int_gpio;
#endif
	bool max_peak_detect_mode;

	/* Device Settings */
	bool autosleep;

	struct adxl372_activity_threshold activity_th;
	struct adxl372_activity_threshold activity2_th;
	struct adxl372_activity_threshold inactivity_th;
	struct adxl372_fifo_config fifo_config;

	enum adxl372_bandwidth bw;
	enum adxl372_hpf_corner hpf;
	enum adxl372_odr odr;
	enum adxl372_wakeup_rate wur;
	enum adxl372_act_proc_mode act_proc_mode;
	enum adxl372_instant_on_th_mode	th_mode;
	enum adxl372_filter_settle filter_settle;
	enum adxl372_op_mode op_mode;

	u16_t inactivity_time;
	u8_t activity_time;
	u8_t int1_config;
	u8_t int2_config;
};

#ifdef CONFIG_ADXL372_TRIGGER
int adxl372_get_status(struct device *dev,
		       u8_t *status1, u8_t *status2, u16_t *fifo_entries);

int adxl372_reg_write_mask(struct device *dev,
			   u8_t reg_addr, u32_t mask, u8_t data);

int adxl372_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adxl372_init_interrupt(struct device *dev);
#endif /* CONFIG_ADT7420_TRIGGER */

#define SYS_LOG_DOMAIN "ADXL372"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL372_ADXL372_H_ */
