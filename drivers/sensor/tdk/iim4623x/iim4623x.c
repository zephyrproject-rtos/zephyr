/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "iim4623x.h"
#include "iim4623x_reg.h"
#include "iim4623x_bus.h"
#include "iim4623x_decoder.h"
#include "iim4623x_stream.h"
#include <zephyr/dt-bindings/sensor/iim4623x.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(iim4623x, CONFIG_SENSOR_LOG_LEVEL);

int iim4623x_prepare_cmd(const struct device *dev, uint8_t cmd_type, uint8_t *payload,
			 size_t payload_len)
{
	struct iim4623x_data *data = dev->data;
	size_t out_buf_len = ARRAY_SIZE(data->trx_buf);
	uint8_t *out_buf = data->trx_buf;
	uint8_t *out_buf_head = out_buf;
	struct iim4623x_pck_preamble preamble = {
		.header = sys_cpu_to_be16(IIM4623X_PCK_HEADER_TX),
		.length = IIM4623X_PACKET_LEN(payload_len),
		.type = cmd_type,
	};
	struct iim4623x_pck_postamble postamble = {
		.checksum = 0x00,
		.footer = sys_cpu_to_be16(IIM4623X_PCK_FOOTER),
	};

	/* Check for undersized buffer */
	if (out_buf_len < IIM4623X_TX_LEN(payload_len)) {
		return -ENOMEM;
	}

	/* Copy preamble to buffer */
	memcpy(out_buf_head, &preamble, sizeof(preamble));
	out_buf_head += sizeof(preamble);

	/* Copy payload to buffer, if provided */
	if (payload) {
		memcpy(out_buf_head, payload, payload_len);
		out_buf_head += payload_len;
	}

	/* Calculate checksum and correct endianness for the wire */
	postamble.checksum = sys_cpu_to_be16(iim4623x_calc_checksum(out_buf));

	/* Copy postamble to buffer */
	memcpy(out_buf_head, &postamble, sizeof(postamble));
	out_buf_head += sizeof(postamble);

	/* Zero pad to adhere to minimum tx length if necessary */
	if (preamble.length < IIM4623X_MIN_TX_LEN) {
		memset(out_buf_head, 0x00, IIM4623X_MIN_TX_LEN - preamble.length);
		out_buf_head += (IIM4623X_MIN_TX_LEN - preamble.length);
	}

	return (out_buf_head - out_buf);
}

static int iim4623x_check_ack(uint8_t *buf)
{
	struct iim4623x_pck_resp *packet = (struct iim4623x_pck_resp *)buf;
	struct iim4623x_pck_postamble *postamble;
	uint16_t checksum;

	if (packet->preamble.header != sys_cpu_to_be16(IIM4623X_PCK_HEADER_RX)) {
		LOG_ERR("Invalid packet header: 0x%04X", sys_be16_to_cpu(packet->preamble.header));
		return -EIO;
	}

	if (packet->preamble.length != IIM4623X_PCK_ACK_LEN) {
		LOG_ERR("Invalid packet length: %d", packet->preamble.length);
		return -EIO;
	}

	if (packet->ack.error_code != IIM4623X_EC_ACK) {
		LOG_ERR("ACK error code: 0x%02X", packet->ack.error_code);
		return -EIO;
	}

	postamble = IIM4623X_GET_POSTAMBLE(buf);
	checksum = iim4623x_calc_checksum(buf);
	if (checksum != sys_be16_to_cpu(postamble->checksum)) {
		LOG_ERR("Bad checksum, exp: 0x%.4x, got: 0x%.4x", checksum,
			sys_be16_to_cpu(postamble->checksum));
		return -EIO;
	}

	return 0;
}

