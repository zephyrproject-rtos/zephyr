/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define TEST_GPIO DT_NODELABEL(test_gpio_0)
#define TEST_I2C DT_NODELABEL(test_i2c)
#define TEST_DEVA DT_NODELABEL(test_dev_a)
#define TEST_GPIOX DT_NODELABEL(test_gpiox)
#define TEST_DEVB DT_NODELABEL(test_dev_b)
#define TEST_DEVC DT_NODELABEL(test_dev_c)
#define TEST_PARTITION DT_NODELABEL(test_p0)
#define TEST_GPIO_INJECTED DT_NODELABEL(test_gpio_injected)

static const struct device *devlist;
static const struct device *devlist_end;

static device_handle_t init_order[10];

static int dev_init(const struct device *dev)
{
	static uint8_t init_idx;

	__ASSERT_NO_MSG(init_idx < ARRAY_SIZE(init_order));
	init_order[init_idx++] = device_handle_get(dev);

	return 0;
}

DEVICE_DT_DEFINE(TEST_GPIO, dev_init, NULL,
		 NULL, NULL, PRE_KERNEL_1, 90, NULL);
DEVICE_DT_DEFINE(TEST_I2C, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 10, NULL);
DEVICE_DT_DEFINE(TEST_DEVA, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 20, NULL);
/* NB: Intentional init devb before required gpiox */
DEVICE_DT_DEFINE(TEST_DEVB, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 30, NULL);
DEVICE_DT_DEFINE(TEST_GPIOX, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 40, NULL);
DEVICE_DT_DEFINE(TEST_DEVC, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 50, NULL);
DEVICE_DT_DEFINE(TEST_PARTITION, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 60, NULL);
/* Device with both an existing and missing injected dependency */
DEVICE_DT_DEFINE(TEST_GPIO_INJECTED, dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 70, NULL, DT_DEP_ORD(TEST_DEVB), 999);
/* Manually specified device */
DEVICE_DEFINE(manual_dev, "Manual Device", dev_init, NULL,
		 NULL, NULL, POST_KERNEL, 80, NULL);

#define DEV_HDL(node_id) device_handle_get(DEVICE_DT_GET(node_id))
#define DEV_HDL_NAME(name) device_handle_get(DEVICE_GET(name))

ZTEST(devicetree_devices, test_init_get)
{
	/* Check device pointers */
	zassert_equal(DEVICE_INIT_DT_GET(TEST_GPIO)->dev,
		      DEVICE_DT_GET(TEST_GPIO), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_I2C)->dev,
		      DEVICE_DT_GET(TEST_I2C), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_DEVA)->dev,
		      DEVICE_DT_GET(TEST_DEVA), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_DEVB)->dev,
		      DEVICE_DT_GET(TEST_DEVB), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_GPIOX)->dev,
		      DEVICE_DT_GET(TEST_GPIOX), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_DEVC)->dev,
		      DEVICE_DT_GET(TEST_DEVC), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_PARTITION)->dev,
		      DEVICE_DT_GET(TEST_PARTITION), NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_GPIO_INJECTED)->dev,
		      DEVICE_DT_GET(TEST_GPIO_INJECTED), NULL);
	zassert_equal(DEVICE_INIT_GET(manual_dev)->dev,
		      DEVICE_GET(manual_dev), NULL);

	/* Check init functions */
	zassert_equal(DEVICE_INIT_DT_GET(TEST_GPIO)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_I2C)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_DEVA)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_DEVB)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_GPIOX)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_DEVC)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_PARTITION)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_DT_GET(TEST_GPIO_INJECTED)->init, dev_init, NULL);
	zassert_equal(DEVICE_INIT_GET(manual_dev)->init, dev_init, NULL);
}

