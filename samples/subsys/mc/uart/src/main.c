/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>

#include "core/error.h"
#include "core/version.h"
#include "core/time.h"
#include "io/communication/mc_communication.h"

#define MC_UART_NODE         DT_ALIAS(mc_uart)
#define COMPLETE_COUNT       200U
#define TEST_TIMEOUT_US      30000000U
#define RECEIVER_WINDOW_SIZE 59U
#define SENDER_WINDOW_SIZE   1157U
#define UART_RING_SIZE       131072U
#define STORAGE_SIZE         20000U
#define UART_FRAME_END       0xC0U
#define UART_FRAME_ESCAPE    0xDBU
#define UART_ESCAPED_END     0xDCU
#define UART_ESCAPED_ESCAPE  0xDDU

typedef struct {
	mc_time_t last_time;
	uint32_t duration_ms;
	bool is_connected;
} periodic_connection_t;

typedef struct {
	const char *name;
	uint32_t iterations;
	uint32_t timeout_us;
	bool timeout_allowed;
} phase_info_t;

static const struct device *const Uart = DEVICE_DT_GET(MC_UART_NODE);
RING_BUF_DECLARE(RxRing, UART_RING_SIZE);
static uint8_t SendBuffer[SENDER_WINDOW_SIZE];
static uint8_t RxFrame[SENDER_WINDOW_SIZE];
static uint32_t RxFrameSize;
static uint32_t RxDroppedBytes;
static bool RxFrameStarted;
static bool RxFrameEscaped;
static bool RxFrameDiscarded;
static uint32_t PendingTailSize;
static periodic_connection_t Periodic = {0U, 0U, true};
static uint32_t TestIterations = COMPLETE_COUNT;
static uint32_t TimeOutUS = TEST_TIMEOUT_US;
static uint32_t PeriodicDurationMS;
static uint8_t LossRate;
static bool RepetitiveSendEnable;
static bool TimeoutAllowed;
static bool SendPacketDrop;
static bool ReceiveBitCorruption;
static uint8_t HeaderCorruption;
static uint32_t RecvCounter;
static uint32_t SendCounter;
static uint32_t RecvFailedCounter;
static uint32_t SendFailedCounter;
static uint32_t RandomState = 1U;

static uint32_t next_random(void)
{
	RandomState = (RandomState * 1103515245U) + 12345U;
	return RandomState;
}

static bool simulate_loss(void)
{
	return ((next_random() % 100U) < LossRate);
}

static void periodic_update(void)
{
	if ((Periodic.duration_ms != 0U) &&
	    ((mc_now_m() - Periodic.last_time) > Periodic.duration_ms)) {
		Periodic.last_time = mc_now_m();
		Periodic.is_connected = !Periodic.is_connected;
	}
}

static void cfg_set_repetitive_send(bool enable)
{
	RepetitiveSendEnable = enable;
}
static void cfg_set_loss_rate(uint8_t rate)
{
	LossRate = rate;
}
static uint8_t cfg_get_loss_rate(void)
{
	return LossRate;
}

static void cfg_set_iterations(uint32_t iterations)
{
	TestIterations = iterations;
	RecvCounter = 0U;
	SendCounter = 0U;
	RecvFailedCounter = 0U;
	SendFailedCounter = 0U;
}

static void cfg_set_periodic_duration(uint32_t duration_ms)
{
	PeriodicDurationMS = duration_ms;
	Periodic.duration_ms = duration_ms;
	Periodic.last_time = mc_now_m();
	Periodic.is_connected = true;
}

static uint32_t cfg_get_periodic_duration(void)
{
	return PeriodicDurationMS;
}
static uint32_t cfg_get_iterations(void)
{
	return TestIterations;
}
static uint32_t cfg_get_recv_counter(void)
{
	return RecvCounter;
}
static uint32_t cfg_get_send_counter(void)
{
	return SendCounter;
}
static uint32_t cfg_get_recv_failed_counter(void)
{
	return RecvFailedCounter;
}
static uint32_t cfg_get_send_failed_counter(void)
{
	return SendFailedCounter;
}
static void cfg_set_timeout_us(uint32_t timeout_us)
{
	TimeOutUS = timeout_us;
}
static uint32_t cfg_get_timeout_us(void)
{
	return TimeOutUS;
}
static void cfg_set_timeout_allowed(bool allowed)
{
	TimeoutAllowed = allowed;
}
static bool cfg_get_timeout_allowed(void)
{
	return TimeoutAllowed;
}

