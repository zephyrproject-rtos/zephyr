/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>

#include "max30101.h"

LOG_MODULE_REGISTER(MAX30101_STREAM, CONFIG_SENSOR_LOG_LEVEL);

void max30101_drop_data(struct max30101_data *data, struct max30101_encoded_data *edata, uint8_t keep)
{
	int index = 0;
	for (int slot_chan = 0; slot_chan < MAX30101_MAX_NUM_CHANNELS; slot_chan++) {
		for (int j = 0; j < data->num_channels[index]; j++) {
			if (!(keep & BIT(data->map[index][j]))) {
				LOG_DBG("Drop channel[%d](%d): [%d]", index, j, data->map[index][j]);
				switch (index)
				{
				case MAX30101_LED_CHANNEL_RED:
					edata->has_red--;
					break;

				case MAX30101_LED_CHANNEL_IR:
					edata->has_ir--;
					break;
				
				case MAX30101_LED_CHANNEL_GREEN:
					edata->has_green--;
					break;
				
				default:
					LOG_ERR("Unsupported channel index %d", index);
					break;
				}
			}
		}
		index++;
	}
}

void max30101_stream_config(const struct device *dev, struct sensor_stream_trigger *trigger, uint8_t *include, uint8_t *drop, uint8_t *nop)
{
	struct max30101_data *data = dev->data;
	uint8_t led_chan;

	if (trigger->chan_spec.chan_type != SENSOR_CHAN_ALL) {
		switch (trigger->chan_spec.chan_type)
		{
		case SENSOR_CHAN_RED:
			led_chan = MAX30101_LED_CHANNEL_RED;
			break;

		case SENSOR_CHAN_IR:
			led_chan = MAX30101_LED_CHANNEL_IR;
			break;

		case SENSOR_CHAN_GREEN:
			led_chan = MAX30101_LED_CHANNEL_GREEN;
			break;

#if CONFIG_MAX30101_DIE_TEMPERATURE
		case SENSOR_CHAN_DIE_TEMP:
			if (trigger->opt == SENSOR_STREAM_DATA_INCLUDE) {
				*include = 0b1000;
			}

			if (trigger->opt == SENSOR_STREAM_DATA_DROP) {
				LOG_WRN("DROP option not supported on DIE_TEMPERATURE channel");
				*nop = 0b1000;
			}

			if (trigger->opt == SENSOR_STREAM_DATA_NOP) {
				*nop = 0b1000;
			}
			return;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
		
		default:
			LOG_ERR("Unsupported channel type: %d", trigger->chan_spec.chan_type);
			return;
		}

		if (trigger->opt == SENSOR_STREAM_DATA_INCLUDE) {
			*include = 1 << data->map[led_chan][trigger->chan_spec.chan_idx];
		}

		if (trigger->opt == SENSOR_STREAM_DATA_DROP) {
			*drop = 1 << data->map[led_chan][trigger->chan_spec.chan_idx];
		}

		if (trigger->opt == SENSOR_STREAM_DATA_NOP) {
			*nop = 1 << data->map[led_chan][trigger->chan_spec.chan_idx];
		}
	} else {
#if CONFIG_MAX30101_DIE_TEMPERATURE
		*include = (trigger->opt == SENSOR_STREAM_DATA_INCLUDE) ? 0b1111 : 0;
		*nop = (trigger->opt == SENSOR_STREAM_DATA_NOP) ? 0b1111 : 0;
#else
		*include = (trigger->opt == SENSOR_STREAM_DATA_INCLUDE) ? 0b111 : 0;
		*nop = (trigger->opt == SENSOR_STREAM_DATA_NOP) ? 0b111 : 0;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
		*drop = (trigger->opt == SENSOR_STREAM_DATA_DROP) ? 0b111 : 0;
	}
}

void max30101_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct max30101_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct max30101_stream_config stream_cfg = {0};
	uint8_t include = 0, drop = 0, nop = 0;

