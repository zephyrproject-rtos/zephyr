/*
 * Copyright (c) 2023 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL367_ADXL367_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL367_ADXL367_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT  adi_adxl367
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define ADXL367_BUS_SPI
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define ADXL367_BUS_I2C
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT  adi_adxl366
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define ADXL367_BUS_SPI
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define ADXL367_BUS_I2C
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
#undef DT_DRV_COMPAT

#ifdef ADXL367_BUS_SPI
#include <zephyr/drivers/spi.h>
#endif /* ADXL367_BUS_SPI */

#ifdef ADXL367_BUS_I2C
#include <zephyr/drivers/i2c.h>
#endif /* ADXL367_BUS_I2C */

#define ADXL367_CHIP_ID		0
#define ADXL366_CHIP_ID		1

/*
 * ADXL367 registers definition
 */
#define ADXL367_DEVID		0x00u  /* Analog Devices accelerometer ID */
#define ADXL367_DEVID_MST	0x01u  /* Analog Devices MEMS device ID */
#define ADXL367_PART_ID		0x02u  /* Device ID */
#define ADXL367_REV_ID		0x03u  /* product revision ID*/
#define ADXL367_SERIAL_NR_3	0x04u  /* Serial Number 3 */
#define ADXL367_SERIAL_NR_2	0x05u  /* Serial Number 2 */
#define ADXL367_SERIAL_NR_1	0x06u  /* Serial Number 1 */
#define ADXL367_SERIAL_NR_0	0x07u  /* Serial Number 0 */
#define ADXL367_XDATA		0x08u  /* X-axis acceleration data [13:6] */
#define ADXL367_YDATA		0x09u  /* Y-axis acceleration data [13:6] */
#define ADXL367_ZDATA		0x0Au  /* Z-axis acceleration data [13:6] */
#define ADXL367_STATUS		0x0Bu  /* Status */
#define ADXL367_FIFO_ENTRIES_L	0x0Cu  /* FIFO Entries Low */
#define ADXL367_FIFO_ENTRIES_H	0x0Du  /* FIFO Entries High */
#define ADXL367_X_DATA_H	0x0Eu  /* X-axis acceleration data [13:6] */
#define ADXL367_X_DATA_L	0x0Fu  /* X-axis acceleration data [5:0] */
#define ADXL367_Y_DATA_H	0x10u  /* Y-axis acceleration data [13:6] */
#define ADXL367_Y_DATA_L	0x11u  /* Y-axis acceleration data [5:0] */
#define ADXL367_Z_DATA_H	0x12u  /* Z-axis acceleration data [13:6] */
#define ADXL367_Z_DATA_L	0x13u  /* Z-axis acceleration data [5:0] */
#define ADXL367_TEMP_H		0x14u  /* Temperate data [13:6] */
#define ADXL367_TEMP_L		0x15u  /* Temperate data [5:0] */
#define ADXL367_EX_ADC_H	0x16u  /* Extended ADC data [13:6] */
#define ADXL367_EX_ADC_L	0x17u  /* Extended ADC data [5:0] */
#define ADXL367_I2C_FIFO_DATA	0x18u  /* I2C FIFO Data address */
#define ADXL367_SOFT_RESET	0x1Fu  /* Software reset register */
#define ADXL367_THRESH_ACT_H	0x20u  /* Activity Threshold [12:6] */
#define ADXL367_THRESH_ACT_L	0x21u  /* Activity Threshold [5:0] */
#define ADXL367_TIME_ACT	0x22u  /* Activity Time */
#define ADXL367_THRESH_INACT_H	0x23u  /* Inactivity Threshold [12:6] */
#define ADXL367_THRESH_INACT_L	0x24u  /* Inactivity Threshold [5:0] */
#define ADXL367_TIME_INACT_H	0x25u  /* Inactivity Time [12:6] */
#define ADXL367_TIME_INACT_L	0x26u  /* Inactivity Time [5:0] */
#define ADXL367_ACT_INACT_CTL	0x27u  /* Activity Inactivity Control */
#define ADXL367_FIFO_CONTROL	0x28u  /* FIFO Control */
#define ADXL367_FIFO_SAMPLES	0x29u  /* FIFO Samples */
#define ADXL367_INTMAP1_LOWER	0x2Au  /* Interrupt 1 mapping control lower */
#define ADXL367_INTMAP2_LOWER   0x2Bu  /* Interrupt 2 mapping control lower */
#define ADXL367_FILTER_CTL	0x2Cu  /* Filter Control register */
#define ADXL367_POWER_CTL	0x2Du  /* Power Control register */
#define ADXL367_SELF_TEST	0x2Eu  /* Self Test */
#define ADXL367_TAP_THRESH	0x2Fu  /* Tap Threshold */
#define ADXL367_TAP_DUR		0x30u  /* Tap Duration */
#define ADXL367_TAP_LATENT	0x31u  /* Tap Latency */
#define ADXL367_TAP_WINDOW	0x32u  /* Tap Window */
#define ADXL367_X_OFFSET	0x33u  /* X-axis offset */
#define ADXL367_Y_OFFSET	0x34u  /* Y-axis offset */
#define ADXL367_Z_OFFSET	0x35u  /* Z-axis offset */
#define ADXL367_X_SENS		0x36u  /* X-axis user sensitivity */
#define ADXL367_Y_SENS		0x37u  /* Y-axis user sensitivity */
#define ADXL367_Z_SENS		0x38u  /* Z-axis user sensitivity */
#define ADXL367_TIMER_CTL	0x39u  /* Timer Control */
#define ADXL367_INTMAP1_UPPER	0x3Au  /* Interrupt 1 mapping control upper */
#define ADXL367_INTMAP2_UPPER   0x3Bu  /* Interrupt 2 mapping control upper */
#define ADXL367_ADC_CTL		0x3Cu  /* ADC Control Register */
#define ADXL367_TEMP_CTL	0x3Du  /* Temperature Control Register */
#define ADXL367_TEMP_ADC_OTH_H	0x3Eu  /* Temperature ADC Over Threshold [12:6]*/
#define ADXL367_TEMP_ADC_OTH_L	0x3Fu  /* Temperature ADC Over Threshold [5:0]*/
#define ADXL367_TEMP_ADC_UTH_H	0x40u  /* Temperature ADC Under Threshold [12:6]*/
#define ADXL367_TEMP_ADC_UTH_L	0x41u  /* Temperature ADC Under Threshold [5:0]*/
#define ADXL367_TEMP_ADC_TIMER	0x42u  /* Temperature Activiy Inactivity Timer */
#define ADXL367_AXIS_MASK	0x43u  /* Axis Mask Register */
#define ADXL367_STATUS_COPY	0x44u  /* Status Copy Register */
#define ADXL367_STATUS2		0x45u  /* Status 2 Register */