static void uart_rx_callback(const struct device *dev, void *user_data)
{
	uint8_t buffer[64];

	ARG_UNUSED(user_data);
	uart_irq_update(dev);
	while (uart_irq_rx_ready(dev)) {
		const int length = uart_fifo_read(dev, buffer, sizeof(buffer));

		if (length <= 0) {
			break;
		}
		const uint32_t stored = ring_buf_put(&RxRing, buffer, (uint32_t)length);

		RxDroppedBytes += (uint32_t)length - stored;
	}
}

static uint32_t uart_read_frame(uint8_t *data, uint32_t size)
{
	uint8_t byte;

	while (ring_buf_get(&RxRing, &byte, 1U) == 1U) {
		if (byte == UART_FRAME_END) {
			if (RxFrameStarted && !RxFrameDiscarded &&
			    !RxFrameEscaped && (RxFrameSize == size)) {
				memcpy(data, RxFrame, size);
				RxFrameSize = 0U;
				RxFrameStarted = true;
				return size;
			}
			RxFrameSize = 0U;
			RxFrameStarted = true;
			RxFrameEscaped = false;
			RxFrameDiscarded = false;
			continue;
		}
		if (!RxFrameStarted || RxFrameDiscarded) {
			continue;
		}
		if (RxFrameEscaped) {
			RxFrameEscaped = false;
			if (byte == UART_ESCAPED_END) {
				byte = UART_FRAME_END;
			} else if (byte == UART_ESCAPED_ESCAPE) {
				byte = UART_FRAME_ESCAPE;
			} else {
				RxFrameDiscarded = true;
				continue;
			}
		} else if (byte == UART_FRAME_ESCAPE) {
			RxFrameEscaped = true;
			continue;
		}
		if (RxFrameSize < sizeof(RxFrame)) {
			RxFrame[RxFrameSize++] = byte;
		} else {
			RxFrameDiscarded = true;
		}
	}
	return 0U;
}

static uint32_t uart_recv(void *data, uint32_t size)
{
	uint32_t read_size;
	uint32_t logical_size;

	periodic_update();

	/* Finish a shortened logical packet with synthetic tail bytes. This
	 * consumes no bytes from the next physical UART frame and lets mc_comm's
	 * CRC reject the incomplete frame without stream desynchronization.
	 */
	if (PendingTailSize != 0U) {
		const uint32_t tail_size = (PendingTailSize < size) ? PendingTailSize : size;

		memset(data, 0, tail_size);
		PendingTailSize -= tail_size;
		return tail_size;
	}

	read_size = uart_read_frame((uint8_t *)data, size);
	if (read_size == 0U) {
		return 0U;
	}
	k_usleep(159U);

	if (!Periodic.is_connected) {
		return 0U;
	}

	RecvCounter++;
	if (simulate_loss()) {
		RecvFailedCounter++;
		ReceiveBitCorruption = !ReceiveBitCorruption;
		HeaderCorruption++;
		if (HeaderCorruption > 30U) {
			HeaderCorruption = 0U;
			((uint8_t *)data)[1] ^= 1U;
			logical_size = (read_size * 74U) / 100U;
			PendingTailSize = read_size - logical_size;
			return logical_size;
		}
		if (!ReceiveBitCorruption) {
			return 0U;
		}
		((uint8_t *)data)[29] ^= 1U;
	}
	return read_size;
}

static uint32_t uart_write_frame(const uint8_t *data, uint32_t size)
{
	uart_poll_out(Uart, UART_FRAME_END);
	for (uint32_t index = 0U; index < size; index++) {
		if ((data[index] == UART_FRAME_END) || (data[index] == UART_FRAME_ESCAPE)) {
			uart_poll_out(Uart, UART_FRAME_ESCAPE);
			uart_poll_out(Uart, data[index] == UART_FRAME_END ?
				      UART_ESCAPED_END : UART_ESCAPED_ESCAPE);
			continue;
		}
		uart_poll_out(Uart, data[index]);
	}
	uart_poll_out(Uart, UART_FRAME_END);
	return size;
}

