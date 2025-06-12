/*
 * Copyright (c) 2025 Nick Ward
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/backend/quectel_i2c.h>

#include <string.h>

/* I2C peripheral buffer capacity */
#define READ_I2C_DATA_LENGTH 255

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_quectel_i2c, CONFIG_MODEM_MODULES_LOG_LEVEL);

static void modem_backend_quectel_i2c_receive_ready_handler(struct k_work *work)
{
	struct modem_backend_quectel_i2c *backend =
		CONTAINER_OF(work, struct modem_backend_quectel_i2c, notify_receive_ready_work);

	modem_pipe_notify_receive_ready(&backend->pipe);
}

static void modem_backend_quectel_i2c_transmit_idle_handler(struct k_work *work)
{
	struct modem_backend_quectel_i2c *backend =
		CONTAINER_OF(work, struct modem_backend_quectel_i2c, notify_transmit_idle_work);

	modem_pipe_notify_transmit_idle(&backend->pipe);
}

static void modem_backend_quectel_i2c_notify_closed_handler(struct k_work *work)
{
	struct modem_backend_quectel_i2c *backend =
		CONTAINER_OF(work, struct modem_backend_quectel_i2c, notify_closed_work);

	modem_pipe_notify_closed(&backend->pipe);
}

static void modem_backend_quectel_i2c_poll_work_handler(struct k_work *work)
{
	struct modem_backend_quectel_i2c *backend =
		CONTAINER_OF(k_work_delayable_from_work(work),
			     struct modem_backend_quectel_i2c, poll_work);
	uint8_t buf[READ_I2C_DATA_LENGTH];
	bool receive_ready = false;
	k_spinlock_key_t key;
	bool lf;
	int ret;

	if (!backend->open) {
		/* Then we previously couldn't immediately stop this work */
		k_work_submit(&backend->notify_closed_work);
		return;
	}

	ret = i2c_read_dt(&backend->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("i2c_read: %d", ret);
		modem_pipe_notify_closed(&backend->pipe);
		return;
	}

	key = k_spin_lock(&backend->receive_rb_lock);

	/* Determine if we have received data and filter out consecutive LFs */
	for (size_t i = 0; i < sizeof(buf); i++) {
		if (buf[i] == '\0') {
			/* After exiting backup mode the read often contains zeros */
			continue;
		}
		lf = (buf[i] == '\n');
		if (!lf || (lf && !backend->suppress_next_lf)) {
			ring_buf_put(&backend->receive_ring_buf, &buf[i], 1);
			receive_ready = true;
		}
		backend->suppress_next_lf = lf;
	}

	k_spin_unlock(&backend->receive_rb_lock, key);

	if (receive_ready) {
		modem_pipe_notify_receive_ready(&backend->pipe);
	}

	k_work_schedule(&backend->poll_work, K_MSEC(backend->i2c_poll_interval_ms));
}

static int modem_backend_quectel_i2c_open(void *data)
{
	struct modem_backend_quectel_i2c *backend = (struct modem_backend_quectel_i2c *)data;

	backend->open = true;
	k_work_schedule(&backend->poll_work, K_NO_WAIT);

	modem_pipe_notify_opened(&backend->pipe);

	return 0;
}

static int modem_backend_quectel_i2c_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_quectel_i2c *backend = (struct modem_backend_quectel_i2c *)data;
	int ret;

	for (size_t i = 0; i < size; i++) {
		backend->transmit_buf[backend->transmit_i] = buf[i];
		backend->transmit_i++;
		if (buf[i] == '\n') {
			k_work_cancel(&backend->notify_transmit_idle_work);

#if CONFIG_MODEM_STATS
			modem_stats_buffer_advertise_length(&backend->receive_buf_stats,
							    backend->transmit_i);
#endif

			k_sleep(K_TIMEOUT_ABS_MS(backend->next_cmd_earliest_time));

			ret = i2c_write_dt(&backend->i2c, backend->transmit_buf,
					   backend->transmit_i);
			if (ret < 0) {
				LOG_ERR("i2c_write: %d", ret);
				k_work_submit(&backend->notify_closed_work);
				return ret;
			}

			backend->next_cmd_earliest_time = k_uptime_get() + 10;

			k_work_submit(&backend->notify_transmit_idle_work);

			backend->transmit_i = 0;
		} else if (backend->transmit_i >= backend->transmit_buf_size) {
			LOG_ERR("%u bytes of TX data dropped:", backend->transmit_i);
			backend->transmit_i = 0;
		}
	}