//	LOG_WRN("max30101_submit_stream");
	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			stream_cfg.irq_data_rdy |= 1;
			max30101_stream_config(dev, &cfg->triggers[i], &include, &drop, &nop);
			stream_cfg.data_rdy_incl |= include;
			stream_cfg.data_rdy_drop |= drop;
			stream_cfg.data_rdy_nop |= nop;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
//			LOG_DBG("(%d) MAX30101 WATERMARK trigger [%d](%d)", i, cfg->triggers[i].chan_spec.chan_type, cfg->triggers[i].chan_spec.chan_idx);
			stream_cfg.irq_watermark |= 1;
			max30101_stream_config(dev, &cfg->triggers[i], &include, &drop, &nop);
			stream_cfg.watermark_incl |= include;
			stream_cfg.watermark_drop |= drop;
			stream_cfg.watermark_nop |= nop;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_OVERFLOW) {
			if (cfg->triggers[i].opt != SENSOR_STREAM_DATA_NOP) {
				LOG_ERR("MAX30101 OVERFLOW trigger only support SENSOR_STREAM_DATA_NOP");
				return;
				LOG_ERR("MAX30101 OVERFLOW trigger only support SENSOR_STREAM_DATA_NOP");
				return;
			}
			stream_cfg.irq_overflow = 1;
		} else {
			LOG_ERR("(%d) MAX30101 trigger not supported", i);
		}
	}

	/* if any change in trig_cfg for DRDY triggers */
	if (stream_cfg.irq_data_rdy != data->stream_cfg.irq_data_rdy) {
		if ((stream_cfg.data_rdy_nop & 0b111) && ((stream_cfg.data_rdy_incl || stream_cfg.data_rdy_drop) & 0b111)) {
			LOG_ERR("[DATA READY]IRG: NOP cannot be used with INCLUDE or DROP");
			return;
		}

		/* enable/disable drdy events */
		data->stream_cfg.irq_data_rdy = stream_cfg.irq_data_rdy;
		data->stream_cfg.data_rdy_incl = stream_cfg.data_rdy_incl;
		data->stream_cfg.data_rdy_drop = stream_cfg.data_rdy_drop;
		data->stream_cfg.data_rdy_nop = stream_cfg.data_rdy_nop;
		uint8_t enable = (stream_cfg.data_rdy_incl | stream_cfg.data_rdy_drop)? 0xFF : 0;
		LOG_DBG("[DATA READY]IRG: trig_cfg changed [0->%d]:[0x%02X][0x%02X][0x%02X]", enable,
			stream_cfg.data_rdy_incl, stream_cfg.data_rdy_drop, stream_cfg.data_rdy_nop);

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN1, MAX30101_INT_PPG_MASK, enable)) {
			LOG_ERR("Data ready config_interruption failed");
			return;
		}

