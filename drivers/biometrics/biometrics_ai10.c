/*
 * Copyright (c) 2026 Siratul Islam <siratul.islam@linux.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hilink_ai10

#include <zephyr/drivers/biometrics.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/ring_buffer.h>
#include <string.h>

#define AI10_REPLY          0x00U
#define AI10_RESET          0x10U
#define AI10_VERIFY         0x12U
#define AI10_ENROLL         0x1DU
#define AI10_DELETE         0x20U
#define AI10_DELETE_ALL     0x21U
#define AI10_LIST           0x24U
#define AI10_MAX_ID         2000U
#define AI10_FACE_MAX_ID    1000U
#define AI10_PACKET_SIZE    512U
#define AI10_RX_SIZE        1024U
#define AI10_COMMAND_MS     3000U
#define AI10_FRAME_MS       1000U
#define AI10_TIMEOUT_MAX_MS 255000U

#define AI10_STATUS_OK                   0U
#define AI10_STATUS_REJECTED             1U
#define AI10_STATUS_ABORTED              2U
#define AI10_STATUS_INVALID_PARAM        6U
#define AI10_STATUS_NO_MEMORY            7U
#define AI10_STATUS_UNKNOWN_USER         8U
#define AI10_STATUS_STORAGE_FULL         9U
#define AI10_STATUS_ALREADY_ENROLLED     10U
#define AI10_STATUS_LIVENESS_FAILED      12U
#define AI10_STATUS_TIMEOUT              13U
#define AI10_STATUS_AUTHORIZATION_FAILED 14U

#define AI10_SYNC_1               0xEFU
#define AI10_SYNC_2               0xAAU
#define AI10_REPLY_COMMAND_OFFSET 0U
#define AI10_REPLY_STATUS_OFFSET  1U
#define AI10_REPLY_HEADER_SIZE    2U
#define AI10_REPLY_ID_OFFSET      2U
#define AI10_ID_REPLY_MIN_SIZE    4U

#define AI10_VERIFY_SINGLE         0U
#define AI10_VERIFY_CONTINUOUS     1U
#define AI10_VERIFY_PARAMS_SIZE    2U
#define AI10_VERIFY_MODE_OFFSET    0U
#define AI10_VERIFY_TIMEOUT_OFFSET 1U
#define AI10_VERIFY_REPLY_MIN_SIZE 38U

#define AI10_ENROLL_PARAMS_SIZE    35U
#define AI10_ENROLL_TIMEOUT_OFFSET 34U
#define AI10_ENROLL_REPLY_MIN_SIZE 5U

#define AI10_LIST_COUNT_OFFSET      2U
#define AI10_LIST_IDS_OFFSET        3U
#define AI10_LIST_REPLY_HEADER_SIZE 3U
#define AI10_LIST_IDS_PER_PACKET    100U

enum ai10_operation {
	AI10_IDLE,
	AI10_IDENTIFY,
	AI10_ENROLLING,
};

struct ai10_config {
	const struct device *uart;
	uint32_t startup_delay_ms;
};

struct ai10_packet {
	uint8_t type;
	uint16_t size;
	uint8_t payload[AI10_PACKET_SIZE];
};

struct ai10_data {
	struct k_mutex lock;
	struct k_spinlock rx_lock;
	struct k_spinlock state_lock;
	struct ring_buf rx;
	uint8_t rx_buffer[AI10_RX_SIZE];
	struct k_sem rx_ready;
	struct k_sem request;
	struct k_sem done;
	struct k_thread thread;
	atomic_t operation;
	atomic_t cancel;
	atomic_t rx_error;
	biometric_event_callback_t callback;
	void *user_data;
	uint8_t timeout_s;
	bool continuous;
	int stop_result;
	int64_t ready_at;
	struct ai10_packet packet;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_BIOMETRICS_AI10_STACK_SIZE);
};

static int ai10_status(uint8_t status)
{
	switch (status) {
	case AI10_STATUS_OK:
		return 0;
	case AI10_STATUS_REJECTED:
		return -EBUSY;
	case AI10_STATUS_ABORTED:
		return -ECANCELED;
	case AI10_STATUS_INVALID_PARAM:
		return -EINVAL;
	case AI10_STATUS_NO_MEMORY:
		return -ENOMEM;
	case AI10_STATUS_UNKNOWN_USER:
		return -ENOENT;
	case AI10_STATUS_STORAGE_FULL:
		return -ENOSPC;
	case AI10_STATUS_ALREADY_ENROLLED:
		return -EEXIST;
	case AI10_STATUS_LIVENESS_FAILED:
	case AI10_STATUS_AUTHORIZATION_FAILED:
		return -EACCES;
	case AI10_STATUS_TIMEOUT:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

static int ai10_lock(const struct device *dev)
{
	struct ai10_data *data = dev->data;

	if (k_is_in_isr() || k_current_get() == &data->thread) {
		return -EWOULDBLOCK;
	}

	return k_mutex_lock(&data->lock, K_NO_WAIT) == 0 ? 0 : -EBUSY;
}

static int ai10_lock_idle(const struct device *dev)
{
	struct ai10_data *data = dev->data;
	int ret;

	ret = ai10_lock(dev);
	if (ret == 0 && atomic_get(&data->operation) != AI10_IDLE) {
		k_mutex_unlock(&data->lock);
		ret = -EBUSY;
	}

	return ret;
}

static void ai10_uart_callback(const struct device *uart, void *user_data)
{
	struct ai10_data *data = user_data;
	uint8_t bytes[32];
	int count;

	uart_irq_update(uart);

	while (uart_irq_rx_ready(uart) > 0) {
		k_spinlock_key_t key;
		uint32_t stored;

		count = uart_fifo_read(uart, bytes, sizeof(bytes));
		if (count <= 0) {
			break;
		}

		key = k_spin_lock(&data->rx_lock);
		stored = ring_buf_put(&data->rx, bytes, count);
		k_spin_unlock(&data->rx_lock, key);

		if (stored != (uint32_t)count) {
			atomic_set(&data->rx_error, 1);
		}

		k_sem_give(&data->rx_ready);
		uart_irq_update(uart);
	}
}

static void ai10_send(const struct device *dev, uint8_t command, const uint8_t *payload,
		      uint16_t size)
{
	uint8_t header[] = {AI10_SYNC_1, AI10_SYNC_2, command, 0U, 0U};
	const struct ai10_config *cfg = dev->config;
	uint8_t checksum;

	sys_put_be16(size, &header[3]);
	checksum = header[2] ^ header[3] ^ header[4];

	for (size_t i = 0U; i < sizeof(header); i++) {
		uart_poll_out(cfg->uart, header[i]);
	}

	for (uint16_t i = 0U; i < size; i++) {
		checksum ^= payload[i];
		uart_poll_out(cfg->uart, payload[i]);
	}

	uart_poll_out(cfg->uart, checksum);
}

static int ai10_get_byte(struct ai10_data *data, uint8_t *byte, int64_t deadline, bool cancellable)
{
	k_spinlock_key_t key;
	int64_t remaining;
	uint32_t count;
	int ret;

	while (true) {
		if (cancellable && atomic_get(&data->cancel) != 0) {
			return -ECANCELED;
		}

		if (atomic_get(&data->rx_error) != 0) {
			return -ENOBUFS;
		}

		if (deadline != INT64_MAX && k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}

		key = k_spin_lock(&data->rx_lock);
		count = ring_buf_get(&data->rx, byte, 1U);
		k_spin_unlock(&data->rx_lock, key);
		if (count == 1U) {
			return 0;
		}

		remaining = deadline == INT64_MAX ? INT64_MAX : deadline - k_uptime_get();

		if (remaining <= 0) {
			return -ETIMEDOUT;
		}

		ret = k_sem_take(&data->rx_ready,
				 deadline == INT64_MAX ? K_FOREVER : K_MSEC(remaining));
		if (ret != 0) {
			return -ETIMEDOUT;
		}
	}
}

/* Bound partial frames even when continuous recognition is waiting indefinitely. */
static int ai10_receive(struct ai10_data *data, int64_t deadline, bool cancellable)
{
	struct ai10_packet *packet = &data->packet;
	bool sync = false;
	uint8_t header[3];
	uint8_t checksum;
	uint8_t byte;
	int ret;

	while (true) {
		ret = ai10_get_byte(data, &byte, deadline, cancellable);
		if (ret != 0) {
			return ret;
		}

		if (sync && byte == AI10_SYNC_2) {
			break;
		}

		sync = byte == AI10_SYNC_1;
		if (sync) {
			deadline = MIN(deadline, k_uptime_get() + AI10_FRAME_MS);
		}
	}

	deadline = MIN(deadline, k_uptime_get() + AI10_FRAME_MS);

	for (size_t i = 0U; i < sizeof(header); i++) {
		ret = ai10_get_byte(data, &header[i], deadline, cancellable);
		if (ret != 0) {
			return ret;
		}
	}

	packet->type = header[0];
	packet->size = sys_get_be16(&header[1]);
	if (packet->size > sizeof(packet->payload)) {
		return -EMSGSIZE;
	}

	checksum = header[0] ^ header[1] ^ header[2];
	for (uint16_t i = 0U; i < packet->size; i++) {
		ret = ai10_get_byte(data, &packet->payload[i], deadline, cancellable);
		if (ret != 0) {
			return ret;
		}

		checksum ^= packet->payload[i];
	}

	ret = ai10_get_byte(data, &byte, deadline, cancellable);
	if (ret != 0) {
		return ret;
	}

	return checksum == byte ? 0 : -EBADMSG;
}

