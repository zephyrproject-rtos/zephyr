/*
 * Copyright (c) 2026 Neoventor AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../test_work.h"
#include <udc_common.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/net_buf.h>
#include <zephyr/usb/usbd.h>

static bool fail_enqueue;
static bool refuse_always;
static bool resume_on_refusal;
static uint32_t enqueue_failures;
static uint32_t out_armed;

/* Hold only test-side allocation, after the driver's enabled check. */
static K_SEM_DEFINE(notification_entered, 0, 1);
static K_SEM_DEFINE(notification_release, 0, 1);
static K_SEM_DEFINE(notification_queued, 0, 1);
static bool notification_test;
static bool notification_hold;
static bool notification_accept;
static bool notification_detached;
static uint32_t notification_allocations;
static uint8_t notification_ep;

static struct net_buf *notification_alloc(const struct usbd_class_data *c_data,
					 uint8_t ep, size_t size)
{
	if (notification_test && USB_EP_DIR_IS_IN(ep)) {
		notification_ep = ep;
		notification_allocations++;
		if (notification_hold) {
			k_sem_give(&notification_entered);
			(void)k_sem_take(&notification_release, K_MSEC(250));
		}
		if (c_data->uds_ctx == NULL) {
			/* Observe the bug without dereferencing the detached context. */
			notification_detached = true;
			return NULL;
		}
	}
	return usbd_ep_buf_alloc(c_data, ep, size);
}

static int enqueue_seam(const struct usbd_class_data *c_data, struct net_buf *buf)
{
	bool out = USB_EP_DIR_IS_OUT(udc_get_buf_info(buf)->ep);
	int ret;

	if (out && (fail_enqueue || refuse_always)) {
		fail_enqueue = false;
		enqueue_failures++;
		if (resume_on_refusal) {
			resume_on_refusal = false;
			c_data->api->resumed((struct usbd_class_data *)c_data);
		}
		return -EIO;
	}

	if (notification_test && udc_get_buf_info(buf)->ep == notification_ep) {
		k_sem_give(&notification_queued);
		if (notification_accept) {
			/* The test controller owns it; completion is released separately. */
			net_buf_unref(buf);
			return 0;
		}
		return -EIO;
	}

	ret = usbd_ep_enqueue(c_data, buf);
	if (out && ret == 0) {
		out_armed++;
	}
	return ret;
}

/* Observe entry before the send helper's enabled-state guard. */
static const struct device *tx_enqueue_device;
static const struct usbd_class_data *tx_enqueue_class;
static bool tx_enqueue_detached;

static void tx_enqueue_entered(const struct device *dev)
{
	if (dev == tx_enqueue_device && tx_enqueue_class != NULL &&
	    tx_enqueue_class->uds_ctx == NULL) {
		tx_enqueue_detached = true;
	}
}

/* Compile the complete driver once, in its usual library. */

#define usbd_ep_buf_alloc notification_alloc
#define usbd_ep_enqueue enqueue_seam
#define k_work_submit(work) test_submit_to_queue(&k_sys_work_q, work)
#define k_work_schedule(work, delay) test_schedule_for_queue(&k_sys_work_q, work, delay)
#define k_work_reschedule(work, delay) test_reschedule_for_queue(&k_sys_work_q, work, delay)
#define k_work_submit_to_queue test_submit_to_queue
#define k_work_schedule_for_queue test_schedule_for_queue
#define k_work_reschedule_for_queue test_reschedule_for_queue
#define k_work_cancel test_cancel
#define k_work_cancel_sync test_cancel_sync
#define k_work_cancel_delayable_sync test_cancel_delayable_sync
#include <usbd_cdc_acm_test_driver.c>
#undef k_work_cancel_delayable_sync
#undef k_work_cancel_sync
#undef k_work_cancel
#undef k_work_reschedule_for_queue
#undef k_work_schedule_for_queue
#undef k_work_submit_to_queue
#undef k_work_reschedule
#undef k_work_schedule
#undef k_work_submit
#undef usbd_ep_enqueue
#undef usbd_ep_buf_alloc


USBD_CONFIGURATION_DEFINE(rx_fs, 0, 50, NULL);
USBD_CONFIGURATION_DEFINE(rx_hs, 0, 50, NULL);
USBD_DEVICE_DEFINE(rx_device, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);

