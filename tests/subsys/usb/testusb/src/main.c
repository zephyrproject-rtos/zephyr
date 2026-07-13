/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/ztest.h>
#include <sample_usbd.h>

#include "usbh_testusb.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define TESTUSB_DEFAULT_COUNT		1000

/* Number of "sudden device disconnect" test iterations */
#define TESTUSB_NUMOF_DISCONNECTS	100

/* Delay before the device is disabled, so transfers are queued in the meantime */
#define DISCONNECT_DELAY		K_MSEC(5)

/* Upper bound on the time to wait for the device to (re)connect and bind */
#define RECONNECT_TIMEOUT_MS		1000

static K_THREAD_STACK_DEFINE(disconnect_stack, 512);
static struct k_thread disconnect_thread;
static K_SEM_DEFINE(disconnect_sem, 0, 1);

static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
static struct usbh_context *const uhs_ctx = &test_uhs_ctx;

static void run_test(const uint32_t test, const uint32_t length,
		     const uint32_t count, const uint32_t vary)
{
	const struct usbh_testusb_param param = {
		.test = test,
		.length = length,
		.count = count,
		.vary = vary,
	};
	int err;

	err = usbh_testusb_exec(0, &param);
	zassert_ok(err, "Test %u (length %u count %u vary %u) failed with %d",
		   test, length, count, vary, err);
}

/* NOP, only verifies that device is connected */
ZTEST(testusb, test_nop)
{
	run_test(0, 0, 1, 0);
}

/* Bulk endpoint transfer sizes */
static const uint32_t bulk_sizes[] = {1, 64, 512, 513, 1024};

ZTEST(testusb, test_bulk_out)
{
	ARRAY_FOR_EACH(bulk_sizes, i) {
		run_test(1, bulk_sizes[i], TESTUSB_DEFAULT_COUNT, 0);
	}
}

ZTEST(testusb, test_bulk_in)
{
	ARRAY_FOR_EACH(bulk_sizes, i) {
		run_test(2, bulk_sizes[i], TESTUSB_DEFAULT_COUNT, 0);
	}
}

ZTEST(testusb, test_bulk_out_vary)
{
	static const uint32_t vary[] = {1, 64};

	ARRAY_FOR_EACH(vary, i) {
		run_test(3, 1024, TESTUSB_DEFAULT_COUNT, vary[i]);
	}
}

ZTEST(testusb, test_bulk_in_vary)
{
	static const uint32_t vary[] = {1, 64};

	ARRAY_FOR_EACH(vary, i) {
		run_test(4, 1024, TESTUSB_DEFAULT_COUNT, vary[i]);
	}
}

/* Set and clear the halt feature on the bulk IN and OUT endpoints. */
ZTEST(testusb, test_halt)
{
	run_test(13, 0, TESTUSB_DEFAULT_COUNT, 0);
}

/* Vendor control loopback: write a pattern and read it back unchanged. */
ZTEST(testusb, test_control)
{
	static const uint32_t vary[] = {1, 64};
	static const uint32_t count[ARRAY_SIZE(vary)] = {1024, 16};

	ARRAY_FOR_EACH(vary, i) {
		run_test(14, 1024, count[i], vary[i]);
	}
}

/* Interrupt endpoint MPS sized transfers, one transfer at a time. */
ZTEST(testusb, test_interrupt_out)
{
	run_test(25, 64, TESTUSB_DEFAULT_COUNT, 0);
}

ZTEST(testusb, test_interrupt_in)
{
	run_test(26, 64, TESTUSB_DEFAULT_COUNT, 0);
}

/* Disable device support with a delay after the semaphore is released. */
static void disconnect_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&disconnect_sem, K_FOREVER);
		k_sleep(DISCONNECT_DELAY);
		(void)usbd_disable(test_usbd);
	}
}

/* Wait until the loopback device is connected and bound to the class driver. */
static void wait_for_device(void)
{
	const struct usbh_testusb_param nop = {
		.test = 0,
		.count = 1,
	};

	for (int i = 0; i < RECONNECT_TIMEOUT_MS; i++) {
		if (usbh_testusb_exec(0, &nop) == 0) {
			return;
		}

		k_msleep(1);
	}

	zassert_unreachable("Device did not connect within %u ms",
			    RECONNECT_TIMEOUT_MS);
}

/*
 * Perform the disconnect while the USB controller is submitting transfers.
 * The parameter should trigger a large enough run to outlast the disconnect.
 */
static void disconnect_cycle(const struct usbh_testusb_param *const param)
{
	int err;

	/* Make sure the device is present before disconnecting it */
	wait_for_device();

	/* Release disconnect_fn() and allow it to disconnect the device shortly */
	k_sem_give(&disconnect_sem);

	/* The test must abort with an error, not hang */
	err = usbh_testusb_exec(0, param);
	zassert_true(err != 0, "Transfer test should fail on disconnect");

	/*
	 * The transfer test only returns once the disconnect handling is
	 * done, so the worker has finished usbd_disable() by now and the
	 * device can be reconnected for the next iteration.
	 */
	err = usbd_enable(test_usbd);
	zassert_ok(err, "Failed to re-enable device");
}

/*
 * This exercises usbh_class_api:removed and the host stack teardown with
 * transfers still in flight.
 */
ZTEST(testusb, test_disconnect)
{
	const struct usbh_testusb_param param = {
		.test = 1,
		.length = 512,
		.count = 100000,
		.vary = 0,
	};
	k_tid_t tid;

	tid = k_thread_create(&disconnect_thread, disconnect_stack,
			      K_THREAD_STACK_SIZEOF(disconnect_stack),
			      disconnect_fn, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	for (uint32_t i = 0; i < TESTUSB_NUMOF_DISCONNECTS; i++) {
		disconnect_cycle(&param);
	}

	k_thread_abort(tid);

	/* Check that the stack is recovered */
	wait_for_device();
	run_test(1, 512, TESTUSB_DEFAULT_COUNT, 0);
}

static void *testusb_setup(void)
{
	int ret;

	ret = usbh_init(uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	ret = uhc_bus_reset(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus reset");

	ret = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus resume");

	ret = uhc_sof_enable(uhs_ctx->dev);
	zassert_ok(ret, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	test_usbd = sample_usbd_setup_device(NULL);
	zassert_not_null(test_usbd, "Failed to setup USB device");

	ret = usbd_init(test_usbd);
	zassert_ok(ret, "Failed to initialize device support");

	ret = usbd_enable(test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/* Give the host time to enumerate and configure the device. */
	k_msleep(500);

	return NULL;
}

static void testusb_teardown(void *f)
{
	int ret;

	ARG_UNUSED(f);

	ret = usbd_disable(test_usbd);
	zassert_ok(ret, "Failed to disable device support");

	ret = usbd_shutdown(test_usbd);
	zassert_ok(ret, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	ret = usbh_disable(uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	ret = usbh_shutdown(uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(testusb, NULL, testusb_setup, NULL, NULL, testusb_teardown);
