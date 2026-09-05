/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DH_LIS2DH_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DH_LIS2DH_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>

#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/rtio/rtio.h>
#endif

#define LIS2DH_REG_WAI			0x0f
#define LIS2DH_CHIP_ID			0x33
#define LIS2DH_POR_WAIT_MS		5

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define LIS2DH_AUTOINCREMENT_ADDR	BIT(7)

#define LIS2DH_REG_CTRL0		0x1e
#define LIS2DH_SDO_PU_DISC_MASK		BIT(7)

#define LIS2DH_REG_CTRL1		0x20
#define LIS2DH_ACCEL_X_EN_BIT		BIT(0)
#define LIS2DH_ACCEL_Y_EN_BIT		BIT(1)
#define LIS2DH_ACCEL_Z_EN_BIT		BIT(2)
#define LIS2DH_ACCEL_EN_BITS		(LIS2DH_ACCEL_X_EN_BIT | \
					LIS2DH_ACCEL_Y_EN_BIT | \
					LIS2DH_ACCEL_Z_EN_BIT)
#define LIS2DH_ACCEL_XYZ_MASK		BIT_MASK(3)

#define LIS2DH_LP_EN_BIT_MASK		BIT(3)
#if defined(CONFIG_LIS2DH_OPER_MODE_LOW_POWER)
	#define LIS2DH_LP_EN_BIT	BIT(3)
#else
	#define LIS2DH_LP_EN_BIT	0
#endif

#define LIS2DH_SUSPEND			0

#define LIS2DH_ODR_1			1
#define LIS2DH_ODR_2			2
#define LIS2DH_ODR_3			3
#define LIS2DH_ODR_4			4
#define LIS2DH_ODR_5			5
#define LIS2DH_ODR_6			6
#define LIS2DH_ODR_7			7
#define LIS2DH_ODR_8			8
#define LIS2DH_ODR_9			9

#if defined(CONFIG_LIS2DH_ODR_1)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_1
#elif defined(CONFIG_LIS2DH_ODR_2)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_2
#elif defined(CONFIG_LIS2DH_ODR_3)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_3
#elif defined(CONFIG_LIS2DH_ODR_4) || defined(CONFIG_LIS2DH_ODR_RUNTIME)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_4
#elif defined(CONFIG_LIS2DH_ODR_5)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_5
#elif defined(CONFIG_LIS2DH_ODR_6)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_6
#elif defined(CONFIG_LIS2DH_ODR_7)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_7
#elif defined(CONFIG_LIS2DH_ODR_8)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_8
#elif defined(CONFIG_LIS2DH_ODR_9_NORMAL) || defined(CONFIG_LIS2DH_ODR_9_LOW)
	#define LIS2DH_ODR_IDX		LIS2DH_ODR_9
#endif

#define LIS2DH_ODR_SHIFT		4
#define LIS2DH_ODR_RATE(r)		((r) << LIS2DH_ODR_SHIFT)
#define LIS2DH_ODR_BITS			(LIS2DH_ODR_RATE(LIS2DH_ODR_IDX))
#define LIS2DH_ODR_MASK			(BIT_MASK(4) << LIS2DH_ODR_SHIFT)

#define LIS2DH_REG_CTRL2		0x21
#define LIS2DH_HPIS1_EN_BIT		BIT(0)
#define LIS2DH_HPIS2_EN_BIT		BIT(1)
#define LIS2DH_HPCLICK_EN_BIT		BIT(2)
#define LIS2DH_FDS_EN_BIT		BIT(3)
#define LIS2DH_HPCF0_EN_BIT		BIT(4)
#define LIS2DH_HPCF1_EN_BIT		BIT(5)
#define LIS2DH_HPM0_EN_BIT		BIT(6)
#define LIS2DH_HPM1_EN_BIT		BIT(7)

#define LIS2DH_REG_CTRL3		0x22
#define LIS2DH_EN_CLICK_INT1		BIT(7)
#define LIS2DH_EN_IA_INT1		BIT(6)
#define LIS2DH_EN_DRDY1_INT1		BIT(4)
#define LIS2DH_EN_FIFO_WTM_INT1         BIT(2)
#define LIS2DH_EN_FIFO_OVRN_INT1        BIT(1)