ZTEST(devicetree_devices, test_init_order)
{
	zassert_equal(init_order[0], DEV_HDL(TEST_GPIO), NULL);
	zassert_equal(init_order[1], DEV_HDL(TEST_I2C), NULL);
	zassert_equal(init_order[2], DEV_HDL(TEST_DEVA), NULL);
	zassert_equal(init_order[3], DEV_HDL(TEST_DEVB), NULL);
	zassert_equal(init_order[4], DEV_HDL(TEST_GPIOX), NULL);
	zassert_equal(init_order[5], DEV_HDL(TEST_DEVC), NULL);
	zassert_equal(init_order[6], DEV_HDL(TEST_PARTITION), NULL);
	zassert_equal(init_order[7], DEV_HDL(TEST_GPIO_INJECTED), NULL);
	zassert_equal(init_order[8], DEV_HDL_NAME(manual_dev), NULL);
}

static bool check_handle(device_handle_t hdl,
			 const device_handle_t *hdls,
			 size_t nhdls)
{
	const device_handle_t *hdle = hdls + nhdls;

	while (hdls < hdle) {
		if (*hdls == hdl) {
			return true;
		}
		++hdls;
	}

	return false;
}

struct visitor_context {
	uint8_t ndevs;
	const struct device *rdevs[2];
};

static int device_visitor(const struct device *dev,
			  void *context)
{
	struct visitor_context *ctx = context;
	const struct device **rdp = ctx->rdevs;

	while (rdp < (ctx->rdevs + ctx->ndevs)) {
		if (*rdp == NULL) {
			*rdp = dev;
			return 0;
		}

		++rdp;
	}

	return -ENOSPC;
}

ZTEST(devicetree_devices, test_requires)
{
	size_t nhdls = 0;
	const device_handle_t *hdls;
	const struct device *dev;
	struct visitor_context ctx = { 0 };

	/* TEST_GPIO: no req */
	dev = device_get_binding(DT_LABEL(TEST_GPIO));
	zassert_equal(dev, DEVICE_DT_GET(TEST_GPIO), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);
	zassert_equal(0, device_required_foreach(dev, device_visitor, &ctx),
		      NULL);

	/* TEST_GPIO_INJECTED: no req */
	dev = device_get_binding(DT_LABEL(TEST_GPIO_INJECTED));
	zassert_equal(dev, DEVICE_DT_GET(TEST_GPIO_INJECTED), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);
	zassert_equal(0, device_required_foreach(dev, device_visitor, &ctx),
		      NULL);

	/* TEST_I2C: no req */
	dev = device_get_binding(DT_LABEL(TEST_I2C));
	zassert_equal(dev, DEVICE_DT_GET(TEST_I2C), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);
	zassert_equal(0, device_required_foreach(dev, device_visitor, &ctx),
		      NULL);

	/* TEST_DEVA: TEST_I2C GPIO */
	dev = device_get_binding(DT_LABEL(TEST_DEVA));
	zassert_equal(dev, DEVICE_DT_GET(TEST_DEVA), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 2, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_I2C), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(TEST_GPIO), hdls, nhdls), NULL);

	/* Visit fails if not enough space */
	ctx = (struct visitor_context){
		.ndevs = 1,
	};
	zassert_equal(-ENOSPC, device_required_foreach(dev, device_visitor, &ctx),
		      NULL);

	/* Visit succeeds if enough space. */
	ctx = (struct visitor_context){
		.ndevs = 2,
	};
	zassert_equal(2, device_required_foreach(dev, device_visitor, &ctx),
		      NULL);
	zassert_true((ctx.rdevs[0] == device_from_handle(DEV_HDL(TEST_I2C)))
		     || (ctx.rdevs[1] == device_from_handle(DEV_HDL(TEST_I2C))),
		     NULL);
	zassert_true((ctx.rdevs[0] == device_from_handle(DEV_HDL(TEST_GPIO)))
		     || (ctx.rdevs[1] == device_from_handle(DEV_HDL(TEST_GPIO))),
		     NULL);

	/* TEST_GPIOX: TEST_I2C */
	dev = device_get_binding(DT_LABEL(TEST_GPIOX));
	zassert_equal(dev, DEVICE_DT_GET(TEST_GPIOX), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 1, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_I2C), hdls, nhdls), NULL);
	ctx = (struct visitor_context){
		.ndevs = 3,
	};
	zassert_equal(1, device_required_foreach(dev, device_visitor, &ctx),
		      NULL);
	zassert_true(ctx.rdevs[0] == device_from_handle(DEV_HDL(TEST_I2C)),
		     NULL);

	/* TEST_DEVB: TEST_I2C TEST_GPIOX */
	dev = device_get_binding(DT_LABEL(TEST_DEVB));
	zassert_equal(dev, DEVICE_DT_GET(TEST_DEVB), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 2, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_I2C), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(TEST_GPIOX), hdls, nhdls), NULL);

	/* TEST_GPIO_INJECTED: NONE */
	dev = device_get_binding(DT_LABEL(TEST_GPIO_INJECTED));
	zassert_equal(dev, DEVICE_DT_GET(TEST_GPIO_INJECTED), NULL);
	hdls = device_required_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);
}