static uint32_t uart_send(cvoid *data, uint32_t size)
{
	uint32_t count;
	uint32_t sent_size = 0U;
	const uint8_t *data_buffer = data;

	periodic_update();
	if (!Periodic.is_connected) {
		return 0U;
	}
	k_usleep(171U);
	count = RepetitiveSendEnable ? 2U : 1U;
	if (simulate_loss()) {
		SendFailedCounter++;
		SendPacketDrop = !SendPacketDrop;
		if (SendPacketDrop) {
			SendCounter++;
			return 0U;
		}
		memcpy(SendBuffer, data, size);
		SendBuffer[29] ^= 1U;
		data_buffer = SendBuffer;
	}
	while (count != 0U) {
		SendCounter++;
		sent_size = uart_write_frame(data_buffer, size);
		/* Allow the peer's UART interrupt path to drain the frame before
		 * sending the next frame, especially the duplicate retry copy.
		 */
		k_msleep(5U);
		count--;
	}
	return sent_size;
}

static void uart_reset_rx(void)
{
	uart_irq_rx_disable(Uart);
	ring_buf_reset(&RxRing);
	RxFrameSize = 0U;
	RxFrameStarted = false;
	RxFrameEscaped = false;
	RxFrameDiscarded = false;
	RxDroppedBytes = 0U;
	PendingTailSize = 0U;
	uart_irq_rx_enable(Uart);
}

static bool setup_uart(void)
{
	if (!device_is_ready(Uart)) {
		printk("UART device is not ready\n");
		return false;
	}
	if (uart_irq_callback_user_data_set(Uart, uart_rx_callback, NULL) != 0) {
		printk("UART callback setup failed\n");
		return false;
	}
	uart_irq_rx_enable(Uart);
	return true;
}

static uint32_t dummy_io_recv(void *data, uint32_t size)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	return 0U;
}

static uint32_t dummy_io_send(cvoid *data, uint32_t size)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	return 0U;
}

static int invalid_creation(void)
{
	char memory[1024];

	const mc_buffer alloc_buffer = mc_buffer_char(memory, sizeof(memory));
	mc_ptr result_ptr = {0};
	mc_comm_cfg config;

	config = mc_comm_cfg(mc_io(NULL, dummy_io_send), mc_comm_wnd(1024U, 3U),
			     mc_comm_wnd(1024U, 3U));
	result_ptr = mc_comm_init(alloc_buffer, config);
	if ((result_ptr.error == MC_SUCCESS) || (result_ptr.data != NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	config = mc_comm_cfg(mc_io(dummy_io_recv, NULL), mc_comm_wnd(1024U, 3U),
			     mc_comm_wnd(1024U, 3U));
	result_ptr = mc_comm_init(alloc_buffer, config);
	if ((result_ptr.error == MC_SUCCESS) || (result_ptr.data != NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send), mc_comm_wnd(1024U, 3U),
			     mc_comm_wnd(0U, 3U));
	result_ptr = mc_comm_init(alloc_buffer, config);
	if ((result_ptr.error == MC_SUCCESS) || (result_ptr.data != NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send), mc_comm_wnd(0U, 3U),
			     mc_comm_wnd(1024U, 3U));
	result_ptr = mc_comm_init(alloc_buffer, config);
	if ((result_ptr.error == MC_SUCCESS) || (result_ptr.data != NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send), mc_comm_wnd(1024U, 3U),
			     mc_comm_wnd(1024U, 3U));
	result_ptr = mc_comm_init(alloc_buffer, config);
	if ((result_ptr.error == MC_SUCCESS) || (result_ptr.data != NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send), mc_comm_wnd(1U, 3U),
			     mc_comm_wnd(1U, 3U));
	result_ptr = mc_comm_init(alloc_buffer, config);
	if ((result_ptr.error == MC_SUCCESS) || (result_ptr.data != NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	return MC_SUCCESS;
}

static int invalid_argument(void)
{
	char temp[10];
	char alloc_buffer[200];
	const mc_comm_cfg config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send),
					       mc_comm_wnd(15U, 1U), mc_comm_wnd(15U, 1U));
	const mc_u32 required = mc_comm_req_size(config);
	const mc_ptr initialized =
		mc_comm_init(mc_buffer_char(alloc_buffer, required.value), config);
	mc_u32 result_u32;
	mc_bool result_bool;

	if (initialized.error != MC_SUCCESS) {
		return initialized.error;
	}
	result_u32 = mc_comm_recv(NULL, temp, sizeof(temp), 100U);
	if ((result_u32.error != MC_ERR_INVALID_ARGUMENT) || (result_u32.value != 0U)) {
		return MC_ERR_RUNTIME;
	}
	result_u32 = mc_comm_recv(initialized.data, NULL, sizeof(temp), 100U);
	if ((result_u32.error != MC_ERR_INVALID_ARGUMENT) || (result_u32.value != 0U)) {
		return MC_ERR_RUNTIME;
	}
	result_u32 = mc_comm_send(NULL, temp, sizeof(temp), 100U);
	if ((result_u32.error != MC_ERR_INVALID_ARGUMENT) || (result_u32.value != 0U)) {
		return MC_ERR_RUNTIME;
	}
	result_u32 = mc_comm_send(initialized.data, NULL, sizeof(temp), 100U);
	if ((result_u32.error != MC_ERR_INVALID_ARGUMENT) || (result_u32.value != 0U)) {
		return MC_ERR_RUNTIME;
	}
	if (mc_comm_update(NULL) != MC_ERR_INVALID_ARGUMENT) {
		return MC_ERR_RUNTIME;
	}
	result_bool = mc_comm_flush(NULL, 100U);
	if ((result_bool.error != MC_ERR_INVALID_ARGUMENT) || result_bool.value) {
		return MC_ERR_RUNTIME;
	}
	return MC_SUCCESS;
}

static int valid_creation(void)
{
	char memory[1024];
	const mc_comm_cfg config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send),
					       mc_comm_wnd(5U * sizeof(uint32_t), 3U),
					       mc_comm_wnd(5U * sizeof(uint32_t), 3U));
	const mc_ptr result = mc_comm_init(mc_buffer_char(memory, sizeof(memory)), config);

	if ((result.error != MC_SUCCESS) || (result.data == NULL)) {
		return MC_ERR_BAD_ALLOC;
	}
	return MC_SUCCESS;
}

