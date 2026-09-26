/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_AD5940_STREAM

#include "ad5940.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ad5940, CONFIG_SENSOR_LOG_LEVEL);

void ad5940_stream_stop_timer(const struct device *dev)
{
	struct ad5940_data *data = dev->data;

	if (!data->timer_running) {
		return;
	}

	ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, 0x0u);
	ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON, 0x0u);
	data->timer_running = false;
}

void ad5940_submit_stream(const struct device *dev,
			  struct rtio_iodev_sqe *iodev_sqe)
{
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	uint32_t backlog = 0u;
	uint32_t intcsel0 = 0u;
	uint32_t req_samples = 0u;
	uint32_t wuptmr = 0u;
	unsigned int key = irq_lock();

	data->rtio_ctx  = iodev_sqe->r;
	data->active_sqe = iodev_sqe;
	irq_unlock(key);


	if (ad5940_reg_read(dev, AD5940_REG_INTCSEL0, &intcsel0) == 0) {
		intcsel0 |= AFEINTSRC_DATAFIFOTHRESH;
		intcsel0 &= ~(AFEINTSRC_DFTRDY | AFEINTSRC_ENDSEQ);
		ad5940_reg_write(dev, AD5940_REG_INTCSEL0, intcsel0);
	}

	ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);

	if (cfg->int_gpio.port != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		req_samples = (data->mode == AD5940_MODE_EIS) ?
			      AD5940_EIS_WORDS_PER_FRAME : 1u;


		if (ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &backlog) == 0 &&
		    FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, backlog) >=
			    req_samples) {
			ad5940_trigger_resubmit(dev);
		}
	}

	if (!data->timer_running) {
		ad5940_stream_prime(dev);

		ad5940_reg_write(dev, AD5940_REG_SEQCON, AD5940_SEQCON_SEQEN_MSK);

		ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON,
				 AD5940_ALLON_TMRCON_TMRINTEN_MSK);

		wuptmr = AD5940_TMRCON_WUPTEN_MSK |
			 FIELD_PREP(AD5940_TMRCON_ENDSEQ_MSK, AD5940_TMRCON_ENDSEQ_A);
		ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, wuptmr);
		data->timer_running = true;

		LOG_DBG("stream: primed + SEQCON+timer started");
	}
}

#endif /* CONFIG_AD5940_STREAM */
