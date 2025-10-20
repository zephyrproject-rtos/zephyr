/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>

#include "max30101.h"

LOG_MODULE_REGISTER(MAX30101_STREAM, CONFIG_SENSOR_LOG_LEVEL);

void max30101_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct max30101_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct max30101_stream_config stream_cfg = {0};

//	LOG_WRN("max30101_submit_stream");
	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			stream_cfg.irq_data_rdy = 1;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			stream_cfg.irq_watermark = (cfg->triggers[i].opt == SENSOR_STREAM_DATA_INCLUDE)? 3 : 1;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_OVERFLOW) {
			if (cfg->triggers[i].opt != SENSOR_STREAM_DATA_NOP) {
				LOG_WRN("MAX30101 OVERFLOW trigger only support SENSOR_STREAM_DATA_NOP");
				stream_cfg.irq_overflow = 0;
			}
			stream_cfg.irq_overflow = 1;
		} else {
			LOG_ERR("(%d) MAX30101 trigger not supported", i);
		}
	}

	/* if any change in trig_cfg for DRDY triggers */
	if (stream_cfg.irq_data_rdy != data->stream_cfg.irq_data_rdy) {
		/* enable/disable drdy events */
		data->stream_cfg.irq_data_rdy = stream_cfg.irq_data_rdy;
		uint8_t enable = (data->stream_cfg.irq_data_rdy) ? 0xFF : 0;

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN1, MAX30101_INT_PPG_MASK, enable)) {
			return;
		}

#if CONFIG_MAX30101_DIE_TEMPERATURE
		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN2, MAX30101_INT_TEMP_MASK, enable)) {
			return;
		}

		/* Start die temperature acquisition */
		if (max30101_start_temperature_measurement(dev)) {
			return;
		}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	}

	/* if any change in trig_cfg for FIFO triggers */
	if (stream_cfg.irq_watermark != data->stream_cfg.irq_watermark) {
		/* enable/disable the FIFO */
		data->stream_cfg.irq_watermark = stream_cfg.irq_watermark;
		uint8_t enable = (data->stream_cfg.irq_watermark) ? 0xFF : 0;

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN1, MAX30101_INT_FULL_MASK, enable)) {
			return;
		}
	}

	/* if any change in trig_cfg for OVERFLOW triggers */
	if (stream_cfg.irq_overflow != data->stream_cfg.irq_overflow) {
		/* enable/disable the overlfow events */
		data->stream_cfg.irq_overflow = stream_cfg.irq_overflow;
		uint8_t enable = (data->stream_cfg.irq_overflow) ? 0xFF : 0;

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN1, MAX30101_INT_ALC_OVF_MASK, enable)) {
			return;
		}
	}

//	LOG_ERR("max30101_submit_stream");
	data->streaming_sqe = iodev_sqe;
}

/*
 * Called by bus driver to complete the sqe.
 */
static void max30101_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	const struct device *dev = arg;
	struct max30101_data *data = dev->data;

//	LOG_WRN("max30101_complete_op_cb");
	if (result) { /* reads failed → finish the original op with error */
        rtio_iodev_sqe_err(data->streaming_sqe, result);
        return;
    }

//	LOG_ERR("max30101_complete_op_cb");
	/*
	 * Mark operation has completed
	 */
	data->streaming_sqe = NULL;
	rtio_iodev_sqe_ok(sqe->userdata, 0);
}

/*
 * Called by bus driver to complete the MAX30101_STATUS_REG read op.
 * If drdy is active it reads data from MAX30101 internal fifo reg.
 */
static void max30101_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	const struct device *dev = arg;
	struct max30101_data *data = dev->data;
	struct sensor_read_config *read_config;

