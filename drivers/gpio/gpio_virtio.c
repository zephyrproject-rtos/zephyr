/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the VIRTIO GPIO device (virtio spec 1.3, section 5.16).
 *
 * Every operation is a round trip to the virtio device, so none of the GPIO API
 * calls can be issued from an ISR, including from the interrupt callbacks this
 * driver fires. Callbacks needing to touch the port must defer the work to a
 * thread.
 */

#define DT_DRV_COMPAT virtio_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(gpio_virtio, CONFIG_GPIO_LOG_LEVEL);

#define VIRTIO_GPIO_REQUESTQ 0
#define VIRTIO_GPIO_EVENTQ   1

#define VIRTIO_GPIO_F_IRQ 0

#define VIRTIO_GPIO_MSG_GET_DIRECTION 0x0002
#define VIRTIO_GPIO_MSG_SET_DIRECTION 0x0003
#define VIRTIO_GPIO_MSG_GET_VALUE     0x0004
#define VIRTIO_GPIO_MSG_SET_VALUE     0x0005
#define VIRTIO_GPIO_MSG_IRQ_TYPE      0x0006

#define VIRTIO_GPIO_STATUS_OK 0x0

#define VIRTIO_GPIO_DIRECTION_NONE 0x00
#define VIRTIO_GPIO_DIRECTION_OUT  0x01
#define VIRTIO_GPIO_DIRECTION_IN   0x02

#define VIRTIO_GPIO_IRQ_TYPE_NONE         0x00
#define VIRTIO_GPIO_IRQ_TYPE_EDGE_RISING  0x01
#define VIRTIO_GPIO_IRQ_TYPE_EDGE_FALLING 0x02
#define VIRTIO_GPIO_IRQ_TYPE_EDGE_BOTH    0x03
#define VIRTIO_GPIO_IRQ_TYPE_LEVEL_HIGH   0x04
#define VIRTIO_GPIO_IRQ_TYPE_LEVEL_LOW    0x08

#define VIRTIO_GPIO_IRQ_STATUS_VALID 0x1

/** A Zephyr GPIO port is at most 32 lines wide, lines beyond that are ignored */
#define GPIO_VIRTIO_MAX_PINS 32

#define GPIO_VIRTIO_NPINS(inst) MIN(DT_INST_PROP(inst, ngpios), GPIO_VIRTIO_MAX_PINS)

struct virtio_gpio_config {
	uint16_t ngpio;
	uint8_t padding[2];
	uint32_t gpio_names_size;
} __packed;

struct virtio_gpio_request {
	uint16_t type;
	uint16_t gpio;
	uint32_t value;
} __packed;

struct virtio_gpio_response {
	uint8_t status;
	uint8_t value;
} __packed;

struct virtio_gpio_irq_request {
	uint16_t gpio;
} __packed;

struct virtio_gpio_irq_response {
	uint8_t status;
} __packed;

/** Per line state of the buffer chain sitting in the event virtqueue */
struct gpio_virtio_irq_line {
	const struct device *dev;
	struct virtio_gpio_irq_request req;
	struct virtio_gpio_irq_response res;
	gpio_pin_t pin;
	bool queued;
	bool enabled;
};

struct gpio_virtio_config {
	/* port_pin_mask is filled in at init from the device configuration */
	struct gpio_driver_config common;
	const struct device *vdev;
	struct gpio_virtio_irq_line *irq_lines;
	uint16_t max_pins;
};

struct gpio_virtio_data {
	struct gpio_driver_data common;
	struct virtq *requestq;
	struct virtq *eventq;
	/* serializes the single in-flight request and the state tracked below */
	struct k_mutex lock;
	struct k_sem done;
	uint32_t res_len;
	struct virtio_gpio_request req;
	struct virtio_gpio_response res;
	struct k_spinlock irq_lock;
	sys_slist_t callbacks;
	/* lines given a direction, i.e. the ones that have a value to report */
	gpio_port_pins_t configured;
	gpio_port_value_t out_state;
	uint16_t ngpio;
	bool irq_supported;
};

