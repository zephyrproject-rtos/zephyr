/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/ztest.h>

#include <hardware/regs/otp.h>
#include <hardware/structs/otp.h>
#include <pico/bootrom.h>
#include <sample_usbd.h>

#include <rp2350_heterogeneous_test.h>

#define BURST_SIZE 4U
#define BURST_COUNT 32U
#define WAIT_STEP K_MSEC(1)

static const struct device *const ipm = DEVICE_DT_GET(DT_NODELABEL(ipm_mbox));
static volatile struct shared_status *const status = (void *)SHARED_STATUS_ADDR;
static volatile uint32_t received[BURST_SIZE];
static volatile uint32_t received_count;

static bool wait_for_value(volatile uint32_t *value, uint32_t expected, k_timeout_t timeout)
{
	int64_t deadline = k_uptime_get() + k_ticks_to_ms_floor64(timeout.ticks);

	while (*value != expected && k_uptime_get() < deadline) {
		k_sleep(WAIT_STEP);
	}

	return *value == expected;
}

static void mailbox_callback(const struct device *dev, void *user_data, uint32_t id,
			     volatile void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(id);

	if (data != NULL && received_count < ARRAY_SIZE(received)) {
		received[received_count++] = *(volatile uint32_t *)data;
	}
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(ipm), "IPM device is not ready");
	ipm_register_callback(ipm, mailbox_callback, NULL);
	zassert_ok(ipm_set_enabled(ipm, 1), "cannot enable IPM interrupt");
	zassert_true(wait_for_value(&status->magic, SHARED_MAGIC, K_SECONDS(2)),
		     "Hazard3 image did not start");
	zassert_equal(status->hart_id, 1U, "remote image is not running on hart 1");
	zassert_true(wait_for_value(&status->mailbox_ready, 1U, K_SECONDS(1)),
		     "Hazard3 mailbox is not ready");

	return NULL;
}

ZTEST(rp2350_heterogeneous, test_architecture_and_remote_timer)
{
	uint32_t before = status->counter;

	zassert_not_equal(otp_hw->archsel_status & OTP_ARCHSEL_STATUS_CORE1_BITS, 0U,
			  "CPU1 is not running as RISC-V");
	k_sleep(K_MSEC(350));
	zassert_true(status->counter - before >= 2U,
		     "Hazard3 timer did not wake repeated sleeps");
}

ZTEST(rp2350_heterogeneous, test_queued_mailbox_bursts)
{
	for (uint32_t burst = 0; burst < BURST_COUNT; burst++) {
		uint32_t remote_before = status->mailbox_responses;

		zassert_ok(ipm_set_enabled(ipm, 0), "cannot mask CPU0 IPM interrupt");
		received_count = 0U;

		for (uint32_t slot = 0; slot < BURST_SIZE; slot++) {
			uint32_t request = burst * BURST_SIZE + slot + 1U;
			int64_t deadline = k_uptime_get() + 100;
			int ret;

			do {
				ret = ipm_send(ipm, 0, 0, &request, sizeof(request));
			} while (ret == -EBUSY && k_uptime_get() < deadline);
			zassert_ok(ret, "request %u was not queued", request);
		}

		zassert_true(wait_for_value(&status->mailbox_responses,
					    remote_before + BURST_SIZE, K_MSEC(500)),
			     "Hazard3 did not return all responses in burst %u", burst);
		zassert_ok(ipm_set_enabled(ipm, 1), "cannot unmask CPU0 IPM interrupt");
		zassert_true(wait_for_value(&received_count, BURST_SIZE, K_MSEC(100)),
			     "CPU0 ISR lost queued responses in burst %u", burst);

		for (uint32_t slot = 0; slot < BURST_SIZE; slot++) {
			uint32_t request = burst * BURST_SIZE + slot + 1U;

			zassert_equal(received[slot], request ^ MAILBOX_RESPONSE_XOR,
				      "response %u is corrupt", request);
		}
	}

	zassert_equal(status->mailbox_responses, BURST_SIZE * BURST_COUNT,
		      "remote response total is wrong");
}

ZTEST_SUITE(rp2350_heterogeneous, NULL, suite_setup, NULL, NULL, NULL);

static void bootsel_thread(void)
{
	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	for (;;) {
		uint8_t input;

		if (device_is_ready(console) && uart_poll_in(console, &input) == 0 &&
		    input == 'b') {
			k_sleep(K_MSEC(50));
			reset_usb_boot(0U, 0U);
		}
		k_sleep(K_MSEC(10));
	}
}

K_THREAD_DEFINE(bootsel_tid, 768, bootsel_thread, NULL, NULL, NULL, 0, 0, 0);

void test_main(void)
{
	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	struct usbd_context *sample_usbd = sample_usbd_init_device(NULL);
	uint32_t dtr = 0U;

	if (sample_usbd == NULL ||
	    (!usbd_can_detect_vbus(sample_usbd) && usbd_enable(sample_usbd) != 0)) {
		return;
	}

	while (dtr == 0U) {
		(void)uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(10));
	}

	ztest_run_all(NULL, false, 1, 1);
	ztest_verify_all_test_suites_ran();
}