//	LOG_WRN("max30101_read_fifo_cb");
	if (result) { /* reads failed → finish the original op with error */
		LOG_ERR("FIFO read failed");
		rtio_iodev_sqe_err(data->streaming_sqe, result);
		return;
	}

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(data->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)data->streaming_sqe->sqe.iodev->data;
	__ASSERT_NO_MSG(read_config != NULL);
	__ASSERT_NO_MSG(read_config->is_streaming == true);

	struct max30101_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint8_t count;

	if (rtio_sqe_rx_buf(data->streaming_sqe, sizeof(struct max30101_decoder_header),
				sizeof(struct max30101_decoder_header), &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer read_fifo");
		rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
		data->streaming_sqe = NULL;
		return;
	}

	edata = (struct max30101_encoded_data *)buf;

	if (edata->header.fifo_info[0] == edata->header.fifo_info[2]) {
		count = 32;
	} else if (edata->header.fifo_info[0] < edata->header.fifo_info[2]) {
		count = edata->header.fifo_info[0] + (32 - edata->header.fifo_info[2]);
	} else {
		count = edata->header.fifo_info[0] - edata->header.fifo_info[2];
	}

	uint32_t req_len = sizeof(struct max30101_encoded_data) + sizeof(struct max30101_reading) * (count - 1);

	if (rtio_sqe_rx_buf(data->streaming_sqe, req_len, req_len, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get full buffer read_fifo");
		rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
		data->streaming_sqe = NULL;
		return;
	}

	// LOG_WRN("FIFO: [0x%02X][0x%02X][0x%02X](%d)", edata->header.fifo_info[0],
	// 	edata->header.fifo_info[1], edata->header.fifo_info[2], count);
	// edata = (struct max30101_encoded_data *)buf;
//	LOG_INF("edata Timestamp: [%lld]", edata->header.timestamp);
//	LOG_WRN("FIFO: [0x%02X][0x%02X][0x%02X](%d)", edata->header.fifo_info[0],
//		edata->header.fifo_info[1], edata->header.fifo_info[2], count);

	edata->header.reading_count = count;

	/* Check if the requested channels are supported */
	const struct sensor_chan_spec all_channel[] = {{.chan_type=SENSOR_CHAN_ALL, .chan_idx=0}};

	uint8_t data_channel = max30101_encode_channels(data, edata, all_channel, ARRAY_SIZE(all_channel));
	edata->has_temp = 0;

	/* Drop data */
	if (!(data->stream_cfg.irq_watermark & 0x02)) {
		edata->has_red = 0;
		edata->has_ir = 0;
		edata->has_green = 0;
	}

	uint8_t *read_data = (uint8_t *)&edata->reading[0].raw;
	struct rtio_regs fifo_regs;
	struct rtio_regs_list regs_list[] = {
		{
			MAX30101_REG_FIFO_DATA,
			read_data,
			count * max30101_sample_bytes[data_channel],
		},
	};

	fifo_regs.rtio_regs_list = regs_list;
	fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

	/*
	 * Prepare rtio enabled bus to read MAX30101_REG_INT_STS1/2 registers.
	 * Then max30101_read_status_cb callback will be invoked.
	 */
	rtio_read_regs_async(data->rtio_ctx, data->iodev, data->bus_type,
					&fifo_regs, data->streaming_sqe, dev,
					max30101_complete_op_cb);
}

/*
 * Called by bus driver to complete the MAX30101_STATUS_REG read op.
 * If drdy is active it reads data from MAX30101 internal fifo reg.
 */
static void max30101_read_status_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	const struct device *dev = arg;
	struct max30101_data *data = dev->data;
	struct rtio *rtio = data->rtio_ctx;
	struct sensor_read_config *read_config;

//	LOG_WRN("max30101_read_status_cb");
	if (result) { /* reads failed → finish the original op with error */
		LOG_ERR("Status read failed");
        rtio_iodev_sqe_err(data->streaming_sqe, result);
        return;
    }

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(data->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)data->streaming_sqe->sqe.iodev->data;
	__ASSERT_NO_MSG(read_config != NULL);
	__ASSERT_NO_MSG(read_config->is_streaming == true);

	/* parse the configuration in search for any configured trigger */
	struct sensor_stream_trigger *data_rdy = NULL, *watermark = NULL, *overflow = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			data_rdy = &read_config->triggers[i];
		} else if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			watermark = &read_config->triggers[i];
		} else if (read_config->triggers[i].trigger == SENSOR_TRIG_OVERFLOW) {
			overflow = &read_config->triggers[i];
		}
	}

	/* flush completion */
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(rtio);
		if (cqe != NULL) {
			if ((cqe->result < 0) && (res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(rtio, cqe);
		}
	} while (cqe != NULL);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		LOG_ERR("Status read failed");
		rtio_iodev_sqe_err(data->streaming_sqe, res);
		data->streaming_sqe = NULL;
		return;
	}

	struct max30101_encoded_data *edata;
	bool rdy_event = false, watermark_event = false, overflow_event = false;

	if (data_rdy != NULL) {
		rdy_event = (data_rdy->opt != SENSOR_STREAM_DATA_NOP) && ((data->status[0] & MAX30101_INT_PPG_MASK) != 0);
#if CONFIG_MAX30101_DIE_TEMPERATURE
		data->temp_available |= !!(data->status[1] & MAX30101_INT_TEMP_MASK);
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	}

	if (watermark != NULL) {
		watermark_event = (watermark->opt != SENSOR_STREAM_DATA_NOP) && ((data->status[0] & MAX30101_INT_FULL_MASK) != 0);
	}

	if (overflow != NULL) {
		overflow_event = !!(data->status[0] & MAX30101_INT_ALC_OVF_MASK);
	}

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(data->streaming_sqe, sizeof(struct max30101_encoded_data),
				sizeof(struct max30101_encoded_data), &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer read_status");
		rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
		data->streaming_sqe = NULL;
		return;
	}

	memset(buf, 0, buf_len);
	edata = (struct max30101_encoded_data *)buf;
	edata->header.timestamp = data->timestamp;
	edata->header.reading_count = 0;
	edata->has_red = 0;
	edata->has_ir = 0;
	edata->has_green = 0;
	edata->has_temp = 0;
	/* Store interruptions status */
	edata->has_data_rdy = rdy_event;
	edata->has_watermark = watermark_event;
	edata->has_overflow = overflow_event;