#define ADXL367_DEVID_VAL	0xADu  /* Analog Devices accelerometer ID */
#define ADXL367_MST_DEVID_VAL	0x1Du  /* Analog Devices MEMS device ID */
#define ADXL367_PARTID_VAL	0xF7u  /* Device ID */
#define ADXL367_REVID_VAL	0x03u  /* product revision ID*/
#define ADXL367_RESET_CODE	0x52u  /* Writing code 0x52 resets the device */

#define ADXL367_READ					0x01u
#define ADXL367_REG_READ(x)				(((x & 0xFF) << 1) | ADXL367_READ)
#define ADXL367_REG_WRITE(x)				((x & 0xFF) << 1)
#define ADXL367_TO_REG(x)				((x) >> 1)
#define ADXL367_SPI_WRITE_REG				0x0Au
#define ADXL367_SPI_READ_REG				0x0Bu
#define ADXL367_SPI_READ_FIFO				0x0Du

#define ADXL367_ABSOLUTE				0x00
#define ADXL367_REFERENCED				0x01

/* ADXL367_POWER_CTL */
#define ADXL367_POWER_CTL_EXT_CLK_MSK			BIT(6)
#define ADXL367_POWER_CTL_NOISE_MSK			GENMASK(5, 4)
#define ADXL367_POWER_CTL_WAKEUP_MSK			BIT(3)
#define ADXL367_POWER_CTL_AUTOSLEEP_MSK			BIT(2)
#define ADXL367_POWER_CTL_MEASURE_MSK			GENMASK(1, 0)