static const struct device *const uart = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));

static struct usbd_class_data *rx_class(void)
{
	const struct cdc_acm_uart_config *cfg = uart->config;

	return cfg->c_data;
}

static struct k_work *rx_work(void)
{
	struct cdc_acm_uart_data *data = uart->data;

#if RX_WORK_DELAYABLE
	return &data->rx_fifo_work.work;
#else
	return &data->rx_fifo_work;
#endif
}

static struct net_buf *take_out(void)
{
	struct udc_ep_config *cfg;

	cfg = udc_get_ep_cfg(rx_device.dev, cdc_acm_get_bulk_out(rx_class()));
	zassert_not_null(cfg);
	return udc_buf_get(cfg);
}

static void release_endpoint(uint8_t ep)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(rx_device.dev, ep);
	struct net_buf *buf;

	zassert_not_null(cfg);
	while ((buf = udc_buf_get(cfg)) != NULL) {
		zassert_ok(usbd_cdc_acm_request(rx_class(), buf, -ECONNABORTED));
	}
}

static void cleanup(void)
{
	struct cdc_acm_uart_data *data = uart->data;
	struct net_buf *buf;

	usbd_cdc_acm_disable(rx_class());
	test_work_reset();
	cdc_acm_irq_callback_set(uart, NULL, NULL);
	cdc_acm_irq_rx_disable(uart);
	cdc_acm_irq_tx_disable(uart);
	release_endpoint(cdc_acm_get_bulk_out(rx_class()));
	release_endpoint(cdc_acm_get_bulk_in(rx_class()));
	while ((buf = k_fifo_get(data->rx_fifo.bufs, K_NO_WAIT)) != NULL) {
		net_buf_unref(buf);
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	cleanup();
	fail_enqueue = false;
	refuse_always = false;
	resume_on_refusal = false;
	enqueue_failures = 0;
	out_armed = 0;

	zassert_ok(usbd_cdc_acm_init(rx_class()));
	usbd_cdc_acm_enable(rx_class());
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	cleanup();
}

static void *setup(void)
{
	zassert_true(device_is_ready(uart));
	zassert_ok(usbd_add_configuration(&rx_device, USBD_SPEED_FS, &rx_fs));
	zassert_ok(usbd_add_configuration(&rx_device, USBD_SPEED_HS, &rx_hs));
	zassert_ok(usbd_register_class(&rx_device, "cdc_acm_0", USBD_SPEED_FS, 1));
	zassert_ok(usbd_register_class(&rx_device, "cdc_acm_0", USBD_SPEED_HS, 1));
	zassert_ok(usbd_init(&rx_device));
	zassert_ok(usbd_enable(&rx_device));
	zassert_ok(udc_ep_enable(rx_device.dev, cdc_acm_get_bulk_out(rx_class()),
				 USB_EP_TYPE_BULK, cdc_acm_get_bulk_mps(rx_class()), 0));
	zassert_ok(udc_ep_enable(rx_device.dev, cdc_acm_get_bulk_in(rx_class()),
				 USB_EP_TYPE_BULK, cdc_acm_get_bulk_mps(rx_class()), 0));
	return NULL;
}

ZTEST(cdc_acm_work, test_retry_backoff_and_success_reset)
{
	static const uint32_t delays[] = {1U, 2U, 4U, 8U, 16U, 32U, 64U, 128U, 128U};
	struct net_buf *buf;
	unsigned char byte;
	uint32_t failures;

	refuse_always = true;
	test_advance(K_NO_WAIT);
	for (size_t i = 0; i < ARRAY_SIZE(delays); i++) {
		zassert_equal(enqueue_failures, i + 1U);
		zassert_equal(test_work_delay(rx_work()), K_MSEC(delays[i]).ticks,
			      "retry delay did not follow capped backoff");
		if (i + 1U < ARRAY_SIZE(delays)) {
			test_advance(K_MSEC(delays[i]));
		}
	}

	refuse_always = false;
	test_advance(K_MSEC(128));
	zassert_true(out_armed > 0, "retry did not recover");
	buf = take_out();
	zassert_not_null(buf);
	net_buf_add_u8(buf, 0x5a);
	zassert_ok(usbd_cdc_acm_request(rx_class(), buf, 0));
	refuse_always = true;
	zassert_ok(cdc_acm_poll_in(uart, &byte));
	test_advance(K_NO_WAIT);
	zassert_equal(test_work_delay(rx_work()), K_MSEC(1).ticks,
		      "successful enqueue did not reset backoff");
	usbd_cdc_acm_resumed(rx_class());
	zassert_equal(test_work_delay(rx_work()), 0, "resume waited for backoff");
	/* A resume can arrive before the controller returns its refusal. */
	resume_on_refusal = true;
	failures = enqueue_failures;
	test_advance(K_NO_WAIT);
	zassert_equal(enqueue_failures, failures + 2U,
		      "refusal replaced an immediate lifecycle event with backoff");
}

/* A bounded stand-in for usbd_wakeup_request() taking the same mutex. */
static K_SEM_DEFINE(device_locked, 0, 1);
static K_SEM_DEFINE(callback_entered, 0, 1);
static K_SEM_DEFINE(shutdown_done, 0, 1);
static K_SEM_DEFINE(release_device_lock, 0, 1);
static K_THREAD_STACK_DEFINE(shutdown_stack, 2048);
static struct k_thread shutdown_thread;
static uint32_t blocked_callbacks;

static void blocking_callback(const struct device *dev, void *user_data)
{
	struct usbd_context *ctx = user_data;
	uint8_t byte = 0x5a;
	int ret;

	blocked_callbacks++;
	k_sem_give(&callback_entered);
	/* A timeout also frees the red control's synchronous canceller. */
	ret = k_mutex_lock(&ctx->mutex, K_MSEC(250));
	if (ret == 0) {
		k_mutex_unlock(&ctx->mutex);
	}
	uart_irq_tx_enable(dev);
	uart_irq_rx_enable(dev);
	(void)uart_fifo_fill(dev, &byte, 1);
	uart_poll_out(dev, byte);
}

static void run_shutdown(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	(void)k_mutex_lock(&rx_device.mutex, K_FOREVER);
	k_sem_give(&device_locked);
	if (k_sem_take(&callback_entered, K_SECONDS(1)) == 0) {
		rx_class()->api->shutdown(rx_class());
		k_sem_give(&shutdown_done);
	}
	(void)k_sem_take(&release_device_lock, K_SECONDS(1));
	k_mutex_unlock(&rx_device.mutex);
}

ZTEST(cdc_acm_work, test_shutdown_does_not_wait_for_application)
{
	struct cdc_acm_uart_data *data = uart->data;
	struct usbd_context *ctx = rx_class()->uds_ctx;
	struct k_work_sync sync;
	uint32_t runs;
	int ret;
	int shutdown_join;

	if (!IS_ENABLED(CONFIG_USBD_CDC_ACM_WORKQUEUE)) {
		/* A blocked callback would also block virtual-host system work. */
		ztest_test_skip();
		return;
	}

	zassert_not_null(rx_class()->api->shutdown, "class shutdown is not registered");
	refuse_always = true;
	test_advance(K_NO_WAIT);
	blocked_callbacks = 0U;
	k_sem_reset(&device_locked);
	k_sem_reset(&callback_entered);
	k_sem_reset(&shutdown_done);
	k_sem_reset(&release_device_lock);
	cdc_acm_irq_callback_set(uart, blocking_callback, ctx);
	/* Run the callback on the dedicated queue, leaving UHC events free. */
	test_real_work = &data->irq_cb_work;
	k_thread_create(&shutdown_thread, shutdown_stack,
			K_THREAD_STACK_SIZEOF(shutdown_stack), run_shutdown,
			NULL, NULL, NULL, 1, 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&device_locked, K_SECONDS(1)));
	cdc_acm_irq_tx_enable(uart);
	ret = k_sem_take(&shutdown_done, K_MSEC(50));
	if (ret == 0) {
		/* Detach only in green; the callback still waits for this lock. */
		rx_class()->uds_ctx = NULL;
	}
	k_sem_give(&release_device_lock);
	shutdown_join = k_thread_join(&shutdown_thread, K_SECONDS(1));
	(void)k_work_flush(&data->irq_cb_work, &sync);
	test_real_work = NULL;
	rx_class()->uds_ctx = ctx;
	zassert_ok(shutdown_join);
	zassert_ok(ret, "shutdown waited for application code under the device lock");
	zassert_equal(blocked_callbacks, 1U);
	zassert_false(test_work_pending(&data->irq_cb_work));
	zassert_false(test_work_pending(&data->tx_fifo_work.work));
	zassert_false(test_work_pending(rx_work()));
	runs = test_work_runs;
	rx_class()->uds_ctx = NULL;
	/* Late producers and already dequeued handlers must also refuse. */
	usbd_cdc_acm_enable(rx_class());
	usbd_cdc_acm_resumed(rx_class());
	cdc_acm_irq_tx_enable(uart);
	cdc_acm_irq_rx_enable(uart);
	cdc_acm_tx_fifo_handler(&data->tx_fifo_work.work);
	cdc_acm_rx_fifo_handler(rx_work());
	cdc_acm_irq_cb_handler(&data->irq_cb_work);
	test_advance(K_SECONDS(1));
	rx_class()->uds_ctx = ctx;
	zassert_equal(test_work_runs, runs, "class work ran after shutdown");
	zassert_equal(blocked_callbacks, 1U, "callback restarted after shutdown");
}