#define LIS2DH_REG_CTRL4		0x23
#define LIS2DH_CTRL4_ST_SHIFT		1
#define LIS2DH_CTRL4_ST_MASK		(BIT_MASK(2) << LIS2DH_CTRL4_ST_SHIFT)
#define LIS2DH_CTRL4_BDU_BIT		BIT(7)
#define LIS2DH_FS_SHIFT			4
#define LIS2DH_FS_MASK			(BIT_MASK(2) << LIS2DH_FS_SHIFT)

#if defined(CONFIG_LIS2DH_ACCEL_RANGE_2G) ||\
	defined(CONFIG_LIS2DH_ACCEL_RANGE_RUNTIME)
	#define LIS2DH_FS_IDX		0
#elif defined(CONFIG_LIS2DH_ACCEL_RANGE_4G)
	#define LIS2DH_FS_IDX		1
#elif defined(CONFIG_LIS2DH_ACCEL_RANGE_8G)
	#define LIS2DH_FS_IDX		2
#elif defined(CONFIG_LIS2DH_ACCEL_RANGE_16G)
	#define LIS2DH_FS_IDX		3
#endif

#define LIS2DH_FS_SELECT(fs)		((fs) << LIS2DH_FS_SHIFT)
#define LIS2DH_FS_BITS			(LIS2DH_FS_SELECT(LIS2DH_FS_IDX))
#if defined(CONFIG_LIS2DH_OPER_MODE_HIGH_RES)
	#define LIS2DH_HR_BIT		BIT(3)
#else
	#define LIS2DH_HR_BIT		0
#endif

#define LIS2DH_REG_CTRL5		0x24
#define LIS2DH_EN_FIFO                  BIT(6)
#define LIS2DH_EN_LIR_INT2		BIT(1)
#define LIS2DH_EN_LIR_INT1		BIT(3)

#define LIS2DH_REG_CTRL6		0x25
#define LIS2DH_EN_CLICK_INT2		BIT(7)
#define LIS2DH_EN_IA_INT2		BIT(5)

#define LIS2DH_REG_REFERENCE		0x26

#define LIS2DH_REG_STATUS		0x27
#define LIS2DH_STATUS_ZYZ_OVR		BIT(7)
#define LIS2DH_STATUS_Z_OVR		BIT(6)
#define LIS2DH_STATUS_Y_OVR		BIT(5)
#define LIS2DH_STATUS_X_OVR		BIT(4)
#define LIS2DH_STATUS_OVR_MASK		(BIT_MASK(4) << 4)
#define LIS2DH_STATUS_ZYX_DRDY		BIT(3)
#define LIS2DH_STATUS_Z_DRDY		BIT(2)
#define LIS2DH_STATUS_Y_DRDY		BIT(1)
#define LIS2DH_STATUS_X_DRDY		BIT(0)
#define LIS2DH_STATUS_DRDY_MASK		BIT_MASK(4)

#define LIS2DH_REG_ACCEL_X_LSB		0x28
#define LIS2DH_REG_ACCEL_Y_LSB		0x2A
#define LIS2DH_REG_ACCEL_Z_LSB		0x2C
#define LIS2DH_REG_ACCEL_X_MSB		0x29
#define LIS2DH_REG_ACCEL_Y_MSB		0x2B
#define LIS2DH_REG_ACCEL_Z_MSB		0x2D

#define LIS2DH_REG_FIFO_CTRL    0x2E
#define LIS2DH_FIFO_MODE_BYPASS 0U
#define LIS2DH_FIFO_MODE_STREAM BIT(7)
#define LIS2DH_FIFO_FTH_MASK    BIT_MASK(5)

#define LIS2DH_REG_FIFO_SRC     0x2F
#define LIS2DH_FIFO_WTM         BIT(7)
#define LIS2DH_FIFO_OVRN        BIT(6)
#define LIS2DH_FIFO_EMPTY       BIT(5)
#define LIS2DH_FIFO_FSS_MASK    BIT_MASK(5)
#define LIS2DH_FIFO_MAX_SAMPLES 32U
#define LIS2DH_FIFO_SAMPLE_SIZE 6U
#define LIS2DH_FIFO_MAX_BYTES   (LIS2DH_FIFO_MAX_SAMPLES * LIS2DH_FIFO_SAMPLE_SIZE)

