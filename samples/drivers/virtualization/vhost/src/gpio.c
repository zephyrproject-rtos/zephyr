/*
 * Copyright (c) 2025 Omori Fuma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/vhost.h>
#include <zephyr/drivers/vhost/vringh.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

/* Possible values of the status field */
#define VIRTIO_GPIO_STATUS_OK  0x0
#define VIRTIO_GPIO_STATUS_ERR 0x1

/* GPIO message types */
#define VIRTIO_GPIO_MSG_GET_LINE_NAMES 0x0001
#define VIRTIO_GPIO_MSG_GET_DIRECTION  0x0002
#define VIRTIO_GPIO_MSG_SET_DIRECTION  0x0003
#define VIRTIO_GPIO_MSG_GET_VALUE      0x0004
#define VIRTIO_GPIO_MSG_SET_VALUE      0x0005
#define VIRTIO_GPIO_MSG_SET_IRQ_TYPE   0x0006

/* GPIO Direction types */
#define VIRTIO_GPIO_DIRECTION_NONE 0x00
#define VIRTIO_GPIO_DIRECTION_OUT  0x01
#define VIRTIO_GPIO_DIRECTION_IN   0x02

/* GPIO interrupt types */
#define VIRTIO_GPIO_IRQ_TYPE_NONE         0x00
#define VIRTIO_GPIO_IRQ_TYPE_EDGE_RISING  0x01
#define VIRTIO_GPIO_IRQ_TYPE_EDGE_FALLING 0x02
#define VIRTIO_GPIO_IRQ_TYPE_EDGE_BOTH    0x03
#define VIRTIO_GPIO_IRQ_TYPE_LEVEL_HIGH   0x04
#define VIRTIO_GPIO_IRQ_TYPE_LEVEL_LOW    0x08

#define GPIO0_NODE DT_NODELABEL(gpio0)

LOG_MODULE_REGISTER(vhost_gpio);

static struct vhost_iovec riovec[16];
static struct vhost_iovec wiovec[16];

static struct vringh_iov riov = {
	.iov = riovec,
	.max_num = 16,
};

static struct vringh_iov wiov = {
	.iov = wiovec,
	.max_num = 16,
};

struct virtio_gpio_request {
	uint16_t type;
	uint16_t gpio;
	uint32_t value;
};

struct virtio_gpio_response {
	uint8_t status;
	uint8_t value[];
};

static struct vringh vrh_inst;

static const uint8_t pin_names[] = {
	CONFIG_GPIO_LINE_NAME_0 "\0"
	CONFIG_GPIO_LINE_NAME_1 "\0"
	CONFIG_GPIO_LINE_NAME_2 "\0"
	CONFIG_GPIO_LINE_NAME_3 "\0"
	CONFIG_GPIO_LINE_NAME_4 "\0"
	CONFIG_GPIO_LINE_NAME_5 "\0"
	CONFIG_GPIO_LINE_NAME_6 "\0"
	CONFIG_GPIO_LINE_NAME_7 "\0"
	CONFIG_GPIO_LINE_NAME_8 "\0"
	CONFIG_GPIO_LINE_NAME_9 "\0"
	CONFIG_GPIO_LINE_NAME_10 "\0"
	CONFIG_GPIO_LINE_NAME_11 "\0"
	CONFIG_GPIO_LINE_NAME_12 "\0"
	CONFIG_GPIO_LINE_NAME_13 "\0"
	CONFIG_GPIO_LINE_NAME_14 "\0"
	CONFIG_GPIO_LINE_NAME_15 "\0"
	CONFIG_GPIO_LINE_NAME_16 "\0"
	CONFIG_GPIO_LINE_NAME_17 "\0"
	CONFIG_GPIO_LINE_NAME_18 "\0"
	CONFIG_GPIO_LINE_NAME_19 "\0"
	CONFIG_GPIO_LINE_NAME_20 "\0"
	CONFIG_GPIO_LINE_NAME_21 "\0"
	CONFIG_GPIO_LINE_NAME_22 "\0"
	CONFIG_GPIO_LINE_NAME_23 "\0"
	CONFIG_GPIO_LINE_NAME_24 "\0"
	CONFIG_GPIO_LINE_NAME_25 "\0"
	CONFIG_GPIO_LINE_NAME_26 "\0"
	CONFIG_GPIO_LINE_NAME_27 "\0"
	CONFIG_GPIO_LINE_NAME_28 "\0"
	CONFIG_GPIO_LINE_NAME_29 "\0"
	CONFIG_GPIO_LINE_NAME_30 "\0"
	CONFIG_GPIO_LINE_NAME_31 "\0"
};