static int ai10_reply(struct ai10_data *data, uint8_t command, int64_t deadline, bool cancellable)
{
	while (true) {
		int ret;

		ret = ai10_receive(data, deadline, cancellable);
		if (ret != 0) {
			return ret;
		}

		if (data->packet.type != AI10_REPLY) {
			continue;
		}

		if (data->packet.size < AI10_REPLY_HEADER_SIZE) {
			return -EBADMSG;
		}

		if (data->packet.payload[AI10_REPLY_COMMAND_OFFSET] == command) {
			return 0;
		}
	}
}

/* RESET acknowledgment is the protocol boundary between successive operations. */
static int ai10_reset(const struct device *dev)
{
	struct ai10_data *data = dev->data;
	int64_t delay = data->ready_at - k_uptime_get();
	k_spinlock_key_t key;
	int ret;

	if (delay > 0) {
		k_msleep(delay);
	}

	key = k_spin_lock(&data->rx_lock);
	ring_buf_reset(&data->rx);
	atomic_clear(&data->rx_error);
	k_spin_unlock(&data->rx_lock, key);

	ai10_send(dev, AI10_RESET, NULL, 0U);

	ret = ai10_reply(data, AI10_RESET, k_uptime_get() + AI10_COMMAND_MS, false);
	return ret == 0 ? ai10_status(data->packet.payload[AI10_REPLY_STATUS_OFFSET]) : ret;
}

