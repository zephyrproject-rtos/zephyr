/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test suite sets up a modem_backend_uart instance connected to a UART which has its
 * RX and TX pins wired together to provide loopback functionality. A large number of bytes
 * containing a sequence of pseudo random numbers are then transmitted, received, and validated.
 *
 * The test suite repeats three times, opening and clsoing the modem_pipe attached to the
 * modem_backend_uart instance before and after the tests respectively.
 */

/*************************************************************************************************/
/*                                        Dependencies                                           */
/*************************************************************************************************/
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/modem/backend/uart.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/
/*                                          Mock pipe                                            */
/*************************************************************************************************/
static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(dut));
static struct modem_backend_uart uart_backend;
static struct modem_pipe *pipe;
K_SEM_DEFINE(receive_ready_sem, 0, 1);

/*************************************************************************************************/
/*                                          Buffers                                              */
/*************************************************************************************************/
static uint8_t backend_receive_buffer[4096];
static uint8_t backend_transmit_buffer[4096];
RING_BUF_DECLARE(transmit_ring_buf, 4096);
static uint8_t receive_buffer[4096];

/*************************************************************************************************/
/*                                     Modem pipe callback                                       */
/*************************************************************************************************/
static void modem_pipe_callback_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
					void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		k_sem_give(&receive_ready_sem);
		break;
	default:
		break;
	}
}

/*************************************************************************************************/
/*                                          Helpers                                              */
/*************************************************************************************************/
static uint32_t transmit_prng_state = 1234;
static uint32_t receive_prng_state = 1234;
static uint32_t transmit_size_prng_state;

static uint8_t transmit_prng_random(void)
{
	transmit_prng_state = ((1103515245 * transmit_prng_state) + 12345) % (1U << 31);
	return (uint8_t)(transmit_prng_state & 0xFF);
}

static uint8_t receive_prng_random(void)
{
	receive_prng_state = ((1103515245 * receive_prng_state) + 12345) % (1U << 31);
	return (uint8_t)(receive_prng_state & 0xFF);
}

static void prng_reset(void)
{
	transmit_prng_state = 1234;
	receive_prng_state = 1234;
	transmit_size_prng_state = 0;
}

static void fill_transmit_ring_buf(void)
{
	uint32_t space = ring_buf_space_get(&transmit_ring_buf);
	uint8_t data;

	for (uint32_t i = 0; i < space; i++) {
		data = transmit_prng_random();
		ring_buf_put(&transmit_ring_buf, &data, 1);
	}
}

static uint32_t transmit_size_prng_random(void)
{
	uint32_t size = 1;

	for (uint8_t i = 0; i < transmit_size_prng_state; i++) {
		size = size * 2;
	}

	transmit_size_prng_state = transmit_size_prng_state == 11
				 ? 0
				 : transmit_size_prng_state + 1;

	return size;
}

static int transmit_prng(uint32_t remaining)
{
	uint8_t *reserved;
	uint32_t reserved_size;
	uint32_t transmit_size;
	int ret;

	fill_transmit_ring_buf();
	reserved_size = ring_buf_get_claim(&transmit_ring_buf, &reserved, UINT32_MAX);
	transmit_size = MIN(transmit_size_prng_random(), reserved_size);
	transmit_size = MIN(remaining, transmit_size);
	ret = modem_pipe_transmit(pipe, reserved, transmit_size);
	if (ret < 0) {
		return ret;
	}
	printk("TX: %u,%u\n", transmit_size, (uint32_t)ret);
	__ASSERT(ret <= remaining, "Impossible number of bytes sent %u", (uint32_t)ret);
	ring_buf_get_finish(&transmit_ring_buf, ret);
	return ret;
}

static int receive_prng(void)
{
	int ret = 0;

	if (k_sem_take(&receive_ready_sem, K_NO_WAIT) == 0) {
		ret = modem_pipe_receive(pipe, receive_buffer, sizeof(receive_buffer));
		if (ret < 0) {
			return -EFAULT;
		}
		for (uint32_t i = 0; i < (uint32_t)ret; i++) {
			if (receive_prng_random() != receive_buffer[i]) {
				return -EFAULT;
			}
		}
		printk("RX: %u\n", (uint32_t)ret);
	}

	return ret;
}

/*************************************************************************************************/
/*                                         Test setup                                            */
/*************************************************************************************************/
static void *test_modem_backend_uart_setup(void)
{
	const struct modem_backend_uart_config config = {
		.uart = uart,
		.receive_buf = backend_receive_buffer,
		.receive_buf_size = 1024,
		.transmit_buf = backend_transmit_buffer,
		.transmit_buf_size = 1024,
	};

	pipe = modem_backend_uart_init(&uart_backend, &config);
	modem_pipe_attach(pipe, modem_pipe_callback_handler, NULL);
	return NULL;
}

static void test_modem_backend_uart_before(void *f)
{
	prng_reset();
	ring_buf_reset(&transmit_ring_buf);
	k_sem_reset(&receive_ready_sem);
	__ASSERT_NO_MSG(modem_pipe_open(pipe) == 0);
}

static void test_modem_backend_uart_after(void *f)
{
	__ASSERT_NO_MSG(modem_pipe_close(pipe) == 0);
}

/*************************************************************************************************/
/*                                             Tests                                             */
/*************************************************************************************************/
ZTEST(modem_backend_uart_suite, test_transmit_receive)
{
	int32_t remaining = 8192;
	uint32_t received = 0;
	uint32_t transmitted = 0;
	int ret;

	while ((remaining != 0) || (received < 8192)) {
		ret = transmit_prng(remaining);
		zassert(ret > -1, "Failed to transmit data");
		remaining -= (uint32_t)ret;
		transmitted += (uint32_t)ret;
		printk("TX ACC: %u\n", transmitted);

		while (received < transmitted) {
			ret = receive_prng();
			zassert(ret > -1, "Received data is corrupted");
			received += (uint32_t)ret;
			k_yield();
		}
	}
}

ZTEST_SUITE(modem_backend_uart_suite, NULL, test_modem_backend_uart_setup,
	    test_modem_backend_uart_before, test_modem_backend_uart_after, NULL);
