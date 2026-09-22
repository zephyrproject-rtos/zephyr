/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define OPAMP_SUPPORT_PROGRAMMABLE_GAIN					\
	DT_NODE_HAS_PROP(DT_PHANDLE(DT_PATH(zephyr_user), opamp),	\
			programmable_gain)

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
static const enum opamp_gain gain[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_PHANDLE(DT_PATH(zephyr_user), opamp),
				programmable_gain, DT_ENUM_IDX_BY_IDX, (,))
};
#endif

const struct device *get_opamp_device(void)
{
	return DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), opamp));
}

static const struct device *init_opamp(void)
{
	int ret;

	const struct device *const opamp_dev =
				DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), opamp));

	ret = device_is_ready(opamp_dev);

	zassert_true(ret, "OPAMP device %s is not ready", opamp_dev->name);

	return opamp_dev;
}

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
/* Test OPAMP opamp_set_gain API, Only OPAMPs
 * that support PGA need to test this API.
 */
ZTEST(opamp, test_gain_set)
{
	int ret;

	const struct device *opamp_dev = init_opamp();

	for (uint8_t index = 0; index < ARRAY_SIZE(gain); ++index) {
		ret = opamp_set_gain(opamp_dev, gain[index]);
		zassert_ok(ret, "opamp_set_gain() failed with code %d", ret);
	}
}
#endif

ZTEST(opamp, test_init_opamp)
{
	const struct device *opamp_dev = init_opamp();

	ARG_UNUSED(opamp_dev);
}

static void *opamp_setup(void)
{
	k_object_access_grant(get_opamp_device(), k_current_get());

	return NULL;
}

ZTEST_SUITE(opamp, NULL, opamp_setup, NULL, NULL, NULL);
