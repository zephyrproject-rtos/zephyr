/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad5940.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ad5940, CONFIG_SENSOR_LOG_LEVEL);

static void ad5940_gpio_callback(const struct device *port,
				 struct gpio_callback *cb,
				 uint32_t pin);

void ad5940_trigger_resubmit(const struct device *dev)
{
	struct ad5940_data *data = dev->data;

#if defined(CONFIG_AD5940_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_AD5940_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#else
	ARG_UNUSED(data);
#endif
}

static void ad5940_handle_trigger(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	int guard = 0;
	uint32_t discard_buf[AD5940_FIFO_BURST_MAX_WORDS];
	uint32_t backlog = 0u;
	uint32_t cnt_raw = 0u;
	uint32_t words;
	int ret;

#ifdef CONFIG_AD5940_STREAM
	if (data->active_sqe != NULL) {
		ad5940_stream_irq_handler(dev);
		return;
	}
#endif

	if (data->trigger_handler != NULL && data->trigger != NULL) {
		data->trigger_handler(dev, data->trigger);
	}

	ret = ad5940_wakeup(dev);
	if (ret) {
		return;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, 0x0u);
	if (ret) {
		return;
	}

	while (guard++ < AD5940_FIFO_DRAIN_MAX_ITERS) {
		cnt_raw = 0u;

		ret = ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &cnt_raw);
		if (ret) {
			break;
		}
		words = FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, cnt_raw);

		words = (words / AD5940_EIS_WORDS_PER_FRAME) * AD5940_EIS_WORDS_PER_FRAME;
		if (words == 0u) {
			break;
		}
		if (words > ARRAY_SIZE(discard_buf)) {
			words = ARRAY_SIZE(discard_buf);
		}
		ret = ad5940_fifo_read_words(dev, discard_buf, (uint16_t)words);
		if (ret) {
			break;
		}
	}

	/* Best-effort cleanup: clear interrupts and re-lock sequencer. */
	ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);

	if (cfg->int_gpio.port != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						GPIO_INT_EDGE_TO_ACTIVE);

		if (ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &backlog) == 0 &&
		    FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, backlog) >=
			    AD5940_EIS_WORDS_PER_FRAME) {
			ad5940_trigger_resubmit(dev);
		}
	}
}

#if defined(CONFIG_AD5940_TRIGGER_OWN_THREAD)

static void ad5940_trigger_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct ad5940_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		ad5940_handle_trigger(dev);
	}
}

#elif defined(CONFIG_AD5940_TRIGGER_GLOBAL_THREAD)

static void ad5940_work_handler(struct k_work *work)
{
	struct ad5940_data *data = CONTAINER_OF(work, struct ad5940_data, work);

	ad5940_handle_trigger(data->dev);
}

#endif /* CONFIG_AD5940_TRIGGER_OWN_THREAD */

static void ad5940_gpio_callback(const struct device *port,
				 struct gpio_callback *cb,
				 uint32_t pin)
{
	struct ad5940_data *data = CONTAINER_OF(cb, struct ad5940_data, gpio_cb);
	const struct ad5940_config *cfg = data->dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_AD5940_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);

#elif defined(CONFIG_AD5940_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);

#else
	/* TRIGGER_NONE — should not reach here */
#endif
}

int ad5940_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	uint32_t intcsel0;
	int ret;

	if (cfg->int_gpio.port == NULL) {
		LOG_ERR("AD5940: no int-gpios configured");
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_FIFO_WATERMARK &&
	    trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->trigger_handler = handler;
	data->trigger = trig;

	if (handler == NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, 0x0u);
		ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON, 0x0u);
#ifdef CONFIG_AD5940_STREAM
		data->timer_running = false;
#endif
		k_mutex_unlock(&data->lock);
		return 0;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_INTCSEL0, &intcsel0);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	intcsel0 |= AFEINTSRC_DATAFIFOTHRESH;
	intcsel0 &= ~(AFEINTSRC_DFTRDY | AFEINTSRC_ENDSEQ);

	ret = ad5940_reg_write(dev, AD5940_REG_INTCSEL0, intcsel0);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQCON, AD5940_SEQCON_SEQEN_MSK);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_SEQTRGSLP, AD5940_SEQTRGSLP_EN);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON,
			       AD5940_ALLON_TMRCON_TMRINTEN_MSK);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON,
			       AD5940_TMRCON_WUPTEN_MSK |
			       FIELD_PREP(AD5940_TMRCON_ENDSEQ_MSK, AD5940_TMRCON_ENDSEQ_A));
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

#ifdef CONFIG_AD5940_STREAM
	data->timer_running = true;
#endif

	k_mutex_unlock(&data->lock);

	return 0;
}

