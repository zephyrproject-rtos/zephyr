/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/atomic.h>
#include <esp_hosted_mcu.h>

enum { TEST_PIN_reset_gpios, TEST_PIN_handshake_gpios, TEST_PIN_dataready_gpios };
static int mock_gpio_get(const struct gpio_dt_spec *spec);
static int mock_gpio_set(const struct gpio_dt_spec *spec, int value);
static int mock_gpio_configure(const struct gpio_dt_spec *spec, gpio_flags_t flags);
static int mock_gpio_callback(const struct gpio_dt_spec *spec, struct gpio_callback *cb);
static int mock_spi(const struct spi_dt_spec *spec, const struct spi_buf_set *tx,
		    const struct spi_buf_set *rx);

/* Compile the actual backend. Only the hardware boundary is replaced; mutexes,
 * semaphores, timeouts, atomics and scheduling use the real Zephyr kernel.
 */
#undef SPI_DT_SPEC_INST_GET
#define SPI_DT_SPEC_INST_GET(...) {0}
#undef GPIO_DT_SPEC_INST_GET
#define GPIO_DT_SPEC_INST_GET(inst, prop) {.pin = TEST_PIN_##prop}
#define spi_is_ready_dt(...) true
#define gpio_is_ready_dt(...) true
#define gpio_pin_get_dt mock_gpio_get
#define gpio_pin_set_dt mock_gpio_set
#define gpio_pin_configure_dt mock_gpio_configure
#define gpio_pin_interrupt_configure_dt mock_gpio_configure
#define gpio_add_callback_dt mock_gpio_callback
#define spi_transceive_dt mock_spi
#include "esp_hosted_mcu_spi.c"

static const struct esp_hosted_mcu_config core_config = {
	.transport = &esp_hosted_mcu_spi_api,
	.transport_config = &esp_hosted_mcu_spi_config_0,
};
static const struct device test_device = {.config = &core_config};
static uint8_t tx_frame[ESP_HOSTED_MCU_FRAME_SIZE] __aligned(ESP_HOSTED_MCU_DMA_ALIGN);
static uint8_t rx_frame[ESP_HOSTED_MCU_FRAME_SIZE] __aligned(ESP_HOSTED_MCU_DMA_ALIGN);
static uint8_t worker_rx[ESP_HOSTED_MCU_FRAME_SIZE] __aligned(ESP_HOSTED_MCU_DMA_ALIGN);
static atomic_t handshake, dataready, calls, spi_busy, hold_next;
static int spi_result;
static uint16_t payload_len;
static bool malformed;
static K_SEM_DEFINE(handshake_seen, 0, 1);
static K_SEM_DEFINE(spi_entered, 0, 1);
static K_SEM_DEFINE(spi_release_gate, 0, 1);
static K_SEM_DEFINE(worker_started, 0, 2);
static K_SEM_DEFINE(worker_done, 0, 2);
static K_THREAD_STACK_DEFINE(sender_stack, 2048);
static K_THREAD_STACK_DEFINE(receiver_stack, 2048);
static struct k_thread sender_thread, receiver_thread;
static int sender_result, receiver_result;

static int mock_gpio_set(const struct gpio_dt_spec *spec, int value)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(value);
	return 0;
}

static int mock_gpio_configure(const struct gpio_dt_spec *spec, gpio_flags_t flags)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(flags);
	return 0;
}

static int mock_gpio_callback(const struct gpio_dt_spec *spec, struct gpio_callback *cb)
{
	ARG_UNUSED(spec);
	ARG_UNUSED(cb);
	return 0;
}

static int mock_gpio_get(const struct gpio_dt_spec *spec)
{
	if (spec->pin == TEST_PIN_handshake_gpios) {
		zassert_equal(esp_hosted_mcu_spi_data_0.lock.owner, k_current_get(),
			      "handshake sampled without transaction lock");
		k_sem_give(&handshake_seen);
		return atomic_get(&handshake);
	}
	return atomic_get(&dataready);
}

