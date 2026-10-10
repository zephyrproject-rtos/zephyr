/*
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>

#include "tmag5170.h"
#include "tmag5170_bus.h"
#include "tmag5170_decoder.h"
#include "tmag5170_stream.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TMAG5170_STREAM, CONFIG_SENSOR_LOG_LEVEL);

BUILD_ASSERT(TMAG5170_STREAM_TX_FRAMES_COUNT == TMAG5170_RESULT_IDX_COUNT,
	     "Stream TX frame count must match the number of result registers");
BUILD_ASSERT(TMAG5170_STREAM_FRAME_LEN == TMAG5170_SPI_BUFFER_LEN,
	     "Stream frame length must match the SPI frame length");

/* Enum indices from ti,tmag5170.yaml for magnetic-channels property */
enum tmag5170_mag_channels_idx {
	TMAG5170_MAG_CHANNELS_NONE = 0,
	TMAG5170_MAG_CHANNELS_X = 1,
	TMAG5170_MAG_CHANNELS_Y = 2,
	TMAG5170_MAG_CHANNELS_XY = 3,
	TMAG5170_MAG_CHANNELS_Z = 4,
	TMAG5170_MAG_CHANNELS_ZX = 5,
	TMAG5170_MAG_CHANNELS_YZ = 6,
	TMAG5170_MAG_CHANNELS_XYZ = 7,
	TMAG5170_MAG_CHANNELS_XYX = 8,
	TMAG5170_MAG_CHANNELS_YXY = 9,
	TMAG5170_MAG_CHANNELS_YZY = 10,
	TMAG5170_MAG_CHANNELS_ZYZ = 11,
	TMAG5170_MAG_CHANNELS_ZXZ = 12,
	TMAG5170_MAG_CHANNELS_XZX = 13,
	TMAG5170_MAG_CHANNELS_XYZYX = 14,
	TMAG5170_MAG_CHANNELS_XYZZYX = 15,
};

static inline uint8_t tmag5170_get_stream_channels_mask(const struct tmag5170_dev_config *cfg)
{
	uint8_t channels = 0;

	switch (cfg->magnetic_channels) {
	case TMAG5170_MAG_CHANNELS_X:
		channels |= BIT(TMAG5170_RESULT_IDX_X);
		break;
	case TMAG5170_MAG_CHANNELS_Y:
		channels |= BIT(TMAG5170_RESULT_IDX_Y);
		break;
	case TMAG5170_MAG_CHANNELS_XY:
	case TMAG5170_MAG_CHANNELS_XYX:
	case TMAG5170_MAG_CHANNELS_YXY:
		channels |= BIT(TMAG5170_RESULT_IDX_X) | BIT(TMAG5170_RESULT_IDX_Y);
		break;
	case TMAG5170_MAG_CHANNELS_Z:
		channels |= BIT(TMAG5170_RESULT_IDX_Z);
		break;
	case TMAG5170_MAG_CHANNELS_ZX:
	case TMAG5170_MAG_CHANNELS_ZXZ:
	case TMAG5170_MAG_CHANNELS_XZX:
		channels |= BIT(TMAG5170_RESULT_IDX_Z) | BIT(TMAG5170_RESULT_IDX_X);
		break;
	case TMAG5170_MAG_CHANNELS_YZ:
	case TMAG5170_MAG_CHANNELS_YZY:
	case TMAG5170_MAG_CHANNELS_ZYZ:
		channels |= BIT(TMAG5170_RESULT_IDX_Y) | BIT(TMAG5170_RESULT_IDX_Z);
		break;
	case TMAG5170_MAG_CHANNELS_XYZ:
	case TMAG5170_MAG_CHANNELS_XYZYX:
	case TMAG5170_MAG_CHANNELS_XYZZYX:
		channels |= BIT(TMAG5170_RESULT_IDX_X) | BIT(TMAG5170_RESULT_IDX_Y) |
			    BIT(TMAG5170_RESULT_IDX_Z);
		break;
	default:
		break;
	}

	if (cfg->angle_measurement != 0) {
		channels |= BIT(TMAG5170_RESULT_IDX_ANGLE);
	}

	if (cfg->temperature_measurement) {
		channels |= BIT(TMAG5170_RESULT_IDX_TEMP);
	}

	if (channels == 0) {
		channels = (uint8_t)BIT_MASK(TMAG5170_RESULT_IDX_COUNT);
	}

	return channels;
}