void ad5940_stream_irq_handler(const struct device *dev)
{
#ifdef CONFIG_AD5940_STREAM
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->active_sqe;
	uint32_t fifo_cnt = 0u;
	uint32_t fifo_words = 0u;
	uint8_t req_samples = 0u;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0u;
	uint64_t cycles = 0u;
	uint32_t stray;
	uint32_t discard[4];
	uint32_t min_sz;
	uint32_t ideal_sz;
	uint32_t max_words;
	uint32_t *dst;
	uint32_t remaining;
	uint32_t backlog;
	uint16_t chunk;
	struct ad5940_fifo_hdr *hdr;
	int rd;

	if (iodev_sqe == NULL) {
		return;
	}

	(void)sensor_clock_get_cycles(&cycles);
	data->timestamp_ns = sensor_clock_cycles_to_ns(cycles);

	req_samples = (data->mode == AD5940_MODE_EIS) ?
		      (uint8_t)AD5940_EIS_WORDS_PER_FRAME : 1u;

	if (ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &fifo_cnt) != 0) {
		LOG_ERR("AD5940 stream: FIFOCNTSTA read failed");
		goto rearm;
	}

	fifo_words = FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, fifo_cnt);

	if (fifo_words < req_samples) {
		goto rearm;
	}

	if ((fifo_words % req_samples) != 0u) {
		stray = fifo_words % req_samples;

		LOG_WRN("AD5940 stream: realigning FIFO, dropped %u stray word(s)", stray);
		if (ad5940_fifo_read_words(dev, discard, (uint16_t)stray) != 0) {
			LOG_ERR("AD5940 stream: stray FIFO read failed");
		}
		goto rearm;
	}

	fifo_words = (fifo_words / req_samples) * req_samples;

	min_sz = sizeof(struct ad5940_fifo_hdr) +
		 (uint32_t)req_samples * AD5940_FIFO_WORD_BYTES;
	ideal_sz = sizeof(struct ad5940_fifo_hdr) + fifo_words * AD5940_FIFO_WORD_BYTES;

	if (rtio_sqe_rx_buf(iodev_sqe, min_sz, ideal_sz, &buf, &buf_len) != 0) {
		LOG_ERR("AD5940 stream: buffer alloc failed");
		ad5940_stream_stop_timer(dev);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		data->active_sqe = NULL;
		return;
	}

	max_words = (buf_len - sizeof(struct ad5940_fifo_hdr)) / AD5940_FIFO_WORD_BYTES;
	fifo_words = MIN(fifo_words, (max_words / req_samples) * req_samples);

	hdr = (struct ad5940_fifo_hdr *)buf;

	hdr->is_fifo         = 1u;
	hdr->data_type       = (uint8_t)data->fifo_src;
	hdr->req_samples     = req_samples;
	hdr->int_status      = AFEINTSRC_DATAFIFOTHRESH;
	hdr->sample_set_size = (uint32_t)req_samples * AD5940_FIFO_WORD_BYTES;
	hdr->fifo_byte_count = (uint16_t)(fifo_words * AD5940_FIFO_WORD_BYTES);
	hdr->odr             = 0u;
	hdr->timestamp       = data->timestamp_ns;
	hdr->rtia_mag_ohms   = data->calibrated ? data->rtia_cal[0] : 0.0f;
	hdr->rtia_phase_rad  = data->calibrated ? data->rtia_cal[1] : 0.0f;
	hdr->freq_hz         = data->eis.freq_hz;
	data->last_stream_freq_hz = data->eis.freq_hz;

	rd = ad5940_wakeup(dev);
	if (rd != 0) {
		LOG_ERR("AD5940 stream: wakeup failed: %d", rd);
		ad5940_stream_stop_timer(dev);
		rtio_iodev_sqe_err(iodev_sqe, rd);
		data->active_sqe = NULL;
		return;
	}
	rd = ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, 0x0u);
	if (rd != 0) {
		LOG_ERR("AD5940 stream: unlock failed: %d", rd);
		ad5940_stream_stop_timer(dev);
		rtio_iodev_sqe_err(iodev_sqe, rd);
		data->active_sqe = NULL;
		return;
	}

	rd = 0;
	dst = (uint32_t *)(buf + sizeof(struct ad5940_fifo_hdr));
	remaining = fifo_words;

	while (remaining > 0u) {
		chunk = (uint16_t)MIN(remaining, AD5940_FIFO_BURST_MAX_WORDS);

		rd = ad5940_fifo_read_words(dev, dst, chunk);
		if (rd != 0) {
			break;
		}
		dst += chunk;
		remaining -= chunk;
	}

	if (rd != 0) {
		LOG_ERR("AD5940 stream: FIFO read failed: %d", rd);
		ad5940_stream_stop_timer(dev);
		rtio_iodev_sqe_err(iodev_sqe, rd);
		data->active_sqe = NULL;
		return;
	}

	/* Best-effort cleanup: clear interrupts and re-lock sequencer. */
	ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
	if (data->mode == AD5940_MODE_EIS && data->sweep_enabled) {
		ad5940_advance_sweep_freq(dev);
	}
#endif

	rtio_iodev_sqe_ok(iodev_sqe, 0);
	return;

rearm:
	/* Best-effort cleanup: clear interrupts and re-lock sequencer. */
	ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);
	if (cfg->int_gpio.port != NULL) {
		backlog = 0u;

		gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);

		if (ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &backlog) == 0 &&
		    FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, backlog) >=
			    req_samples) {
			ad5940_trigger_resubmit(dev);
		}
	}
#else
	ARG_UNUSED(dev);
#endif
}

int ad5940_trigger_init(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	int ret;

	if (cfg->int_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("AD5940: INT GPIO device not ready");
		return -ENODEV;
	}

	ret = ad5940_gpio_cfg(dev, cfg->int_ad5940_pin, AD5940_GP0CON_PIN0_INT0,
			      true, false, cfg->int_ad5940_pull_en, false);
	if (ret) {
		LOG_ERR("AD5940: GP0 INT0 cfg failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret) {
		LOG_ERR("AD5940: failed to configure INT GPIO: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, ad5940_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (ret) {
		LOG_ERR("AD5940: failed to add GPIO callback: %d", ret);
		return ret;
	}

#if defined(CONFIG_AD5940_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, 1);

	k_thread_create(&data->thread, data->thread_stack,
			K_KERNEL_STACK_SIZEOF(data->thread_stack),
			ad5940_trigger_thread,
			(void *)dev, NULL, NULL,
			CONFIG_AD5940_THREAD_PRIORITY,
			0, K_NO_WAIT);

	k_thread_name_set(&data->thread, "ad5940_trig");

#elif defined(CONFIG_AD5940_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, ad5940_work_handler);
#endif

	return 0;
}