static void vringh_kick_handler(struct vringh *vrh)
{
	LOG_DBG("%s: queue_id=%lu", __func__, vrh->queue_id);
	const struct device *const dev = DEVICE_DT_GET(GPIO0_NODE);
	uint16_t head;

	if (!device_is_ready(dev)) {
		LOG_ERR("device %s is not ready", dev->name);
		return;
	}

	while (true) {
		int ret = vringh_getdesc(vrh, &riov, &wiov, &head);
		uint32_t total_len = 0;

		if (ret < 0) {
			LOG_ERR("vringh_getdesc failed: %d", ret);
			return;
		}

		if (ret == 0) {
			return;
		}

		for (uint32_t s = 0; s < riov.used; s++) {
			struct virtio_gpio_response *resp = wiov.iov[s].iov_base;
			const size_t payload_len = wiov.iov[s].iov_len - 1;
			struct virtio_gpio_request req;
			mem_addr_t addr_base = (mem_addr_t)riov.iov[s].iov_base;
			gpio_flags_t flags = 0;

			req.type = sys_read16(addr_base + 0);
			req.gpio = sys_read16(addr_base + 2);
			req.value = sys_read32(addr_base + 4);

			switch (req.type) {
			case VIRTIO_GPIO_MSG_GET_LINE_NAMES:
				const size_t names_len = MIN(payload_len, sizeof(pin_names));

				if (payload_len < sizeof(pin_names)) {
					LOG_WRN("Too short buffer to copy LINE_NAMES.");
				}

				memset(resp->value, 0, payload_len);
				memcpy(resp->value, pin_names, names_len);
				resp->status = VIRTIO_GPIO_STATUS_OK;

				break;
			case VIRTIO_GPIO_MSG_GET_DIRECTION:
				ret = gpio_pin_get_config(dev, req.gpio, &flags);

				if (ret < 0) {
					LOG_ERR("gpio_pin_get_config failed: %d", ret);

					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}
				flags &= GPIO_DIR_MASK;

				switch (flags) {
				case GPIO_DISCONNECTED:
					resp->status = VIRTIO_GPIO_STATUS_OK;
					resp->value[0] = VIRTIO_GPIO_DIRECTION_NONE;
					break;
				case GPIO_OUTPUT:
					resp->status = VIRTIO_GPIO_STATUS_OK;
					resp->value[0] = VIRTIO_GPIO_DIRECTION_OUT;
					break;
				case GPIO_INPUT:
					resp->status = VIRTIO_GPIO_STATUS_OK;
					resp->value[0] = VIRTIO_GPIO_DIRECTION_IN;
					break;
				default:
					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}
				break;
			case VIRTIO_GPIO_MSG_SET_DIRECTION:
				if (req.value > VIRTIO_GPIO_DIRECTION_IN) {
					LOG_ERR("Unexpeded value (req.value): %d", req.value);
					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}

				ret = gpio_pin_get_config(dev, req.gpio, &flags);
				if (ret < 0) {
					LOG_ERR("gpio_pin_get_config failed: %d", ret);
					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}

				flags &= ~GPIO_DIR_MASK;

				switch (req.value) {
				case VIRTIO_GPIO_DIRECTION_NONE:
					flags |= GPIO_DISCONNECTED;
					break;
				case VIRTIO_GPIO_DIRECTION_OUT:
					flags |= GPIO_OUTPUT;
					break;
				case VIRTIO_GPIO_DIRECTION_IN:
					flags |= GPIO_INPUT;
					break;
					/* Invalid values are already cheched */
				}

				ret = gpio_pin_configure(dev, req.gpio, flags);
				if (ret < 0) {
					LOG_ERR("gpio_pin_config failed: %d", ret);
					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}

				resp->status = VIRTIO_GPIO_STATUS_OK;
				resp->value[0] = 0;
				break;
			case VIRTIO_GPIO_MSG_GET_VALUE:
				ret = gpio_pin_get(dev, req.gpio);
				if (ret < 0) {
					LOG_ERR("gpio_pin_get failed: %d", ret);
					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}

				resp->status = VIRTIO_GPIO_STATUS_OK;
				resp->value[0] = ret;
				break;
			case VIRTIO_GPIO_MSG_SET_VALUE:
				ret = gpio_pin_set(dev, req.gpio, req.value);
				if (ret < 0) {
					LOG_ERR("gpio_pin_set failed: %d", ret);
					resp->status = VIRTIO_GPIO_STATUS_ERR;
					resp->value[0] = 0;
					break;
				}

				resp->status = VIRTIO_GPIO_STATUS_OK;
				resp->value[0] = 0;
				break;
			case VIRTIO_GPIO_MSG_SET_IRQ_TYPE:
				LOG_ERR("VIRTIO_GPIO_MSG_SET_IRQ_TYPE is not implemented\n");
				resp->status = VIRTIO_GPIO_STATUS_ERR;
				resp->value[0] = 0;
				break;
			}

			total_len += wiov.iov[s].iov_len;
		}

		barrier_dmem_fence_full();

		vringh_complete(vrh, head, total_len);

		if (vringh_need_notify(vrh) > 0) {
			vringh_notify(vrh);
		}

		/* Reset iovecs for next iteration */
		vringh_iov_reset(&riov);
		vringh_iov_reset(&wiov);
	}
}

void gpio_queue_ready(const struct device *dev, uint16_t qid, void *data)
{
	LOG_DBG("%s(dev=%p, qid=%u, data=%p)", __func__, dev, qid, data);

	/* Initialize iovecs before descriptor processing */
	vringh_iov_init(&riov, riov.iov, riov.max_num);
	vringh_iov_init(&wiov, wiov.iov, wiov.max_num);

	int err = vringh_init_device(&vrh_inst, dev, qid, vringh_kick_handler);

	if (err) {
		LOG_ERR("vringh_init_device failed: %d", err);
		return;
	}
}