static uint8_t late_tx_bytes[USBD_MAX_BULK_MPS];
static size_t late_tx_tailroom;
static int late_tx_written;
static bool late_tx_full;

static void late_filling_callback(const struct device *dev, void *user_data)
{
	struct usbd_class_data *c_data = user_data;
	struct usbd_context *ctx = c_data->uds_ctx;
	struct cdc_acm_uart_data *data = dev->data;

	/* Continue an admitted callback after shutdown and class detachment. */
	c_data->api->shutdown(c_data);
	c_data->uds_ctx = NULL;
	late_tx_written = uart_fifo_fill(dev, late_tx_bytes, late_tx_tailroom);
	late_tx_full = data->tx_fifo.current != NULL &&
		       net_buf_tailroom(data->tx_fifo.current) == 0U;
	uart_irq_tx_disable(dev);
	c_data->uds_ctx = ctx;
}

ZTEST(cdc_acm_work, test_shutdown_full_tx_buffer_stays_in_worker)
{
	struct cdc_acm_uart_data *data = uart->data;

	zassert_not_null(rx_class()->api->shutdown, "class shutdown is not registered");
	test_advance(K_NO_WAIT);
	zassert_not_null(data->tx_fifo.current);
	late_tx_tailroom = net_buf_tailroom(data->tx_fifo.current);
	zassert_true(late_tx_tailroom > 0U && late_tx_tailroom <= sizeof(late_tx_bytes));
	late_tx_written = 0;
	late_tx_full = false;
	tx_enqueue_detached = false;
	tx_enqueue_device = uart;
	tx_enqueue_class = rx_class();
	data->echo_mitigated = true;
	cdc_acm_irq_callback_set(uart, late_filling_callback, rx_class());
	cdc_acm_irq_tx_enable(uart);
	test_advance(K_NO_WAIT);
	tx_enqueue_class = NULL;
	tx_enqueue_device = NULL;
	zassert_equal(late_tx_written, late_tx_tailroom, "late callback did not fill TX buffer");
	zassert_true(late_tx_full, "late callback left TX buffer space");
	zassert_false(tx_enqueue_detached, "late FIFO fill entered send enqueue after detachment");
	zassert_false(test_work_pending(&data->tx_fifo_work.work));
}

