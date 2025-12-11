/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_api.h>

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/llext/symbol.h>

#include <string.h>

#define MAX_SUBSCRIBERS 64
#define SUBSCRIBER_THREAD_STACK_SIZE 1024
#define SUBSCRIBER_THREAD_PRIORITY K_PRIO_PREEMPT(1)

ZBUS_CHAN_DEFINE(tick_chan,
		struct channel_tick_data,
		NULL,
		NULL,
		ZBUS_OBSERVERS(default_sub),
		ZBUS_MSG_INIT(.l = 0));

ZBUS_SUBSCRIBER_DEFINE(default_sub, 4);

K_THREAD_STACK_DEFINE(subscriber_thread_stack, SUBSCRIBER_THREAD_STACK_SIZE);
static struct k_thread subscriber_thread;

static struct subs {
	struct {
		k_tid_t thread;
		struct k_event *evt;
	} subscribers[MAX_SUBSCRIBERS];
	struct k_mutex subscribers_mtx;
	int subscribers_count;
	const struct zbus_channel *chan;
} channel_subscribers[] = {
	/* Empty one first, so no channel is zero (first item on enum == 1) */
	{{ }},
	{ .chan = &tick_chan },
	{{ }}
};

static int remove_subscriber(k_tid_t thread, struct subs *sus)
{
	int i;

	for (i = 0; i < sus->subscribers_count; i++) {
		if (sus->subscribers[i].thread == thread) {
			sus->subscribers[i].thread = NULL;
			sus->subscribers[i].evt = NULL;
			break;
		}
	}

	if (i == sus->subscribers_count) {
		return -ENOENT;
	}

	/* Move all entries after excluded one, to keep things tidy */
	memmove(&sus->subscribers[i], &sus->subscribers[i + 1],
			(sus->subscribers_count - i - 1) *
			sizeof(sus->subscribers[0]));
	sus->subscribers_count--;

	return 0;
}

static int add_subscriber(k_tid_t thread, struct subs *sus,
			  struct k_event *evt)
{
	if (sus->subscribers_count >= MAX_SUBSCRIBERS) {
		return -ENOMEM;
	}

	sus->subscribers[sus->subscribers_count].thread = thread;
	sus->subscribers[sus->subscribers_count].evt = evt;
	sus->subscribers_count++;

	printk("[app]Thread [%p] registered event [%p]\n", thread, evt);
	return 0;
}

static void notify_subscribers(enum Channels channel)
{
	int i;
	struct subs *subs = &channel_subscribers[channel];

	for (i = 0; i < subs->subscribers_count; i++) {
		k_event_post(subs->subscribers[i].evt, channel);
	}
}

static void subscriber_thread_fn(void *p0, void *p1, void *p2)
{
	const struct zbus_channel *chan;
	int i;

	printk("[app]Subscriber thread [%p] started.\n", k_current_get());
	while (zbus_sub_wait(&default_sub, &chan, K_FOREVER) == 0) {
		printk("[app][subscriber_thread]Got channel %s\n",
		       zbus_chan_name(chan));

		for (i = 0; i < CHAN_LAST; i++) {
			if (channel_subscribers[i].chan == chan) {
				notify_subscribers((enum Channels)i);
				break;
			}
		}
	}
}

k_tid_t start_subscriber_thread(void)
{
	return k_thread_create(&subscriber_thread, subscriber_thread_stack,
			       SUBSCRIBER_THREAD_STACK_SIZE,
			       subscriber_thread_fn, NULL, NULL, NULL,
			       SUBSCRIBER_THREAD_PRIORITY, 0, K_NO_WAIT);
}

int z_impl_publish(enum Channels channel, void *data, size_t data_len)
{
	const struct zbus_channel *chan = channel_subscribers[channel].chan;

	if (channel == CHAN_LAST) {
		return -EINVAL;
	}

	return zbus_chan_pub(chan, data, K_NO_WAIT);
}
EXPORT_SYMBOL(z_impl_publish);

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_publish(enum Channels channel, void *data,
				 size_t data_len)
{
	int ret;
	void *copy_data;

	copy_data = k_usermode_alloc_from_copy(data, data_len);
	if (copy_data == NULL) {
		return -EINVAL;
	}

	ret = z_impl_publish(channel, copy_data, data_len);

	k_free(copy_data);

	return ret;
}
#include <zephyr/syscalls/publish_mrsh.c>
#endif

int z_impl_receive(enum Channels channel, void *data, size_t data_len)
{
	size_t msg_size;

	if (channel == CHAN_LAST || data == NULL) {
		return -EINVAL;
	}

	msg_size = channel_subscribers[channel].chan->message_size;
	if (data_len < msg_size) {
		return -EINVAL;
	}

	return zbus_chan_read(channel_subscribers[channel].chan, data,
			      K_NO_WAIT);
}
EXPORT_SYMBOL(z_impl_receive);

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_receive(enum Channels channel, void *data,
				 size_t data_len)
{
	if (K_SYSCALL_MEMORY_WRITE(data, data_len)) {
		return -EINVAL;
	}

	return z_impl_receive(channel, data, data_len);
}
#include <zephyr/syscalls/receive_mrsh.c>
#endif

int z_impl_register_subscriber(enum Channels channel, struct k_event *evt)
{
	struct subs *subs = &channel_subscribers[channel];
	int ret;

	if (channel == CHAN_LAST) {
		return -EINVAL;
	}

	k_mutex_lock(&subs->subscribers_mtx, K_FOREVER);

	if (evt == NULL) {
		ret = remove_subscriber(k_current_get(), subs);
	} else {
		ret = add_subscriber(k_current_get(), subs, evt);
	}

	k_mutex_unlock(&subs->subscribers_mtx);

	return ret;
}
EXPORT_SYMBOL(z_impl_register_subscriber);

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_register_subscriber(enum Channels channel,
		struct k_event *evt)
{
	if (K_SYSCALL_OBJ(evt, K_OBJ_EVENT)) {
		return -EINVAL;
	}

	return z_impl_register_subscriber(channel, evt);
}
#include <zephyr/syscalls/register_subscriber_mrsh.c>
#endif
