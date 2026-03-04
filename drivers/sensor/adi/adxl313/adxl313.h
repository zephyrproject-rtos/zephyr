/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADI ADXL313 3-axis accelerometer sensor driver header
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_

/**
 * This header file provides the internal API and data structures for the
 * ADXL313 accelerometer driver. It includes register definitions, enumerations
 * for device configuration (ODR, range, FIFO modes), and function declarations
 * for device initialization and operation.
 *
 * @defgroup adxl313_api ADXL313 Sensor Driver Internal API
 * @ingroup sensor_interface
 * @{
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/sensor/adxl313.h>

#ifdef CONFIG_ADXL313_STREAM
#include <zephyr/rtio/rtio.h>
#endif /* CONFIG_ADXL313_STREAM */

#define DT_DRV_COMPAT adi_adxl313

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

/* communication */
#define ADXL313_WRITE_CMD      0x00
#define ADXL313_READ_CMD       0x80
#define ADXL313_MULTIBYTE_FLAG 0x40

#define ADXL313_REG_READ(x)           (FIELD_GET(GENMASK(5, 0), x) | ADXL313_READ_CMD)
#define ADXL313_REG_READ_MULTIBYTE(x) (ADXL313_REG_READ(x) | ADXL313_MULTIBYTE_FLAG)

#define ADXL313_COMPLEMENT_MASK(x) GENMASK(15, (x))
#define ADXL313_COMPLEMENT         GENMASK(15, 10)

/* registers */
#define ADXL313_REG_DEVID0 0x00
#define ADXL313_REG_DEVID1 0x01

#define ADXL313_REG_SOFT_RESET    0x18
#define ADXL313_REG_OFSX          0x1e
#define ADXL313_REG_OFSY          0x1f
#define ADXL313_REG_OFSZ          0x20
#define ADXL313_REG_THRESH_ACT    0x24
#define ADXL313_REG_THRESH_INACT  0x25
#define ADXL313_REG_TIME_INACT    0x26
#define ADXL313_REG_ACT_INACT_CTL 0x27
#define ADXL313_REG_RATE          0x2c
#define ADXL313_REG_POWER_CTL     0x2d
#define ADXL313_REG_INT_ENABLE    0x2e
#define ADXL313_REG_INT_MAP       0x2f
#define ADXL313_REG_INT_SOURCE    0x30
#define ADXL313_REG_DATA_FORMAT   0x31
#define ADXL313_REG_DATA_XYZ_REGS 0x32
#define ADXL313_REG_DATA_X0_REG   0x32
#define ADXL313_REG_DATA_X1_REG   0x33
#define ADXL313_REG_DATA_Y0_REG   0x34
#define ADXL313_REG_DATA_Y1_REG   0x35
#define ADXL313_REG_DATA_Z0_REG   0x36
#define ADXL313_REG_DATA_Z1_REG   0x37
#define ADXL313_REG_FIFO_CTL      0x38
#define ADXL313_REG_FIFO_STATUS   0x39

/* register fields / content values */
#define ADXL313_EXPECTED_DEVID0 0xad
#define ADXL313_EXPECTED_DEVID1 0x1d

/* bw / rate */
#define ADXL313_RATE_ODR_MSK GENMASK(3, 0)

/**
 * @brief The Output Data Rate (ODR).
 *
 * How many times per second the sensor outputs a new measurement. It is measured in Hz. Recommended
 * ODR for impact detection by datasheet is betwen 12.5Hz and 400Hz. The enum is then used to
 * indicate a selected ODR.
 */
enum adxl313_odr {
	ADXL313_ODR_6_25HZ = ADXL313_DT_ODR_6_25, /**< 6.25 Hz */
	ADXL313_ODR_12_5HZ = ADXL313_DT_ODR_12_5, /**< 12.5 Hz */
	ADXL313_ODR_25HZ = ADXL313_DT_ODR_25,     /**< 25 Hz */
	ADXL313_ODR_50HZ = ADXL313_DT_ODR_50,     /**< 50 Hz */
	ADXL313_ODR_100HZ = ADXL313_DT_ODR_100,   /**< 100 Hz */
	ADXL313_ODR_200HZ = ADXL313_DT_ODR_200,   /**< 200 Hz */
	ADXL313_ODR_400HZ = ADXL313_DT_ODR_400,   /**< 400 Hz */
	ADXL313_ODR_800HZ = ADXL313_DT_ODR_800,   /**< 800 Hz */
	ADXL313_ODR_1600HZ = ADXL313_DT_ODR_1600, /**< 1600 Hz */
	ADXL313_ODR_3200HZ = ADXL313_DT_ODR_3200, /**< 3200 Hz */
};