ZTEST(cdc_acm_work, test_shutdown_cancels_pending_retry)
{
	struct usbd_class_data *c_data = rx_class();
	struct usbd_context *ctx = c_data->uds_ctx;
	struct cdc_acm_uart_data *data = uart->data;
	uint32_t runs;

	refuse_always = true;
	test_advance(K_NO_WAIT);
	zassert_equal(enqueue_failures, 1U);
	zassert_true(test_work_pending(rx_work()));
	if (c_data->api->shutdown != NULL) {
		c_data->api->shutdown(c_data);
	}
	/* Assert before detaching: a missing shutdown handler must not fault. */
	zassert_false(test_work_pending(rx_work()), "shutdown left RX retry pending");
	zassert_false(test_work_pending(&data->tx_fifo_work.work));
	zassert_false(test_work_pending(&data->irq_cb_work));
	runs = test_work_runs;
	c_data->uds_ctx = NULL;
	test_advance(K_SECONDS(1));
	c_data->uds_ctx = ctx;
	zassert_equal(test_work_runs, runs, "work ran after shutdown returned");
	zassert_equal(enqueue_failures, 1U);
}

static void interface_reenable(bool completion_first)
{
	const struct cdc_acm_uart_config *cfg = uart->config;
	struct net_buf *cancelled[16];
	size_t count = 0U;
	uint32_t armed_before;

	test_advance(K_NO_WAIT);
	zassert_false(test_work_pending(rx_work()));
	/* Hold cancelled buffers until before or after the update callback. */
	while (count < ARRAY_SIZE(cancelled)) {
		cancelled[count] = take_out();
		if (cancelled[count] == NULL) {
			break;
		}
		count++;
	}
	zassert_true(count > 0U && count < ARRAY_SIZE(cancelled));
	armed_before = out_armed;
	if (!completion_first) {
		usbd_cdc_acm_update(rx_class(), cfg->desc->if1.bInterfaceNumber, 0);
		test_advance(K_NO_WAIT);
	}
	for (size_t i = 0; i < count; i++) {
		zassert_ok(usbd_cdc_acm_request(rx_class(), cancelled[i], -ECONNABORTED));
	}
	if (completion_first) {
		/* Run before update too: a refused enqueue may already be pending. */
		test_advance(K_NO_WAIT);
		usbd_cdc_acm_update(rx_class(), cfg->desc->if1.bInterfaceNumber, 0);
	}
	test_advance(K_NO_WAIT);
	zassert_true(out_armed > armed_before, "interface re-enable left RX unarmed");
}