static bool init_comm(mc_comm **message, uint8_t *storage, uint32_t storage_size, bool sender)
{
	const mc_comm_cfg config = mc_comm_cfg(
		mc_io(uart_recv, uart_send),
		mc_comm_wnd(sender ? RECEIVER_WINDOW_SIZE : SENDER_WINDOW_SIZE, sender ? 2U : 3U),
		mc_comm_wnd(sender ? SENDER_WINDOW_SIZE : RECEIVER_WINDOW_SIZE, sender ? 4U : 1U));
	const mc_u32 required = mc_comm_req_size(config);

	if (required.error != MC_SUCCESS) {
		return false;
	}
	if (storage_size < required.value) {
		printk("[%s] Not enough space for test\n", sender ? "Send" : "Receive");
		return false;
	}
	const mc_ptr result = mc_comm_init(mc_buffer_char(storage, required.value), config);

	if (result.error != MC_SUCCESS) {
		return false;
	}
	*message = result.data;
	return true;
}

static bool send_data(mc_comm *message, cvoid *data, uint32_t size)
{
	const mc_u32 result = mc_comm_send(message, data, size, cfg_get_timeout_us());

	return (result.error == MC_SUCCESS) && (result.value == size);
}

static bool receive_data(mc_comm *message, void *data, uint32_t size)
{
	const mc_u32 result = mc_comm_recv(message, data, size, cfg_get_timeout_us());

	return (result.error == MC_SUCCESS) && (result.value == size);
}

static bool send_string(mc_comm *message, uint32_t seed)
{
	char data[9] = {0};
	(void)snprintk(data, sizeof(data), "!p%03u.?I", seed % 1000U);
	return send_data(message, data, sizeof(data));
}

static uint32_t variadic_count(uint32_t seed)
{
	return (((seed * 1664525U) + 1013904223U) % 997U) + 27U;
}

static bool send_variadic_size(mc_comm *message, uint32_t seed)
{
	uint32_t data[1024] = {0};
	const uint32_t count = variadic_count(seed);

	for (uint32_t index = 0U; index < count; index++) {
		const uint32_t coeff = (index & 1U) ? 2654435761U : 40503U;

		data[index] = (coeff * (index + 1U) * (seed + 1U)) + index;
	}
	return send_data(message, data, count * sizeof(*data));
}

static bool send_tiny_size(mc_comm *message, uint32_t seed)
{
	const bool data = (seed & 1U) != 0U;

	return send_data(message, &data, sizeof(data));
}