static int iim4623x_read_reg(const struct device *dev, uint8_t page, uint8_t reg, uint8_t *buf,
			     size_t len)
{
	struct iim4623x_data *data = dev->data;
	struct iim4623x_pck_resp *packet;
	struct iim4623x_pck_postamble *reply_postamble;
	uint8_t cmd[] = {
		0x00 /* reserved */,
		len,
		reg,
		page,
	};
	uint16_t checksum;
	int ret;

	ret = iim4623x_prepare_cmd(dev, IIM4623X_CMD_READ_USER_REGISTER, cmd, ARRAY_SIZE(cmd));
	if (ret < 0) {
		LOG_ERR("Preparing cmd, ret: %d", ret);
		return ret;
	}

	ret = iim4623x_bus_write_then_read(dev, data->trx_buf, ret, data->trx_buf,
					   IIM4623X_READ_REG_RESP_LEN(len));
	if (ret) {
		LOG_ERR("Sending read user register command, ret: %d", ret);
		return ret;
	}

	/* Parse reply */
	packet = (struct iim4623x_pck_resp *)data->trx_buf;

	if (sys_be16_to_cpu(packet->preamble.header) != IIM4623X_PCK_HEADER_RX) {
		LOG_ERR("Bad reply header");
		return -ENODEV;
	}

	if (packet->preamble.type != IIM4623X_CMD_READ_USER_REGISTER) {
		LOG_ERR("Bad reply cmd type, exp: 0x%.2x, got: 0x%.2x",
			IIM4623X_CMD_READ_USER_REGISTER, packet->preamble.type);
		return -EIO;
	}

	if ((packet->read_user_reg.error_code & packet->read_user_reg.error_mask) !=
	    IIM4623X_EC_ACK) {
		LOG_ERR("Reply with error, code: 0x%.2x",
			packet->read_user_reg.error_code & packet->read_user_reg.error_mask);
		return -EIO;
	}

	if (packet->read_user_reg.addr != reg) {
		LOG_ERR("Addr mismatch, reply_addr: 0x%.2x, reg: 0x%.2x",
			packet->read_user_reg.addr, reg);
		return -EIO;
	}

	if (packet->read_user_reg.read_len != len) {
		LOG_ERR("Length mismatch, read_led: 0x%.2x, len: 0x%.2x",
			packet->read_user_reg.read_len, len);
		return -EIO;
	}

	/* Locate postamble by advancing past the reply reg_val buffer */
	reply_postamble = (struct iim4623x_pck_postamble *)(packet->read_user_reg.reg_val + len);

	/* Verify checksum */
	checksum = iim4623x_calc_checksum((uint8_t *)packet);
	if (checksum != sys_be16_to_cpu(reply_postamble->checksum)) {
		LOG_ERR("Bad checksum, exp: 0x%.4x, got: 0x%.4x", checksum,
			sys_be16_to_cpu(reply_postamble->checksum));
		return -EIO;
	}

	/* Copy register contents */
	(void)memcpy(buf, packet->read_user_reg.reg_val, len);

	/**
	 * Allow iim46234 to be ready for a new command
	 * Refer to datasheet 5.3.1.4 which states 0.3ms after DRDY deasserts. It seems that it
	 * takes ~3.1us from CS deassert until DRDY deasserts, so just use a single delay of >300us
	 */
	/**
	 * TODO: it would be great if the delay could be scheduled to block the rtio context from
	 * executing SQEs without also having to block the current thread
	 */
	k_usleep(400);

	return 0;
}

static int iim4623x_read_cfg_reg(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
	return iim4623x_read_reg(dev, IIM4623X_PAGE_CFG, reg, buf, len);
}

static int iim4623x_read_data_reg(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
	return iim4623x_read_reg(dev, IIM4623X_PAGE_SENSOR_DATA, reg, buf, len);
}

