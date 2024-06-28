/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/pipe.h>

#define PIPE_EVENT_OPENED_BIT        BIT(0)
#define PIPE_EVENT_CLOSED_BIT        BIT(1)
#define PIPE_EVENT_RECEIVE_READY_BIT BIT(2)
#define PIPE_EVENT_TRANSMIT_IDLE_BIT BIT(3)

static void pipe_set_callback(struct modem_pipe *pipe,
			      modem_pipe_api_callback callback,
			      void *user_data)
{
	K_SPINLOCK(&pipe->spinlock) {
		pipe->callback = callback;
		pipe->user_data = user_data;
	}
}

static void pipe_call_callback(struct modem_pipe *pipe, enum modem_pipe_event event)
{
	K_SPINLOCK(&pipe->spinlock) {
		if (pipe->callback != NULL) {
			pipe->callback(pipe, event, pipe->user_data);
		}
	}
}

static uint32_t pipe_test_events(struct modem_pipe *pipe, uint32_t events)
{
	return k_event_test(&pipe->event, events);
}

static uint32_t pipe_await_events(struct modem_pipe *pipe, uint32_t events)
{
	return k_event_wait(&pipe->event, events, false, K_MSEC(10000));
}

static void pipe_post_events(struct modem_pipe *pipe, uint32_t events)
{
	k_event_post(&pipe->event, events);
}

static void pipe_clear_events(struct modem_pipe *pipe, uint32_t events)
{
	k_event_clear(&pipe->event, events);
}

static void pipe_set_events(struct modem_pipe *pipe, uint32_t events)
{
	k_event_set(&pipe->event, events);
}

static int pipe_call_open(struct modem_pipe *pipe)
{
	return pipe->api->open(pipe->data);
}

static int pipe_call_transmit(struct modem_pipe *pipe, const uint8_t *buf, size_t size)
{
	return pipe->api->transmit(pipe->data, buf, size);
}

static int pipe_call_receive(struct modem_pipe *pipe, uint8_t *buf, size_t size)
{
	return pipe->api->receive(pipe->data, buf, size);
}

static int pipe_call_close(struct modem_pipe *pipe)
{
	return pipe->api->close(pipe->data);
}

void modem_pipe_init(struct modem_pipe *pipe, void *data, const struct modem_pipe_api *api)
{
	__ASSERT_NO_MSG(pipe != NULL);
	__ASSERT_NO_MSG(data != NULL);
	__ASSERT_NO_MSG(api != NULL);

	pipe->data = data;
	pipe->api = api;
	pipe->callback = NULL;
	pipe->user_data = NULL;
	k_event_init(&pipe->event);
}

int modem_pipe_open(struct modem_pipe *pipe)
{
	int ret;

	if (pipe_test_events(pipe, PIPE_EVENT_OPENED_BIT)) {
		return 0;
	}

	ret = pipe_call_open(pipe);
	if (ret < 0) {
		return ret;
	}

	if (!pipe_await_events(pipe, PIPE_EVENT_OPENED_BIT)) {
		return -EAGAIN;
	}

	return 0;
}

int modem_pipe_open_async(struct modem_pipe *pipe)
{
	if (pipe_test_events(pipe, PIPE_EVENT_OPENED_BIT)) {
		pipe_call_callback(pipe, MODEM_PIPE_EVENT_OPENED);
		return 0;
	}

	return pipe_call_open(pipe);
}

void modem_pipe_attach(struct modem_pipe *pipe, modem_pipe_api_callback callback, void *user_data)
{
	pipe_set_callback(pipe, callback, user_data);

	if (pipe_test_events(pipe, PIPE_EVENT_RECEIVE_READY_BIT)) {
		pipe_call_callback(pipe, MODEM_PIPE_EVENT_RECEIVE_READY);
	}

	if (pipe_test_events(pipe, PIPE_EVENT_TRANSMIT_IDLE_BIT)) {
		pipe_call_callback(pipe, MODEM_PIPE_EVENT_TRANSMIT_IDLE);
	}
}

int modem_pipe_transmit(struct modem_pipe *pipe, const uint8_t *buf, size_t size)
{
	if (!pipe_test_events(pipe, PIPE_EVENT_OPENED_BIT)) {
		return -EPERM;
	}

	pipe_clear_events(pipe, PIPE_EVENT_TRANSMIT_IDLE_BIT);
	return pipe_call_transmit(pipe, buf, size);
}

int modem_pipe_receive(struct modem_pipe *pipe, uint8_t *buf, size_t size)
{
	if (!pipe_test_events(pipe, PIPE_EVENT_OPENED_BIT)) {
		return -EPERM;
	}

	pipe_clear_events(pipe, PIPE_EVENT_RECEIVE_READY_BIT);
	return pipe_call_receive(pipe, buf, size);
}

void modem_pipe_release(struct modem_pipe *pipe)
{
	pipe_set_callback(pipe, NULL, NULL);
}

int modem_pipe_close(struct modem_pipe *pipe)
{
	int ret;

	if (pipe_test_events(pipe, PIPE_EVENT_CLOSED_BIT)) {
		return 0;
	}

	ret = pipe_call_close(pipe);
	if (ret < 0) {
		return ret;
	}

	if (!pipe_await_events(pipe, PIPE_EVENT_CLOSED_BIT)) {
		return -EAGAIN;
	}

	return 0;
}

int modem_pipe_close_async(struct modem_pipe *pipe)
{
	if (pipe_test_events(pipe, PIPE_EVENT_CLOSED_BIT)) {
		pipe_call_callback(pipe, MODEM_PIPE_EVENT_CLOSED);
		return 0;
	}

	return pipe_call_close(pipe);
}

void modem_pipe_notify_opened(struct modem_pipe *pipe)
{
	pipe_set_events(pipe, PIPE_EVENT_OPENED_BIT | PIPE_EVENT_TRANSMIT_IDLE_BIT);
	pipe_call_callback(pipe, MODEM_PIPE_EVENT_OPENED);
	pipe_call_callback(pipe, MODEM_PIPE_EVENT_TRANSMIT_IDLE);
}

void modem_pipe_notify_closed(struct modem_pipe *pipe)
{
	pipe_set_events(pipe, PIPE_EVENT_TRANSMIT_IDLE_BIT | PIPE_EVENT_CLOSED_BIT);
	pipe_call_callback(pipe, MODEM_PIPE_EVENT_CLOSED);
}

void modem_pipe_notify_receive_ready(struct modem_pipe *pipe)
{
	pipe_post_events(pipe, PIPE_EVENT_RECEIVE_READY_BIT);
	pipe_call_callback(pipe, MODEM_PIPE_EVENT_RECEIVE_READY);
}

void modem_pipe_notify_transmit_idle(struct modem_pipe *pipe)
{
	pipe_post_events(pipe, PIPE_EVENT_TRANSMIT_IDLE_BIT);
	pipe_call_callback(pipe, MODEM_PIPE_EVENT_TRANSMIT_IDLE);
}