static bool receive_string(mc_comm *message, uint32_t seed, mc_err *result)
{
	char data[9] = {0};
	char expected[9] = {0};

	if (!receive_data(message, data, sizeof(data))) {
		if (!cfg_get_timeout_allowed()) {
			printk("[ERR Data 1] Incomplete receiving\n");
		}
		return false;
	}
	(void)snprintk(expected, sizeof(expected), "!p%03u.?I", seed % 1000U);
	if (memcmp(data, expected, sizeof(data)) != 0) {
		*result = MC_ERR_RUNTIME;
		printk("[ERR Data 1] wrong data received\n");
		return false;
	}
	return true;
}

static bool receive_variadic_size(mc_comm *message, uint32_t seed, mc_err *result)
{
	uint32_t data[1024] = {0};
	const uint32_t count = variadic_count(seed);

	if (!receive_data(message, data, count * sizeof(*data))) {
		if (!cfg_get_timeout_allowed()) {
			printk("[ERR Data 2] Incomplete receiving\n");
		}
		return false;
	}
	for (uint32_t index = 0U; index < count; index++) {
		const uint32_t coeff = (index & 1U) ? 2654435761U : 40503U;
		const uint32_t expected = (coeff * (index + 1U) * (seed + 1U)) + index;

		if (data[index] != expected) {
			*result = MC_ERR_RUNTIME;
			printk("[ERR Data 2] Received: %u, Expected: %u\n", data[index], expected);
			return false;
		}
	}
	return true;
}

static bool receive_tiny_size(mc_comm *message, uint32_t seed, mc_err *result)
{
	bool data = false;

	if (!receive_data(message, &data, sizeof(data))) {
		if (!cfg_get_timeout_allowed()) {
			printk("[ERR Data 3] Incomplete receiving\n");
		}
		return false;
	}
	if (data != ((seed & 1U) != 0U)) {
		*result = MC_ERR_RUNTIME;
		printk("[ERR Data 3] wrong data received\n");
		return false;
	}
	return true;
}

static void print_progress(bool sender, uint32_t completed, uint32_t total)
{
	static uint32_t last_step[2] = {UINT32_MAX, UINT32_MAX};
	const char *role = sender ? "Sender" : "Receiving";
	const uint32_t bar_length = 20U;
	const uint32_t role_index = sender ? 0U : 1U;
	const uint32_t step = (completed * bar_length) / total;

	if (completed == 0U) {
		last_step[role_index] = UINT32_MAX;
	}
	if (step == last_step[role_index]) {
		return;
	}

	printk("%s Progress: [", role);
	for (uint32_t index = 0U; index < bar_length; index++) {
		printk("%c", index < step ? '#' : '-');
	}
	printk("] %u%%\n", step * 5U);
	last_step[role_index] = step;
}

static void print_log(mc_time_t begin_time, mc_time_t end_time)
{
	const float duration_s = (float)(end_time - begin_time) / 1000000000.0F;
	const uint32_t size_k_byte_ps =
		(9U + (1024U * sizeof(uint32_t)) + sizeof(bool)) * cfg_get_iterations() / 1024U;
	const float throughput = (duration_s > 0.0F) ? size_k_byte_ps / duration_s : 0.0F;
	const uint32_t recv_count = cfg_get_recv_counter();
	const uint32_t send_count = cfg_get_send_counter();
	const uint32_t recv_failed = cfg_get_recv_failed_counter();
	const uint32_t send_failed = cfg_get_send_failed_counter();

	printk("[IO] Completed{Recv: %u, Send: %u} - Failed{Recv: %u(%.2f%%), Send: %u(%.2f%%)} - "
	       "Throughput: %.2f KBps\n",
	       recv_count, send_count, recv_failed,
	       (double)((recv_count != 0U) ? 100.0F * ((float)recv_failed / recv_count) : 0.0F),
	       send_failed,
	       (double)((send_count != 0U) ? 100.0F * ((float)send_failed / send_count) : 0.0F),
	       (double)throughput);
	if (RxDroppedBytes != 0U) {
		printk("[UART] Dropped %u buffered bytes\n", RxDroppedBytes);
	}
}

static void wait_for_sender(mc_comm *message)
{
	const mc_time_t end_time =
		mc_now_m() + cfg_get_periodic_duration() + (100U * cfg_get_loss_rate());
	while (mc_now_m() < end_time) {
		(void)mc_comm_update(message);
	}
}