static int iim4623x_write_reg(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
	struct iim4623x_data *data = dev->data;
	/* Allocate for maximum write size */
	uint8_t cmd[12] = {
		0x00 /* reserved */,
		len,
		reg,
		IIM4623X_PAGE_CFG /* All regs in IIM4623X_PAGE_SENSOR_DATA are read-only */,
	};
	int ret;

	if (len > 8) {
		LOG_ERR("Write length too big");
		return -EINVAL;
	}

	/* Add the user data to the cmd */
	(void)memcpy(&cmd[4], buf, len);

	ret = iim4623x_prepare_cmd(dev, IIM4623X_CMD_WRITE_USER_REGISTER, cmd, len + 4);
	if (ret < 0) {
		LOG_ERR("Preparing write user register command, ret: %d", ret);
		return ret;
	}

	ret = iim4623x_bus_write_then_read(dev, data->trx_buf, ret, data->trx_buf,
					   IIM4623X_PCK_ACK_LEN);
	if (ret) {
		LOG_ERR("Sending write user register command, ret: %d", ret);
		return ret;
	}

	ret = iim4623x_check_ack(data->trx_buf);
	if (ret) {
		LOG_ERR("Checking ack, ret: %d", ret);
		return ret;
	}

	/**
	 * Allow iim46234 to be ready for a new command
	 * Refer to datasheet 5.3.1.4 which states 0.3ms after DRDY deasserts. It seems that it
	 * takes ~3.1us from CS deassert until DRDY deasserts, so just use a single delay of >300us
	 */
	/**
	 * TODO: it would be great if the delay could be scheduled to block the rtio context from
	 * executing SQEs without also having to block the current thread
	 */
	k_usleep(400);

	return 0;
}

static void iim4623x_irq_handler(const struct device *port, struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{
	struct iim4623x_data *data = CONTAINER_OF(cb, struct iim4623x_data, int_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (data->await_sqe) {
		rtio_sqe_signal(data->await_sqe);
		data->await_sqe = NULL;
#ifdef CONFIG_IIM4623X_STREAM
	} else if (data->stream.iodev_sqe) {
		iim4623x_stream_event(data->dev);
#endif
	} else {
		LOG_ERR("Spurious interrupt");
	}
}

static int iim4623x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct iim4623x_data *data = dev->data;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		return -ENOTSUP;
	};

	ret = iim4623x_read_data_reg(dev, IIM4623X_REG_SENSOR_STATUS,
				     (uint8_t *)&data->edata.payload,
				     sizeof(struct iim4623x_pck_strm_payload));
	if (ret) {
		LOG_ERR("Fetching sample, ret: %d", ret);
		return ret;
	}

	/* Convert wire endianness to cpu */
	iim4623x_payload_be_to_cpu(&data->edata.payload);

	return 0;
}

static inline void iim4623x_accel_ms(float in, struct sensor_value *out)
{
	sensor_ug_to_ms2((in * 1000000), out);
}

static inline void iim4623x_gyro_rads(float in, struct sensor_value *out)
{
	sensor_10udegrees_to_rad((in * 100000), out);
}

static int iim4623x_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct iim4623x_data *data = dev->data;
	int ret = 0;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		iim4623x_accel_ms(data->edata.payload.accel.x, val);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		iim4623x_accel_ms(data->edata.payload.accel.y, val);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		iim4623x_accel_ms(data->edata.payload.accel.z, val);
		break;
	case SENSOR_CHAN_GYRO_X:
		iim4623x_gyro_rads(data->edata.payload.gyro.x, val);
		break;
	case SENSOR_CHAN_GYRO_Y:
		iim4623x_gyro_rads(data->edata.payload.gyro.y, val);
		break;
	case SENSOR_CHAN_GYRO_Z:
		iim4623x_gyro_rads(data->edata.payload.gyro.z, val);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		ret = sensor_value_from_float(val, data->edata.payload.temp.val);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		iim4623x_accel_ms(data->edata.payload.accel.x, &val[0]);
		iim4623x_accel_ms(data->edata.payload.accel.y, &val[1]);
		iim4623x_accel_ms(data->edata.payload.accel.z, &val[2]);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		iim4623x_gyro_rads(data->edata.payload.gyro.x, &val[0]);
		iim4623x_gyro_rads(data->edata.payload.gyro.y, &val[1]);
		iim4623x_gyro_rads(data->edata.payload.gyro.z, &val[2]);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