/* act/inact ctl */
#define ADXL313_ACT_INACT_CTL_ACT_XYZ   GENMASK(6, 4)
#define ADXL313_ACT_INACT_CTL_INACT_XYZ GENMASK(2, 0)

/* power ctl */
#define ADXL313_POWER_CTL_MEASURE     BIT(3)
#define ADXL313_POWER_CTL_AUTO_SLEEP  BIT(4)
#define ADXL313_POWER_CTL_LINK        BIT(5)
#define ADXL313_POWER_CTL_I2C_DISABLE BIT(6)

/* interrupt: en, map and source */
#define ADXL313_INT_OVERRUN   BIT(0)
#define ADXL313_INT_WATERMARK BIT(1)
#define ADXL313_INT_INACT     BIT(3)
#define ADXL313_INT_ACT       BIT(4)
#define ADXL313_INT_DATA_RDY  BIT(7)

/* data format */
#define ADXL313_DATA_FORMAT_RANGE      GENMASK(1, 0)
#define ADXL313_DATA_FORMAT_LJUSTIFY   BIT(2) /* enable left justified */
#define ADXL313_DATA_FORMAT_FULL_RES   BIT(3)
#define ADXL313_DATA_FORMAT_INT_INVERT BIT(5) /* enable int active low */
#define ADXL313_DATA_FORMAT_3WIRE_SPI  BIT(6) /* enable 3-wire SPI */
#define ADXL313_DATA_FORMAT_SELF_TEST  BIT(7) /* enable self test */

/**
 * @brief The sensor g-range selector.
 *
 * The g-range specifies the range between the maximum and the minimum acceleration levels the
 * device can accurately measure and represent in its output. The measurement unit is in ±g units
 * (where 1 g ≈ 9.80665 m/s²) and will be used in the decoder.
 * This enum is used to indicate the configured g range. It is used to select a particular g-range
 * in the array range_to_shift. Its units here are array positions.
 */
enum adxl313_range {
	ADXL313_RANGE_0_5G = 0, /**< Configures to 0.5 g */
	ADXL313_RANGE_1G,       /**< Configures to 1 g */
	ADXL313_RANGE_2G,       /**< Configures to 2 g */
	ADXL313_RANGE_4G,       /**< Configures to 3 g */
};

/**
 * @brief The array holding the valid g-range options.
 *
 * Together with the adxl313_range as index, a particular shift in the decoder for a particular
 * g-range is configured. The unit here is bit positions to shift.
 */
static const uint32_t range_to_shift[] = {
	[ADXL313_RANGE_0_5G] = 5, /**< Positions to shift for 0.5 g */
	[ADXL313_RANGE_1G] = 6,   /**< Positions to shift for 1 g */
	[ADXL313_RANGE_2G] = 7,   /**< Positions to shift for 2 g */
	[ADXL313_RANGE_4G] = 8,   /**< Positions to shift for 4 g */
};

/* fifo: ctl and status */
#define ADXL313_FIFO_SAMPLE_SIZE 6

#define ADXL313_FIFO_CTL_MODE_BYPASSED  0x00
#define ADXL313_FIFO_CTL_MODE_OLD_SAVED 0x40
#define ADXL313_FIFO_CTL_MODE_STREAMED  0x80
#define ADXL313_FIFO_CTL_MODE_TRIGGERED 0xc0
#define ADXL313_FIFO_CTL_SAMPLES_MSK    GENMASK(4, 0)
#define ADXL313_FIFO_STATUS_ENTRIES_MSK GENMASK(5, 0)
#define ADXL313_FIFO_MAX_SIZE           32

/**
 * @brief The FIFO mode of the device.
 *
 * The FIFO of the device supports four modi. The FIFO can either be BYPASSED and not used. In FIFO
 * "old saved" mode and STREAM mode, data from measurements of the x-, y- and z-axes are stored in
 * the FIFO. When the number of samples in the FIFO equals the level specified in the samples bits
 * of the FIFO_CTL register, the watermark interrupt is set. In FIFO_OLD_SAVED the FIFO then fills
 * up until 32 samples and stops, where in STREAM mode it continues collecting data, discarding
 * older data as new data arrives. In FIFO TRIGGER mode, in the context of Analog Devices, the FIFO
 * accumulates samples, holding the latest 32 samples from measurements of the x-, y-, and z-axes.
 * After a trigger event occurs and an interrupt is sent to the configured INT pin, FIFO keeps the
 * last n samples and then operates similar to the FIFO_OLD_SAVED mode.
 * Note, this refers only to Analog Device's FIFO modes, by the similarity of its naming, it must
 * not be confused with Zephyr's STREAM or TRIGGER APIs! To this respect, the sensor operates
 * mainly in BYPASSED (with no interrupt line available) or in STREAM mode when interrupt line is
 * configured and hence for all Zephyr triggers and zephyr's STREAM API.
 */