static mc_err sender_start(mc_comm *message)
{
	mc_err result = MC_SUCCESS;
	bool timed_out = false;
	mc_time_t begin_time;

	k_msleep(200U);
	begin_time = mc_now();
	for (uint32_t counter = 0U; counter <= cfg_get_iterations(); counter++) {
		if ((mc_comm_update(message) != MC_SUCCESS) || !send_string(message, counter) ||
		    !send_variadic_size(message, counter) || !send_tiny_size(message, counter)) {
			timed_out = true;
			break;
		}
		print_progress(true, counter, cfg_get_iterations());
	}
	if (timed_out && !cfg_get_timeout_allowed()) {
		result = MC_ERR_TIMEOUT;
	}
	if ((result == MC_SUCCESS) && !timed_out) {
		const mc_bool flushed = mc_comm_flush(message, cfg_get_timeout_us());

		if (((flushed.error != MC_SUCCESS) || !flushed.value) &&
		    !cfg_get_timeout_allowed()) {
			printk("mc_comm_flush failed\n");
			result = MC_ERR_TIMEOUT;
		}
	}
	print_log(begin_time, mc_now());
	return result;
}

static mc_err receiver_start(mc_comm *message)
{
	mc_err result = MC_SUCCESS;
	bool timed_out = false;
	const mc_time_t begin_time = mc_now();
	mc_time_t end_time;

	for (uint32_t counter = 0U; counter <= cfg_get_iterations(); counter++) {
		if ((mc_comm_update(message) != MC_SUCCESS) ||
		    !receive_string(message, counter, &result) ||
		    !receive_variadic_size(message, counter, &result) ||
		    !receive_tiny_size(message, counter, &result)) {
			timed_out = (result == MC_SUCCESS);
			break;
		}
		print_progress(false, counter, cfg_get_iterations());
	}
	if (timed_out && !cfg_get_timeout_allowed()) {
		result = MC_ERR_TIMEOUT;
	}
	if ((result == MC_SUCCESS) && !timed_out) {
		const mc_bool flushed = mc_comm_flush(message, cfg_get_timeout_us());

		if (((flushed.error != MC_SUCCESS) || !flushed.value) &&
		    !cfg_get_timeout_allowed()) {
			printk("mc_comm_flush failed\n");
			result = MC_ERR_TIMEOUT;
		}
	}
	end_time = mc_now();
	if ((result == MC_SUCCESS) && !timed_out) {
		wait_for_sender(message);
	}
	print_log(begin_time, end_time);
	return result;
}

static mc_err singly_direction(bool sender)
{
	static uint8_t storage[STORAGE_SIZE] __aligned(8);
	mc_comm *message = NULL;

	uart_reset_rx();
	if (!init_comm(&message, storage, sizeof(storage), sender)) {
		return MC_ERR_BAD_ALLOC;
	}
	return sender ? sender_start(message) : receiver_start(message);
}

static mc_err singly_timed_out(void)
{
	char alloc_buffer[200];
	char temp[100];
	const mc_comm_cfg config = mc_comm_cfg(mc_io(dummy_io_recv, dummy_io_send),
					       mc_comm_wnd(15U, 1U), mc_comm_wnd(15U, 1U));
	const mc_u32 required = mc_comm_req_size(config);
	const mc_ptr initialized =
		mc_comm_init(mc_buffer_char(alloc_buffer, required.value), config);
	if (initialized.error != MC_SUCCESS) {
		return initialized.error;
	}
	mc_u32 result = mc_comm_send(initialized.data, temp, sizeof(temp), 10U);

	if ((result.error != MC_ERR_TIMEOUT) || (result.value == sizeof(temp))) {
		return MC_ERR_RUNTIME;
	}
	result = mc_comm_recv(initialized.data, temp, sizeof(temp), 10U);
	if ((result.error != MC_ERR_TIMEOUT) || (result.value == sizeof(temp))) {
		return MC_ERR_RUNTIME;
	}
	const mc_bool flushed = mc_comm_flush(initialized.data, 1U);

	if ((flushed.error != MC_ERR_TIMEOUT) || flushed.value) {
		return MC_ERR_RUNTIME;
	}
	return MC_SUCCESS;
}

