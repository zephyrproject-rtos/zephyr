/*
 * Copyright (c) 2022, tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "input_internal.h"

int input_internal_setup(struct input_dev *dev)
{
	int retval = 0;

	if (dev->buf == NULL) {
		return -EFAULT;
	}

	ring_buf_reset(dev->buf);

#ifndef CONFIG_ENABLE_INPUT_ISR_LOCK
	retval = k_mutex_init(&(dev->mutex));
	if (retval < 0) {
		return retval;
	}
#endif

	retval = k_sem_init(&(dev->sem), 0, 1);
	if (retval < 0) {
		return retval;
	}

	dev->readtimeo = K_FOREVER;

	return 0;
}

int input_internal_release(struct input_dev *dev)
{
	k_sem_reset(&(dev->sem));

	return 0;
}

int input_internal_attr_get(struct input_dev *dev,
			enum input_attr_type type, union input_attr_data *data)
{
	int retval = 0;

	switch (type) {
	case INPUT_ATTR_EVENT_READ_TIMEOUT:
	{
		data->timeout = dev->readtimeo;
		break;
	}

	default:
		retval = -ENOTSUP;
		break;
	}

	return retval;
}

int input_internal_attr_set(struct input_dev *dev,
			enum input_attr_type type, union input_attr_data *data)
{
	int retval = 0;

	switch (type) {
	case INPUT_ATTR_EVENT_READ_TIMEOUT:
	{
		dev->readtimeo = data->timeout;

		k_sem_give(&(dev->sem));

		break;
	}

	default:
		retval = -ENOTSUP;
		break;
	}

	return retval;
}

#ifdef CONFIG_ENABLE_INPUT_ISR_LOCK

#define INPUT_LOCK() ({ \
	key = k_spin_lock(&(dev->lock)); \
	0; \
})

#define INPUT_UNLOCK() ({ \
	k_spin_unlock(&(dev->lock), key); \
	0; \
})

#else

#define INPUT_LOCK() \
	k_mutex_lock(&(dev->mutex), K_FOREVER);

#define INPUT_UNLOCK() \
	k_mutex_unlock(&(dev->mutex));

#endif

int input_internal_event_read(struct input_dev *dev, struct input_event *event)
{
#ifdef CONFIG_ENABLE_INPUT_ISR_LOCK
	k_spinlock_key_t key;
#endif
	uint32_t size = 0;
	int retval = 0;

	if (dev->buf == NULL) {
		return -EFAULT;
	}

	INPUT_ASSERT(!k_is_in_isr(), "Can't read event in isr context.");

	retval = INPUT_LOCK();
	if (retval < 0) {
		return retval;
	}

	size = ring_buf_size_get(dev->buf);
	if (size > 0) {
		/* Ring buf size must be a multiple of input event size */
		INPUT_ASSERT(!(size % input_event_size), "Invalid ring buf size");

		size = ring_buf_get(dev->buf, (uint8_t *)event, input_event_size);
	} else if (!K_TIMEOUT_EQ(dev->readtimeo, K_NO_WAIT)) {
		do {

			INPUT_UNLOCK();

			retval = k_sem_take(&(dev->sem), dev->readtimeo);
			if (retval < 0) {
				return retval;
			}

			retval = INPUT_LOCK();
			if (retval < 0) {
				return retval;
			}

			size = ring_buf_size_get(dev->buf);
			if (size > 0) {
				/* Ring buf size must be a multiple of input event size */
				INPUT_ASSERT(!(size % input_event_size), "Invalid ring buf size");

				size = ring_buf_get(dev->buf, (uint8_t *)event, input_event_size);
				break;
			}
		} while (1);
	} else {
		retval = -EAGAIN;
	}

	INPUT_UNLOCK();

	return retval;
}

int input_event(struct input_dev *dev, uint16_t type, uint16_t code, int32_t value)
{
	struct input_event event;
	int64_t time_stamp;
#ifdef CONFIG_ENABLE_INPUT_ISR_LOCK
	k_spinlock_key_t key;
#endif
	uint32_t size = 0;
	int retval = 0;

#ifndef CONFIG_ENABLE_INPUT_ISR_LOCK
	INPUT_ASSERT(!k_is_in_isr(), "Can't report event in isr context.");
#endif

	if (dev->buf == NULL) {
		return -EFAULT;
	}

	/* capture initial time stamp */
	time_stamp = k_uptime_get();

	event.time.tv_sec  = (time_stamp / 1000);
	event.time.tv_usec = (time_stamp % 1000) * 1000;

	event.type  = type;
	event.code  = code;
	event.value = value;

	retval = INPUT_LOCK();
	if (retval < 0) {
		return retval;
	}

	size = ring_buf_space_get(dev->buf);
	if (size > 0) {
		/* Ring buf space must be a multiple of input event size */
		INPUT_ASSERT(!(size % input_event_size), "Invalid ring buf space");

		size = ring_buf_put(dev->buf, (const uint8_t *)&event, input_event_size);

		k_sem_give(&(dev->sem));
	} else {
		retval = -EAGAIN;
	}

	INPUT_UNLOCK();

	return retval;
}
