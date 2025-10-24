/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh.h>

#include "usbh_desc.h"

#include "test_descriptor.h"

static const uint8_t test_hub_descriptor[] = {TEST_HUB_DESCRIPTOR};

static const uint8_t test_hub_device_descriptor[] = {TEST_HUB_DEVICE_DESCRIPTOR};
static const uint8_t test_hub_interface_alt0_descriptor[] = {TEST_HUB_INTERFACE_ALT0_DESCRIPTOR};
static const uint8_t test_hub_interface_alt1_descriptor[] = {TEST_HUB_INTERFACE_ALT1_DESCRIPTOR};
static const uint8_t test_hub_endpoint_descriptor[] = {TEST_HUB_ENDPOINT_DESCRIPTOR};

ZTEST(host_desc, test_desc_get_by_type)
{
	const uint8_t *const start_addr = test_hub_descriptor;
	const uint8_t *const end_addr = test_hub_descriptor + sizeof(test_hub_descriptor);
	const struct usb_desc_header *desc;
	uint32_t mask;

	zassert_is_null(usbh_desc_get_by_type(start_addr, end_addr, 0x00),
			"requesting nothing needs to always return nothing");

	/* From "test_descriptor.h":
	 * #0 TEST_HUB_DEVICE_DESCRIPTOR
	 * #1 TEST_HUB_CONFIG_DESCRIPTOR
	 * #2 TEST_HUB_INTERFACE_ALT0_DESCRIPTOR
	 * #3 TEST_HUB_ENDPOINT_DESCRIPTOR
	 * #4 TEST_HUB_INTERFACE_ALT1_DESCRIPTOR
	 * #5 TEST_HUB_ENDPOINT_DESCRIPTOR
	 */
	desc = (const struct usb_desc_header *)start_addr;

	/* desc at #0 TEST_HUB_DEVICE_DESCRIPTOR */

	mask = BIT(USB_DESC_DEVICE);

	desc = usbh_desc_get_by_type(desc, end_addr, mask);
	zassert_not_null(desc);
	zassert_mem_equal(desc, test_hub_device_descriptor,
			  sizeof(test_hub_device_descriptor),
			  "needs to be able to reach the device descriptor");

	/* desc at #0 TEST_HUB_DEVICE_DESCRIPTOR */

	mask = BIT(USB_DESC_INTERFACE) | BIT(USB_DESC_ENDPOINT);

	desc = usbh_desc_get_by_type(desc, end_addr, mask);
	zassert_not_null(desc);
	zassert_mem_equal(desc, test_hub_interface_alt0_descriptor,
			  sizeof(test_hub_interface_alt0_descriptor),
			  "needs to return the first match (interface 0 alt 1) only");

	/* desc at #2 TEST_HUB_INTERFACE_ALT0_DESCRIPTOR */

	desc = usbh_desc_get_next(desc, end_addr);
	zassert_not_null(desc);
	zassert_mem_equal(desc, test_hub_endpoint_descriptor,
			  sizeof(test_hub_endpoint_descriptor),
			  "needs to properly shift to the next descriptor");

	/* desc at #3 TEST_HUB_ENDPOINT_DESCRIPTOR */

	mask = BIT(USB_DESC_INTERFACE);

	desc = usbh_desc_get_by_type((const uint8_t *)desc, end_addr, mask);
	zassert_not_null(desc);
	zassert_mem_equal(desc, test_hub_interface_alt1_descriptor,
			  sizeof(test_hub_interface_alt1_descriptor),
			  "needs to return the interface 0 alt 1 descriptor");

	/* desc at #4 TEST_HUB_INTERFACE_ALT1_DESCRIPTOR */

	desc = usbh_desc_get_next(desc, end_addr);
	zassert_not_null(desc);
	zassert_mem_equal(desc, test_hub_endpoint_descriptor,
			  sizeof(test_hub_endpoint_descriptor),
			  "needs to return the last descriptor (endpoint)");

	/* desc at #5 TEST_HUB_ENDPOINT_DESCRIPTOR */

	desc = usbh_desc_get_next(desc, end_addr);
	zassert_is_null(desc,
			"needs to return NULL when running past the end");
}

ZTEST_SUITE(host_desc, NULL, NULL, NULL, NULL, NULL);
