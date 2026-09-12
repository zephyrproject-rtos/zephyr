/*
 * Copyright (c) Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/ti_k3_intr_router.h>

#define TEST_ROUTER_NODE DT_NODELABEL(test_intr_router)

static const struct device *const router_dev = DEVICE_DT_GET(TEST_ROUTER_NODE);

ZTEST(ti_k3_intr_router, test_device_ready)
{
	zassert_true(device_is_ready(router_dev), "Interrupt router device not ready");
}

ZTEST(ti_k3_intr_router, test_prerouted_routes)
{
	uint16_t in_idx = 0xFFFF;
	bool enabled = false;
	int ret;

	/* Verify Timer preroute (Output 20 -> Input 162) */
	ret = ti_k3_intr_router_get_route(router_dev, 20, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route for output 20: %d", ret);
	zassert_equal(in_idx, 162, "Expected Timer input 162 on output 20, got %u", in_idx);
	zassert_true(enabled, "Expected output 20 to be enabled");

	/* Verify RPMsg/IPC preroute (Output 21 -> Input 190) */
	ret = ti_k3_intr_router_get_route(router_dev, 21, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route for output 21: %d", ret);
	zassert_equal(in_idx, 190, "Expected RPMsg input 190 on output 21, got %u", in_idx);
	zassert_true(enabled, "Expected output 21 to be enabled");
}

ZTEST(ti_k3_intr_router, test_set_and_get_route)
{
	uint16_t in_idx = 0xFFFF;
	bool enabled = false;
	int ret;

	ret = ti_k3_intr_router_set_route(router_dev, 5, 42);
	zassert_equal(ret, 0, "Failed to set route on output 5: %d", ret);

	ret = ti_k3_intr_router_get_route(router_dev, 5, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route on output 5: %d", ret);
	zassert_equal(in_idx, 42, "Expected input 42, got %u", in_idx);
	zassert_true(enabled, "Expected output 5 to be enabled");
}

ZTEST(ti_k3_intr_router, test_enable_disable_route)
{
	uint16_t in_idx = 0xFFFF;
	bool enabled = true;
	int ret;

	/* Configure output 6 */
	ret = ti_k3_intr_router_set_route(router_dev, 6, 88);
	zassert_equal(ret, 0, "Failed to set route: %d", ret);

	/* Disable output 6 */
	ret = ti_k3_intr_router_disable_route(router_dev, 6);
	zassert_equal(ret, 0, "Failed to disable route: %d", ret);

	ret = ti_k3_intr_router_get_route(router_dev, 6, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route: %d", ret);
	zassert_false(enabled, "Expected route to be disabled");
	zassert_equal(in_idx, 88, "Expected input index to be preserved, got %u", in_idx);

	/* Re-enable output 6 */
	ret = ti_k3_intr_router_enable_route(router_dev, 6);
	zassert_equal(ret, 0, "Failed to enable route: %d", ret);

	ret = ti_k3_intr_router_get_route(router_dev, 6, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route: %d", ret);
	zassert_true(enabled, "Expected route to be enabled");
}

ZTEST(ti_k3_intr_router, test_dynamic_alloc_and_free)
{
	uint16_t out1 = 0xFFFF;
	uint16_t out2 = 0xFFFF;
	uint16_t in_idx = 0;
	bool enabled = false;
	int ret;

	ret = ti_k3_intr_router_alloc_and_route(router_dev, 10, &out1);
	zassert_equal(ret, 0, "Dynamic allocation 1 failed: %d", ret);
	zassert_not_equal(out1, 20, "Allocated output must not collide with preroute 20");
	zassert_not_equal(out1, 21, "Allocated output must not collide with preroute 21");

	ret = ti_k3_intr_router_alloc_and_route(router_dev, 11, &out2);
	zassert_equal(ret, 0, "Dynamic allocation 2 failed: %d", ret);
	zassert_not_equal(out1, out2, "Allocated outputs must be distinct");

	/* Verify out1 route */
	ret = ti_k3_intr_router_get_route(router_dev, out1, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route: %d", ret);
	zassert_equal(in_idx, 10, "Expected input 10, got %u", in_idx);
	zassert_true(enabled, "Expected route to be enabled");

	/* Free out1 */
	ret = ti_k3_intr_router_free_route(router_dev, out1);
	zassert_equal(ret, 0, "Failed to free route: %d", ret);

	ret = ti_k3_intr_router_get_route(router_dev, out1, &in_idx, &enabled);
	zassert_equal(ret, 0, "Failed to get route: %d", ret);
	zassert_false(enabled, "Expected freed route to be disabled");

	/* Clean up out2 */
	ti_k3_intr_router_free_route(router_dev, out2);
}

ZTEST(ti_k3_intr_router, test_invalid_parameters)
{
	uint16_t out = 0;
	uint16_t in_idx = 0;
	bool enabled = false;

	/* Input index >= num_inputs (198) */
	zassert_equal(ti_k3_intr_router_set_route(router_dev, 0, 198), -EINVAL,
		      "Expected -EINVAL for out-of-range input");
	zassert_equal(ti_k3_intr_router_alloc_and_route(router_dev, 198, &out), -EINVAL,
		      "Expected -EINVAL for out-of-range input in alloc");

	/* Output index >= num_outputs (36) */
	zassert_equal(ti_k3_intr_router_set_route(router_dev, 36, 0), -EINVAL,
		      "Expected -EINVAL for out-of-range output");
	zassert_equal(ti_k3_intr_router_enable_route(router_dev, 36), -EINVAL,
		      "Expected -EINVAL for out-of-range output");
	zassert_equal(ti_k3_intr_router_disable_route(router_dev, 36), -EINVAL,
		      "Expected -EINVAL for out-of-range output");
	zassert_equal(ti_k3_intr_router_get_route(router_dev, 36, &in_idx, &enabled), -EINVAL,
		      "Expected -EINVAL for out-of-range output");
	zassert_equal(ti_k3_intr_router_free_route(router_dev, 36), -EINVAL,
		      "Expected -EINVAL for out-of-range output");

	/* NULL pointers */
	zassert_equal(ti_k3_intr_router_alloc_and_route(router_dev, 0, NULL), -EINVAL,
		      "Expected -EINVAL for NULL out pointer");
	zassert_equal(ti_k3_intr_router_get_route(router_dev, 0, NULL, &enabled), -EINVAL,
		      "Expected -EINVAL for NULL in_idx pointer");
	zassert_equal(ti_k3_intr_router_get_route(router_dev, 0, &in_idx, NULL), -EINVAL,
		      "Expected -EINVAL for NULL enabled pointer");
}

ZTEST(ti_k3_intr_router, test_resource_exhaustion)
{
	uint16_t allocated[36];
	size_t count = 0;
	uint16_t out;
	int ret;

	/* Allocate all remaining routes */
	while ((ret = ti_k3_intr_router_alloc_and_route(router_dev, 1, &out)) == 0) {
		allocated[count++] = out;
	}

	zassert_equal(ret, -ENOSPC, "Expected -ENOSPC when outputs exhausted, got %d", ret);

	/* Free one allocated route */
	zassert_true(count > 0, "Expected at least one dynamic allocation");
	ret = ti_k3_intr_router_free_route(router_dev, allocated[0]);
	zassert_equal(ret, 0, "Failed to free route: %d", ret);

	/* Now allocation should succeed again */
	ret = ti_k3_intr_router_alloc_and_route(router_dev, 2, &out);
	zassert_equal(ret, 0, "Expected allocation to succeed after free, got %d", ret);
	zassert_equal(out, allocated[0], "Expected re-allocation of freed output line");

	/* Clean up */
	ti_k3_intr_router_free_route(router_dev, out);
	for (size_t i = 1; i < count; i++) {
		ti_k3_intr_router_free_route(router_dev, allocated[i]);
	}
}

ZTEST_SUITE(ti_k3_intr_router, NULL, NULL, NULL, NULL, NULL);