//	LOG_DBG("Flags: [%d][%d][%d]", rdy_event, watermark_event, overflow_event);
	/* If we're not interested in the data, just complete the request */
	if (!rdy_event && !watermark_event && !overflow_event) {
//		LOG_ERR("max30101_read_status_cb NOP");
		/* complete request with ok */
		data->streaming_sqe = NULL;
		rtio_iodev_sqe_ok(sqe->userdata, 0);

		return;
	}

	/* Store sensor pointer */
	edata->sensor = dev;

	struct rtio_sqe *write_addr;
	struct rtio_sqe *read_reg;
	struct rtio_sqe *complete_op;
	uint32_t acquirable = rtio_sqe_acquirable(data->rtio_ctx);

	if (acquirable < 3) {
		LOG_ERR("MAX30101 Not enough sqes available: [%d/%d]", 3, acquirable);
		rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
		return;
	}

	if (rdy_event) {
		edata->header.reading_count = 1;

		/* Check if the requested channels are supported */
		const struct sensor_chan_spec all_channel[] = {{.chan_type=SENSOR_CHAN_ALL, .chan_idx=0}};
		uint8_t data_channel = max30101_encode_channels(data, edata, all_channel, ARRAY_SIZE(all_channel));

		if (!!data_channel) {
			uint8_t reg_addr = MAX30101_REG_FIFO_DATA;
			uint8_t *read_data = (uint8_t *)&edata->reading[0].raw;

			/* Defined if the value should be dropped */
			bool keep = (data_rdy->opt != SENSOR_STREAM_DATA_DROP);
			edata->has_red &= keep;
			edata->has_ir &= keep;
			edata->has_green &= keep;

			write_addr = rtio_sqe_acquire(data->rtio_ctx);
			read_reg = rtio_sqe_acquire(data->rtio_ctx);
			// Is check acquired not NULL, necessary ?
			acquirable -= 2;

			rtio_sqe_prep_tiny_write(write_addr, data->iodev, RTIO_PRIO_NORM,
						&reg_addr, 1, NULL);
			write_addr->flags = RTIO_SQE_TRANSACTION;

			rtio_sqe_prep_read(read_reg, data->iodev, RTIO_PRIO_NORM, read_data,
					max30101_sample_bytes[data_channel], NULL);
			read_reg->flags = RTIO_SQE_CHAINED;
			read_reg->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;

//			LOG_ERR("max30101_read_status_cb PPG");
#if CONFIG_MAX30101_DIE_TEMPERATURE
			if (data->temp_available && (edata->has_temp)) {
				data->temp_available = false;
				reg_addr = MAX30101_REG_TINT;
				read_data = (uint8_t *)&edata->die_temp;
				uint8_t enable_buf[] = {MAX30101_REG_TEMP_CFG, 1};

				if (acquirable < 3) {
					LOG_ERR("TEMP Not enough sqes available: [%d/%d]", 3, acquirable);
					rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
					return;
				}

				write_addr = rtio_sqe_acquire(data->rtio_ctx);
				read_reg = rtio_sqe_acquire(data->rtio_ctx);
				// Is check acquired not NULL, necessary ?
				acquirable -= 2;

				rtio_sqe_prep_tiny_write(write_addr, data->iodev, RTIO_PRIO_NORM,
							&reg_addr, 1, NULL);
				write_addr->flags = RTIO_SQE_TRANSACTION;

				rtio_sqe_prep_read(read_reg, data->iodev, RTIO_PRIO_NORM, read_data,
						2, NULL);
				read_reg->flags = RTIO_SQE_CHAINED;
				read_reg->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;

				struct rtio_sqe *write_en = rtio_sqe_acquire(data->rtio_ctx);
				// Is check acquired not NULL, necessary ?
				acquirable -= 1;

				rtio_sqe_prep_tiny_write(write_en, data->iodev, RTIO_PRIO_NORM,
						enable_buf, ARRAY_SIZE(enable_buf), NULL);
				write_en->flags = RTIO_SQE_CHAINED;
				write_en->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;

//				LOG_ERR("max30101_read_status_cb TEMP");
			} else {
				edata->has_temp = 0;
			}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
		} else {
			edata->header.reading_count = 0;
			edata->has_red = 0;
			edata->has_ir = 0;
			edata->has_green = 0;
			edata->has_temp = 0;
		}

		complete_op = rtio_sqe_acquire(data->rtio_ctx);
		if (complete_op == NULL) {
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
			rtio_sqe_drop_all(data->rtio_ctx);
			return;
		}

//		LOG_ERR("max30101_read_status_cb complete_op");
		rtio_sqe_prep_callback_no_cqe(complete_op, max30101_complete_op_cb, (void *)dev, data->streaming_sqe);
		rtio_submit(data->rtio_ctx, 0);
	}

	if (watermark_event) {
		uint8_t reg_addr = MAX30101_REG_FIFO_WR;
		uint8_t *read_data = (uint8_t *)&edata->header.fifo_info;

		write_addr = rtio_sqe_acquire(data->rtio_ctx);
		read_reg = rtio_sqe_acquire(data->rtio_ctx);
		// Is check acquired not NULL, necessary ?
		acquirable -= 2;

		rtio_sqe_prep_tiny_write(write_addr, data->iodev, RTIO_PRIO_NORM,
					&reg_addr, 1, NULL);
		write_addr->flags = RTIO_SQE_TRANSACTION;

		/* Reads : MAX30101_REG_FIFO_WR, MAX30101_REG_FIFO_OVF, MAX30101_REG_FIFO_RD */
		rtio_sqe_prep_read(read_reg, data->iodev, RTIO_PRIO_NORM, read_data,
				3, NULL);
		read_reg->flags = RTIO_SQE_CHAINED;
		read_reg->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;

//		LOG_ERR("max30101_read_status_cb FIFO");
		complete_op = rtio_sqe_acquire(data->rtio_ctx);
		if (complete_op == NULL) {
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
			rtio_sqe_drop_all(data->rtio_ctx);
			return;
		}

//		LOG_ERR("max30101_read_status_cb complete_op");
		rtio_sqe_prep_callback_no_cqe(complete_op, max30101_read_fifo_cb, (void *)dev, data->streaming_sqe);
		rtio_submit(data->rtio_ctx, 0);
	}
}

