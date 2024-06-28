/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_test, LOG_LEVEL_INF);

/*
 * Simple test for API rules, allocation, queue, and dequeu
 * of the endpoint requests. USB device controller should not be
 * connected to the host as this state is not covered by this test.
 */

#define FALSE_EP_ADDR		0x0FU

K_MSGQ_DEFINE(test_msgq, sizeof(struct udc_event), 8, sizeof(uint32_t));
static K_KERNEL_STACK_DEFINE(test_udc_stack, 512);
static struct k_thread test_udc_thread_data;
static K_SEM_DEFINE(ep_queue_sem, 0, 1);
static uint8_t last_used_ep;
static uint8_t test_event_ctx;

static int test_udc_event_handler(const struct device *dev,
				  const struct udc_event *const event)
{
	return k_msgq_put(&test_msgq, event, K_NO_WAIT);
}

static void event_ep_request(const struct device *dev, struct udc_event event)
{
	struct udc_buf_info *bi;
	int err;

	bi = udc_get_buf_info(event.buf);

	err = udc_ep_buf_free(dev, event.buf);
	zassert_ok(err, "Failed to free request buffer");

	if (bi->err == -ECONNABORTED && bi->ep == last_used_ep) {
		k_sem_give(&ep_queue_sem);
	}
}

static void test_udc_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct udc_event event;

	while (true) {
		k_msgq_get(&test_msgq, &event, K_FOREVER);

		zassert_equal(udc_get_event_ctx(event.dev), &test_event_ctx,
			      "Wrong pointer to higher layer context");

		switch (event.type) {
		case UDC_EVT_VBUS_REMOVED:
			LOG_DBG("VBUS remove event");
			break;
		case UDC_EVT_VBUS_READY:
			LOG_DBG("VBUS detected event");
			break;
		case UDC_EVT_SUSPEND:
			LOG_DBG("Suspend event");
			break;
		case UDC_EVT_RESUME:
			LOG_DBG("Resume event");
			break;
		case UDC_EVT_RESET:
			LOG_DBG("Reset event");
			break;
		case UDC_EVT_SOF:
			LOG_DBG("SoF event");
			break;
		case UDC_EVT_EP_REQUEST:
			event_ep_request(dev, event);
			break;
		case UDC_EVT_ERROR:
			LOG_DBG("Error event");
			break;
		default:
			break;
		};
	}
}

static void test_udc_ep_try_config(const struct device *dev,
				   struct usb_ep_descriptor *ed)
{
	uint16_t mps = ed->wMaxPacketSize;
	int err;

	err = udc_ep_try_config(dev, ed->bEndpointAddress,
				ed->bmAttributes, &mps,
				ed->bInterval);
	zassert_equal(err, 0, "Failed to test endpoint configuration");

	if (ed->bmAttributes == USB_EP_TYPE_CONTROL ||
	    ed->bmAttributes == USB_EP_TYPE_ISO) {
		/*
		 * Skip subsequent test since udc_ep_try_config() does not
		 * update mps argument for control and iso endpoints.
		 */
		return;
	}

	mps = 0;
	err = udc_ep_try_config(dev, ed->bEndpointAddress,
				ed->bmAttributes, &mps,
				ed->bInterval);
	zassert_equal(err, 0, "Failed to test endpoint configuration");
	zassert_not_equal(mps, 0, "Failed to test endpoint configuration");
}

static void test_udc_ep_enable(const struct device *dev,
			       struct usb_ep_descriptor *ed)
{
	uint8_t ctrl_ep = USB_EP_DIR_IS_IN(ed->bEndpointAddress) ?
			  USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
	/* Possible return values 0, -EINVAL, -ENODEV, -EALREADY, -EPERM. */
	int err1, err2, err3, err4;

	err1 = udc_ep_enable(dev, ed->bEndpointAddress, ed->bmAttributes,
			     ed->wMaxPacketSize, ed->bInterval);
	err2 = udc_ep_enable(dev, ed->bEndpointAddress, ed->bmAttributes,
			     ed->wMaxPacketSize, ed->bInterval);
	err3 = udc_ep_enable(dev, FALSE_EP_ADDR, ed->bmAttributes,
			     ed->wMaxPacketSize, ed->bInterval);
	err4 = udc_ep_enable(dev, ctrl_ep, ed->bmAttributes,
			     ed->wMaxPacketSize, ed->bInterval);

