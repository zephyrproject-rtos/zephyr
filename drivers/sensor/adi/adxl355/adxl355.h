/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL355_ADXL355_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL355_ADXL355_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/sensor/adxl355.h>
#include <zephyr/drivers/sensor/adxl355.h>

#define DT_DRV_COMPAT adi_adxl355
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define ADXL355_BUS_SPI
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define ADXL355_BUS_I2C
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
#undef DT_DRV_COMPAT

#ifdef ADXL355_BUS_SPI
#include <zephyr/drivers/spi.h>
#endif /* ADXL355_BUS_SPI */

#ifdef ADXL355_BUS_I2C
#include <zephyr/drivers/i2c.h>
#endif /* ADXL355_BUS_I2C */

/**
 * @brief ADXL355 SPI read/write macros
 *
 */
#define ADXL355_SPI_READ(x) (((x) << 1) | 0x01u)

/**
 * @brief ADXL355 SPI write macro
 *
 */
#define ADXL355_SPI_WRITE(x) (((x) << 1) | 0x00u)

/*
 * ADXL355 registers definition
 */
#define ADXL355_DEVID_AD     0x00u /* Analog Devices accelerometer ID */
#define ADXL355_DEVID_MST    0x01u /* Analog Devices MEMS device ID */
#define ADXL355_PARTID       0x02u /* PART ID */
#define ADXL355_REVID        0x03u /* REV ID */
#define ADXL355_STATUS       0x04u /* STATUS Register */
#define ADXL355_FIFO_ENTRIES 0x05u /* FIFO Entries */
#define ADXL355_TEMP2        0x06u /* Temperature Data MSB */
#define ADXL355_TEMP1        0x07u /* Temperature Data LSB */
#define ADXL355_XDATA3       0x08u /* X-Axis Data MSB */
#define ADXL355_XDATA2       0x09u /* X-Axis Data */
#define ADXL355_XDATA1       0x0Au /* X-Axis Data LSB */
#define ADXL355_YDATA3       0x0Bu /* Y-Axis Data MSB */
#define ADXL355_YDATA2       0x0Cu /* Y-Axis Data */
#define ADXL355_YDATA1       0x0Du /* Y-Axis Data LSB */
#define ADXL355_ZDATA3       0x0Eu /* Z-Axis Data MSB */
#define ADXL355_ZDATA2       0x0Fu /* Z-Axis Data */
#define ADXL355_ZDATA1       0x10u /* Z-Axis Data LSB */
#define ADXL355_FIFO_DATA    0x11u /* FIFO Data */
#define ADXL355_OFFSET_X_H   0x1Eu /* X-Axis Offset High Byte */
#define ADXL355_OFFSET_X_L   0x1Fu /* X-Axis Offset Low Byte */
#define ADXL355_OFFSET_Y_H   0x20u /* Y-Axis Offset High Byte */
#define ADXL355_OFFSET_Y_L   0x21u /* Y-Axis Offset Low Byte */
#define ADXL355_OFFSET_Z_H   0x22u /* Z-Axis Offset High Byte */
#define ADXL355_OFFSET_Z_L   0x23u /* Z-Axis Offset Low Byte */
#define ADXL355_ACT_EN       0x24u /* Activity Enable */
#define ADXL355_ACT_THRESH_H 0x25u /* Activity Threshold High Byte */
#define ADXL355_ACT_THRESH_L 0x26u /* Activity Threshold Low Byte */
#define ADXL355_ACT_COUNT    0x27u /* Activity Count */
#define ADXL355_FILTER       0x28u /* Filter Register */
#define ADXL355_FIFO_SAMPLES 0x29u /* FIFO Samples */
#define ADXL355_INT_MAP      0x2Au /* Interrupt Map */
#define ADXL355_SYNC         0x2Bu /* Sync Register */
#define ADXL355_RANGE        0x2Cu /* Range Register */
#define ADXL355_POWER_CTL    0x2Du /* Power Control Register */
#define ADXL355_SELF_TEST    0x2Eu /* Self Test Register */
#define ADXL355_RESET        0x2Fu /* Reset Register */