#ifdef CONFIG_SENSOR_ASYNC_API
static void iim4623x_complete_one_shot(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	const struct device *dev = arg;
	struct iim4623x_data *data = dev->data;
	struct iim4623x_pck_resp *packet = (struct iim4623x_pck_resp *)data->trx_buf;
	struct iim4623x_pck_postamble *postamble;
	uint16_t checksum;
	const uint32_t min_buf_len = sizeof(struct iim4623x_encoded_data);
	struct iim4623x_encoded_data *edata;
	uint32_t buf_len;
	uint8_t *buf;
	int ret;

	/* Check reply */
	if (sys_be16_to_cpu(packet->preamble.header) != IIM4623X_PCK_HEADER_RX) {
		LOG_ERR("Bad reply header");
		ret = -EIO;
		goto out;
	}

	if (packet->preamble.type != IIM4623X_CMD_READ_USER_REGISTER) {
		LOG_ERR("Bad reply cmd type, exp: 0x%.2x, got: 0x%.2x",
			IIM4623X_CMD_READ_USER_REGISTER, packet->preamble.type);
		ret = -EIO;
		goto out;
	}

	if (packet->read_user_reg.addr != IIM4623X_REG_SENSOR_STATUS) {
		LOG_ERR("Addr mismatch, reply_addr: 0x%.2x, reg: 0x%.2x",
			packet->read_user_reg.addr, IIM4623X_REG_SENSOR_STATUS);
		ret = -EIO;
		goto out;
	}

	if (packet->read_user_reg.read_len != sizeof(struct iim4623x_pck_strm_payload)) {
		LOG_ERR("Length mismatch, read_led: 0x%.2x, len: 0x%.2x",
			packet->read_user_reg.read_len, sizeof(struct iim4623x_pck_strm_payload));
		ret = -EIO;
		goto out;
	}

	/* Locate postamble by advancing past the reply reg_val buffer */
	postamble = IIM4623X_GET_POSTAMBLE(packet);

	/* Verify checksum */
	checksum = iim4623x_calc_checksum((uint8_t *)packet);
	if (checksum != sys_be16_to_cpu(postamble->checksum)) {
		LOG_ERR("Bad checksum, exp: 0x%.4x, got: 0x%.4x", checksum,
			sys_be16_to_cpu(postamble->checksum));
		ret = -EIO;
		goto out;
	}

	/* Copy register contents */
	ret = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (ret || !buf || buf_len < min_buf_len) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		goto out;
	}

	edata = (struct iim4623x_encoded_data *)buf;

	ret = iim4623x_encode(dev, edata);
	if (ret) {
		LOG_ERR("Failed encode one-shot, ret: %d", ret);
		goto out;
	}

	(void)memcpy(&edata->payload, packet->read_user_reg.reg_val,
		     sizeof(struct iim4623x_pck_strm_payload));

	/* Convert wire endianness to cpu */
	iim4623x_payload_be_to_cpu(&edata->payload);

	edata->header.data_ready = true;

