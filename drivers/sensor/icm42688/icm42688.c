/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42688

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/rtio_executor_simple.h>
#include "icm42688.h"
#include "icm42688_reg.h"
#include "icm42688_spi.h"
#include "icm42688_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688, CONFIG_SENSOR_LOG_LEVEL);

struct icm42688_sensor_data {
	struct icm42688_dev_data dev_data;

	int16_t readings[7];

#ifdef CONFIG_ICM42688_RTIO
	const struct device *dev;
	struct rtio *r;
	struct rtio_iodev *fifo_iodev;
	struct rtio_iodev_sqe *iodev_sqe;
	bool checked_out;
	uint32_t overflows;
	struct gpio_callback gpio_cb;
#ifdef CONFIG_SPI_RTIO
	struct rtio_iodev *spi_iodev;
	uint8_t int_status;
	uint16_t fifo_count;
	atomic_t reading_fifo;
#endif /* CONFIG_SPI_RTIO */
#endif /* CONFIG_ICM42688_RTIO */

#if CONFIG_ICM42688_TRIGGER_OWN_THREAD
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42688_THREAD_STACK_SIZE);
#endif
};


#if defined(CONFIG_SPI_RTIO) && defined(CONFIG_ICM42688_RTIO)
#define ICM42688_USE_SPI_RTIO 1
#else
#define ICM42688_USE_SPI_RTIO 0
#endif

struct icm42688_sensor_config {
	struct icm42688_dev_cfg dev_cfg;
};

static void icm42688_convert_accel(struct sensor_value *val, int16_t raw_val,
				   struct icm42688_cfg *cfg)
{
	icm42688_accel_ms(cfg, (int32_t)raw_val, &val->val1, &val->val2);
}

static void icm42688_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  struct icm42688_cfg *cfg)
{
	icm42688_gyro_rads(cfg, (int32_t)raw_val, &val->val1, &val->val2);
}

static inline void icm42688_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	icm42688_temp_c((int32_t)raw_val, &val->val1, &val->val2);
}