	if (!udc_is_initialized(dev) && !udc_is_enabled(dev)) {
		zassert_equal(err1, -EPERM, "Not failed to enable endpoint");
		zassert_equal(err2, -EPERM, "Not failed to enable endpoint");
		zassert_equal(err3, -EPERM, "Not failed to enable endpoint");
		zassert_equal(err4, -EINVAL, "Not failed to enable endpoint");
	} else if (udc_is_initialized(dev) && !udc_is_enabled(dev)) {
		zassert_equal(err1, -EPERM, "Not failed to enable endpoint");
		zassert_equal(err2, -EPERM, "Not failed to enable endpoint");
		zassert_equal(err3, -EPERM, "Not failed to enable endpoint");
		zassert_equal(err4, -EINVAL, "Not failed to enable endpoint");
	} else {
		zassert_equal(err1, 0, "Failed to enable endpoint");
		zassert_equal(err2, -EALREADY, "Not failed to enable endpoint");
		zassert_equal(err3, -ENODEV, "Not failed to enable endpoint");
		zassert_equal(err4, -EINVAL, "Not failed to enable endpoint");
	}
}

static void test_udc_ep_disable(const struct device *dev,
				struct usb_ep_descriptor *ed)
{
	uint8_t ctrl_ep = USB_EP_DIR_IS_IN(ed->bEndpointAddress) ?
			  USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
	/* Possible return values 0, -EINVAL, -ENODEV, -EALREADY, -EPERM. */
	int err1, err2, err3, err4;

	err1 = udc_ep_disable(dev, ed->bEndpointAddress);
	err2 = udc_ep_disable(dev, ed->bEndpointAddress);
	err3 = udc_ep_disable(dev, FALSE_EP_ADDR);
	err4 = udc_ep_disable(dev, ctrl_ep);

	if (!udc_is_initialized(dev) && !udc_is_enabled(dev)) {
		zassert_equal(err1, -EPERM, "Not failed to disable endpoint");
		zassert_equal(err2, -EPERM, "Not failed to disable endpoint");
		zassert_equal(err3, -EPERM, "Not failed to disable endpoint");
		zassert_equal(err4, -EINVAL, "Not failed to disable endpoint");
	} else if (udc_is_initialized(dev) && !udc_is_enabled(dev)) {
		zassert_equal(err1, -EALREADY, "Failed to disable endpoint");
		zassert_equal(err2, -EALREADY, "Not failed to disable endpoint");
		zassert_equal(err3, -ENODEV, "Not failed to disable endpoint");
		zassert_equal(err4, -EINVAL, "Not failed to disable endpoint");
	} else {
		zassert_equal(err1, 0, "Failed to disable endpoint");
		zassert_equal(err2, -EALREADY, "Not failed to disable endpoint");
		zassert_equal(err3, -ENODEV, "Not failed to disable endpoint");
		zassert_equal(err4, -EINVAL, "Not failed to disable endpoint");
	}
}

static struct net_buf *test_udc_ep_buf_alloc(const struct device *dev,
					     struct usb_ep_descriptor *ed)
{
	struct net_buf *buf;

	buf = udc_ep_buf_alloc(dev, ed->bEndpointAddress,
			       ed->wMaxPacketSize);

	zassert_not_null(buf, "Failed to allocate request");

	return buf;
}

static void test_udc_ep_buf_free(const struct device *dev,
				 struct net_buf *buf)
{
	int err;

	if (buf == NULL) {
		return;
	}

	err = udc_ep_buf_free(dev, buf);
	zassert_ok(err, "Failed to free request");
}

static void test_udc_ep_halt(const struct device *dev,
			     struct usb_ep_descriptor *ed)
{
	/* Possible return values 0, -ENODEV, -ENOTSUP, -EPERM. */
	int err1, err2;

	err1 = udc_ep_set_halt(dev, ed->bEndpointAddress);
	err2 = udc_ep_set_halt(dev, FALSE_EP_ADDR);

	if (udc_is_enabled(dev)) {
		if (ed->bmAttributes == USB_EP_TYPE_ISO) {
			zassert_equal(err1, -ENOTSUP, "Not failed to set halt");
		} else {
			zassert_equal(err1, 0, "Failed to set halt");
		}

		zassert_equal(err2, -ENODEV, "Not failed to set halt");
	} else {
		zassert_equal(err1, -EPERM, "Not failed to set halt");
		zassert_equal(err2, -EPERM, "Not failed to set halt");
	}