ZTEST(cdc_acm_work, test_interface_update_before_cancel_completion)
{
	interface_reenable(false);
}

ZTEST(cdc_acm_work, test_interface_update_after_cancel_completion)
{
	interface_reenable(true);
}

ZTEST(cdc_acm_work, test_interface_update_skips_disabled_endpoint_backoff)
{
	const struct cdc_acm_uart_config *cfg = uart->config;
	struct net_buf *cancelled[16];
	size_t count = 0U;
	uint32_t armed_before;
	int64_t update_ticks;
	uint8_t ep = cdc_acm_get_bulk_out(rx_class());

	test_advance(K_NO_WAIT);
	zassert_false(test_work_pending(rx_work()));
	while (count < ARRAY_SIZE(cancelled)) {
		cancelled[count] = take_out();
		if (cancelled[count] == NULL) {
			break;
		}
		count++;
	}
	zassert_true(count > 0U && count < ARRAY_SIZE(cancelled));
	armed_before = out_armed;
	/* Return every held cancellation while the endpoint is still off. */
	zassert_ok(udc_ep_disable(rx_device.dev, ep));
	for (size_t i = 0; i < count; i++) {
		zassert_ok(usbd_cdc_acm_request(rx_class(), cancelled[i], -ECONNABORTED));
	}
	test_advance(K_NO_WAIT);
	zassert_equal(out_armed, armed_before, "disabled endpoint accepted RX");
	zassert_ok(udc_ep_enable(rx_device.dev, ep, USB_EP_TYPE_BULK,
				 cdc_acm_get_bulk_mps(rx_class()), 0));
	update_ticks = test_ticks;
	usbd_cdc_acm_update(rx_class(), cfg->desc->if1.bInterfaceNumber, 0);
	/* Run immediate work only: the refused enqueue's retry is still later. */
	test_advance(K_NO_WAIT);
	zassert_equal(test_ticks, update_ticks, "interface update advanced retry time");
	zassert_true(out_armed > armed_before, "interface update waited for RX backoff");
}

ZTEST(cdc_acm_work, test_enqueue_failure_rearms)
{
	fail_enqueue = true;
	test_advance(K_NO_WAIT);
	zassert_equal(enqueue_failures, 1, "enqueue seam was not reached");
	zassert_equal(out_armed, 0);
	test_advance(K_MSEC(1));
	zassert_true(out_armed > 0, "failed enqueue left RX unarmed");
}

ZTEST(cdc_acm_work, test_resume_rearms)
{
	/* Consume the initial work while suspended: there is no pending retry. */
	usbd_cdc_acm_suspended(rx_class());
	test_advance(K_NO_WAIT);
	zassert_equal(out_armed, 0);
	zassert_false(test_work_pending(rx_work()));
	usbd_cdc_acm_resumed(rx_class());
	test_advance(K_NO_WAIT);
	zassert_true(out_armed > 0, "resume did not rearm RX");
}