out:
	atomic_cas(&data->busy, 1, 0);

	ret = rtio_flush_completion_queue(ctx);
	if (ret) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void iim4623x_oneshot_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct iim4623x_data *data = dev->data;
	struct rtio_sqe *comp_sqe;
	/* TODO: read_len depends on enabled channels, just use worst case for simplicity */
	const uint8_t read_len = sizeof(struct iim4623x_pck_strm_payload);
	uint8_t cmd[] = {
		0x00 /* reserved */,
		read_len,
		IIM4623X_REG_SENSOR_STATUS,
		IIM4623X_PAGE_SENSOR_DATA,
	};
	int ret;

	/**
	 * TODO: this is actually kind of a bad idea since _if_ any of the SQEs are cancelled or
	 * fail otherwise, the completion callback won't run and `busy` will be stuck forever. Use
	 * something like: https://github.com/zephyrproject-rtos/zephyr/pull/93227 to improve error
	 * handling.
	 * Note that this is just one place in the driver where this is a problem.
	 */
	if (!atomic_cas(&data->busy, 0, 1)) {
		LOG_ERR("Submit oneshot busy");
		ret = -EBUSY;
		goto err_out;
	}

	ret = iim4623x_prepare_cmd(dev, IIM4623X_CMD_READ_USER_REGISTER, cmd, ARRAY_SIZE(cmd));
	if (ret < 0) {
		LOG_ERR("Preparing cmd, ret: %d", ret);
		goto err_out;
	}

	ret = iim4623x_bus_prep_write_read(dev, data->trx_buf, ret, data->trx_buf,
					   IIM4623X_READ_REG_RESP_LEN(read_len), &comp_sqe);
	if (ret < 0) {
		LOG_ERR("Prepping read user register command, ret: %d", ret);
		goto err_out;
	}

	rtio_sqe_prep_callback_no_cqe(comp_sqe, iim4623x_complete_one_shot, (void *)dev, iodev_sqe);

	rtio_submit(data->rtio.ctx, 0);
	return;

err_out:
	rtio_iodev_sqe_err(iodev_sqe, ret);
}

static void iim4623x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		iim4623x_oneshot_submit(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_IIM4623X_STREAM)) {
		iim4623x_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
#endif /* #ifdef CONFIG_SENSOR_ASYNC_API */

static DEVICE_API(sensor, iim4623x_api) = {
	.sample_fetch = iim4623x_sample_fetch,
	.channel_get = iim4623x_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = iim4623x_submit,
	.get_decoder = iim4623x_get_decoder,
#endif /* #ifdef CONFIG_SENSOR_ASYNC_API */
};

static int iim4623x_init(const struct device *dev)
{
	const struct iim4623x_config *config = dev->config;
	struct iim4623x_data *data = dev->data;
	uint8_t chip_id = 0;
	uint8_t tmp;
	int ret;

	data->dev = dev;

	if (!spi_is_ready_iodev(data->rtio.iodev)) {
		LOG_ERR("Spi iodev not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Configuring interrupt GPIO, ret: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->int_cb, iim4623x_irq_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_cb);
	if (ret) {
		LOG_ERR("Adding interrupt callback, ret: %d", ret);
		return ret;
	}

	/**
	 * Datasheet has a vague mention of reset pulse width down to 1us but specifies that the
	 * value is based off simulations. In addition it mentions some power-on reset conditions at
	 * 10ms which may be tied to supply ramping.
	 *
	 * Just assert reset for 10ms to be on the safe side in cases where the supplies are still
	 * ramping up.
	 */
	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		LOG_ERR("Configuring reset GPIO, ret: %d", ret);
		return ret;
	}

	k_msleep(10);

	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret) {
		LOG_ERR("Deasserting reset, ret: %d", ret);
		return ret;
	}

	/* Wait for device registers to be available, datasheet specifies up to 200ms */
	k_msleep(200);

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Configuring interrupt, ret: %d", ret);
		return ret;
	}

	/* Check chip identifier */
	ret = iim4623x_read_cfg_reg(dev, IIM4623X_REG_WHO_AM_I, &chip_id, 1);
	switch (chip_id) {
	case IIM4623X_WHO_AM_I_46230:
	case IIM4623X_WHO_AM_I_46234:
		break;
	default:
		LOG_ERR("Failed to identify iim4623x, ret: %d", ret);
		return -ENODEV;
	}

	/* Synchronize DT configuration to the chip */

	/* Write LSB of the ODR divider register */
	ret = iim4623x_write_reg(dev, IIM4623X_REG_SAMPLE_RATE_DIV + 1, &config->odr_div, 1);
	if (ret) {
		LOG_ERR("Failed to set ODR, ret: %d", ret);
		return ret;
	};

	/* Write accelerometer and gyroscope full-scale selection */
	tmp = data->edata.header.accel_fs << IIM4623X_ACCEL_CFG_SHIFT;
	ret = iim4623x_write_reg(dev, IIM4623X_REG_ACCEL_CFG, &tmp, 1);
	if (ret) {
		LOG_ERR("Failed to set accel_fs, ret: %d", ret);
		return ret;
	};

	tmp = data->edata.header.gyro_fs << IIM4623X_GYRO_CFG_SHIFT;
	ret = iim4623x_write_reg(dev, IIM4623X_REG_GYRO_CFG, &tmp, 1);
	if (ret) {
		LOG_ERR("Failed to set gyro_fs, ret: %d", ret);
		return ret;
	};

	/* Write accelerometer and gyroscope bandwidth selection */
	tmp = IIM4623X_BW_CFG_PACK(data->edata.header.accel_bw, data->edata.header.gyro_bw);
	ret = iim4623x_write_reg(dev, IIM4623X_REG_BW_CFG, &tmp, 1);
	if (ret) {
		LOG_ERR("Failed to set accel_fs, ret: %d", ret);
		return ret;
	};

	return 0;
}