static size_t tmag5170_fill_stream_chans(uint8_t mask,
					 struct sensor_chan_spec chans[TMAG5170_RESULT_IDX_COUNT])
{
	size_t count = 0;

	if (mask & BIT(TMAG5170_RESULT_IDX_X)) {
		chans[count++] = (struct sensor_chan_spec){SENSOR_CHAN_MAGN_X, 0};
	}
	if (mask & BIT(TMAG5170_RESULT_IDX_Y)) {
		chans[count++] = (struct sensor_chan_spec){SENSOR_CHAN_MAGN_Y, 0};
	}
	if (mask & BIT(TMAG5170_RESULT_IDX_Z)) {
		chans[count++] = (struct sensor_chan_spec){SENSOR_CHAN_MAGN_Z, 0};
	}
	if (mask & BIT(TMAG5170_RESULT_IDX_ANGLE)) {
		chans[count++] = (struct sensor_chan_spec){SENSOR_CHAN_ROTATION, 0};
	}
	if (mask & BIT(TMAG5170_RESULT_IDX_TEMP)) {
		chans[count++] = (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0};
	}

	return count;
}

static const struct sensor_stream_trigger *
tmag5170_get_read_config_trigger(const struct sensor_read_config *cfg,
				 enum sensor_trigger_type trig)
{
	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == trig) {
			return &cfg->triggers[i];
		}
	}

	return NULL;
}

/** Only SENSOR_STREAM_DATA_INCLUDE actually transfers the result registers;
 * NOP/DROP only report the data-ready event through the header.
 */
static inline bool tmag5170_should_read_data(const struct sensor_read_config *read_cfg)
{
	const struct sensor_stream_trigger *trig =
		tmag5170_get_read_config_trigger(read_cfg, SENSOR_TRIG_DATA_READY);

	return trig != NULL && trig->opt == SENSOR_STREAM_DATA_INCLUDE;
}