ZTEST(cdc_acm_work, test_retry_consumed_during_suspend)
{
	fail_enqueue = true;
	test_advance(K_NO_WAIT);
	zassert_equal(enqueue_failures, 1);
	usbd_cdc_acm_suspended(rx_class());
	test_advance(K_MSEC(1));
	zassert_equal(out_armed, 0, "RX enqueued while suspended");
	zassert_false(test_work_pending(rx_work()));
	usbd_cdc_acm_resumed(rx_class());
	test_advance(K_NO_WAIT);
	zassert_true(out_armed > 0, "suspended retry lost recovery on resume");
}

ZTEST(cdc_acm_work, test_disable_stops_retry)
{
	fail_enqueue = true;
	test_advance(K_NO_WAIT);
	zassert_equal(enqueue_failures, 1);
	usbd_cdc_acm_disable(rx_class());
	test_advance(K_MSEC(1));
	zassert_equal(out_armed, 0);
	zassert_false(test_work_pending(rx_work()));
	usbd_cdc_acm_resumed(rx_class());
	test_advance(K_NO_WAIT);
	zassert_equal(out_armed, 0, "resume armed a disabled class");
}

ZTEST(cdc_acm_work, test_no_fault_arms)
{
	test_advance(K_NO_WAIT);
	zassert_true(out_armed > 0);
	zassert_equal(enqueue_failures, 0);
}


static void fill_rx_pool(void)
{
	struct net_buf *buf;

	test_advance(K_NO_WAIT);
	/* Complete real queued OUT buffers into the driver's receive FIFO. */
	while ((buf = take_out()) != NULL) {
		net_buf_add_u8(buf, 0x5a);
		zassert_ok(usbd_cdc_acm_request(rx_class(), buf, 0));
	}
	test_advance(K_NO_WAIT);
	zassert_false(test_work_pending(rx_work()), "normal pool pressure must not poll");
}

ZTEST(cdc_acm_work, test_poll_drain_rearms_without_polling)
{
	uint32_t armed_before;
	unsigned char byte;

	fill_rx_pool();
	armed_before = out_armed;
	test_advance(K_MSEC(10));
	zassert_equal(out_armed, armed_before);
	zassert_ok(cdc_acm_poll_in(uart, &byte));
	zassert_equal(byte, 0x5a);
	test_advance(K_NO_WAIT);
	zassert_true(out_armed > armed_before, "poll drain did not rearm RX");
}

static void drain_callback(const struct device *dev, void *user_data)
{
	uint8_t byte;

	ARG_UNUSED(user_data);
	cdc_acm_irq_update(dev);
	if (cdc_acm_irq_rx_ready(dev) != 0) {
		zassert_equal(cdc_acm_fifo_read(dev, &byte, 1), 1);
	}
}

ZTEST(cdc_acm_work, test_irq_drain_rearms_without_polling)
{
	uint32_t armed_before;

	fill_rx_pool();
	armed_before = out_armed;
	cdc_acm_irq_callback_set(uart, drain_callback, NULL);
	cdc_acm_irq_rx_enable(uart);
	test_advance(K_NO_WAIT);
	zassert_true(out_armed > armed_before, "IRQ drain did not rearm RX");
}


static K_SEM_DEFINE(notification_done, 0, 1);
static K_SEM_DEFINE(notification_shutdown_started, 0, 1);
static K_SEM_DEFINE(notification_shutdown_done, 0, 1);
static K_THREAD_STACK_DEFINE(notification_stack, 2048);
static K_THREAD_STACK_DEFINE(notification_shutdown_stack, 2048);
static struct k_thread notification_thread;
static struct k_thread notification_shutdown_thread;
static int notification_result;

static void run_notification(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	notification_result = uart_line_ctrl_set(uart, UART_LINE_CTRL_DCD, 1U);
	k_sem_give(&notification_done);
}