static int run_local_result(const char *name, int (*test)(void))
{
	const mc_time_t begin_time = mc_now_u();

	printk("[%s]\n", name);
	const int result = test();

	if (result != MC_SUCCESS) {
		printk("FAILED: %u\n\n", (uint32_t)result);
		return result;
	}
	printk("PASSED - %u(us)\n\n", (uint32_t)(mc_now_u() - begin_time));
	return MC_SUCCESS;
}

static int run_phase(const phase_info_t *phase, bool sender)
{
	const mc_time_t begin_time = mc_now_u();

	printk("[%s]\n", phase->name);
	const mc_err result = singly_direction(sender);

	if (result != MC_SUCCESS) {
		printk("FAILED: %u\n\n", result);
		return result;
	}
	printk("PASSED - %u(us)\n\n", (uint32_t)(mc_now_u() - begin_time));
	return MC_SUCCESS;
}

static int run_all_tests(bool sender)
{
	const phase_info_t normal = {"singly_direction", COMPLETE_COUNT, TEST_TIMEOUT_US, false};
	const phase_info_t repetitive = {"singly_repetitive", COMPLETE_COUNT, TEST_TIMEOUT_US,
					 false};
	const phase_info_t low_loss = {"singly_low_lossy", COMPLETE_COUNT, TEST_TIMEOUT_US, false};
	const phase_info_t high_loss = {"singly_high_lossy", 100U, 120000000U, true};

	if (run_local_result("invalid_creation", invalid_creation) != MC_SUCCESS ||
	    run_local_result("invalid_argument", invalid_argument) != MC_SUCCESS ||
	    run_local_result("valid_creation", valid_creation) != MC_SUCCESS) {
		return MC_ERR_RUNTIME;
	}

	cfg_set_repetitive_send(false);
	cfg_set_iterations(normal.iterations);
	cfg_set_periodic_duration(0U);
	cfg_set_timeout_us(normal.timeout_us);
	cfg_set_timeout_allowed(normal.timeout_allowed);
	if (run_phase(&normal, sender) != MC_SUCCESS) {
		return MC_ERR_RUNTIME;
	}

	cfg_set_repetitive_send(true);
	cfg_set_iterations(repetitive.iterations);
	cfg_set_periodic_duration(0U);
	cfg_set_timeout_us(repetitive.timeout_us);
	cfg_set_timeout_allowed(repetitive.timeout_allowed);
	if (run_phase(&repetitive, sender) != MC_SUCCESS) {
		return MC_ERR_RUNTIME;
	}
	cfg_set_repetitive_send(true);

	cfg_set_loss_rate(20U);
	cfg_set_iterations(low_loss.iterations);
	cfg_set_periodic_duration(100U);
	cfg_set_timeout_us(low_loss.timeout_us);
	cfg_set_timeout_allowed(low_loss.timeout_allowed);
	if (run_phase(&low_loss, sender) != MC_SUCCESS) {
		return MC_ERR_RUNTIME;
	}
	cfg_set_loss_rate(0U);

	cfg_set_loss_rate(96U);
	cfg_set_iterations(high_loss.iterations);
	cfg_set_periodic_duration(5000U);
	cfg_set_timeout_us(high_loss.timeout_us);
	cfg_set_timeout_allowed(high_loss.timeout_allowed);
	if (run_phase(&high_loss, sender) != MC_SUCCESS) {
		return MC_ERR_RUNTIME;
	}
	cfg_set_timeout_allowed(false);
	cfg_set_loss_rate(0U);

	cfg_set_loss_rate(98U);
	cfg_set_periodic_duration(5000U);
	cfg_set_timeout_us(50U);
	if (run_local_result("singly_timed_out", (int (*)(void))singly_timed_out) != MC_SUCCESS) {
		return MC_ERR_RUNTIME;
	}
	cfg_set_loss_rate(0U);
	return MC_SUCCESS;
}

int main(void)
{
	const bool sender = IS_ENABLED(CONFIG_MC_UART_SENDER);

	printk("[MICRO CORE %u.%u.%u - IO - COMMUNICATION] %s\n", MC_VERSION_MAJOR,
	       MC_VERSION_MINOR, MC_VERSION_PATCH, sender ? "Sender" : "Receiving");
	if (!setup_uart()) {
		return 1;
	}
	if (run_all_tests(sender) != MC_SUCCESS) {
		printk("MC UART communication test FAIL\n");
		return 1;
	}
	printk("MC UART communication test PASS\n");
	return 0;
}