#if CONFIG_MAX30101_DIE_TEMPERATURE
		if ((stream_cfg.data_rdy_nop & 0b1000) && (stream_cfg.data_rdy_incl & 0b1000)) {
			LOG_ERR("[DATA READY]DIE TEMP: NOP cannot be used with INCLUDE or DROP");
			return;
		}

		enable = ((stream_cfg.data_rdy_incl | stream_cfg.data_rdy_nop) & 0b1000) ? 0xFF : 0;
		LOG_DBG("[DATA READY]DIE TEMP: trig_cfg changed [0->%d]:[0x%02X][0x%02X][0x%02X]", enable,
			stream_cfg.data_rdy_incl, stream_cfg.data_rdy_drop, stream_cfg.data_rdy_nop);

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN2, MAX30101_INT_TEMP_MASK, enable)) {
			LOG_ERR("Die temperature config_interruption failed");
			return;
		}

		/* Start die temperature acquisition */
		if (max30101_start_temperature_measurement(dev)) {
			LOG_ERR("Could not start die temperature acquisition");
			return;
		}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	}

	/* if any change in trig_cfg for FIFO triggers */
	if (stream_cfg.irq_watermark != data->stream_cfg.irq_watermark) {
		if (stream_cfg.watermark_nop && (stream_cfg.watermark_incl || stream_cfg.watermark_drop)) {
			LOG_ERR("[FIFO]: NOP cannot be used with INCLUDE or DROP");
			return;
		}

		/* enable/disable the FIFO */
		data->stream_cfg.irq_watermark = stream_cfg.irq_watermark;
		data->stream_cfg.watermark_incl = stream_cfg.watermark_incl;
		data->stream_cfg.watermark_drop = stream_cfg.watermark_drop;
		data->stream_cfg.watermark_nop = stream_cfg.watermark_nop;
		uint8_t enable = (data->stream_cfg.irq_watermark) ? 0xFF : 0;

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN1, MAX30101_INT_FULL_MASK, enable)) {
			LOG_ERR("FIFO config_interruption failed");
			return;
		}
	}

	/* if any change in trig_cfg for OVERFLOW triggers */
	if (stream_cfg.irq_overflow != data->stream_cfg.irq_overflow) {
		/* enable/disable the overlfow events */
		data->stream_cfg.irq_overflow = stream_cfg.irq_overflow;
		uint8_t enable = (data->stream_cfg.irq_overflow) ? 0xFF : 0;

		if (max30101_config_interruption(dev, MAX30101_REG_INT_EN1, MAX30101_INT_ALC_OVF_MASK, enable)) {
			LOG_ERR("Overflow config_interruption failed");
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
		LOG_ERR("Complete operation failed");
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

//	LOG_INF("edata Timestamp: [%lld]", edata->header.timestamp);
//	LOG_DBG("FIFO: [0x%02X][0x%02X][0x%02X](%d)", edata->header.fifo_info[0],
//		edata->header.fifo_info[1], edata->header.fifo_info[2], count);

	edata->header.reading_count = count;

	/* Check if the requested channels are supported */
	const struct sensor_chan_spec all_channel[] = {{.chan_type=SENSOR_CHAN_ALL, .chan_idx=0}};
	uint8_t data_channel = max30101_encode_channels(data, edata, all_channel, ARRAY_SIZE(all_channel));

	/* Drop data */
	uint8_t keep = (data->stream_cfg.watermark_incl & ~data->stream_cfg.watermark_drop);
	if (keep != 0b111) {
		max30101_drop_data(data, edata, keep);
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
	bool rdy_event = false, temp_event = false, watermark_event = false, overflow_event = false;
	uint8_t proc_data_rdy = (data->stream_cfg.data_rdy_incl | data->stream_cfg.data_rdy_drop) & ~data->stream_cfg.data_rdy_nop;
//	LOG_INF("Status: [0x%02X][0x%02X]", data->status[0], data->status[1]);

	if (data->stream_cfg.irq_data_rdy) {
		rdy_event = (proc_data_rdy != 0) && ((data->status[0] & MAX30101_INT_PPG_MASK) != 0);
#if CONFIG_MAX30101_DIE_TEMPERATURE
		temp_event = !!(data->status[1] & MAX30101_INT_TEMP_MASK);
		data->temp_available |= temp_event;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	}

	if (data->stream_cfg.irq_watermark) {
		watermark_event = (((data->stream_cfg.watermark_incl | data->stream_cfg.watermark_drop) & ~data->stream_cfg.watermark_nop) != 0) && ((data->status[0] & MAX30101_INT_FULL_MASK) != 0);
	}

	if (data->stream_cfg.irq_overflow) {
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
#if CONFIG_MAX30101_DIE_TEMPERATURE
	edata->has_data_rdy |= temp_event;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

//	LOG_ERR("Flags: (%d)[%d][%d][%d]", temp_event, rdy_event, watermark_event, overflow_event);
	/* If we're not interested in the data, just complete the request */
	if (!rdy_event && !watermark_event) {
//		LOG_ERR("max30101_read_status_cb NOP");
		if (temp_event && (data->stream_cfg.data_rdy_incl & 0b1000)) {
			return;
		}

		/* complete request with ok */
		data->streaming_sqe = NULL;
		rtio_iodev_sqe_ok(sqe->userdata, 0);

		return;
	}

	/* Store sensor pointer */
	edata->sensor = dev;

	/* Check if the requested channels are supported */
	const struct sensor_chan_spec all_channel[] = {{.chan_type=SENSOR_CHAN_ALL, .chan_idx=0}};
	uint8_t data_channel = max30101_encode_channels(data, edata, all_channel, ARRAY_SIZE(all_channel));

	struct rtio_sqe *write_addr;
	struct rtio_sqe *read_reg;
	struct rtio_sqe *complete_op;
	uint32_t acquirable = rtio_sqe_acquirable(data->rtio_ctx);

	/* WATERMARK event is higher priority than DATA_RDY */
	if (watermark_event) {
#if CONFIG_MAX30101_DIE_TEMPERATURE
		if (data->temp_available && (proc_data_rdy & 0b1000) && edata->has_temp) {
			data->temp_available = false;
			edata->header.reading_count = 1;
			uint8_t reg_addr = MAX30101_REG_TINT;
			uint8_t *read_data = (uint8_t *)&edata->die_temp;
			uint8_t enable_buf[] = {MAX30101_REG_TEMP_CFG, 1};

			if (acquirable < 3) {
				LOG_ERR("TEMP Not enough sqes available TEMP: [%d/%d]", 3, acquirable);
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

//			LOG_ERR("max30101_read_status_cb TEMP");
		} else {
			edata->has_temp = 0;
		}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

		if (acquirable < 3) {
			LOG_ERR("FIFO Not enough sqes available: [%d/%d]", 3, acquirable);
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);

			if (!!edata->header.reading_count && edata->has_temp) {
				rtio_sqe_drop_all(data->rtio_ctx);
			}

			return;
		}

		uint8_t reg_addr = MAX30101_REG_FIFO_WR;
		uint8_t *read_data = (uint8_t *)&edata->header.fifo_info;

		write_addr = rtio_sqe_acquire(data->rtio_ctx);
		read_reg = rtio_sqe_acquire(data->rtio_ctx);
		// Is check acquired not NULL, necessary ?
		acquirable -= 2;

		/* Reset data flags fir RED, IR, GREEN */
		edata->has_red = 0;
		edata->has_ir = 0;
		edata->has_green = 0;

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

		return;
	}

	if (rdy_event) {
		edata->header.reading_count = 1;

		if (!!data_channel && (proc_data_rdy & 0b111)) {
			if (acquirable < 2) {
				LOG_ERR("DATA Not enough sqes available TEMP: [%d/%d]", 2, acquirable);
				rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
				return;
			}

			uint8_t reg_addr = MAX30101_REG_FIFO_DATA;
			uint8_t *read_data = (uint8_t *)&edata->reading[0].raw;

			/* Drop data */
			uint8_t keep = data->stream_cfg.data_rdy_incl & ~data->stream_cfg.data_rdy_drop;
			if (keep != 0b111) {
				max30101_drop_data(data, edata, keep);
			}

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
		} else {
			edata->header.reading_count = 0;
			edata->has_red = 0;
			edata->has_ir = 0;
			edata->has_green = 0;
		}

#if CONFIG_MAX30101_DIE_TEMPERATURE
		if (data->temp_available && (proc_data_rdy & 0b1000) && (edata->has_temp)) {
			data->temp_available = false;
			edata->header.reading_count = 1;
			uint8_t reg_addr = MAX30101_REG_TINT;
			uint8_t *read_data = (uint8_t *)&edata->die_temp;
			uint8_t enable_buf[] = {MAX30101_REG_TEMP_CFG, 1};

			if (acquirable < 3) {
				LOG_ERR("TEMP Not enough sqes available: [%d/%d]", 3, acquirable);
				rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
				rtio_sqe_drop_all(data->rtio_ctx);

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

//			LOG_ERR("max30101_read_status_cb TEMP");
		} else {
			edata->has_temp = 0;
		}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

		complete_op = rtio_sqe_acquire(data->rtio_ctx);
		if (complete_op == NULL) {
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
			rtio_sqe_drop_all(data->rtio_ctx);

			return;
		}

//		LOG_ERR("max30101_read_status_cb complete_op");
		rtio_sqe_prep_callback_no_cqe(complete_op, max30101_complete_op_cb, (void *)dev, data->streaming_sqe);
		rtio_submit(data->rtio_ctx, 0);

		return;
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