/* REGISTER MASKS */
/* Status Register */
#define ADXL355_STATUS_DATA_RDY_MSK  BIT(0)
#define ADXL355_STATUS_FIFO_FULL_MSK BIT(1)
#define ADXL355_STATUS_FIFO_OVR_MSK  BIT(2)
#define ADXL355_STATUS_ACTIVITY_MSK  BIT(3)
#define ADXL355_STATUS_NVM_BUSY_MSK  BIT(4)

/* FIFO_ENTRIES */
#define ADXL355_FIFO_ENTRIES_MSK GENMASK(6, 0)

/* Temperature Bits */
#define ADXL355_TEMP_BITS_MSB GENMASK(3, 0)
#define ADXL355_TEMP_BITS_LSB GENMASK(7, 0)

/* ACT ENABLE Register*/
#define ADXL355_ACT_EN_X_MSK BIT(0)
#define ADXL355_ACT_EN_Y_MSK BIT(1)
#define ADXL355_ACT_EN_Z_MSK BIT(2)

/* FILTER Register */
#define ADXL355_FILTER_ODR_MSK  GENMASK(3, 0)
#define ADXL355_FILTER_HPF_MASK GENMASK(6, 4)

/* Interrupt Map Register */
#define ADXL355_INT_MAP_DATA_RDY_EN1_MSK  BIT(0)
#define ADXL355_INT_MAP_FIFO_FULL_EN1_MSK BIT(1)
#define ADXL355_INT_MAP_FIFO_OVR_EN1_MSK  BIT(2)
#define ADXL355_INT_MAP_ACTIVITY_EN1_MSK  BIT(3)
#define ADXL355_INT_MAP_DATA_RDY_EN2_MSK  BIT(4)
#define ADXL355_INT_MAP_FIFO_FULL_EN2_MSK BIT(5)
#define ADXL355_INT_MAP_FIFO_OVR_EN2_MSK  BIT(6)
#define ADXL355_INT_MAP_ACTIVITY_EN2_MSK  BIT(7)

/* SYNC Register */
#define ADXL355_SYNC_EXT_SYNC_MSK GENMASK(1, 0)
#define ADXL355_SYNC_EXT_CLK_MSK  BIT(2)

/* RANGE Register */
#define ADXL355_I2C_HS_MSK  BIT(7)
#define ADXL355_INT_POL_MSK BIT(6)
#define ADXL355_RANGE_MSK   GENMASK(1, 0)

/* POWER_CTL Register */
#define ADXL355_POWER_CTL_STANDBY_MSK  BIT(0)
#define ADXL355_POWER_CTL_TEMP_OFF_MSK BIT(1)
#define ADXL355_POWER_CTL_DRDY_OFF_MSK BIT(2)

/* SELF_TEST Register */
#define ADXL355_SELF_TEST_ST1_MSK BIT(0)
#define ADXL355_SELF_TEST_ST2_MSK BIT(1)

#define ADXL355_DEVID_AD_VAL  0xADu /* Analog Devices accelerometer ID */
#define ADXL355_DEVID_MST_VAL 0x1Du /* Analog Devices MEMS device ID */
#define ADXL355_PARTID_VAL    0xEDu /* PART ID */
#define ADXL355_REVID_VAL     0x01u /* REV ID */
#define ADXL355_RESET_CMD     0x52u /* RESET command */

/* Sensitivity Values */
#define ADXL355_SENSITIVITY_2G 256000 /* in LSB/g */
#define ADXL355_SENSITIVITY_4G 128000 /* in LSB/g */
#define ADXL355_SENSITIVITY_8G 64000  /* in LSB/g */

#define ADXL355_SELF_TEST_MIN_X (SENSOR_G * 0.1) /* in micro m/s^2 */
#define ADXL355_SELF_TEST_MAX_X (SENSOR_G * 0.6) /* in micro m/s^2 */