static void tmag5170_stream_result(const struct device *dev, int result)
{
	struct tmag5170_data *drv_data = dev->data;
	const struct tmag5170_dev_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe;
	k_spinlock_key_t key;

	/* Disable the interrupt BEFORE the submission is cleared: this closes
	 * the window in which the interrupt is armed while iodev_sqe is NULL,
	 * which used to make a concurrent ALERT land in the "no submission"
	 * branch of the event handler and disable the interrupt for good. The
	 * interrupt is only re-armed at the very end of tmag5170_stream_submit(),
	 * i.e. once a new submission (from a successful re-arm) is in place.
	 */
	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	key = k_spin_lock(&drv_data->stream.lock);
	iodev_sqe = drv_data->stream.iodev_sqe;
	drv_data->stream.iodev_sqe = NULL;
	k_spin_unlock(&drv_data->stream.lock, key);

	(void)atomic_set(&drv_data->stream.state, TMAG5170_STREAM_OFF);

	if (iodev_sqe == NULL) {
		return;
	}

	if (result < 0) {
		rtio_iodev_sqe_err(iodev_sqe, result);
	} else {
		/* Re-arms the stream: rtio_iodev_sqe_ok() drives the multishot
		 * submission back to tmag5170_stream_submit(), which re-enables
		 * the interrupt once the new submission is in place.
		 */
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void tmag5170_stream_complete(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				     void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct tmag5170_data *drv_data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct tmag5170_encoded_data *edata;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;
	int err = (result < 0) ? result : 0;

	drv_data->stream.completions++;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			if (cqe->result < 0 && err == 0) {
				err = cqe->result;
			}
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err != 0) {
		/* Only a real bus/transport error reported through the CQE result
		 * is fatal: it means the SPI transfer itself failed, which cannot
		 * be recovered from within this completion.
		 */
		drv_data->stream.bus_errors++;
		LOG_ERR_RATELIMIT_RATE(1000, "Bus error in stream completion of %s: %d", dev->name,
				       err);
		tmag5170_stream_result(dev, err);
		return;
	}

	err = rtio_sqe_rx_buf(iodev_sqe, 0, 0, &buf, &buf_len);
	if (err != 0) {
		/* Out of mempool blocks: drop this event but keep the stream
		 * armed, the next completion may well succeed again.
		 */
		drv_data->stream.nomem_errors++;
		LOG_ERR_RATELIMIT_RATE(1000, "No RX buffer for stream completion of %s: %d",
				       dev->name, err);
		tmag5170_stream_result(dev, 0);
		return;
	}

	edata = (struct tmag5170_encoded_data *)buf;

	for (uint8_t idx = 0U; idx < TMAG5170_RESULT_IDX_COUNT; idx++) {
		if ((edata->header.channels & BIT(idx)) == 0U) {
			continue;
		}

		if (tmag5170_frame_decode(edata->payload.frames[idx], NULL) != 0) {
			/* A CRC mismatch is not fatal either: mark the payload as
			 * empty so the decoder reports -ENODATA instead of garbage,
			 * and keep the stream armed for the next sample.
			 */
			drv_data->stream.crc_errors++;
			LOG_ERR_RATELIMIT_RATE(1000, "CRC mismatch of result register %u of %s",
					       idx, dev->name);
			edata->header.events = 0;
			edata->header.channels = 0;
			break;
		}
	}

	tmag5170_stream_result(dev, 0);
}

static void tmag5170_stream_event_handler(const struct device *dev)
{
	struct tmag5170_data *drv_data = dev->data;
	const struct tmag5170_dev_config *cfg = dev->config;
	const struct sensor_read_config *read_cfg;
	struct rtio_iodev_sqe *iodev_sqe;
	k_spinlock_key_t key;
	uint64_t cycles;
	bool claimed;
	bool with_data;
	int err;

	/* Snapshot iodev_sqe and claim the stream (ON -> BUSY) as a single
	 * critical section: this is the same lock tmag5170_stream_disable()
	 * uses to claim an idle (ON) submission, so the two can never race
	 * on the same submission.
	 */
	key = k_spin_lock(&drv_data->stream.lock);
	iodev_sqe = drv_data->stream.iodev_sqe;

	if (iodev_sqe == NULL) {
		k_spin_unlock(&drv_data->stream.lock, key);
		/* Residual edge: tmag5170_stream_result()/_disable() already
		 * took care of the interrupt and of iodev_sqe. Drop the event
		 * but leave the interrupt armed, so the next edge after a
		 * successful re-arm is serviced. No lasting state may be
		 * changed here.
		 */
		drv_data->stream.spurious_irqs++;
		return;
	}

	if (FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {
		/* The application cancelled the multishot submission, so
		 * nobody will ever re-arm it. Disable the interrupt and
		 * finalize the submission right here: rtio_sqe_cancel() only
		 * sets the flag, so a cancelled SQE that is never completed
		 * keeps its mempool block allocated forever and starves the
		 * pool after a few restart cycles.
		 *
		 * Clearing iodev_sqe under the lock makes this mutually
		 * exclusive with tmag5170_stream_disable(): whichever of the
		 * two grabs the pointer first finalizes it, the other one
		 * sees NULL and does nothing.
		 */
		drv_data->stream.iodev_sqe = NULL;
		k_spin_unlock(&drv_data->stream.lock, key);

		(void)atomic_set(&drv_data->stream.state, TMAG5170_STREAM_OFF);
		(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
		LOG_WRN("Streaming submission of %s cancelled - interrupt disabled", dev->name);
		return;
	}

	claimed = atomic_cas(&drv_data->stream.state, TMAG5170_STREAM_ON, TMAG5170_STREAM_BUSY);
	k_spin_unlock(&drv_data->stream.lock, key);

	if (!claimed) {
		/* An ALERT arrived while the previous readout was still in
		 * flight. Drop the event, but intentionally leave the
		 * interrupt armed so the next edge after completion is
		 * serviced.
		 */
		drv_data->stream.spurious_irqs++;
		LOG_WRN_RATELIMIT_RATE(1000,
				       "Event handler of %s triggered while a stream is in "
				       "progress - ignoring",
				       dev->name);
		return;
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		LOG_ERR("Failed to get timestamp: %d", err);
		cycles = 0;
	}
	drv_data->stream.timestamp = sensor_clock_cycles_to_ns(cycles);

	read_cfg = iodev_sqe->sqe.iodev->data;
	with_data = tmag5170_should_read_data(read_cfg);

	/* Without SENSOR_STREAM_DATA_INCLUDE only the event is reported, so a
	 * header sized buffer is enough and no frame has to be shifted in.
	 */
	const uint32_t min_buf_len = with_data ? sizeof(struct tmag5170_encoded_data)
					       : sizeof(struct tmag5170_encoded_header);
	uint8_t *buf;
	uint32_t buf_len;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err != 0) {
		/* Out of mempool blocks: drop this event, but keep the stream
		 * armed instead of killing it.
		 */
		drv_data->stream.nomem_errors++;
		LOG_ERR_RATELIMIT_RATE(1000, "No RX buffer for stream of %s: %d", dev->name, err);
		rtio_sqe_drop_all(drv_data->bus.rtio.ctx);
		tmag5170_stream_result(dev, 0);
		return;
	}

	struct sensor_chan_spec chans[TMAG5170_RESULT_IDX_COUNT];
	uint8_t mask = tmag5170_get_stream_channels_mask(cfg);
	size_t num_chans = tmag5170_fill_stream_chans(mask, chans);

	err = tmag5170_encode(dev, chans, num_chans, buf);
	if (err != 0) {
		/* Not a bus/transport error, so not fatal either: drop this
		 * event and keep the stream armed.
		 */
		drv_data->stream.sqe_errors++;
		LOG_ERR_RATELIMIT_RATE(1000, "Failed to encode sensor data of %s: %d", dev->name,
				       err);
		rtio_sqe_drop_all(drv_data->bus.rtio.ctx);
		tmag5170_stream_result(dev, 0);
		return;
	}

	struct tmag5170_encoded_data *edata = (struct tmag5170_encoded_data *)buf;

	edata->header.timestamp = drv_data->stream.timestamp;
	edata->header.events = TMAG5170_EVENT_DATA_READY;

	if (!with_data) {
		/* Announce the event only. Clearing the channel mask makes the
		 * decoder report -ENODATA instead of handing out the payload,
		 * which has not been transferred at all.
		 */
		edata->header.channels = 0;
		tmag5170_stream_result(dev, 0);
		return;
	}

	struct rtio_sqe *sqe = NULL;

	for (uint8_t idx = 0U; idx < TMAG5170_RESULT_IDX_COUNT; idx++) {
		if ((edata->header.channels & BIT(idx)) == 0U) {
			continue;
		}

		tmag5170_frame_encode_read(drv_data->stream.tx_frames[idx],
					   tmag5170_result_regs[idx], 0U);

		err = tmag5170_prep_frame_rtio_async(&drv_data->bus,
						     drv_data->stream.tx_frames[idx],
						     edata->payload.frames[idx], &sqe);
		if (err < 0) {
			goto err_sqe;
		}
		sqe->flags |= RTIO_SQE_CHAINED;
	}

	sqe = rtio_sqe_acquire(drv_data->bus.rtio.ctx);
	if (!sqe) {
		goto err_sqe;
	}
	rtio_sqe_prep_callback_no_cqe(sqe, tmag5170_stream_complete, (void *)dev, iodev_sqe);

	rtio_submit(drv_data->bus.rtio.ctx, 0);
	drv_data->stream.submissions++;
	return;

err_sqe:
	/* -ENOMEM from rtio_sqe_acquire()/tmag5170_prep_frame_rtio_async():
	 * drop this event and keep the stream armed instead of killing it.
	 * The header above already announced a data-ready event with the
	 * requested channels, but the frame chain was never (fully)
	 * submitted, so its payload never got filled with real data. Clear
	 * it, same as for a CRC mismatch, so the decoder reports -ENODATA
	 * instead of handing out garbage.
	 */
	drv_data->stream.sqe_errors++;
	LOG_ERR_RATELIMIT_RATE(1000, "Failed to acquire SQE for stream of %s", dev->name);
	edata->header.events = 0;
	edata->header.channels = 0;
	rtio_sqe_drop_all(drv_data->bus.rtio.ctx);
	tmag5170_stream_result(dev, 0);
}

static void tmag5170_stream_gpio_callback(const struct device *gpio_dev, struct gpio_callback *cb,
					  uint32_t pins)
{
	ARG_UNUSED(gpio_dev);
	ARG_UNUSED(pins);

	struct tmag5170_data *drv_data = CONTAINER_OF(cb, struct tmag5170_data, stream.gpio_cb);
	const struct device *dev = drv_data->stream.dev;

	tmag5170_stream_event_handler(dev);
}

int tmag5170_stream_init(const struct device *dev)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	int err;

	drv_data->stream.dev = dev;
	(void)atomic_set(&drv_data->stream.state, TMAG5170_STREAM_OFF);

	if (!cfg->int_gpio.port) {
		LOG_ERR("int-gpios not configured for %s (required for streaming)", dev->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->int_gpio.port);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", err);
		return err;
	}

	gpio_init_callback(&drv_data->stream.gpio_cb, tmag5170_stream_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	err = gpio_add_callback(cfg->int_gpio.port, &drv_data->stream.gpio_cb);
	if (err) {
		LOG_ERR("Failed to add GPIO callback: %d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	if (err) {
		LOG_ERR("Failed to disable interrupt: %d", err);
		return err;
	}

	return 0;
}

void tmag5170_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	k_spinlock_key_t key;
	int err;

	if (tmag5170_get_read_config_trigger(read_cfg, SENSOR_TRIG_DATA_READY) == NULL) {
		LOG_ERR("Unsupported streaming trigger; only SENSOR_TRIG_DATA_READY is supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	key = k_spin_lock(&drv_data->stream.lock);
	drv_data->stream.iodev_sqe = iodev_sqe;
	k_spin_unlock(&drv_data->stream.lock, key);

	(void)atomic_set(&drv_data->stream.state, TMAG5170_STREAM_ON);

	/* Re-arm the interrupt only now that iodev_sqe and state are in place,
	 * see tmag5170_stream_result() for the matching disable-first order.
	 */
	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_INACTIVE);
	if (err != 0) {
		LOG_ERR("Failed to enable interrupt: %d", err);
		tmag5170_stream_result(dev, err);
	}
}

void tmag5170_stream_disable(const struct device *dev)
{
	struct tmag5170_data *drv_data = dev->data;
	const struct tmag5170_dev_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = NULL;
	k_spinlock_key_t key;

	/* Stop any further ALERT from starting a new transfer. */
	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	key = k_spin_lock(&drv_data->stream.lock);
	if (atomic_cas(&drv_data->stream.state, TMAG5170_STREAM_ON, TMAG5170_STREAM_OFF)) {
		/* The submission was armed but idle: since the interrupt is
		 * now disabled, no completion will ever come for it, so it
		 * has to be finalized here.
		 */
		iodev_sqe = drv_data->stream.iodev_sqe;
		drv_data->stream.iodev_sqe = NULL;
	}
	/* Else: already OFF (nothing to do), or a transfer is currently in
	 * flight (BUSY) - its own completion finalizes iodev_sqe through
	 * tmag5170_stream_result(), touching it here as well would race with
	 * that completion.
	 */
	k_spin_unlock(&drv_data->stream.lock, key);

	if (iodev_sqe != NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
	}
}