static void gpio_virtio_request_cb(void *opaque, uint32_t used_len)
{
	struct gpio_virtio_data *data = opaque;

	data->res_len = used_len;
	k_sem_give(&data->done);
}

/**
 * @brief Run a single request/response transaction on the request virtqueue
 *
 * @param dev virtio GPIO device
 * @param type one of the VIRTIO_GPIO_MSG_* request types
 * @param pin line the request applies to
 * @param value request payload, meaning depends on @p type
 * @param res_value where the response payload is stored, can be NULL
 * @return 0 on success or negative error code on failure
 */
static int gpio_virtio_request(const struct device *dev, uint16_t type, gpio_pin_t pin,
			       uint32_t value, uint8_t *res_value)
{
	const struct gpio_virtio_config *cfg = dev->config;
	struct gpio_virtio_data *data = dev->data;
	struct virtq_buf bufs[] = {
		{.addr = &data->req, .len = sizeof(data->req)},
		{.addr = &data->res, .len = sizeof(data->res)},
	};
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	data->req.type = sys_cpu_to_le16(type);
	data->req.gpio = sys_cpu_to_le16(pin);
	data->req.value = sys_cpu_to_le32(value);
	data->res_len = 0;

	/*
	 * The mutex keeps a single request in flight, and the transport returns
	 * the descriptors before completing it, so the queue never runs out.
	 */
	ret = virtq_add_buffer_chain(data->requestq, bufs, ARRAY_SIZE(bufs), 1,
				     gpio_virtio_request_cb, data, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to queue request %u for pin %u: %d", type, pin, ret);
		goto out;
	}

	virtio_notify_virtqueue(cfg->vdev, VIRTIO_GPIO_REQUESTQ);

	k_sem_take(&data->done, K_FOREVER);

	if (data->res_len != sizeof(data->res)) {
		LOG_ERR("response of incorrect length (%u : %u)", data->res_len,
			(uint32_t)sizeof(data->res));
		ret = -EIO;
		goto out;
	}

	if (data->res.status != VIRTIO_GPIO_STATUS_OK) {
		LOG_DBG("request %u rejected for pin %u", type, pin);
		ret = -EIO;
		goto out;
	}

	if (res_value != NULL) {
		*res_value = data->res.value;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

/* Caller must hold data->lock */
static int gpio_virtio_set_value(const struct device *dev, gpio_pin_t pin, bool value)
{
	struct gpio_virtio_data *data = dev->data;
	int ret;

	ret = gpio_virtio_request(dev, VIRTIO_GPIO_MSG_SET_VALUE, pin, value, NULL);
	if (ret == 0) {
		WRITE_BIT(data->out_state, pin, value);
	}

	return ret;
}

static int gpio_virtio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_virtio_data *data = dev->data;
	uint8_t dir;
	int ret;

	if (pin >= data->ngpio) {
		return -EINVAL;
	}

	if ((flags & (GPIO_SINGLE_ENDED | GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	/* the protocol carries a single direction per line */
	if ((flags & GPIO_INPUT) != 0 && (flags & GPIO_OUTPUT) != 0) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) != 0) {
			/* drive the requested level before enabling the output */
			ret = gpio_virtio_set_value(dev, pin, (flags & GPIO_OUTPUT_INIT_HIGH) != 0);
			if (ret != 0) {
				goto out;
			}
		}
		dir = VIRTIO_GPIO_DIRECTION_OUT;
	} else if ((flags & GPIO_INPUT) != 0) {
		dir = VIRTIO_GPIO_DIRECTION_IN;
	} else {
		dir = VIRTIO_GPIO_DIRECTION_NONE;
	}

	ret = gpio_virtio_request(dev, VIRTIO_GPIO_MSG_SET_DIRECTION, pin, dir, NULL);
	if (ret == 0) {
		WRITE_BIT(data->configured, pin, dir != VIRTIO_GPIO_DIRECTION_NONE);
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_virtio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	struct gpio_virtio_data *data = dev->data;
	gpio_port_pins_t pins;
	gpio_port_value_t val = 0;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * The protocol has no bulk read, and a line left at the default
	 * VIRTIO_GPIO_DIRECTION_NONE has no value to report, so only the lines
	 * given a direction through this driver are read back.
	 */
	pins = data->configured;

	while (pins != 0) {
		gpio_pin_t pin = find_lsb_set(pins) - 1;
		uint8_t pin_val;

		ret = gpio_virtio_request(dev, VIRTIO_GPIO_MSG_GET_VALUE, pin, 0, &pin_val);
		if (ret != 0) {
			goto out;
		}

		WRITE_BIT(val, pin, pin_val);
		pins &= ~BIT(pin);
	}

	*value = val;

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_virtio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct gpio_virtio_config *cfg = dev->config;
	struct gpio_virtio_data *data = dev->data;
	int ret = 0;

	mask &= cfg->common.port_pin_mask;

	k_mutex_lock(&data->lock, K_FOREVER);

	while (mask != 0) {
		gpio_pin_t pin = find_lsb_set(mask) - 1;

		ret = gpio_virtio_set_value(dev, pin, (value & BIT(pin)) != 0);
		if (ret != 0) {
			break;
		}

		mask &= ~BIT(pin);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_virtio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_virtio_port_set_masked_raw(dev, pins, pins);
}

static int gpio_virtio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_virtio_port_set_masked_raw(dev, pins, 0);
}

static int gpio_virtio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_virtio_data *data = dev->data;
	int ret;

	/* the outer lock keeps the shadow state consistent across the update */
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = gpio_virtio_port_set_masked_raw(dev, pins, ~data->out_state);
	k_mutex_unlock(&data->lock);

	return ret;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_virtio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_virtio_config *cfg = dev->config;
	struct gpio_virtio_data *data = dev->data;
	gpio_port_pins_t in = 0;
	gpio_port_pins_t out = 0;
	int ret = 0;

	map &= cfg->common.port_pin_mask;

	k_mutex_lock(&data->lock, K_FOREVER);

	while (map != 0) {
		gpio_pin_t pin = find_lsb_set(map) - 1;
		uint8_t dir;

		ret = gpio_virtio_request(dev, VIRTIO_GPIO_MSG_GET_DIRECTION, pin, 0, &dir);
		if (ret != 0) {
			goto out;
		}

		if (dir == VIRTIO_GPIO_DIRECTION_IN) {
			in |= BIT(pin);
		} else if (dir == VIRTIO_GPIO_DIRECTION_OUT) {
			out |= BIT(pin);
		}

		map &= ~BIT(pin);
	}

	if (inputs != NULL) {
		*inputs = in;
	}

	if (outputs != NULL) {
		*outputs = out;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static void gpio_virtio_event_cb(void *opaque, uint32_t used_len);

/* Caller must hold data->irq_lock */
static int gpio_virtio_arm_irq(const struct device *dev, struct gpio_virtio_irq_line *line)
{
	const struct gpio_virtio_config *cfg = dev->config;
	struct gpio_virtio_data *data = dev->data;
	struct virtq_buf bufs[] = {
		{.addr = &line->req, .len = sizeof(line->req)},
		{.addr = &line->res, .len = sizeof(line->res)},
	};
	int ret;

	if (line->queued) {
		return 0;
	}

	line->req.gpio = sys_cpu_to_le16(line->pin);

	ret = virtq_add_buffer_chain(data->eventq, bufs, ARRAY_SIZE(bufs), 1, gpio_virtio_event_cb,
				     line, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("failed to queue event buffer for pin %u: %d", line->pin, ret);
		return ret;
	}

	line->queued = true;
	virtio_notify_virtqueue(cfg->vdev, VIRTIO_GPIO_EVENTQ);

	return 0;
}

static void gpio_virtio_event_cb(void *opaque, uint32_t used_len)
{
	struct gpio_virtio_irq_line *line = opaque;
	const struct device *dev = line->dev;
	struct gpio_virtio_data *data = dev->data;
	k_spinlock_key_t key;
	bool fire;

	key = k_spin_lock(&data->irq_lock);
	line->queued = false;
	/*
	 * A buffer handed back with an invalid status only means the device
	 * dropped it because the interrupt got disabled, it is not an event.
	 */
	fire = line->enabled && used_len == sizeof(line->res) &&
	       line->res.status == VIRTIO_GPIO_IRQ_STATUS_VALID;
	k_spin_unlock(&data->irq_lock, key);

	if (fire) {
		gpio_fire_callbacks(&data->callbacks, dev, BIT(line->pin));
	}

	/* re-arm only once the event has been reported, to keep them ordered */
	key = k_spin_lock(&data->irq_lock);
	if (line->enabled) {
		(void)gpio_virtio_arm_irq(dev, line);
	}
	k_spin_unlock(&data->irq_lock, key);
}

static int gpio_virtio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_virtio_config *cfg = dev->config;
	struct gpio_virtio_data *data = dev->data;
	struct gpio_virtio_irq_line *line;
	k_spinlock_key_t key;
	uint8_t type;
	int ret;

	if (pin >= data->ngpio) {
		return -EINVAL;
	}

	if (!data->irq_supported) {
		return -ENOTSUP;
	}

	line = &cfg->irq_lines[pin];

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		type = VIRTIO_GPIO_IRQ_TYPE_NONE;
		break;
	case GPIO_INT_MODE_EDGE:
		switch (trig & GPIO_INT_TRIG_BOTH) {
		case GPIO_INT_TRIG_LOW:
			type = VIRTIO_GPIO_IRQ_TYPE_EDGE_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			type = VIRTIO_GPIO_IRQ_TYPE_EDGE_RISING;
			break;
		default:
			type = VIRTIO_GPIO_IRQ_TYPE_EDGE_BOTH;
			break;
		}
		break;
	case GPIO_INT_MODE_LEVEL:
		switch (trig & GPIO_INT_TRIG_BOTH) {
		case GPIO_INT_TRIG_LOW:
			type = VIRTIO_GPIO_IRQ_TYPE_LEVEL_LOW;
			break;
		case GPIO_INT_TRIG_HIGH:
			type = VIRTIO_GPIO_IRQ_TYPE_LEVEL_HIGH;
			break;
		default:
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	if (type == VIRTIO_GPIO_IRQ_TYPE_NONE) {
		/*
		 * Stop reporting events first, the buffer still queued on the
		 * device is handed back once it processes the request.
		 */
		key = k_spin_lock(&data->irq_lock);
		line->enabled = false;
		k_spin_unlock(&data->irq_lock, key);

		return gpio_virtio_request(dev, VIRTIO_GPIO_MSG_IRQ_TYPE, pin, type, NULL);
	}

	ret = gpio_virtio_request(dev, VIRTIO_GPIO_MSG_IRQ_TYPE, pin, type, NULL);
	if (ret != 0) {
		return ret;
	}

	/* the buffer may only be queued once the interrupt is enabled */
	key = k_spin_lock(&data->irq_lock);
	line->enabled = true;
	ret = gpio_virtio_arm_irq(dev, line);
	k_spin_unlock(&data->irq_lock, key);

	return ret;
}

static int gpio_virtio_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_virtio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, gpio_virtio_api) = {
	.pin_configure = gpio_virtio_pin_configure,
	.port_get_raw = gpio_virtio_port_get_raw,
	.port_set_masked_raw = gpio_virtio_port_set_masked_raw,
	.port_set_bits_raw = gpio_virtio_port_set_bits_raw,
	.port_clear_bits_raw = gpio_virtio_port_clear_bits_raw,
	.port_toggle_bits = gpio_virtio_port_toggle_bits,
	.pin_interrupt_configure = gpio_virtio_pin_interrupt_configure,
	.manage_callback = gpio_virtio_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_virtio_port_get_direction,
#endif
};

static uint16_t gpio_virtio_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *opaque)
{
	struct gpio_virtio_data *data = opaque;

	switch (q_index) {
	case VIRTIO_GPIO_REQUESTQ:
		/* a single request is in flight at a time, and takes two descriptors */
		return MIN(2, q_size_max);
	case VIRTIO_GPIO_EVENTQ:
		/* every line may have a buffer chain of its own queued up */
		return MIN(NHPOT(2 * data->ngpio), q_size_max);
	default:
		return 0;
	}
}

static int gpio_virtio_init(const struct device *dev)
{
	struct gpio_virtio_config *cfg = (struct gpio_virtio_config *)dev->config;
	struct gpio_virtio_data *data = dev->data;
	volatile struct virtio_gpio_config *devcfg;
	uint16_t ngpio;
	int ret;

	if (!device_is_ready(cfg->vdev)) {
		LOG_ERR("virtio device not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	k_sem_init(&data->done, 0, 1);

	data->irq_supported = virtio_read_device_feature_bit(cfg->vdev, VIRTIO_GPIO_F_IRQ);
	if (data->irq_supported &&
	    virtio_write_driver_feature_bit(cfg->vdev, VIRTIO_GPIO_F_IRQ, true) != 0) {
		LOG_WRN("could not enable the IRQ feature");
		data->irq_supported = false;
	}

	ret = virtio_commit_feature_bits(cfg->vdev);
	if (ret != 0) {
		LOG_ERR("virtio_commit_feature_bits failed: %d", ret);
		return ret;
	}

	devcfg = virtio_get_device_specific_config(cfg->vdev);
	if (devcfg == NULL) {
		LOG_ERR("could not get device-specific config");
		return -ENODEV;
	}

	ngpio = sys_le16_to_cpu(devcfg->ngpio);
	if (ngpio == 0) {
		LOG_ERR("device exposes no GPIO lines");
		return -ENODEV;
	}

	if (ngpio > cfg->max_pins) {
		LOG_WRN("device exposes %u lines, only the first %u are usable", ngpio,
			cfg->max_pins);
		ngpio = cfg->max_pins;
	}

	data->ngpio = ngpio;
	cfg->common.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(ngpio);

	ret = virtio_init_virtqueues(cfg->vdev, data->irq_supported ? 2 : 1,
				     gpio_virtio_enum_queues_cb, data);
	if (ret != 0) {
		LOG_ERR("virtio_init_virtqueues failed: %d", ret);
		return ret;
	}

	data->requestq = virtio_get_virtqueue(cfg->vdev, VIRTIO_GPIO_REQUESTQ);
	if (data->requestq == NULL) {
		LOG_ERR("failed to get the request virtqueue");
		return -ENODEV;
	}

	if (data->irq_supported) {
		data->eventq = virtio_get_virtqueue(cfg->vdev, VIRTIO_GPIO_EVENTQ);
		if (data->eventq == NULL) {
			LOG_ERR("failed to get the event virtqueue");
			return -ENODEV;
		}

		for (gpio_pin_t pin = 0; pin < ngpio; pin++) {
			cfg->irq_lines[pin].dev = dev;
			cfg->irq_lines[pin].pin = pin;
		}
	}

	virtio_finalize_init(cfg->vdev);

	LOG_DBG("%u lines, interrupts %s", ngpio,
		data->irq_supported ? "supported" : "unsupported");

	return 0;
}

#define GPIO_VIRTIO_DEFINE(inst)                                                                   \
	BUILD_ASSERT(GPIO_VIRTIO_NPINS(inst) > 0, "ngpios must be at least 1");                    \
	static struct gpio_virtio_irq_line gpio_virtio_irq_lines_##inst[GPIO_VIRTIO_NPINS(inst)];  \
	static struct gpio_virtio_data gpio_virtio_data_##inst;                                    \
	static struct gpio_virtio_config gpio_virtio_config_##inst = {                             \
		.vdev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                               \
		.irq_lines = gpio_virtio_irq_lines_##inst,                                         \
		.max_pins = GPIO_VIRTIO_NPINS(inst),                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_virtio_init, NULL, &gpio_virtio_data_##inst,              \
			      &gpio_virtio_config_##inst, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,  \
			      &gpio_virtio_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_VIRTIO_DEFINE)