/* ADXL367_ACT_INACT_CTL */
#define ADXL367_ACT_INACT_CTL_LINKLOOP_MSK		GENMASK(5, 4)
#define ADXL367_ACT_INACT_CTL_INACT_REF_MSK		BIT(3)
#define ADXL367_ACT_INACT_CTL_INACT_EN_MSK		BIT(2)
#define ADXL367_ACT_INACT_CTL_ACT_REF_MSK		BIT(1)
#define ADXL367_ACT_INACT_CTL_ACT_EN_MSK		BIT(0)

/* ADXL367_ACT_INACT_CTL_INACT_EN options */
#define ADXL367_NO_INACTIVITY_DETECTION_ENABLED		0x0
#define ADXL367_INACTIVITY_ENABLE			0x1
#define ADXL367_NO_INACTIVITY_DETECTION_ENABLED_2	0x2
#define ADXL367_REFERENCED_INACTIVITY_ENABLE		0x3

/* ADXL367_ACT_INACT_CTL_ACT_EN options */
#define ADXL367_NO_ACTIVITY_DETECTION			0x0
#define ADXL367_ACTIVITY_ENABLE				0x1
#define ADXL367_NO_ACTIVITY_DETECTION_2			0x2
#define ADXL367_REFERENCED_ACTIVITY_ENABLE		0x3

#define ADXL367_TEMP_OFFSET				1185
#define ADXL367_TEMP_25C				165
#define ADXL367_TEMP_SCALE				18518518LL
#define ADXL367_TEMP_SCALE_DIV				1000000000

#define ADXL367_THRESH_H_MSK				GENMASK(6, 0)
#define ADXL367_THRESH_L_MSK				GENMASK(7, 2)

/* ADXL367_REG_TEMP_CTL definitions. */
#define ADXL367_TEMP_INACT_EN_MSK			BIT(3)
#define ADXL367_TEMP_ACT_EN_MSK				BIT(1)
#define ADXL367_TEMP_EN_MSK				BIT(0)

/* ADXL367_SELF_TEST */
#define ADXL367_SELF_TEST_ST_FORCE_MSK			BIT(1)
#define ADXL367_SELF_TEST_ST_MSK			BIT(0)

/* ADXL367_REG_FILTER_CTL definitions */
#define ADXL367_FILTER_CTL_RANGE_MSK			GENMASK(7, 6)
#define ADXL367_FILTER_I2C_HS				BIT(5)
#define ADXL367_FILTER_CTL_RES				BIT(4)
#define ADXL367_FILTER_CTL_EXT_SAMPLE			BIT(3)
#define ADXL367_FILTER_CTL_ODR_MSK			GENMASK(2, 0)

/* ADXL367_REG_FIFO_CONTROL */
#define ADXL367_FIFO_CONTROL_FIFO_CHANNEL_MSK		GENMASK(6, 3)
#define ADXL367_FIFO_CONTROL_FIFO_SAMPLES_MSK		BIT(2)
#define ADXL367_FIFO_CONTROL_FIFO_MODE_MSK		GENMASK(1, 0)

/* ADXL367_REG_ADC_CTL definitions. */
#define ADXL367_FIFO_8_12BIT_MSK			GENMASK(7, 6)
#define ADXL367_ADC_INACT_EN				BIT(3)
#define ADXL367_ADC_ACT_EN				BIT(1)
#define ADXL367_ADC_EN					BIT(0)

/* ADXL367_REG_STATUS definitions */
#define ADXL367_STATUS_ERR_USER_REGS			BIT(7)
#define ADXL367_STATUS_AWAKE				BIT(6)
#define ADXL367_STATUS_INACT				BIT(5)
#define ADXL367_STATUS_ACT				BIT(4)
#define ADXL367_STATUS_FIFO_OVERRUN			BIT(3)
#define ADXL367_STATUS_FIFO_WATERMARK			BIT(2)
#define ADXL367_STATUS_FIFO_RDY				BIT(1)
#define ADXL367_STATUS_DATA_RDY				BIT(0)

/* ADXL367_INTMAP_LOWER */
#define ADXL367_INT_LOW					BIT(7)
#define ADXL367_AWAKE_INT				BIT(6)
#define ADXL367_INACT_INT				BIT(5)
#define ADXL367_ACT_INT					BIT(4)
#define ADXL367_FIFO_OVERRUN				BIT(3)
#define ADXL367_FIFO_WATERMARK				BIT(2)
#define ADXL367_FIFO_RDY				BIT(1)
#define ADXL367_DATA_RDY				BIT(0)