	err1 = udc_ep_clear_halt(dev, ed->bEndpointAddress);
	err2 = udc_ep_clear_halt(dev, FALSE_EP_ADDR);

	if (udc_is_enabled(dev)) {
		if (ed->bmAttributes == USB_EP_TYPE_ISO) {
			zassert_equal(err1, -ENOTSUP, "Not failed to clear halt");
		} else {
			zassert_equal(err1, 0, "Failed to clear halt ");
		}

		zassert_equal(err2, -ENODEV, "Not failed to clear halt");
	} else {
		zassert_equal(err1, -EPERM, "Not failed to clear halt");
		zassert_equal(err2, -EPERM, "Not failed to clear halt");
	}
}

static void test_udc_ep_enqueue(const struct device *dev,
				struct net_buf *buf)
{
	/* Possible return values 0, -EPERM, -ENODEV, -EACCES(TBD), -EBUSY (TBD) */
	int err1, err2 = 0;
	struct net_buf *false_buf = NULL;

	err1 = udc_ep_enqueue(dev, buf);
	if (udc_is_enabled(dev)) {
		false_buf = udc_ep_buf_alloc(dev, FALSE_EP_ADDR, 64);
		zassert_not_null(false_buf, "Failed to allocate request");
		err2 = udc_ep_enqueue(dev, false_buf);
	}

	if (udc_is_enabled(dev)) {
		zassert_equal(err1, 0, "Failed to queue request");
		zassert_equal(err2, -ENODEV, "Not failed to queue request");
	} else {
		zassert_equal(err1, -EPERM, "Not failed to queue request");
	}

	test_udc_ep_buf_free(dev, false_buf);
}

static void test_udc_ep_dequeue(const struct device *dev,
				struct usb_ep_descriptor *ed)
{
	/* Possible return values 0, -EPERM, -ENODEV, -EACCES(TBD) */
	int err1, err2;

	err1 = udc_ep_dequeue(dev, ed->bEndpointAddress);
	err2 = udc_ep_dequeue(dev, FALSE_EP_ADDR);

	if (!udc_is_initialized(dev)) {
		zassert_equal(err1, -EPERM, "Not failed to dequeue");
		zassert_equal(err2, -EPERM, "Not failed to dequeue");
	} else {
		zassert_equal(err1, 0, "Failed to dequeue");
		zassert_equal(err2, -ENODEV, "Not failed to dequeue");
	}
}

static void test_udc_wakeup(const struct device *dev)
{
	int err;

	err = udc_host_wakeup(dev);

	if (!udc_is_enabled(dev)) {
		zassert_equal(err, -EPERM, "Not failed to request host wakeup");
	}
}

static void test_udc_set_address(const struct device *dev, uint8_t addr)
{
	int err;

	err = udc_set_address(dev, addr);

	if (!udc_is_enabled(dev)) {
		zassert_equal(err, -EPERM, "Not failed to set address");
	}
}

static void test_udc_ep_api(const struct device *dev,
			    struct usb_ep_descriptor *ed)
{
	const int num_of_iterations = 10;
	struct net_buf *buf;
	int err;

	last_used_ep = ed->bEndpointAddress;

	for (int i = 0; i < num_of_iterations; i++) {
		err = udc_ep_enable(dev, ed->bEndpointAddress, ed->bmAttributes,
				     ed->wMaxPacketSize, ed->bInterval);
		zassert_ok(err, "Failed to enable endpoint");

		/* It needs a little reserve for memory management overhead. */
		for (int n = 0; n < (CONFIG_UDC_BUF_COUNT - 4); n++) {
			buf = udc_ep_buf_alloc(dev, ed->bEndpointAddress,
					       ed->wMaxPacketSize);
			zassert_not_null(buf,
					 "Failed to allocate request (%d) for 0x%02x",
					 n, ed->bEndpointAddress);

			udc_ep_buf_set_zlp(buf);
			err = udc_ep_enqueue(dev, buf);
			zassert_ok(err, "Failed to queue request");
			k_yield();
		}

		err = udc_ep_disable(dev, ed->bEndpointAddress);
		zassert_ok(err, "Failed to disable endpoint");

		err = udc_ep_dequeue(dev, ed->bEndpointAddress);
		zassert_ok(err, "Failed to dequeue endpoint");

		err = k_sem_take(&ep_queue_sem, K_MSEC(100));
		zassert_ok(err, "Timeout to dequeue endpoint %x %d", last_used_ep, err);
	}
}