enum adxl313_fifo_mode {
	ADXL313_FIFO_BYPASSED,  /**< FIFO is bypassed */
	ADXL313_FIFO_OLD_SAVED, /**< Currently unused */
	ADXL313_FIFO_STREAMED,  /**< FIFO operates for zephyr's STREAM or TIGGER API */
	ADXL313_FIFO_TRIGGERED  /**< Currently unused */
};

#define ADXL313_BUS_I2C 0
#define ADXL313_BUS_SPI 1

/**
 * @brief Describe the FIFO in the device structure.
 */
struct adxl313_fifo_config {
	enum adxl313_fifo_mode fifo_mode; /**< The currently configured FIFO mode */
	uint8_t fifo_samples; /**< The number of entries to read for STREAM */
};

/**
 * @brief The FIFO and data environment, mainly used for the hand-over to the decoder.
 *
 * Note: Typically this sensor operates always in full-resolution.
 */
struct adxl313_fifo_data {
	uint8_t is_fifo: 1;                   /**< Is the FIFO enabled */
	uint8_t is_full_res: 1;               /**< Is full resolution enabled */
	enum adxl313_range selected_range: 2; /**< The selected g-range */
	uint8_t sample_set_size: 4;           /**< The FIFO sample size */
	uint8_t int_status;                   /**< The interrupt status */
	uint16_t accel_odr: 4;                /**< The ODR used to identify the period */
	uint16_t fifo_byte_count: 12;         /**< The FIFO byte count */
	uint64_t timestamp;                   /**< A timestamp */
} __attribute__((__packed__));

/**
 * @brief The sample representation for x-, y-, and z-axes measurements.
 *
 * Note: Typically this sensor operates always in full-resolution.
 */
struct adxl313_xyz_accel_data {
	enum adxl313_range selected_range; /**< The selected g-range */
	bool is_full_res; /**< Is full resolution enabled */
	int16_t x; /**< The x-axis measurement */
	int16_t y; /**< The y-axis measurement */
	int16_t z; /**< The z-axis measurement */
};

/**
 * @brief The hardware bus representation.
 */
union adxl313_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c; /**< I2C bus is configured */
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi; /**< SPI bus is configured */
#endif
};

/* register cache */
#define ADXL313_CACHE_START ADXL313_REG_SOFT_RESET  /* first entry */
#define ADXL313_CACHE_END   ADXL313_REG_FIFO_STATUS /* last entry */
#define ADXL313_CACHE_SIZE  (ADXL313_CACHE_END - ADXL313_CACHE_START + 1)
#define cache_idx(reg)      ((reg) - ADXL313_CACHE_START)

/**
 * @brief The driver data structure.
 */
struct adxl313_dev_data {
	uint8_t reg_cache[ADXL313_CACHE_SIZE];        /**< Cached bus access */
	struct adxl313_xyz_accel_data sample;         /**< The current sample */
	int8_t fifo_entries;                          /**< The actual read FIFO entries */
	struct adxl313_fifo_config fifo_config;       /**< The current FIFO configuration */
	bool is_full_res;                             /**< Do we operate in full resolution */
	enum adxl313_odr odr;                         /**< The configured ODR rate */
	enum adxl313_range selected_range;            /**< The selected g-range */
#ifdef CONFIG_ADXL313_TRIGGER
	struct k_mutex trigger_mutex;                 /**< A mutex used for trigger handling */
	struct gpio_callback int1_cb;                 /**< The interrupt callback for INT1 */
	struct gpio_callback int2_cb;                 /**< The interrupt callback for INT2 */
	sensor_trigger_handler_t act_handler;         /**< The handler for activity events */
	const struct sensor_trigger *act_trigger;     /**< The trigger for activity events */
	sensor_trigger_handler_t inact_handler;       /**< The handler for inactivity events */
	const struct sensor_trigger *inact_trigger;   /**< The trigger for inactivity events */
	sensor_trigger_handler_t drdy_handler;        /**< The handler for data ready events */
	const struct sensor_trigger *drdy_trigger;    /**< The trigger for data ready events */
	sensor_trigger_handler_t wm_handler;          /**< The handler for watermark events */
	const struct sensor_trigger *wm_trigger;      /**< The trigger for watermark events */
	sensor_trigger_handler_t overrun_handler;     /**< The handler for overrun events */
	const struct sensor_trigger *overrun_trigger; /**< The trigger for overrun events */
	const struct device *dev;                     /**< Pointes to the device */
	uint64_t timestamp;                           /**< A timestamp holder */
	uint8_t reg_int_source;                       /**< Content of the INT_SOURCE register */
#if defined(CONFIG_ADXL313_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADXL313_THREAD_STACK_SIZE); /**< Thread stack */
	struct k_sem gpio_sem;                        /**< A semaphor for GPIO handling */
	struct k_thread thread;                       /**< Holds the thread (for "own thread") */
#elif defined(CONFIG_ADXL313_TRIGGER_GLOBAL_THREAD)
	struct k_work work;                           /**< A work holder for "global thread" */
#endif
#endif /* CONFIG_ADXL313_TRIGGER */
#ifdef CONFIG_ADXL313_STREAM
	struct rtio_iodev_sqe *iodev_sqe;             /**< RTIO pointer to SQE */
	struct rtio *rtio_ctx;                        /**< RTIO context */
	struct rtio_iodev *iodev;                     /**< RTIO used iodev */
	uint8_t reg_fifo_status;                      /**< Content of the FIFO_STATUS register */
	struct rtio *r_cb;                            /**< RTIO callback holder */
#endif /* CONFIG_ADXL313_STREAM */
};

