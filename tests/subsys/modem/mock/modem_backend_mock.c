/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "modem_backend_mock.h"

#include <string.h>

static int modem_backend_mock_open(void *data)
{
	struct modem_backend_mock *mock = (struct modem_backend_mock *)data;

	modem_pipe_notify_opened(&mock->pipe);
	return 0;
}

static bool modem_backend_mock_update(struct modem_backend_mock *mock, const uint8_t *buf,
				      size_t size)
{
	if (mock->transaction == NULL) {
		return false;
	}

	for (size_t i = 0; i < size; i++) {
		__ASSERT(buf[i] == mock->transaction->get[mock->transaction_match_cnt],
			 "Unexpected transmit data");

		mock->transaction_match_cnt++;
		if (mock->transaction_match_cnt == mock->transaction->get_size) {
			return true;
		}
	}

	return false;
}

static int modem_backend_mock_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_mock *mock = (struct modem_backend_mock *)data;
	int ret;

	size = (mock->limit < size) ? mock->limit : size;

	if (mock->bridge) {
		struct modem_backend_mock *t_mock = mock->bridge;

		ret = ring_buf_put(&t_mock->rx_rb, buf, size);
		k_work_submit(&t_mock->receive_ready_work);
		k_work_submit(&mock->transmit_idle_work);
		return ret;
	}

	ret = ring_buf_put(&mock->tx_rb, buf, size);
	if (modem_backend_mock_update(mock, buf, size)) {
		modem_backend_mock_put(mock, mock->transaction->put,
				       mock->transaction->put_size);

		mock->transaction = NULL;
	}

	k_work_submit(&mock->transmit_idle_work);
	return ret;
}

static int modem_backend_mock_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_mock *mock = (struct modem_backend_mock *)data;

	size = (mock->limit < size) ? mock->limit : size;
	return ring_buf_get(&mock->rx_rb, buf, size);
}

static int modem_backend_mock_close(void *data)
{
	struct modem_backend_mock *mock = (struct modem_backend_mock *)data;

	modem_pipe_notify_closed(&mock->pipe);
	return 0;
}

struct modem_pipe_api modem_backend_mock_api = {
	.open = modem_backend_mock_open,
	.transmit = modem_backend_mock_transmit,
	.receive = modem_backend_mock_receive,
	.close = modem_backend_mock_close,
};

static void modem_backend_mock_receive_ready_handler(struct k_work *item)
{
	struct modem_backend_mock *mock =
		CONTAINER_OF(item, struct modem_backend_mock, receive_ready_work);

	modem_pipe_notify_receive_ready(&mock->pipe);
}

static void modem_backend_mock_transmit_idle_handler(struct k_work *item)
{
	struct modem_backend_mock *mock =
		CONTAINER_OF(item, struct modem_backend_mock, transmit_idle_work);

	modem_pipe_notify_transmit_idle(&mock->pipe);
}

struct modem_pipe *modem_backend_mock_init(struct modem_backend_mock *mock,
					   const struct modem_backend_mock_config *config)
{
	memset(mock, 0, sizeof(*mock));

	ring_buf_init(&mock->rx_rb, config->rx_buf_size, config->rx_buf);
	ring_buf_init(&mock->tx_rb, config->tx_buf_size, config->tx_buf);
	k_work_init(&mock->receive_ready_work, modem_backend_mock_receive_ready_handler);
	k_work_init(&mock->transmit_idle_work, modem_backend_mock_transmit_idle_handler);
	mock->limit = config->limit;
	modem_pipe_init(&mock->pipe, mock, &modem_backend_mock_api);
	return &mock->pipe;
}

struct modem_pipe *modem_backend_mock_get_pipe(struct modem_backend_mock *mock)
{
	return &mock->pipe;
}

void modem_backend_mock_reset(struct modem_backend_mock *mock)
{
	ring_buf_reset(&mock->rx_rb);
	ring_buf_reset(&mock->tx_rb);
	mock->transaction = NULL;
	mock->transaction_match_cnt = 0;
}

int modem_backend_mock_get(struct modem_backend_mock *mock, uint8_t *buf, size_t size)
{
	return ring_buf_get(&mock->tx_rb, buf, size);
}

void modem_backend_mock_put(struct modem_backend_mock *mock, const uint8_t *buf, size_t size)
{
	__ASSERT(ring_buf_put(&mock->rx_rb, buf, size) == size,
		 "Mock buffer capacity exceeded");

	k_work_submit(&mock->receive_ready_work);
}

void modem_backend_mock_prime(struct modem_backend_mock *mock,
			      const struct modem_backend_mock_transaction *transaction)
{
	mock->transaction = transaction;
	mock->transaction_match_cnt = 0;
}

void modem_backend_mock_bridge(struct modem_backend_mock *mock_a, struct modem_backend_mock *mock_b)
{
	mock_a->bridge = mock_b;
	mock_b->bridge = mock_a;
}