/* Helper macros to convert the human readable full-scale settings into bit masks */
#define IIM4623X_DT_ACCEL_FS(inst)                                                                 \
	CONCAT(IIM4623X_ACCEL_CFG_FS_, DT_INST_PROP_OR(inst, accel_fs, 8))

#define IIM4623X_DT_GYRO_FS(inst) CONCAT(IIM4623X_GYRO_CFG_FS_, DT_INST_PROP_OR(inst, gyro_fs, 480))

/* Device init macro */
#define IIM4623X_INIT(inst)                                                                        \
	RTIO_DEFINE(iim4623x_rtio_ctx_##inst, 8, 8);                                               \
	SPI_DT_IODEV_DEFINE(iim4623x_bus_##inst, DT_DRV_INST(inst),                                \
			    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0U);          \
                                                                                                   \
	static const struct iim4623x_config iim4623x_cfg_##inst = {                                \
		.reset_gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), reset_gpios),                    \
		.int_gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), int_gpios),                        \
		.odr_div = (1000 / DT_INST_PROP_OR(inst, odr, 1000)),                              \
	};                                                                                         \
                                                                                                   \
	static struct iim4623x_data iim4623x_data_##inst = {                                       \
		.rtio =                                                                            \
			{                                                                          \
				.iodev = &iim4623x_bus_##inst,                                     \
				.ctx = &iim4623x_rtio_ctx_##inst,                                  \
			},                                                                         \
		.trx_buf = {0},                                                                    \
		.edata.header =                                                                    \
			{                                                                          \
				.accel_fs = IIM4623X_DT_ACCEL_FS(inst),                            \
				.gyro_fs = IIM4623X_DT_GYRO_FS(inst),                              \
				.accel_bw = DT_INST_PROP_OR(inst, accel_bw, 0),                    \
				.gyro_bw = DT_INST_PROP_OR(inst, gyro_bw, 0),                      \
				.chans = {.gyro = 1, .accel = 1, .temp = 1},                       \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, iim4623x_init, NULL, &iim4623x_data_##inst,             \
				     &iim4623x_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &iim4623x_api);

/* Run initialization for all compatible strings */
#define DT_DRV_COMPAT invensense_iim46234
DT_INST_FOREACH_STATUS_OKAY(IIM4623X_INIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT invensense_iim46230
DT_INST_FOREACH_STATUS_OKAY(IIM4623X_INIT)
#undef DT_DRV_COMPAT