static void shutdown_notification(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	(void)k_mutex_lock(&rx_device.mutex, K_FOREVER);
	k_sem_give(&notification_shutdown_started);
	if (rx_class()->api->shutdown != NULL) {
		rx_class()->api->shutdown(rx_class());
	}
	/* Match class removal: detach as soon as shutdown returns. */
	rx_class()->uds_ctx = NULL;
	k_sem_give(&notification_shutdown_done);
	k_mutex_unlock(&rx_device.mutex);
}

static void notification_shutdown_case(bool hold_allocation)
{
	struct usbd_context *ctx = rx_class()->uds_ctx;
	struct cdc_acm_uart_data *data = uart->data;
	uint32_t allocations;
	int shutdown_ret;
	int notify_join;
	int shutdown_join;
	int disabled_ret;

	notification_test = true;
	notification_hold = hold_allocation;
	notification_accept = !hold_allocation;
	notification_detached = false;
	notification_allocations = 0U;
	k_sem_reset(&notification_entered);
	k_sem_reset(&notification_release);
	k_sem_reset(&notification_queued);
	k_sem_reset(&notification_done);
	k_sem_reset(&notification_shutdown_started);
	k_sem_reset(&notification_shutdown_done);
	k_sem_reset(&data->notif_sem);
	k_thread_create(&notification_thread, notification_stack,
			K_THREAD_STACK_SIZEOF(notification_stack), run_notification,
			NULL, NULL, NULL, 1, 0, K_NO_WAIT);
	if (hold_allocation) {
		zassert_ok(k_sem_take(&notification_entered, K_SECONDS(1)));
	} else {
		/* Enqueue succeeds, but the test controller withholds completion. */
		zassert_ok(k_sem_take(&notification_queued, K_SECONDS(1)));
	}
	k_thread_create(&notification_shutdown_thread, notification_shutdown_stack,
			K_THREAD_STACK_SIZEOF(notification_shutdown_stack), shutdown_notification,
			NULL, NULL, NULL, 1, 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&notification_shutdown_started, K_SECONDS(1)));
	shutdown_ret = k_sem_take(&notification_shutdown_done, K_MSEC(50));
	/* Release either seam before asserting, including in a broken driver. */
	k_sem_give(&notification_release);
	k_sem_give(&data->notif_sem);
	notify_join = k_thread_join(&notification_thread, K_SECONDS(1));
	shutdown_join = k_thread_join(&notification_shutdown_thread, K_SECONDS(1));
	allocations = notification_allocations;
	/* Still detached: no new notification may even enter allocation. */
	if (atomic_test_bit(&data->state, CDC_ACM_CLASS_ENABLED)) {
		/* Old controls have no shutdown: do not look up an endpoint through NULL. */
		disabled_ret = 0;
	} else {
		disabled_ret = uart_line_ctrl_set(uart, UART_LINE_CTRL_DCD, 0U);
	}
	rx_class()->uds_ctx = ctx;
	notification_test = false;
	test_work_reset();
	zassert_ok(usbd_cdc_acm_init(rx_class()));
	usbd_cdc_acm_enable(rx_class());
	zassert_ok(notify_join);
	zassert_ok(shutdown_join);
	zassert_false(notification_detached, "shutdown detached a notification section");
	if (hold_allocation) {
		zassert_equal(shutdown_ret, -EAGAIN,
			      "shutdown did not join notification allocation");
		zassert_equal(notification_result, -EIO);
	} else {
		zassert_ok(shutdown_ret, "shutdown waited for notification completion");
		zassert_ok(notification_result);
	}
	zassert_equal(disabled_ret, -EACCES, "detached notification was not refused");
	zassert_equal(notification_allocations, allocations,
		      "late notification reached allocation");
}

ZTEST(cdc_acm_work, test_notification_shutdown_joins_section)
{
	notification_shutdown_case(true);
	notification_shutdown_case(false);
}

static uint8_t tx_bytes[USBD_MAX_BULK_MPS * 3U + 7U];
static uint8_t tx_expected[sizeof(tx_bytes) +
	ROUND_UP(DT_PROP(DT_NODELABEL(cdc_acm_uart0), tx_fifo_size), USBD_MAX_BULK_MPS)];
static size_t tx_written;
static bool tx_false_ready;
static bool tx_loop_limit;