static int icm42688_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm42688_sensor_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm42688_convert_accel(&val[0], data->readings[1], &data->dev_data.cfg);
		icm42688_convert_accel(&val[1], data->readings[2], &data->dev_data.cfg);
		icm42688_convert_accel(&val[2], data->readings[3], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm42688_convert_accel(val, data->readings[1], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm42688_convert_accel(val, data->readings[2], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm42688_convert_accel(val, data->readings[3], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm42688_convert_gyro(&val[0], data->readings[4], &data->dev_data.cfg);
		icm42688_convert_gyro(&val[1], data->readings[5], &data->dev_data.cfg);
		icm42688_convert_gyro(&val[2], data->readings[6], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm42688_convert_gyro(val, data->readings[4], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm42688_convert_gyro(val, data->readings[5], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm42688_convert_gyro(val, data->readings[6], &data->dev_data.cfg);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42688_convert_temp(val, data->readings[0]);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int icm42688_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct icm42688_sensor_data *data = dev->data;
	uint8_t readings[14];
	int res;

	res = icm42688_read_all(dev, readings);

	if (res) {
		return res;
	}

	for (int i = 0; i < 7; i++) {
		data->readings[i] = sys_le16_to_cpu((readings[i * 2] << 8) | readings[i * 2 + 1]);
	}

	return 0;
}

static int icm42688_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct icm42688_sensor_data *data = dev->data;
	struct icm42688_cfg new_config = data->dev_data.cfg;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			new_config.accel_odr = icm42688_accel_hz_to_reg(val->val1);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			new_config.accel_fs = icm42688_accel_fs_to_reg(sensor_ms2_to_g(val));
		} else {
			LOG_ERR("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			new_config.gyro_odr = icm42688_gyro_odr_to_reg(val->val1);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			new_config.gyro_fs = icm42688_gyro_fs_to_reg(sensor_rad_to_degrees(val));
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	if (res) {
		return res;
	}
	return icm42688_safely_configure(dev, &new_config);
}

static int icm42688_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm42688_sensor_data *data = dev->data;
	const struct icm42688_cfg *cfg = &data->dev_data.cfg;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			icm42688_accel_reg_to_hz(cfg->accel_odr, val);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			icm42688_accel_reg_to_fs(cfg->accel_fs, val);
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			icm42688_gyro_reg_to_odr(cfg->gyro_odr, val);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			icm42688_gyro_reg_to_fs(cfg->gyro_fs, val);
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	return res;
}

#ifdef CONFIG_ICM42688_RTIO


static int icm42688_fifo_iodev(const struct device *dev, struct rtio_iodev **iodev)
{
	struct icm42688_sensor_data *data = dev->data;

	*iodev = data->fifo_iodev;

	return 0;
}

static int icm42688_fifo_read(const struct device *dev)
{
	/** TODO initiate a fifo read without checking int status */

	return 0;
}


static int icm42688_fifo_start(const struct device *dev)
{
	struct icm42688_dev_data *data = dev->data;
	const struct icm42688_dev_cfg *cfg = dev->config;
	int res;

	/* Disable gpio interrupt */
	res = gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_DISABLE);
	if (res != 0) {
		goto out;
	}

	/* Enable FIFO on chip */
	struct icm42688_cfg sensor_cfg = data->cfg;

	sensor_cfg.fifo_en = true;

	/* TODO figure out a nice way of configuring these with attr set e.g.
	 * fifo_odr, fifo_wm (in bytes or records?) attrs?
	 */
	sensor_cfg.fifo_wm = 1024; /* wm in bytes */
	sensor_cfg.accel_odr = ICM42688_ACCEL_ODR_32000;
	sensor_cfg.gyro_odr = ICM42688_GYRO_ODR_32000;

	res = icm42688_safely_configure(dev, &sensor_cfg);
	if (res != 0) {
		goto out;
	}

	/* Enable the gpio interrupt */
	res = gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);

out:
	return res;
}

static int icm42688_fifo_stop(const struct device *dev)
{
	struct icm42688_sensor_data *data = dev->data;
	const struct icm42688_sensor_config *cfg = dev->config;
	int res;

	/* Disable gpio interrupt */
	res = gpio_pin_interrupt_configure_dt(&cfg->dev_cfg.gpio_int1, GPIO_INT_DISABLE);


	/* Disable FIFO on chip */
	struct icm42688_cfg sensor_cfg = data->dev_data.cfg;

	sensor_cfg.fifo_en = false;
	/* TODO figure out a nice way of configuring these with saved attrs e.g. xyz odrs attrs? */
	sensor_cfg.accel_odr = ICM42688_ACCEL_ODR_1000;
	sensor_cfg.gyro_odr = ICM42688_GYRO_ODR_1000;

	res = icm42688_safely_configure(dev, &sensor_cfg);
	if (res != 0) {
		goto out;
	}

	/* Clear request queue */
	rtio_iodev_cancel_all(data->fifo_iodev);

	/* TODO Possibly re-enable gpio interrupt if trigger_set has been setup */

out:
	return 0;
}
/* Define a simple minimum buffer size of 1 fifo packet plus a copy of the config */
#define ICM42688_MAX_FIFO_PKT_SIZE 20
#define ICM42688_MIN_BUF_SIZE (ICM42688_MAX_FIFO_PKT_SIZE + sizeof(struct icm42688_cfg))

/* Accepts read requests with buffers long enough to store at least a single FIFO packet
 * and appends them to a pending request queue. This is then pulled from and read into
 * when sensor_fifo_read is called.
 */
void icm42688_fifo_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rtio_sqe *sqe = iodev_sqe->sqe;

	if (sqe->op != RTIO_OP_RX || sqe->buf_len < ICM42688_MIN_BUF_SIZE) {
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}

	rtio_mpsc_push((struct rtio_mpsc *)&iodev_sqe->sqe->iodev->iodev_sq, &iodev_sqe->q);
}


static const struct rtio_iodev_api icm42688_fifo_iodev_api = {
	.submit = icm42688_fifo_submit
};

struct fifo_header {
	uint8_t int_status;
	uint16_t gyro_odr: 4;
	uint16_t accel_odr: 4;
	uint16_t gyro_fs: 3;
	uint16_t accel_fs: 3;
	uint16_t packet_format: 2;
} __attribute__((__packed__));

BUILD_ASSERT(sizeof(struct fifo_header) == 3);


#if CONFIG_SPI_RTIO

static void icm42688_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct icm42688_sensor_data *drv_data = dev->data;
	const struct icm42688_sensor_config *drv_cfg = dev->config;

	/* TODO report number of frames or bytes read here */
	rtio_iodev_sqe_ok(drv_data->iodev_sqe, drv_data->fifo_count);
	drv_data->iodev_sqe = NULL;

	gpio_pin_interrupt_configure_dt(&drv_cfg->dev_cfg.gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
}

static void icm42688_fifo_count_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct icm42688_sensor_data *drv_data = dev->data;
	const struct icm42688_sensor_config *drv_cfg = dev->config;
	uint8_t *fifo_count_buf = (uint8_t *)&drv_data->fifo_count;
	uint16_t fifo_count = ((fifo_count_buf[0] << 8) | fifo_count_buf[1]);

	drv_data->fifo_count = fifo_count;

	/* Pull a operation from our device iodev queue, validated to only be reads */
	struct rtio_mpsc_node *next = rtio_mpsc_pop(&drv_data->fifo_iodev->iodev_sq);

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (next == NULL) {
		gpio_pin_interrupt_configure_dt(&drv_cfg->dev_cfg.gpio_int1,
			 GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

	drv_data->iodev_sqe = iodev_sqe;

	const size_t packet_size = drv_data->dev_data.cfg.fifo_hires ? 20 : 16;

	/* Check buffer is big enough for configuration
	 * and at least 1 packet
	 *
	 * TODO determine len to be aligned down to the nearest packet
	 */
	if (iodev_sqe->sqe->buf_len < (sizeof(struct fifo_header) + packet_size)) {
		LOG_WRN("Buffer minimum size not met");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	/* TODO is packet format even needed? the fifo has a header per packet
	 * already
	 */
	struct fifo_header hdr = {
		.int_status = drv_data->int_status,
		.gyro_odr = drv_data->dev_data.cfg.gyro_odr,
		.gyro_fs = drv_data->dev_data.cfg.gyro_fs,
		.accel_odr = drv_data->dev_data.cfg.accel_odr,
		.accel_fs = drv_data->dev_data.cfg.accel_fs,
		.packet_format = 0,
	};

	uint32_t buf_avail = iodev_sqe->sqe->buf_len;

	memcpy(iodev_sqe->sqe->buf, &hdr, sizeof(hdr));
	buf_avail -= sizeof(hdr);

	/* TODO account for other packet sizes */
	const uint32_t pkt_size = 16;

	uint32_t read_len = fifo_count > buf_avail ? buf_avail : fifo_count;
	uint32_t pkts = read_len / pkt_size;

	read_len = pkts*pkt_size;

	__ASSERT_NO_MSG(read_len % pkt_size == 0);

	uint8_t *read_buf = &iodev_sqe->sqe->buf[sizeof(hdr)];

	/* Flush out completions  */
	struct rtio_cqe *cqe = rtio_cqe_consume(drv_data->r);

	while (cqe != NULL) {
		cqe = rtio_cqe_consume(drv_data->r);
	}
	rtio_cqe_release_all(r);

	/* Setup new rtio chain to read the fifo data and report then check the
	 * result
	 */
	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(drv_data->r);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(drv_data->r);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(drv_data->r);
	const uint8_t reg_addr = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_FIFO_DATA);

	rtio_sqe_prep_tiny_write(write_fifo_addr, drv_data->spi_iodev,
				 RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, drv_data->spi_iodev, RTIO_PRIO_NORM,
			   read_buf, read_len, iodev_sqe);
	/* TODO add flush op here, works without because we are using simple executor */
	rtio_sqe_prep_callback(complete_op, icm42688_complete_cb,
			       (void *)dev, NULL);

	rtio_submit(drv_data->r, 0);
}

static void icm42688_int_status_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct icm42688_sensor_data *drv_data = dev->data;
	const struct icm42688_sensor_config *drv_cfg = dev->config;

	if (!(drv_data->int_status & BIT_INT_STATUS_FIFO_THS
	      || drv_data->int_status & BIT_INT_STATUS_FIFO_FULL)) {
		gpio_pin_interrupt_configure_dt(&drv_cfg->dev_cfg.gpio_int1,
						GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* In effect a overrun, as the sensor is producing faster than we are consuming */
	if (drv_data->int_status & BIT_INT_STATUS_FIFO_FULL) {
		drv_data->overflows++;
	}

	/* Flush out completions  */
	struct rtio_cqe *cqe = rtio_cqe_consume(drv_data->r);

	while (cqe != NULL) {
		cqe = rtio_cqe_consume(drv_data->r);
	}
	rtio_cqe_release_all(r);

	struct rtio_sqe *write_fifo_count_reg = rtio_sqe_acquire(drv_data->r);
	struct rtio_sqe *read_fifo_count = rtio_sqe_acquire(drv_data->r);
	struct rtio_sqe *check_fifo_count = rtio_sqe_acquire(drv_data->r);
	uint8_t reg = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_FIFO_COUNTH);
	uint8_t *read_buf = (uint8_t *)&drv_data->fifo_count;

	rtio_sqe_prep_tiny_write(write_fifo_count_reg, drv_data->spi_iodev, RTIO_PRIO_NORM,
		&reg, 1, NULL);
	write_fifo_count_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_count, drv_data->spi_iodev, RTIO_PRIO_NORM,
			   read_buf, 2, NULL);
	/* TODO add flush op here, works without because we are using simple executor */
	rtio_sqe_prep_callback(check_fifo_count, icm42688_fifo_count_cb, arg, NULL);

	rtio_submit(drv_data->r, 0);
}

void icm42688_rtio_fifo_event(const struct device *dev)
{
	struct icm42688_sensor_data *drv_data = dev->data;

	/*
	 * Setup rtio chain of ops with inline calls to make decisions
	 * 1. read int status
	 * 2. call to check int status and get pending RX operation
	 * 4. read fifo len
	 * 5. call to determine read len
	 * 6. read fifo
	 * 7. call to report completion
	 */
	struct rtio_sqe *write_int_reg = rtio_sqe_acquire(drv_data->r);
	struct rtio_sqe *read_int_reg = rtio_sqe_acquire(drv_data->r);
	struct rtio_sqe *check_int_status  = rtio_sqe_acquire(drv_data->r);
	uint8_t reg = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_INT_STATUS);

	rtio_sqe_prep_tiny_write(write_int_reg, drv_data->spi_iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_int_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_int_reg, drv_data->spi_iodev, RTIO_PRIO_NORM,
		&drv_data->int_status, 1, NULL);
	/* TODO add flush op here, works without because we are using simple executor */
	rtio_sqe_prep_callback(check_int_status, icm42688_int_status_cb, (void *)dev, NULL);

	rtio_submit(drv_data->r, 0);
}

#else

void icm42688_rtio_fifo_event(const struct device *dev)
{
	struct icm42688_sensor_data *data = dev->data;
	const struct icm42688_sensor_config *cfg = dev->config;

	int res = 0;
	uint16_t fifo_count, fifo_lost;
	uint8_t int_status;
	uint8_t read_buf[2];

	res = icm42688_spi_read(&cfg->dev_cfg.spi, REG_INT_STATUS, &int_status, 1);
	if (res) {
		goto out;
	}

	if (data->dev_data.cfg.fifo_en != true) {
		goto out;
	}

	if (!(int_status & BIT_INT_STATUS_FIFO_THS || int_status & BIT_INT_STATUS_FIFO_FULL)) {
		goto out;
	}

	/* In effect a overrun, as the sensor is producing faster than we are consuming */
	if (int_status & BIT_INT_STATUS_FIFO_FULL) {
		data->overflows++;
	}

	res = icm42688_spi_read(&cfg->dev_cfg.spi, REG_FIFO_COUNTH, read_buf, 2);

	if (res) {
		goto out;
	}
	fifo_count = ((read_buf[0] << 8) | read_buf[1]);

	if (fifo_count < data->dev_data.cfg.fifo_wm) {
		goto out;
	}

	/* Get a buffer to read into, if one exists */
	struct rtio_mpsc_node *next = rtio_mpsc_pop(&data->fifo_iodev->iodev_sq);

	/* Not inherently an underrun/overrun as we may have a buffer to fill
	 * next time
	 */
	if (next == NULL) {
		/* In effect we want to punish this thread (ugh) for now so
		 * others may run. Without this its very possible all time is
		 * spent jumping from the interrupt to this thread not allowing
		 * other threads to run (like the one adding the buffers to
		 * read into)
		 */
		k_msleep(1);
		goto out;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

	/* Check buffer is big enough for configuration
	 * and at least 1 packet
	 *
	 * TODO determine len to be aligned down to the nearest packet
	 */
	if (iodev_sqe->sqe->buf_len < (sizeof(struct fifo_header) + 20)) {
		LOG_WRN("Buffer minimum size not met");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		goto out;
	}

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	/* TODO is packet format even needed? the fifo has a header per packet
	 * already
	 */
	struct fifo_header hdr = {
		.int_status = int_status,
		.gyro_odr = data->dev_data.cfg.gyro_odr,
		.gyro_fs = data->dev_data.cfg.gyro_fs,
		.accel_odr = data->dev_data.cfg.accel_odr,
		.accel_fs = data->dev_data.cfg.accel_fs,
		.packet_format = 0,
	};

	uint32_t buf_avail = iodev_sqe->sqe->buf_len;

	memcpy(iodev_sqe->sqe->buf, &hdr, sizeof(hdr));
	buf_avail -= sizeof(hdr);

	/* TODO account for other packet sizes */
	const uint32_t pkt_size = 16;
	uint32_t read_len = fifo_count > buf_avail ? buf_avail : fifo_count;
	uint32_t pkts = read_len / pkt_size;

	read_len = pkts*pkt_size;

	__ASSERT_NO_MSG(read_len % pkt_size == 0);

	uint8_t *read_addr = &iodev_sqe->sqe->buf[sizeof(hdr)];

	res = icm42688_spi_read(&cfg->dev_cfg.spi, REG_FIFO_DATA, read_addr,
				read_len);

	if (res != 0) {
		LOG_ERR("Error reading fifo");
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		goto out;
	}

	rtio_iodev_sqe_ok(iodev_sqe, read_len + sizeof(hdr));

out:
	return;
}

#endif /* CONFIG_SPI_RTIO */
#endif /* CONFIG_ICM42688_RTIO */


static const struct sensor_driver_api icm42688_driver_api = {
	.sample_fetch = icm42688_sample_fetch,
	.channel_get = icm42688_channel_get,
	.attr_set = icm42688_attr_set,
	.attr_get = icm42688_attr_get,
#ifdef CONFIG_ICM42688_TRIGGER
	.trigger_set = icm42688_trigger_set,
#endif /* CONFIG_ICM42688_TRIGGER */
#ifdef CONFIG_ICM42688_RTIO
	.fifo_iodev = icm42688_fifo_iodev,
	.fifo_read = icm42688_fifo_read,
	.fifo_start = icm42688_fifo_start,
	.fifo_stop = icm42688_fifo_stop,
#endif /* CONFIG_ICM42688_RTIO */
};

int icm42688_init(const struct device *dev)
{
	struct icm42688_sensor_data *data = dev->data;
	const struct icm42688_sensor_config *cfg = dev->config;
	int res;

	if (!spi_is_ready_dt(&cfg->dev_cfg.spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	if (icm42688_reset(dev)) {
		LOG_ERR("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM42688_TRIGGER
	res = icm42688_trigger_init(dev);
	if (res != 0) {
		LOG_ERR("Failed to initialize triggers");
		return res;
	}

	res = icm42688_trigger_enable_interrupt(dev);
	if (res != 0) {
		LOG_ERR("Failed to enable triggers");
		return res;
	}
#endif

	data->dev_data.cfg.accel_mode = ICM42688_ACCEL_LN;
	data->dev_data.cfg.gyro_mode = ICM42688_GYRO_LN;
	data->dev_data.cfg.accel_fs = ICM42688_ACCEL_FS_2G;
	data->dev_data.cfg.gyro_fs = ICM42688_GYRO_FS_125;
	data->dev_data.cfg.accel_odr = ICM42688_ACCEL_ODR_1000;
	data->dev_data.cfg.gyro_odr = ICM42688_GYRO_ODR_1000;

	res = icm42688_configure(dev, &data->dev_data.cfg);
	if (res != 0) {
		LOG_ERR("Failed to configure");
		return res;
	}

	return res;
}

#ifndef CONFIG_ICM42688_TRIGGER
void icm42688_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}
void icm42688_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}
#endif /* CONFIG_ICM42688_TRIGGER */

/* device defaults to spi mode 0/3 support */
#define ICM42688_SPI_CFG                                                                        \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ICM42688_RTIO_DEFINE(inst)                                                              \
	IF_ENABLED(CONFIG_SPI_RTIO,								\
		(SPI_DT_IODEV_DEFINE(icm42688_spi_iodev_##inst,					\
		 DT_DRV_INST(inst), ICM42688_SPI_CFG, 0U));)					\
	RTIO_EXECUTOR_SIMPLE_DEFINE(icm42688_rtio_exec_##inst);					\
	RTIO_DEFINE(icm42688_rtio_##inst,							\
		(struct rtio_executor *)&icm42688_rtio_exec_##inst, 8, 4);			\
	RTIO_IODEV_DEFINE(icm42688_fifo_iodev_##inst, &icm42688_fifo_iodev_api, NULL);

#define ICM42688_DEFINE_CONFIG(inst) \
	static const struct icm42688_sensor_config icm42688_cfg_##inst = {                      \
		.dev_cfg =                                                                      \
			{                                                                       \
				.spi = SPI_DT_SPEC_INST_GET(inst, ICM42688_SPI_CFG, 0U),        \
				.gpio_int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),    \
			},                                                                      \
	}

#define ICM42688_DEFINE_DATA(inst)								\
	IF_ENABLED(CONFIG_ICM42688_RTIO, (ICM42688_RTIO_DEFINE(inst)));				\
	static struct icm42688_sensor_data icm42688_driver_##inst = {                           \
		IF_ENABLED(CONFIG_ICM42688_RTIO, (						\
		.r = &icm42688_rtio_##inst,							\
		.fifo_iodev = &icm42688_fifo_iodev_##inst,					\
		))										\
		IF_ENABLED(ICM42688_USE_SPI_RTIO, (						\
		.spi_iodev = &icm42688_spi_iodev_##inst,					\
		))										\
	}

#define ICM42688_INIT(inst)									\
	ICM42688_DEFINE_CONFIG(inst);								\
	ICM42688_DEFINE_DATA(inst);								\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm42688_init, NULL, &icm42688_driver_##inst,	\
				     &icm42688_cfg_##inst, POST_KERNEL,				\
				     CONFIG_SENSOR_INIT_PRIORITY, &icm42688_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM42688_INIT)