static int ai10_timeout(k_timeout_t timeout, uint8_t *seconds)
{
	int64_t ms;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER) || timeout.ticks <= 0) {
		return -EINVAL;
	}

	ms = k_ticks_to_ms_ceil64(timeout.ticks);
	if (ms > AI10_TIMEOUT_MAX_MS) {
		return -EINVAL;
	}

	*seconds = DIV_ROUND_UP(ms, MSEC_PER_SEC);

	return 0;
}

static int ai10_identity(const struct ai10_packet *packet, uint16_t *id, uint32_t *modality)
{
	if (packet->size < AI10_ID_REPLY_MIN_SIZE) {
		return -EBADMSG;
	}

	*id = sys_get_be16(&packet->payload[AI10_REPLY_ID_OFFSET]);
	if (*id == 0U || *id > AI10_MAX_ID) {
		return -EBADMSG;
	}

	*modality = *id <= AI10_FACE_MAX_ID ? BIOMETRIC_MODALITY_FACE : BIOMETRIC_MODALITY_PALM;

	return 0;
}

static void ai10_emit(const struct device *dev, const struct biometric_event *event)
{
	struct ai10_data *data = dev->data;

	data->callback(dev, event, data->user_data);
}

static int ai10_identify(const struct device *dev)
{
	struct ai10_data *data = dev->data;
	uint8_t params[AI10_VERIFY_PARAMS_SIZE] = {
		[AI10_VERIFY_MODE_OFFSET] =
			data->continuous ? AI10_VERIFY_CONTINUOUS : AI10_VERIFY_SINGLE,
		[AI10_VERIFY_TIMEOUT_OFFSET] = data->timeout_s,
	};
	int64_t deadline = INT64_MAX;
	int ret;

	if (!data->continuous) {
		deadline = k_uptime_get() + data->timeout_s * MSEC_PER_SEC + AI10_COMMAND_MS;
	}

	ai10_send(dev, AI10_VERIFY, params, sizeof(params));

	while (true) {
		struct biometric_event event = {0};
		uint8_t status;

		ret = ai10_reply(data, AI10_VERIFY, deadline, true);
		if (ret != 0) {
			return ret;
		}

		status = data->packet.payload[AI10_REPLY_STATUS_OFFSET];
		event.status = ai10_status(status);
		if (event.status == 0) {
			if (data->packet.size < AI10_VERIFY_REPLY_MIN_SIZE) {
				return -EBADMSG;
			}

			event.type = BIOMETRIC_EVENT_MATCH;
			ret = ai10_identity(&data->packet, &event.match.template_id,
					    &event.match.modality);
			if (ret != 0) {
				return ret;
			}
		} else if (status == AI10_STATUS_UNKNOWN_USER ||
			   status == AI10_STATUS_LIVENESS_FAILED || status == AI10_STATUS_TIMEOUT) {
			event.type = BIOMETRIC_EVENT_NO_MATCH;
		} else {
			return event.status;
		}

		ai10_emit(dev, &event);

		if (!data->continuous) {
			return 0;
		}
	}
}