static int mock_spi(const struct spi_dt_spec *spec, const struct spi_buf_set *tx,
		    const struct spi_buf_set *rx)
{
	ARG_UNUSED(spec);
	zassert_equal(esp_hosted_mcu_spi_data_0.lock.owner, k_current_get(),
		      "SPI transaction is outside the transport lock");
	zassert_true(atomic_cas(&spi_busy, 0, 1), "overlapping SPI transactions");
	zassert_equal(tx->buffers[0].len, ESP_HOSTED_MCU_FRAME_SIZE);
	zassert_equal(rx->buffers[0].len, ESP_HOSTED_MCU_FRAME_SIZE);
	zassert_equal((uintptr_t)rx->buffers[0].buf % ESP_HOSTED_MCU_DMA_ALIGN, 0);
	int sequence = atomic_inc(&calls) + 1;

	if (atomic_cas(&hold_next, 1, 0)) {
		k_sem_give(&spi_entered);
		zassert_ok(k_sem_take(&spi_release_gate, K_SECONDS(1)));
	}
	uint8_t *bytes = rx->buffers[0].buf;
	struct esp_hosted_mcu_header *hdr = (void *)bytes;

	memset(bytes, sequence, ESP_HOSTED_MCU_FRAME_SIZE);
	memset(hdr, 0, sizeof(*hdr));
	hdr->offset = malformed ? 0 : ESP_HOSTED_MCU_HEADER_SIZE;
	hdr->len = payload_len;
	hdr->if_type = ESP_HOSTED_MCU_AP_IF;
	atomic_clear(&spi_busy);
	return spi_result;
}

static int send_frame(void)
{
	return esp_hosted_mcu_spi_api.transfer(&test_device, tx_frame, sizeof(tx_frame), NULL);
}

static void receive_sequence(unsigned int expected)
{
	zassert_equal(esp_hosted_mcu_spi_api.transfer(&test_device, NULL, 0, rx_frame),
		      ESP_HOSTED_MCU_HEADER_SIZE + payload_len);
	zassert_equal(rx_frame[ESP_HOSTED_MCU_HEADER_SIZE], (uint8_t)expected);
}

static void fill_stash(void)
{
	zassert_ok(send_frame());
	zassert_true(esp_hosted_mcu_spi_api.data_ready(&test_device));
}

static void sender(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	k_sem_give(&worker_started);
	sender_result = send_frame();
	k_sem_give(&worker_done);
}

static void receiver(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	k_sem_give(&worker_started);
	receiver_result = esp_hosted_mcu_spi_api.transfer(&test_device, NULL, 0, worker_rx);
	k_sem_give(&worker_done);
}

static void start_sender(void)
{
	k_thread_create(&sender_thread, sender_stack, K_THREAD_STACK_SIZEOF(sender_stack),
			sender, NULL, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&worker_started, K_SECONDS(1)));
}

static void start_receiver(void)
{
	k_thread_create(&receiver_thread, receiver_stack, K_THREAD_STACK_SIZEOF(receiver_stack),
			receiver, NULL, NULL, NULL, K_PRIO_PREEMPT(4), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&worker_started, K_SECONDS(1)));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	atomic_set(&handshake, 1);
	atomic_clear(&dataready);
	atomic_clear(&calls);
	atomic_clear(&spi_busy);
	atomic_clear(&hold_next);
	spi_result = 0;
	payload_len = 1;
	malformed = false;
	k_sem_reset(&handshake_seen);
	k_sem_reset(&spi_entered);
	k_sem_reset(&spi_release_gate);
	k_sem_reset(&worker_started);
	k_sem_reset(&worker_done);
	zassert_ok(esp_hosted_mcu_spi_api.init(&test_device));
}

ZTEST(hosted_spi, test_repeated_transmits_preserve_received_frames)
{
	for (int round = 0; round < 20; round++) {
		fill_stash();
		/* Coprocessor already released the frames; both GPIOs can be low. */
		atomic_clear(&handshake);
		zassert_true(esp_hosted_mcu_spi_api.data_ready(&test_device));
		receive_sequence(round + 1);
		zassert_false(esp_hosted_mcu_spi_api.data_ready(&test_device));
		zassert_equal(atomic_get(&calls), round + 1);
		atomic_set(&handshake, 1);
	}
}