/* ADXL367_INTMAP_UPPER */
#define ADXL367_ERR_FUSE				BIT(7)
#define ADXL367_ERR_USER_REGS				BIT(6)
#define ADXL367_KPALV_TIMER				BIT(4)
#define ADXL367_TEMP_ADC_HI				BIT(3)
#define ADXL367_TEMP_ADC_LOW				BIT(2)
#define ADXL367_TAP_TWO					BIT(1)
#define ADXL367_TAP_ONE					BIT(0)

/* Min change = 90mg. Sensitivity = 4LSB / mg */
#define ADXL367_SELF_TEST_MIN	(90 * 100 / 25)
/* Max change = 270mg. Sensitivity = 4LSB / mg */
#define ADXL367_SELF_TEST_MAX	(270 * 100 / 25)

/* ADXL367 get fifo sample header */
#define ADXL367_FIFO_HDR_GET_ACCEL_AXIS(x)	(((x) & 0xC000) >> 14)
#define ADXL367_FIFO_HDR_CHECK_TEMP(x)	((((x) & 0xC000) >> 14) == 0x3)

/* ADXL362 scale factors from specifications */
#define ADXL367_ACCEL_2G_LSB_PER_G	4000
#define ADXL367_ACCEL_4G_LSB_PER_G	2000
#define ADXL367_ACCEL_8G_LSB_PER_G	1000

enum adxl367_axis {
	ADXL367_X_AXIS = 0x0,
	ADXL367_Y_AXIS = 0x1,
	ADXL367_Z_AXIS = 0x2
};

enum adxl367_op_mode {
	ADXL367_STANDBY = 0,
	ADXL367_MEASURE = 2,
};

enum adxl367_range {
	ADXL367_2G_RANGE,
	ADXL367_4G_RANGE,
	ADXL367_8G_RANGE,
};

enum adxl367_act_proc_mode {
	ADXL367_DEFAULT = 0,
	ADXL367_LINKED = 1,
	ADXL367_LOOPED = 3,
};

enum adxl367_odr {
	ADXL367_ODR_12P5HZ,
	ADXL367_ODR_25HZ,
	ADXL367_ODR_50HZ,
	ADXL367_ODR_100HZ,
	ADXL367_ODR_200HZ,
	ADXL367_ODR_400HZ,
};

enum adxl367_fifo_format {
	ADXL367_FIFO_FORMAT_XYZ,
	ADXL367_FIFO_FORMAT_X,
	ADXL367_FIFO_FORMAT_Y,
	ADXL367_FIFO_FORMAT_Z,
	ADXL367_FIFO_FORMAT_XYZT,
	ADXL367_FIFO_FORMAT_XT,
	ADXL367_FIFO_FORMAT_YT,
	ADXL367_FIFO_FORMAT_ZT,
	ADXL367_FIFO_FORMAT_XYZA,
	ADXL367_FIFO_FORMAT_XA,
	ADXL367_FIFO_FORMAT_YA,
	ADXL367_FIFO_FORMAT_ZA
};

enum adxl367_fifo_mode {
	ADXL367_FIFO_DISABLED,
	ADXL367_OLDEST_SAVED,
	ADXL367_STREAM_MODE,
	ADXL367_TRIGGERED_MODE
};

enum adxl367_fifo_read_mode {
	ADXL367_12B_CHID,
	ADXL367_8B,
	ADXL367_12B,
	ADXL367_14B_CHID
};

struct adxl367_fifo_config {
	enum adxl367_fifo_mode fifo_mode;
	enum adxl367_fifo_format fifo_format;
	enum adxl367_fifo_read_mode fifo_read_mode;
	uint16_t fifo_samples;
};

struct adxl367_activity_threshold {
	uint16_t value;
	bool referenced;
	bool enable;
};

struct adxl367_xyz_accel_data {
	int16_t x;
	int16_t y;
	int16_t z;
	enum adxl367_range range;
};

struct adxl367_sample_data {
#ifdef CONFIG_ADXL367_STREAM
	uint8_t is_fifo: 1;
	uint8_t res: 7;
#endif /*CONFIG_ADXL367_STREAM*/
	struct adxl367_xyz_accel_data xyz;
	int16_t raw_temp;
};