ZTEST(devicetree_devices, test_injected)
{
	size_t nhdls = 0;
	const device_handle_t *hdls;
	const struct device *dev;

	/* TEST_GPIO: NONE */
	dev = device_get_binding(DT_LABEL(TEST_GPIO));
	hdls = device_injected_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);

	/* TEST_DEVB: NONE */
	dev = device_get_binding(DT_LABEL(TEST_DEVB));
	hdls = device_injected_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);

	/* TEST_GPIO_INJECTED: TEST_DEVB */
	dev = device_get_binding(DT_LABEL(TEST_GPIO_INJECTED));
	hdls = device_injected_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 1, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_DEVB), hdls, nhdls), NULL);
}

ZTEST(devicetree_devices, test_get_or_null)
{
	const struct device *dev;

	dev = DEVICE_DT_GET_OR_NULL(TEST_DEVA);
	zassert_not_equal(dev, NULL, NULL);

	dev = DEVICE_DT_GET_OR_NULL(non_existing_node);
	zassert_equal(dev, NULL, NULL);
}

ZTEST(devicetree_devices, test_supports)
{
	size_t nhdls = 0;
	const device_handle_t *hdls;
	const struct device *dev;
	struct visitor_context ctx = { 0 };

	/* TEST_DEVB: None */
	dev = DEVICE_DT_GET(TEST_DEVB);
	hdls = device_supported_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);

	/* TEST_GPIO_INJECTED: None */
	dev = DEVICE_DT_GET(TEST_GPIO_INJECTED);
	hdls = device_supported_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 0, NULL);

	/* TEST_GPIO: TEST_DEVA */
	dev = DEVICE_DT_GET(TEST_GPIO);
	hdls = device_supported_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 1, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_DEVA), hdls, nhdls), NULL);

	/* Visit fails if not enough space */
	ctx = (struct visitor_context){
		.ndevs = 0,
	};
	zassert_equal(-ENOSPC, device_supported_foreach(dev, device_visitor, &ctx), NULL);

	/* Visit succeeds if enough space. */
	ctx = (struct visitor_context){
		.ndevs = 1,
	};
	zassert_equal(1, device_supported_foreach(dev, device_visitor, &ctx), NULL);
	zassert_true(ctx.rdevs[0] == device_from_handle(DEV_HDL(TEST_DEVA)), NULL);

	/* TEST_I2C: TEST_DEVA TEST_GPIOX TEST_DEVB TEST_DEVC */
	dev = DEVICE_DT_GET(TEST_I2C);
	hdls = device_supported_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 4, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_DEVA), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(TEST_GPIOX), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(TEST_DEVB), hdls, nhdls), NULL);
	zassert_true(check_handle(DEV_HDL(TEST_DEVC), hdls, nhdls), NULL);

	/* Support forwarding (intermediate missing devicetree node)
	 * TEST_DEVC: TEST_PARTITION
	 */
	dev = DEVICE_DT_GET(TEST_DEVC);
	hdls = device_supported_handles_get(dev, &nhdls);
	zassert_equal(nhdls, 1, NULL);
	zassert_true(check_handle(DEV_HDL(TEST_PARTITION), hdls, nhdls), NULL);
}

void *devicetree_devices_setup(void)
{
	size_t ndevs;

	ndevs = z_device_get_all_static(&devlist);
	devlist_end = devlist + ndevs;

	return NULL;
}
ZTEST_SUITE(devicetree_devices, NULL, devicetree_devices_setup, NULL, NULL, NULL);