/*
 * Called when one of the following trigger is active:
 *
 *     - has_data_rdy (SENSOR_TRIG_DATA_READY)
 *     - has_watermark (SENSOR_TRIG_FIFO_WATERMARK)
 *     - has_overflow (SENSOR_TRIG_FIFO_OVERFLOW)
 */
void max30101_stream_irq_handler(const struct device *dev)
{
	struct max30101_data *data = dev->data;
	uint64_t cycles;
	int rc;

//	LOG_WRN("max30101_stream_irq_handler");
	if (data->streaming_sqe == NULL) {
		LOG_WRN("streaming_sqe is NULL");
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(data->streaming_sqe, rc);
		return;
	}

	/* Get timestamp as soon as the irq is received */
	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	for (int i = 0; i < ARRAY_SIZE(data->status); i++) {
		data->status[i] = 0;
	}

	struct rtio_regs fifo_regs;
	struct rtio_regs_list regs_list[] = {
		{
			MAX30101_REG_INT_STS1,
			&data->status[0],
			1,
		},
#if CONFIG_MAX30101_DIE_TEMPERATURE
		{
			MAX30101_REG_INT_STS2,
			&data->status[1],
			1,
		}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	};

	fifo_regs.rtio_regs_list = regs_list;
	fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

//	LOG_ERR("max30101_stream_irq_handler");
	/*
	 * Prepare rtio enabled bus to read MAX30101_REG_INT_STS1/2 registers.
	 * Then max30101_read_status_cb callback will be invoked.
	 */
	rtio_read_regs_async(data->rtio_ctx, data->iodev, data->bus_type,
					&fifo_regs, data->streaming_sqe, dev,
					max30101_read_status_cb);
}