struct adxl367_transfer_function {
	int (*read_reg_multiple)(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint16_t len);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr,
			 uint8_t value);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr,
			uint8_t *value);
	int (*write_reg_mask)(const struct device *dev, uint8_t reg_addr,
			      uint32_t mask, uint8_t value);
};

struct adxl367_data {
	struct adxl367_xyz_accel_data sample;
	int16_t temp_val;
	const struct adxl367_transfer_function *hw_tf;
	struct adxl367_fifo_config fifo_config;
	enum adxl367_act_proc_mode act_proc_mode;
	enum adxl367_range range;
#ifdef CONFIG_ADXL367_TRIGGER
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	const struct device *dev;

#if defined(CONFIG_ADXL367_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADXL367_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADXL367_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ADXL367_TRIGGER */
#ifdef CONFIG_ADXL367_STREAM
	uint8_t status;
	uint8_t fifo_ent[2];
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint64_t timestamp;
	struct rtio *r_cb;
	uint8_t fifo_full_irq: 1;
	uint8_t fifo_wmark_irq: 1;
	uint8_t res: 6;
	enum adxl367_odr odr;
	uint8_t pwr_reg;
#endif /* CONFIG_ADXL367_STREAM */
};

struct adxl367_dev_config {
#ifdef ADXL367_BUS_I2C
	struct i2c_dt_spec i2c;
#endif /* ADXL367_BUS_I2C */
#ifdef ADXL367_BUS_SPI
	struct spi_dt_spec spi;
#endif /* ADXL367_BUS_SPI */
	int (*bus_init)(const struct device *dev);

#ifdef CONFIG_ADXL367_TRIGGER
	struct gpio_dt_spec interrupt;
#endif

	enum adxl367_odr odr;

	/* Device Settings */
	bool autosleep;
	bool low_noise;
	bool temp_en;

	struct adxl367_activity_threshold activity_th;
	struct adxl367_activity_threshold inactivity_th;
	struct adxl367_fifo_config fifo_config;

	enum adxl367_range range;
	enum adxl367_op_mode op_mode;

	uint16_t inactivity_time;
	uint8_t activity_time;
	uint8_t chip_id;
};

struct adxl367_fifo_data {
	uint8_t is_fifo: 1;
	uint8_t res: 7;
	uint8_t packet_size;
	uint8_t fifo_read_mode;
	uint8_t has_tmp: 1;
	uint8_t has_adc: 1;
	uint8_t has_x: 1;
	uint8_t has_y: 1;
	uint8_t has_z: 1;
	uint8_t res1: 3;
	uint8_t int_status;
	uint8_t accel_odr: 4;
	uint8_t range: 4;
	uint16_t fifo_byte_count;
	uint64_t timestamp;
} __attribute__((__packed__));

BUILD_ASSERT(sizeof(struct adxl367_fifo_data) % 4 == 0,
	     "adxl367_fifo_data struct should be word aligned");

int adxl367_spi_init(const struct device *dev);
int adxl367_i2c_init(const struct device *dev);
int adxl367_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adxl367_init_interrupt(const struct device *dev);
void adxl367_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void adxl367_stream_irq_handler(const struct device *dev);

#ifdef CONFIG_SENSOR_ASYNC_API
void adxl367_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int adxl367_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
int adxl367_get_accel_data(const struct device *dev,
			   struct adxl367_xyz_accel_data *accel_data);
int adxl367_get_temp_data(const struct device *dev, int16_t *raw_temp);
void adxl367_accel_convert(struct sensor_value *val, int16_t value,
				enum adxl367_range range);
void adxl367_temp_convert(struct sensor_value *val, int16_t value);
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifdef CONFIG_ADXL367_STREAM
int adxl367_fifo_setup(const struct device *dev,
		       enum adxl367_fifo_mode mode,
		       enum adxl367_fifo_format format,
		       enum adxl367_fifo_read_mode read_mode,
		       uint8_t sets_nb);
int adxl367_set_op_mode(const struct device *dev,
			       enum adxl367_op_mode op_mode);
size_t adxl367_get_packet_size(const struct adxl367_dev_config *cfg);
#endif /* CONFIG_ADXL367_STREAM */

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL367_ADXL367_H_ */