#define LIS2DH_REG_INT1_CFG		0x30
#define LIS2DH_REG_INT1_SRC		0x31
#define LIS2DH_REG_INT1_THS		0x32
#define LIS2DH_REG_INT1_DUR		0x33
#define LIS2DH_REG_INT2_CFG		0x34
#define LIS2DH_REG_INT2_SRC		0x35
#define LIS2DH_REG_INT2_THS		0x36
#define LIS2DH_REG_INT2_DUR		0x37

#define LIS2DH_INT_CFG_MODE_SHIFT	6
#define LIS2DH_INT_CFG_AOI_CFG		BIT(7)
#define LIS2DH_INT_CFG_6D_CFG		BIT(6)
#define LIS2DH_INT_CFG_ZHIE_ZUPE	BIT(5)
#define LIS2DH_INT_CFG_ZLIE_ZDOWNE	BIT(4)
#define LIS2DH_INT_CFG_YHIE_YUPE	BIT(3)
#define LIS2DH_INT_CFG_YLIE_YDOWNE	BIT(2)
#define LIS2DH_INT_CFG_XHIE_XUPE	BIT(1)
#define LIS2DH_INT_CFG_XLIE_XDOWNE	BIT(0)

#define LIS2DH_REG_CFG_CLICK		0x38
#define LIS2DH_EN_CLICK_ZD		BIT(5)
#define LIS2DH_EN_CLICK_ZS		BIT(4)
#define LIS2DH_EN_CLICK_YD		BIT(3)
#define LIS2DH_EN_CLICK_YS		BIT(2)
#define LIS2DH_EN_CLICK_XD		BIT(1)
#define LIS2DH_EN_CLICK_XS		BIT(0)

#define LIS2DH_REG_CLICK_SRC		0x39
#define LIS2DH_CLICK_SRC_DCLICK		BIT(5)
#define LIS2DH_CLICK_SRC_SCLICK		BIT(4)

#define LIS2DH_REG_CFG_CLICK_THS	0x3A
#define LIS2DH_CLICK_LIR		BIT(7)

#define LIS2DH_REG_TIME_LIMIT		0x3B

/* sample buffer size includes status register */
#define LIS2DH_BUF_SZ			7

union lis2dh_sample {
	uint8_t raw[LIS2DH_BUF_SZ];
	struct {
		uint8_t status;
		int16_t xyz[3];
	} __packed;
};

union lis2dh_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

struct temperature {
	uint8_t cfg_addr;
	uint8_t enable_mask;
	uint8_t dout_addr;
	uint8_t fractional_bits;
};

struct lis2dh_config {
	int (*bus_init)(const struct device *dev);
	const union lis2dh_bus_cfg bus_cfg;
#ifdef CONFIG_LIS2DH_TRIGGER
	const struct gpio_dt_spec gpio_drdy;
	const struct gpio_dt_spec gpio_int;
	const uint8_t int1_mode;
	const uint8_t int2_mode;
#endif /* CONFIG_LIS2DH_TRIGGER */
	struct {
		bool is_lsm303agr_dev : 1;
		bool disc_pull_up : 1;
		bool anym_on_int1 : 1;
		bool anym_latch : 1;
		uint8_t anym_mode : 2;
	} hw;
#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
	const struct temperature temperature;
#endif
#ifdef CONFIG_LIS2DH_FIFO
	const uint8_t fifo_watermark;
#endif
};

struct lis2dh_transfer_function {
	int (*read_data)(const struct device *dev, uint8_t reg_addr,
			 uint8_t *value, uint8_t len);
	int (*write_data)(const struct device *dev, uint8_t reg_addr,
			  uint8_t *value, uint8_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr,
			uint8_t *value);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr,
			 uint8_t value);
	int (*update_reg)(const struct device *dev, uint8_t reg_addr,
			  uint8_t mask, uint8_t value);
};

struct lis2dh_data {
	const struct device *bus;
	const struct lis2dh_transfer_function *hw_tf;

	union lis2dh_sample sample;
	/* current scaling factor, in micro m/s^2 / lsb */
	uint32_t scale;

#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
	struct sensor_value temperature;
#endif