/**
 * @brief Function pointer to hold bus ready state in the device config.
 *
 * @param bus The used bus. NULL is not allowed.
 * @return Returns if the bus is ready (true), or not.
 */
typedef bool (*adxl313_bus_is_ready_fn)(const union adxl313_bus *bus);

/**
 * @brief Function pointer to hold register access function in the device config.
 *
 * @param dev The device structure. Null is not allowed.
 * @param cmd The current command.
 * @param reg_addr The register address to operate the command.
 * @param[in,out] data The data field to pass or read in. Must point to valid memory.
 * @param length The length of the valid memory, *data points to.
 * @return 0 in case of success, or a negative error code.
 */
typedef int (*adxl313_reg_access_fn)(const struct device *dev, uint8_t cmd, uint8_t reg_addr,
				     uint8_t *data, size_t length);
/**
 * @brief The device configuration for static device data.
 */
struct adxl313_dev_config {
	const union adxl313_bus bus;          /**< The used hardware bus */
	adxl313_bus_is_ready_fn bus_is_ready; /**< Function to obtain the hardware bus state */
	adxl313_reg_access_fn reg_access;     /**< Function to access the registers over this bus */
	enum adxl313_range selected_range;    /**< The selected sensor g-range */
	enum adxl313_odr odr;                 /**< The configured ODR */
	uint8_t bus_type;                     /**< The type of hardware bus */
#ifdef CONFIG_ADXL313_TRIGGER
	struct gpio_dt_spec gpio_int1;        /**< Devicetree handle to the GPIO for INT1 */
	struct gpio_dt_spec gpio_int2;        /**< Devicetree handle to the GPIO for INT2 */
	int8_t drdy_pad;                      /**< Data-ready handle used in the devicetree */
	uint8_t fifo_samples;                 /**< Number of samples in the FIFO */
#endif
};

void adxl313_accel_convert_q31(q31_t *out, int16_t sample, enum adxl313_range range,
			       bool is_full_res);

int adxl313_set_gpios_en(const struct device *dev, bool enable);
bool adxl313_is_measure_en(const struct device *dev);
int adxl313_set_measure_en(const struct device *dev, bool en);
int adxl313_flush_fifo(const struct device *dev);

#ifdef CONFIG_ADXL313_STREAM
int adxl313_flush_fifo_async(const struct device *dev);
void adxl313_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void adxl313_stream_fifo_irq_handler(const struct device *dev);
#endif

#ifdef CONFIG_ADXL313_TRIGGER
int adxl313_get_fifo_entries(const struct device *dev);
int adxl313_get_status(const struct device *dev, uint8_t *status);

int adxl313_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adxl313_init_interrupt(const struct device *dev);
#endif

int adxl313_reg_write_mask(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t data);

int adxl313_reg_assign_bits(const struct device *dev, uint8_t reg, uint8_t mask, bool en);

int adxl313_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr, uint8_t *data,
		       size_t len);

int adxl313_reg_write(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len);

int adxl313_raw_reg_read(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len);

int adxl313_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val);

int adxl313_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf);

int adxl313_read_sample(const struct device *dev, struct adxl313_xyz_accel_data *sample);

#ifdef CONFIG_SENSOR_ASYNC_API
void adxl313_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int adxl313_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifdef CONFIG_ADXL313_STREAM
int adxl313_configure_fifo(const struct device *dev, enum adxl313_fifo_mode mode,
			   uint8_t fifo_samples);
#endif

/** @} */

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_ */