static int ai10_enroll(const struct device *dev)
{
	struct biometric_event event = {.type = BIOMETRIC_EVENT_ENROLL_COMPLETE};
	/* Normal user, empty name, automatic capture, timeout in seconds. */
	uint8_t params[AI10_ENROLL_PARAMS_SIZE] = {0};
	struct ai10_data *data = dev->data;
	int ret;

	params[AI10_ENROLL_TIMEOUT_OFFSET] = data->timeout_s;
	ai10_send(dev, AI10_ENROLL, params, sizeof(params));

	ret = ai10_reply(data, AI10_ENROLL,
			 k_uptime_get() + data->timeout_s * MSEC_PER_SEC + AI10_COMMAND_MS, true);
	if (ret != 0) {
		return ret;
	}

	ret = ai10_status(data->packet.payload[AI10_REPLY_STATUS_OFFSET]);
	if (ret != 0) {
		return ret;
	}

	if (data->packet.size < AI10_ENROLL_REPLY_MIN_SIZE) {
		return -EBADMSG;
	}

	ret = ai10_identity(&data->packet, &event.enrollment.template_id,
			    &event.enrollment.modality);
	if (ret == 0) {
		ai10_emit(dev, &event);
	}

	return ret;
}

static void ai10_thread(void *arg, void *unused1, void *unused2)
{
	const struct device *dev = arg;
	struct ai10_data *data = dev->data;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		struct biometric_event event = {0};
		k_spinlock_key_t key;
		int ret;

		ret = k_sem_take(&data->request, K_FOREVER);
		if (ret != 0) {
			continue;
		}

		ret = ai10_reset(dev);
		if (ret == 0) {
			if (atomic_get(&data->cancel) != 0) {
				ret = -ECANCELED;
			} else if (atomic_get(&data->operation) == AI10_IDENTIFY) {
				ret = ai10_identify(dev);
			} else {
				ret = ai10_enroll(dev);
			}
		}

		data->stop_result = ai10_reset(dev);
		if (data->stop_result != 0) {
			ret = data->stop_result;
		}

		event.status = ret;

		if (ret != 0 && !(ret == -ECANCELED && atomic_get(&data->cancel) != 0)) {
			event.type = BIOMETRIC_EVENT_ERROR;
			ai10_emit(dev, &event);
		}

		event.type = BIOMETRIC_EVENT_STOPPED;
		ai10_emit(dev, &event);

		key = k_spin_lock(&data->state_lock);
		atomic_set(&data->operation, AI10_IDLE);
		k_sem_give(&data->done);
		k_spin_unlock(&data->state_lock, key);
	}
}

static int ai10_get_capabilities(const struct device *dev, struct biometric_capabilities *caps)
{
	ARG_UNUSED(dev);

	caps->supported_modalities = BIOMETRIC_MODALITY_FACE | BIOMETRIC_MODALITY_PALM;
	caps->max_templates = AI10_MAX_ID;
	caps->template_size = 0U;
	caps->storage_modes = BIOMETRIC_STORAGE_DEVICE;
	caps->enrollment_samples_required = 0U;
	caps->async_operations = BIOMETRIC_ASYNC_IDENTIFY | BIOMETRIC_ASYNC_ENROLL;

	return 0;
}

static int ai10_callback_set(const struct device *dev, biometric_event_callback_t callback,
			     void *user_data)
{
	struct ai10_data *data = dev->data;
	int ret;

	ret = ai10_lock_idle(dev);
	if (ret == 0) {
		data->callback = callback;
		data->user_data = user_data;
		k_mutex_unlock(&data->lock);
	}

	return ret;
}