#if CONFIG_MODEM_STATS
	modem_stats_buffer_advertise_length(&backend->receive_buf_stats, backend->transmit_i);
#endif

	return size;
}

static int modem_backend_quectel_i2c_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_quectel_i2c *backend = (struct modem_backend_quectel_i2c *)data;
	k_spinlock_key_t key;
	uint32_t received;
	bool empty;

	key = k_spin_lock(&backend->receive_rb_lock);

#if CONFIG_MODEM_STATS
	uint32_t length = ring_buf_size_get(&backend->receive_ring_buf);

	modem_stats_buffer_advertise_length(&backend->receive_buf_stats, length);
#endif

	received = ring_buf_get(&backend->receive_ring_buf, buf, size);
	empty = ring_buf_is_empty(&backend->receive_ring_buf);
	k_spin_unlock(&backend->receive_rb_lock, key);

	if (!empty) {
		k_work_submit(&backend->notify_receive_ready_work);
	}

	return (int)received;
}

static int modem_backend_quectel_i2c_close(void *data)
{
	struct modem_backend_quectel_i2c *backend = (struct modem_backend_quectel_i2c *)data;
	int ret;

	ret = k_work_cancel_delayable(&backend->poll_work);
	if (ret == 0) {
		k_work_submit(&backend->notify_closed_work);
	}
	backend->open = false;

	return 0;
}

static const struct modem_pipe_api modem_backend_quectel_i2c_api = {
	.open = modem_backend_quectel_i2c_open,
	.transmit = modem_backend_quectel_i2c_transmit,
	.receive = modem_backend_quectel_i2c_receive,
	.close = modem_backend_quectel_i2c_close,
};

#if CONFIG_MODEM_STATS
static void init_stats(struct modem_backend_quectel_i2c *backend)
{
	char name[CONFIG_MODEM_STATS_BUFFER_NAME_SIZE];
	uint32_t receive_buf_size;

	receive_buf_size = ring_buf_capacity_get(&backend->receive_ring_buf);

	snprintk(name, sizeof(name), "%s_%s", backend->i2c->name, "rx");
	modem_stats_buffer_init(&backend->receive_buf_stats, name, receive_buf_size);

	snprintk(name, sizeof(name), "%s_%s", backend->i2c->name, "tx");
	modem_stats_buffer_init(&backend->transmit_buf_stats, name, transmit_buf_stats);
}
#endif

struct modem_pipe *modem_backend_quectel_i2c_init(struct modem_backend_quectel_i2c *backend,
					   const struct modem_backend_quectel_i2c_config *config)
{
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->i2c.bus != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size > 0);

	memset(backend, 0x00, sizeof(*backend));

	backend->i2c = config->i2c;
	backend->i2c_poll_interval_ms = config->i2c_poll_interval_ms;
	backend->transmit_buf = config->transmit_buf;
	backend->transmit_buf_size = config->transmit_buf_size;
	backend->suppress_next_lf = true;
	backend->next_cmd_earliest_time = 0;

	ring_buf_init(&backend->receive_ring_buf, config->receive_buf_size, config->receive_buf);
	backend->transmit_i = 0;
	backend->open = false;

	k_work_init_delayable(&backend->poll_work, modem_backend_quectel_i2c_poll_work_handler);
	k_work_init(&backend->notify_receive_ready_work,
		    modem_backend_quectel_i2c_receive_ready_handler);
	k_work_init(&backend->notify_transmit_idle_work,
		    modem_backend_quectel_i2c_transmit_idle_handler);
	k_work_init(&backend->notify_closed_work, modem_backend_quectel_i2c_notify_closed_handler);

#if CONFIG_MODEM_STATS
	init_stats(backend);
#endif

	modem_pipe_init(&backend->pipe, backend, &modem_backend_quectel_i2c_api);

	return &backend->pipe;
}