ZTEST(hosted_spi, test_full_stash_times_out_before_clocking)
{
	fill_stash();
	zassert_equal(send_frame(), -ENOBUFS);
	zassert_equal(atomic_get(&calls), 1);
	receive_sequence(1);
	zassert_ok(send_frame());
	receive_sequence(2);
}

ZTEST(hosted_spi, test_cooperative_sender_waits_for_receive_space)
{
	fill_stash();
	start_sender();
	k_msleep(5);
	zassert_equal(atomic_get(&calls), 1);
	receive_sequence(1);
	zassert_ok(k_sem_take(&worker_done, K_SECONDS(1)));
	zassert_ok(k_thread_join(&sender_thread, K_SECONDS(1)));
	zassert_ok(sender_result);
	receive_sequence(2);
}

ZTEST(hosted_spi, test_dummy_and_error_paths_release_reservation)
{
	payload_len = 0;
	for (int i = 0; i < 10; i++) {
		zassert_ok(send_frame());
	}
	payload_len = 1;
	malformed = true;
	zassert_ok(send_frame());
	malformed = false;
	spi_result = -EIO;
	zassert_equal(send_frame(), -EIO);
	spi_result = 0;
	int previous_calls = atomic_get(&calls);

	atomic_clear(&handshake);
	zassert_equal(send_frame(), -ETIMEDOUT);
	atomic_set(&handshake, -EIO);
	zassert_equal(send_frame(), -EIO);
	zassert_equal(atomic_get(&calls), previous_calls);
	zassert_false(esp_hosted_mcu_spi_api.data_ready(&test_device));
	atomic_set(&handshake, 1);
	zassert_ok(send_frame());
	receive_sequence(previous_calls + 1);
}

ZTEST(hosted_spi, test_waiting_receiver_rechecks_stash_without_gpio_edge)
{
	atomic_clear(&handshake);
	start_receiver();
	zassert_ok(k_sem_take(&handshake_seen, K_SECONDS(1)));
	atomic_set(&handshake, 1);
	zassert_ok(send_frame());
	/* No data-ready edge: TX received the frame and the coprocessor is idle. */
	zassert_ok(k_sem_take(&worker_done, K_SECONDS(1)));
	zassert_ok(k_thread_join(&receiver_thread, K_SECONDS(1)));
	zassert_equal(receiver_result, ESP_HOSTED_MCU_HEADER_SIZE + 1);
	zassert_equal(worker_rx[ESP_HOSTED_MCU_HEADER_SIZE], 1);
	zassert_equal(atomic_get(&calls), 1);
}

ZTEST(hosted_spi, test_receive_and_transmit_transactions_are_serialized)
{
	atomic_set(&dataready, 1);
	atomic_set(&hold_next, 1);
	start_receiver();
	zassert_ok(k_sem_take(&spi_entered, K_SECONDS(1)));
	start_sender();
	k_msleep(5);
	zassert_equal(atomic_get(&calls), 1, "TX bypassed an active RX transaction");
	k_sem_give(&spi_release_gate);
	zassert_ok(k_sem_take(&worker_done, K_SECONDS(1)));
	zassert_ok(k_sem_take(&worker_done, K_SECONDS(1)));
	zassert_ok(k_thread_join(&sender_thread, K_SECONDS(1)));
	zassert_ok(k_thread_join(&receiver_thread, K_SECONDS(1)));
	zassert_ok(sender_result);
	zassert_equal(receiver_result, ESP_HOSTED_MCU_HEADER_SIZE + 1);
	zassert_equal(worker_rx[ESP_HOSTED_MCU_HEADER_SIZE], 1);
	receive_sequence(2);
	zassert_equal(atomic_get(&calls), 2);
}

ZTEST(hosted_spi, test_full_size_frame_is_preserved)
{
	payload_len = ESP_HOSTED_MCU_MAX_PAYLOAD;
	zassert_ok(send_frame());
	receive_sequence(1);
	zassert_equal(rx_frame[ESP_HOSTED_MCU_FRAME_SIZE - 1], 1);
}

ZTEST_SUITE(hosted_spi, NULL, NULL, before, NULL, NULL);