	uint8_t reg_ctrl1_active_val;

#if defined(CONFIG_LIS2DH_FIFO) || defined(CONFIG_SENSOR_ASYNC_API)
	/* Serializes register accesses and state transitions, including RTIO. */
	struct k_mutex lock;
#endif
#ifdef CONFIG_LIS2DH_FIFO
	struct {
		int16_t xyz[3];
		uint64_t timestamp_ns;
	} fifo_samples[CONFIG_LIS2DH_FIFO_SW_QUEUE_SAMPLES];
	uint16_t fifo_head;
	uint16_t fifo_tail;
	uint16_t fifo_count;
#ifdef CONFIG_LIS2DH_FIFO_STATS
	uint32_t fifo_dropped_samples;
#endif
	uint64_t fifo_period_ns;
	atomic_t fifo_active;
	bool fifo_faulted;
	bool fifo_restore_pending;
	uint8_t fifo_saved[3];
	bool fifo_cache_valid;
	sensor_trigger_handler_t fifo_handler_watermark;
	const struct sensor_trigger *fifo_trig_watermark;
	sensor_trigger_handler_t fifo_handler_full;
	const struct sensor_trigger *fifo_trig_full;
#ifdef CONFIG_LIS2DH_STREAM
	struct rtio_iodev_sqe *streaming_sqe;
	bool stream_active;
	const struct rtio_iodev *stream_iodev;
	atomic_ptr_t stream_handoff;
	atomic_ptr_t stream_pending;
	uint8_t stream_routes;
	uint8_t stream_nop_events;
	struct k_work_delayable stream_work;
#endif
#endif

#ifdef CONFIG_LIS2DH_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_int1_cb;
	struct gpio_callback gpio_int2_cb;

	sensor_trigger_handler_t handler_drdy;
	const struct sensor_trigger *trig_drdy;
	sensor_trigger_handler_t handler_anymotion;
	const struct sensor_trigger *trig_anymotion;
	sensor_trigger_handler_t handler_tap;
	const struct sensor_trigger *trig_tap;
	atomic_t trig_flags;
	enum sensor_channel chan_drdy;

#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DH_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LIS2DH_TRIGGER */
};

static inline void lis2dh_lock(const struct device *dev)
{
#if defined(CONFIG_LIS2DH_FIFO) || defined(CONFIG_SENSOR_ASYNC_API)
	struct lis2dh_data *data = dev->data;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void lis2dh_unlock(const struct device *dev)
{
#if defined(CONFIG_LIS2DH_FIFO) || defined(CONFIG_SENSOR_ASYNC_API)
	struct lis2dh_data *data = dev->data;

	(void)k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static inline uint64_t lis2dh_timestamp_ns(void)
{
#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	return k_cyc_to_ns_floor64(k_cycle_get_64());
#else
	/* Uptime extends narrow hardware counters across wraps. */
	return k_ticks_to_ns_floor64(k_uptime_ticks());
#endif
}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
int lis2dh_spi_access(struct lis2dh_data *ctx, uint8_t cmd,
		      void *data, size_t length);
#endif

#ifdef CONFIG_LIS2DH_TRIGGER
int lis2dh_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int lis2dh_init_interrupt(const struct device *dev);

int lis2dh_acc_slope_config(const struct device *dev,
			    enum sensor_attribute attr,
			    const struct sensor_value *val);

int lis2dh_trigger_int1_set(const struct device *dev, bool enable);
int lis2dh_trigger_fifo_int1_set(const struct device *dev, bool enable);
#endif

#ifdef CONFIG_LIS2DH_FIFO
int lis2dh_fifo_init(const struct device *dev);
bool lis2dh_fifo_is_active(const struct device *dev);
bool lis2dh_fifo_is_busy(const struct device *dev);
int lis2dh_fifo_start(const struct device *dev);
int lis2dh_fifo_stop(const struct device *dev);
int lis2dh_fifo_handle_irq(const struct device *dev);
int lis2dh_fifo_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler);
int lis2dh_fifo_sample_fetch(const struct device *dev);
int lis2dh_fifo_cache_copy(const struct device *dev, union lis2dh_sample *sample);
#ifdef CONFIG_LIS2DH_STREAM
int lis2dh_fifo_drop(const struct device *dev);
void lis2dh_stream_init(const struct device *dev);
int lis2dh_stream_handle_irq(const struct device *dev);
void lis2dh_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
#endif
#endif

#ifdef CONFIG_SENSOR_ASYNC_API
void lis2dh_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int lis2dh_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif

int lis2dh_spi_init(const struct device *dev);
int lis2dh_i2c_init(const struct device *dev);


#endif /* __SENSOR_LIS2DH__ */