static void filling_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	uart_irq_update(dev);
	for (size_t step = 0; step < 16U; step++) {
		int ready = uart_irq_tx_ready(dev);
		int done;

		if (ready <= 0) {
			return;
		}
		done = uart_fifo_fill(dev, &tx_bytes[tx_written],
				     MIN((size_t)ready, sizeof(tx_bytes) - tx_written));
		if (done == 0) {
			tx_false_ready = uart_irq_tx_ready(dev) > 0;
			uart_irq_tx_disable(dev);
			return;
		}
		tx_written += done;
		if (tx_written == sizeof(tx_bytes)) {
			uart_irq_tx_disable(dev);
			return;
		}
	}
	tx_loop_limit = true;
	uart_irq_tx_disable(dev);
}

static size_t receive_tx(struct udc_ep_config *cfg, size_t received, size_t expected)
{
	struct net_buf *buf;

	while ((buf = udc_buf_get(cfg)) != NULL) {
		zassert_true(received + buf->len <= expected);
		zassert_mem_equal(buf->data, &tx_expected[received], buf->len,
				  "host received TX bytes out of order");
		received += buf->len;
		zassert_ok(usbd_cdc_acm_request(rx_class(), buf, 0));
	}
	return received;
}

ZTEST(cdc_acm_work, test_tx_ready_tracks_writable_buffer)
{
	struct cdc_acm_uart_data *data = uart->data;
	struct udc_ep_config *cfg;
	struct net_buf *buf;
	size_t prefix = 0U;
	size_t expected;
	size_t received = 0U;
	size_t written_before_completion;

	cfg = udc_get_ep_cfg(rx_device.dev, cdc_acm_get_bulk_in(rx_class()));
	zassert_not_null(cfg);
	/* No class work runs until test_advance(): snapshot without consuming. */
	for (buf = udc_buf_peek(cfg); buf != NULL;
	     buf = (struct net_buf *)sys_sflist_peek_next((sys_sfnode_t *)buf)) {
		zassert_true(prefix + buf->len <= sizeof(tx_expected) - sizeof(tx_bytes));
		memcpy(&tx_expected[prefix], buf->data, buf->len);
		prefix += buf->len;
	}
	buf = data->tx_fifo.current;
	if (buf != NULL) {
		zassert_true(prefix + buf->len <= sizeof(tx_expected) - sizeof(tx_bytes));
		memcpy(&tx_expected[prefix], buf->data, buf->len);
		prefix += buf->len;
	}
	for (size_t i = 0; i < sizeof(tx_bytes); i++) {
		tx_bytes[i] = (uint8_t)(i + i / USBD_MAX_BULK_MPS);
	}
	memcpy(&tx_expected[prefix], tx_bytes, sizeof(tx_bytes));
	expected = prefix + sizeof(tx_bytes);
	/* Receive queued data and the echo-mitigation ZLP before the callback. */
	test_advance(K_NO_WAIT);
	received = receive_tx(cfg, received, expected);
	data->echo_mitigated = true;
	data->zlp_needed = false;
	tx_written = 0U;
	tx_false_ready = false;
	tx_loop_limit = false;
	zassert_ok(uart_irq_callback_user_data_set(uart, filling_callback, NULL));
	uart_irq_tx_enable(uart);
	/* A successful echo-mitigation ZLP can leave this delay pending. */
	test_advance(K_MSEC(CONFIG_USBD_CDC_ACM_TX_DELAY_MS));
	written_before_completion = prefix + tx_written - received;
	/* Each packet plus a possible ZLP gets a retry and a send turn.
	 * TX retries take 1 ms; leave one final turn to receive the tail.
	 */
	for (size_t step = 0;
	     step < 2U * (DIV_ROUND_UP(expected, USBD_MAX_BULK_MPS) + 1U) + 1U; step++) {
		received = receive_tx(cfg, received, expected);
		if (received == expected) {
			break;
		}
		test_advance(K_MSEC(1));
	}
	zassert_false(tx_false_ready, "ready stayed positive after FIFO rejected data");
	zassert_false(tx_loop_limit, "TX callback exceeded its iteration limit");
	zassert_true(written_before_completion >= USBD_MAX_BULK_MPS * 2U,
		      "TX rotation did not wake callback");
	zassert_equal(tx_written, sizeof(tx_bytes));
	zassert_equal(received, expected, "TX bytes did not reach the host");
}

ZTEST_SUITE(cdc_acm_work, NULL, setup, before, after, NULL);