#define ADXL355_SELF_TEST_MIN_Y (SENSOR_G * 0.1) /* in micro m/s^2 */
#define ADXL355_SELF_TEST_MAX_Y (SENSOR_G * 0.6) /* in micro m/s^2 */

#define ADXL355_SELF_TEST_MIN_Z    (SENSOR_G * 0.5) /* in micro m/s^2 */
#define ADXL355_SELF_TEST_MAX_Z    (SENSOR_G * 3.0) /* in micro m/s^2 */
/**
 * @brief ADXL355 read command macro
 *
 */
#define ADXL355_REG_READ_CMD(addr) ((addr) << 1 | ADXL355_SPI_READ)

/**
 * @brief ADXL355 operating modes
 *
 */
enum adxl355_op_mode {
	ADXL355_MEASURE = 0,
	ADXL355_STANDBY = 1,
};

/**
 * @brief ADXL355 Output Data Rate options
 *
 */
enum adxl355_odr {
	ADXL355_ODR_4000HZ = ADXL355_DT_ODR_4000,
	ADXL355_ODR_2000HZ = ADXL355_DT_ODR_2000,
	ADXL355_ODR_1000HZ = ADXL355_DT_ODR_1000,
	ADXL355_ODR_500HZ = ADXL355_DT_ODR_500,
	ADXL355_ODR_250HZ = ADXL355_DT_ODR_250,
	ADXL355_ODR_125HZ = ADXL355_DT_ODR_125,
	ADXL355_ODR_62_5HZ = ADXL355_DT_ODR_62_5,
	ADXL355_ODR_31_25HZ = ADXL355_DT_ODR_31_25,
	ADXL355_ODR_15_625HZ = ADXL355_DT_ODR_15_625,
	ADXL355_ODR_7_813HZ = ADXL355_DT_ODR_7_813,
	ADXL355_ODR_3_906HZ = ADXL355_DT_ODR_3_906,
};

/**
 * @brief ADXL355 Measurement Range options
 *
 */
enum adxl355_range {
	ADXL355_RANGE_2G = ADXL355_DT_RANGE_2G,
	ADXL355_RANGE_4G = ADXL355_DT_RANGE_4G,
	ADXL355_RANGE_8G = ADXL355_DT_RANGE_8G,
};

/**
 * @brief ADXL355 bus configuration
 *
 */
union adxl355_bus {
#if defined(ADXL355_BUS_I2C)
	struct i2c_dt_spec i2c;
#endif /* ADXL355_BUS_I2C */
#if defined(ADXL355_BUS_SPI)
	struct spi_dt_spec spi;
#endif /* ADXL355_BUS_SPI */
	uint8_t dummy;
};
/**
 * @brief Function pointer to check if bus is ready
 *
 */
typedef int (*adxl355_bus_is_ready_fn)(const union adxl355_bus *bus);
/**
 * @brief Function pointer for register access
 *
 */
typedef int (*adxl355_reg_access_fn)(const struct device *dev, bool read, uint8_t reg_addr,
				     uint8_t *data, size_t length);

/**
 * @brief
 *
 */
struct adxl355_extra_attr {
	uint8_t pwr_reg;
	uint8_t drdy_mode;
	uint8_t temp_mode;
	uint8_t hpf_corner: 3;
	uint8_t ext_clk: 1;
	uint8_t ext_sync: 2;
	uint8_t i2c_hs: 1;
	uint8_t int_pol: 1;
	int64_t act_threshold;
	uint8_t act_count;
	int64_t offset_x;
	int64_t offset_y;
	int64_t offset_z;
};
/**
 * @brief ADXL355 sample data structure
 *
 */
struct adxl355_sample {
	int32_t x;
	int32_t y;
	int32_t z;
	enum adxl355_range range;
	uint8_t is_fifo: 1;
};

/**
 * @brief ADXL355 driver data structure
 *
 */