static int ai10_start(const struct device *dev, k_timeout_t timeout, enum ai10_operation operation,
		      bool continuous)
{
	struct ai10_data *data = dev->data;
	uint8_t seconds;
	int ret;

	ret = ai10_timeout(timeout, &seconds);
	if (ret != 0) {
		return ret;
	}

	ret = ai10_lock_idle(dev);
	if (ret != 0) {
		return ret;
	}

	if (data->callback == NULL) {
		ret = -EINVAL;
	} else {
		k_spinlock_key_t key = k_spin_lock(&data->state_lock);

		data->timeout_s = seconds;
		data->continuous = continuous;
		atomic_clear(&data->cancel);
		k_sem_reset(&data->done);
		atomic_set(&data->operation, operation);
		k_spin_unlock(&data->state_lock, key);
		k_sem_give(&data->request);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int ai10_match_async(const struct device *dev, enum biometric_match_mode mode,
			    uint16_t template_id, bool continuous, k_timeout_t timeout)
{
	ARG_UNUSED(template_id);

	if (mode != BIOMETRIC_MATCH_IDENTIFY) {
		return mode == BIOMETRIC_MATCH_VERIFY ? -ENOTSUP : -EINVAL;
	}

	return ai10_start(dev, timeout, AI10_IDENTIFY, continuous);
}

static int ai10_enroll_async(const struct device *dev, uint16_t template_id, k_timeout_t timeout)
{
	if (template_id != BIOMETRIC_ID_AUTO) {
		return -ENOTSUP;
	}

	return ai10_start(dev, timeout, AI10_ENROLLING, false);
}

static int ai10_async_stop(const struct device *dev)
{
	struct ai10_data *data = dev->data;
	int ret;

	ret = ai10_lock(dev);
	if (ret != 0) {
		return ret;
	}

	if (atomic_get(&data->operation) == AI10_IDLE) {
		ret = -EALREADY;
	} else {
		atomic_set(&data->cancel, 1);
		k_sem_give(&data->rx_ready);

		ret = k_sem_take(&data->done, K_FOREVER);
		if (ret == 0) {
			ret = data->stop_result;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int ai10_delete_command(const struct device *dev, uint8_t command, const uint8_t *params,
			       uint16_t size)
{
	struct ai10_data *data = dev->data;
	int ret;

	ret = ai10_lock_idle(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ai10_reset(dev);
	if (ret == 0) {
		ai10_send(dev, command, params, size);

		ret = ai10_reply(data, command, k_uptime_get() + AI10_COMMAND_MS, false);
		if (ret == 0) {
			ret = ai10_status(data->packet.payload[AI10_REPLY_STATUS_OFFSET]);
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int ai10_template_delete(const struct device *dev, uint16_t id)
{
	uint8_t params[2];

	if (id == 0U || id > AI10_MAX_ID) {
		return -EINVAL;
	}

	sys_put_be16(id, params);

	return ai10_delete_command(dev, AI10_DELETE, params, sizeof(params));
}

static int ai10_template_delete_all(const struct device *dev)
{
	return ai10_delete_command(dev, AI10_DELETE_ALL, NULL, 0U);
}

static int ai10_template_list(const struct device *dev, uint16_t *ids, size_t max_count,
			      size_t *actual_count)
{
	struct ai10_data *data = dev->data;
	uint8_t param = 0U;
	size_t total = 0U;
	int64_t deadline;
	int ret;

	ret = ai10_lock_idle(dev);
	if (ret != 0) {
		return ret;
	}

	*actual_count = 0U;
	ret = ai10_reset(dev);
	if (ret != 0) {
		goto out;
	}

	ai10_send(dev, AI10_LIST, &param, sizeof(param));
	deadline = k_uptime_get() + AI10_COMMAND_MS;

	do {
		uint8_t count;

		ret = ai10_reply(data, AI10_LIST, deadline, false);
		if (ret != 0) {
			break;
		}

		ret = ai10_status(data->packet.payload[AI10_REPLY_STATUS_OFFSET]);
		if (ret != 0) {
			break;
		}

		if (data->packet.size < AI10_LIST_REPLY_HEADER_SIZE) {
			ret = -EBADMSG;
			break;
		}

		count = data->packet.payload[AI10_LIST_COUNT_OFFSET];

		if (count > AI10_LIST_IDS_PER_PACKET ||
		    data->packet.size != AI10_LIST_REPLY_HEADER_SIZE + sizeof(uint16_t) * count ||
		    total + count > AI10_MAX_ID) {
			ret = -EBADMSG;
			break;
		}

		for (uint8_t i = 0U; i < count; i++) {
			uint16_t id = sys_get_be16(
				&data->packet.payload[AI10_LIST_IDS_OFFSET + sizeof(uint16_t) * i]);

			if (id == 0U || id > AI10_MAX_ID) {
				ret = -EBADMSG;
				goto out;
			}

			if (total < max_count) {
				ids[total] = id;
				(*actual_count)++;
			}

			total++;
		}

		if (count < AI10_LIST_IDS_PER_PACKET) {
			ret = total > max_count ? -ENOMEM : 0;
			break;
		}
	} while (true);
out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int ai10_match(const struct device *dev, enum biometric_match_mode mode,
		      uint16_t template_id, k_timeout_t timeout,
		      struct biometric_match_result *result)
{
	uint8_t params[AI10_VERIFY_PARAMS_SIZE] = {
		[AI10_VERIFY_MODE_OFFSET] = AI10_VERIFY_SINGLE,
	};
	struct ai10_data *data = dev->data;
	uint32_t modality;
	uint16_t id;
	int ret;

	ARG_UNUSED(template_id);

	if (mode != BIOMETRIC_MATCH_IDENTIFY) {
		return mode == BIOMETRIC_MATCH_VERIFY ? -ENOTSUP : -EINVAL;
	}

	ret = ai10_timeout(timeout, &params[AI10_VERIFY_TIMEOUT_OFFSET]);
	if (ret != 0) {
		return ret;
	}

	ret = ai10_lock_idle(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ai10_reset(dev);
	if (ret == 0) {
		int cleanup;

		ai10_send(dev, AI10_VERIFY, params, sizeof(params));
		ret = ai10_reply(data, AI10_VERIFY,
				 k_uptime_get() +
					 params[AI10_VERIFY_TIMEOUT_OFFSET] * MSEC_PER_SEC +
					 AI10_COMMAND_MS,
				 false);
		if (ret == 0) {
			ret = ai10_status(data->packet.payload[AI10_REPLY_STATUS_OFFSET]);
		}

		if (ret == 0) {
			ret = data->packet.size < AI10_VERIFY_REPLY_MIN_SIZE
				      ? -EBADMSG
				      : ai10_identity(&data->packet, &id, &modality);
		}

		if (ret == 0 && result != NULL) {
			*result = (struct biometric_match_result){
				.template_id = id,
				.modality = modality,
			};
		}

		cleanup = ai10_reset(dev);
		if (cleanup != 0) {
			ret = cleanup;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(biometric, ai10_api) = {
	.get_capabilities = ai10_get_capabilities,
	.template_delete = ai10_template_delete,
	.template_delete_all = ai10_template_delete_all,
	.template_list = ai10_template_list,
	.match = ai10_match,
	.callback_set = ai10_callback_set,
	.match_async = ai10_match_async,
	.enroll_async = ai10_enroll_async,
	.async_stop = ai10_async_stop,
};

static int ai10_init(const struct device *dev)
{
	const struct ai10_config *cfg = dev->config;
	struct ai10_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->uart)) {
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	k_sem_init(&data->rx_ready, 0U, 1U);
	k_sem_init(&data->request, 0U, 1U);
	k_sem_init(&data->done, 0U, 1U);
	ring_buf_init(&data->rx, sizeof(data->rx_buffer), data->rx_buffer);
	data->ready_at = k_uptime_get() + cfg->startup_delay_ms;

	ret = uart_irq_callback_user_data_set(cfg->uart, ai10_uart_callback, data);
	if (ret != 0) {
		return ret;
	}

	uart_irq_rx_enable(cfg->uart);

	k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack), ai10_thread,
			(void *)dev, NULL, NULL, CONFIG_BIOMETRICS_AI10_THREAD_PRIORITY, 0,
			K_NO_WAIT);

	return 0;
}

#define AI10_DEFINE(inst)                                                                          \
	static struct ai10_data ai10_data_##inst;                                                  \
	static const struct ai10_config ai10_config_##inst = {                                     \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.startup_delay_ms = DT_INST_PROP(inst, startup_delay_ms),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ai10_init, NULL, &ai10_data_##inst, &ai10_config_##inst,       \
			      POST_KERNEL, CONFIG_BIOMETRICS_INIT_PRIORITY, &ai10_api);

DT_INST_FOREACH_STATUS_OKAY(AI10_DEFINE)