static void test_udc_ep_mps(uint8_t type)
{
	uint16_t mps[] = {8, 16, 32, 64, 512, 1024};
	struct usb_ep_descriptor ed = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = type,
		.wMaxPacketSize = 0,
		.bInterval = 0,
	};
	const struct device *dev;
	uint16_t supported = 0;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));
	zassert_true(device_is_ready(dev), "UDC device not ready");

	err = udc_init(dev, test_udc_event_handler, &test_event_ctx);
	zassert_ok(err, "Failed to initialize UDC driver");

	err = udc_enable(dev);
	zassert_ok(err, "Failed to enable UDC driver");

	if (type == USB_EP_TYPE_INTERRUPT) {
		ed.bInterval = 1;
	}

	for (uint8_t i = 1; i < 16U; i++) {
		err = udc_ep_try_config(dev, i,
					ed.bmAttributes, &supported,
					ed.bInterval);
		if (!err) {
			ed.bEndpointAddress = i;
			break;
		}
	}

	zassert_ok(err, "Failed to determine MPS");

	for (int i = 0; i < ARRAY_SIZE(mps); i++) {
		if (mps[i] > supported) {
			continue;
		}

		ed.wMaxPacketSize = mps[i];
		test_udc_ep_api(dev, &ed);

		ed.bEndpointAddress |= USB_EP_DIR_IN;
		test_udc_ep_api(dev, &ed);
	}

	err = udc_disable(dev);
	zassert_ok(err, "Failed to disable UDC driver");

	err = udc_shutdown(dev);
	zassert_ok(err, "Failed to shoot-down UDC driver");
}

static void *test_udc_device_get(void)
{
	struct udc_device_caps caps;
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));
	zassert_true(device_is_ready(dev), "UDC device not ready");

	caps = udc_caps(dev);
	LOG_INF("UDC device HS: %u", caps.hs);

	k_thread_create(&test_udc_thread_data, test_udc_stack,
			K_KERNEL_STACK_SIZEOF(test_udc_stack),
			test_udc_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&test_udc_thread_data, "test-udc");

	return (void *)dev;
}

static struct usb_ep_descriptor ed_ctrl_out = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = USB_CONTROL_EP_OUT,
	.bmAttributes = USB_EP_TYPE_CONTROL,
	.wMaxPacketSize = 64,
	.bInterval = 0,
};

static struct usb_ep_descriptor ed_ctrl_in = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = USB_CONTROL_EP_IN,
	.bmAttributes = USB_EP_TYPE_CONTROL,
	.wMaxPacketSize = 64,
	.bInterval = 0,
};

static struct usb_ep_descriptor ed_bulk_out = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_EP_TYPE_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 0,
};

static struct usb_ep_descriptor ed_bulk_in = {
	.bLength = sizeof(struct usb_ep_descriptor),
	.bDescriptorType = USB_DESC_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_EP_TYPE_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 0,
};

ZTEST(udc_driver_test, test_udc_not_initialized)
{
	const struct device *dev;
	struct net_buf *buf;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));
	zassert_true(device_is_ready(dev), "UDC device not ready");

	err = udc_init(dev, NULL, NULL);
	zassert_equal(err, -EINVAL, "Not failed to initialize UDC");

	err = udc_shutdown(dev);
	zassert_equal(err, -EALREADY, "Not failed to shutdown UDC");

	err = udc_enable(dev);
	zassert_equal(err, -EPERM, "Not failed to enable UDC driver");

	test_udc_set_address(dev, 0);
	test_udc_set_address(dev, 1);
	test_udc_wakeup(dev);

	test_udc_ep_try_config(dev, &ed_ctrl_out);
	test_udc_ep_try_config(dev, &ed_ctrl_in);
	test_udc_ep_try_config(dev, &ed_bulk_out);
	test_udc_ep_try_config(dev, &ed_bulk_in);

	buf = test_udc_ep_buf_alloc(dev, &ed_bulk_out);
	test_udc_ep_enable(dev, &ed_bulk_out);
	test_udc_ep_enqueue(dev, buf);
	test_udc_ep_halt(dev, &ed_bulk_out);
	test_udc_ep_disable(dev, &ed_bulk_out);
	test_udc_ep_dequeue(dev, &ed_bulk_out);
	test_udc_ep_buf_free(dev, buf);

	err = udc_disable(dev);
	zassert_equal(err, -EALREADY, "Not failed to disable UDC driver");
}