struct adxl355_data {
	struct adxl355_sample samples;
	int16_t temp_val;
	enum adxl355_odr odr;
	enum adxl355_range range;
	uint8_t fifo_watermark: 7;
	enum adxl355_op_mode op_mode;
	struct adxl355_extra_attr extra_attr;
#ifdef CONFIG_ADXL355_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	bool route_to_int2;

	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t act_handler;
	const struct sensor_trigger *act_trigger;
#if defined(CONFIG_ADXL355_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADXL355_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_ADXL355_TRIGGER_OWN_THREAD || CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_ADXL355_TRIGGER */
#ifdef CONFIG_ADXL355_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint8_t status1;
	uint8_t fifo_counter;
	uint64_t timestamp;
	struct rtio *r_cb;
	uint8_t fifo_watermark_irq;
	uint8_t fifo_samples;
	uint16_t fifo_total_bytes;
#endif
};

/**
 * @brief ADXL355 FIFO data header structure
 *
 */
struct adxl355_fifo_data {
	uint8_t is_fifo: 1;
	uint64_t timestamp;
	uint8_t status1;
	uint8_t range;
	uint8_t accel_odr;
	uint8_t fifo_samples;
	uint16_t fifo_byte_count;
	uint8_t sample_set_size;
	int32_t x;
	int32_t y;
	int32_t z;
} __attribute__((__packed__));

/**
 * @brief ADXL355 device configuration structure
 *
 */
struct adxl355_dev_config {
	const union adxl355_bus bus;
	adxl355_bus_is_ready_fn bus_is_ready;
	adxl355_reg_access_fn reg_access;
	enum adxl355_odr odr;
	enum adxl355_range range;
	uint8_t fifo_watermark: 7;
	uint8_t hpf_corner: 3;
	uint8_t ext_clk: 1;
	uint8_t ext_sync: 2;
	uint8_t i2c_hs: 1;
	uint8_t int_pol: 1;
	bool self_test;

#ifdef CONFIG_ADXL355_TRIGGER
	struct gpio_dt_spec interrupt_gpio;
	bool route_to_int2;
#endif
};

/**
 * @brief ADXL355 register read function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Data buffer
 * @param length Number of bytes to read
 * @return int 0 on success, negative error code on failure
 */
int adxl355_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length);

/**
 * @brief ADXL355 register write function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Data buffer
 * @param length Number of bytes to write
 * @return int 0 on success, negative error code on failure
 */
int adxl355_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length);

/**
 * @brief ADXL355 register update function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param mask Bit mask
 * @param value Value to set
 * @return
 */
int adxl355_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);

/**
 * @brief Set ADXL355 operating mode
 *
 * @param dev Device pointer
 * @param op_mode Operating mode to set
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_set_op_mode(const struct device *dev, enum adxl355_op_mode op_mode);

#ifdef CONFIG_ADXL355_TRIGGER
/**
 * @brief Initialize ADXL355 interrupt
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_init_interrupt(const struct device *dev);

/**
 * @brief Set ADXL355 trigger
 *
 * @param dev Device pointer
 * @param trig Trigger pointer
 * @param handler Trigger handler
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
#endif /* CONFIG_ADXL355_TRIGGER */

#ifdef CONFIG_ADXL355_STREAM
/**
 * @brief ADXL355 stream IRQ handler
 *
 * @param dev Device pointer
 */
void adxl355_stream_irq_handler(const struct device *dev);
#endif

#ifdef CONFIG_SENSOR_ASYNC_API
/**
 * @brief Submit ADXL355 I/O operation
 *
 * @param dev Device pointer
 * @param iodev_sqe I/O device SQE pointer
 */
void adxl355_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Submit ADXL355 stream I/O operation
 *
 * @param dev Device pointer
 * @param iodev_sqe I/O device SQE pointer
 */
void adxl355_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Get ADXL355 sensor data decoder
 *
 * @param dev Device pointer
 * @param decoder Decoder API pointer
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif /* CONFIG_SENSOR_ASYNC_API */

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL355_ADXL355_H_ */