ZTEST(udc_driver_test, test_udc_initialized)
{
	const struct device *dev;
	struct net_buf *buf;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));
	zassert_true(device_is_ready(dev), "UDC device not ready");

	err = udc_init(dev, test_udc_event_handler, &test_event_ctx);
	zassert_ok(err, "Failed to initialize UDC driver");

	test_udc_set_address(dev, 0);
	test_udc_set_address(dev, 1);
	test_udc_wakeup(dev);

	test_udc_ep_try_config(dev, &ed_ctrl_out);
	test_udc_ep_try_config(dev, &ed_ctrl_in);
	test_udc_ep_try_config(dev, &ed_bulk_out);
	test_udc_ep_try_config(dev, &ed_bulk_in);

	buf = test_udc_ep_buf_alloc(dev, &ed_bulk_out);
	test_udc_ep_enable(dev, &ed_bulk_out);
	test_udc_ep_enqueue(dev, buf);
	test_udc_ep_halt(dev, &ed_bulk_out);
	test_udc_ep_disable(dev, &ed_bulk_out);
	test_udc_ep_dequeue(dev, &ed_bulk_out);
	test_udc_ep_buf_free(dev, buf);

	err = udc_shutdown(dev);
	zassert_ok(err, "Failed to shootdown UDC driver");
}

ZTEST(udc_driver_test, test_udc_enabled)
{
	const struct device *dev;
	struct net_buf *buf;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0));
	zassert_true(device_is_ready(dev), "UDC device not ready");

	err = udc_init(dev, test_udc_event_handler, &test_event_ctx);
	zassert_ok(err, "Failed to initialize UDC driver");

	err = udc_enable(dev);
	zassert_ok(err, "Failed to enable UDC driver");

	err = udc_enable(dev);
	zassert_equal(err, -EALREADY, "Not failed to enable UDC driver");

	err = udc_disable(dev);
	zassert_ok(err, "Failed to disable UDC driver");

	err = udc_enable(dev);
	zassert_ok(err, "Failed to enable UDC driver");

	test_udc_set_address(dev, 0);
	test_udc_set_address(dev, 1);

	test_udc_ep_try_config(dev, &ed_ctrl_out);
	test_udc_ep_try_config(dev, &ed_ctrl_in);
	test_udc_ep_try_config(dev, &ed_bulk_out);
	test_udc_ep_try_config(dev, &ed_bulk_in);

	buf = test_udc_ep_buf_alloc(dev, &ed_bulk_out);
	test_udc_ep_enable(dev, &ed_bulk_out);
	test_udc_ep_enqueue(dev, buf);
	test_udc_ep_halt(dev, &ed_bulk_out);
	test_udc_ep_disable(dev, &ed_bulk_out);
	test_udc_ep_dequeue(dev, &ed_bulk_out);
	test_udc_ep_buf_free(dev, buf);

	err = udc_shutdown(dev);
	zassert_equal(err, -EBUSY, "Not failed to shoot-down UDC driver");

	err = udc_disable(dev);
	zassert_ok(err, "Failed to disable UDC driver");

	err = udc_shutdown(dev);
	zassert_ok(err, "Failed to shoot-down UDC driver");
}

ZTEST(udc_driver_test, test_udc_ep_buf)
{
	test_udc_ep_mps(USB_EP_TYPE_BULK);
}

ZTEST(udc_driver_test, test_udc_ep_int)
{
	test_udc_ep_mps(USB_EP_TYPE_INTERRUPT);
}

ZTEST(udc_driver_test, test_udc_ep_iso)
{
	test_udc_ep_mps(USB_EP_TYPE_ISO);
}

ZTEST_SUITE(udc_driver_test, NULL, test_udc_device_get, NULL, NULL, NULL);
