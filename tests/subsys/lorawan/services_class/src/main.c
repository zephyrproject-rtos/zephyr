/*
 * Copyright (c) 2026 Filippo Bianchi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

#include "lorawan_services.h"

static enum lorawan_class get_class(void)
{
	enum lorawan_class dev_class;

	zassert_equal(lorawan_get_class(&dev_class), 0);

	return dev_class;
}

ZTEST(services_class, test_get_class)
{
	zassert_equal(lorawan_set_class(LORAWAN_CLASS_C), 0);
	zassert_equal(get_class(), LORAWAN_CLASS_C);

	zassert_equal(lorawan_set_class(LORAWAN_CLASS_A), 0);
	zassert_equal(get_class(), LORAWAN_CLASS_A);

	zassert_equal(lorawan_get_class(NULL), -EINVAL);
}

ZTEST(services_class, test_session_from_class_a)
{
	zassert_equal(lorawan_set_class(LORAWAN_CLASS_A), 0);

	zassert_equal(lorawan_services_class_c_start(), 1);
	zassert_equal(get_class(), LORAWAN_CLASS_C);

	zassert_equal(lorawan_services_class_c_stop(), 0);
	zassert_equal(get_class(), LORAWAN_CLASS_A);
	zassert_equal(lorawan_services_class_c_active(), 0);
}

ZTEST(services_class, test_session_from_class_c)
{
	zassert_equal(lorawan_set_class(LORAWAN_CLASS_C), 0);

	zassert_equal(lorawan_services_class_c_start(), 1);
	zassert_equal(get_class(), LORAWAN_CLASS_C);

	/* the application selected class C before the session: keep it */
	zassert_equal(lorawan_services_class_c_stop(), 0);
	zassert_equal(get_class(), LORAWAN_CLASS_C);
	zassert_equal(lorawan_services_class_c_active(), 0);
}

ZTEST(services_class, test_nested_sessions)
{
	zassert_equal(lorawan_set_class(LORAWAN_CLASS_A), 0);

	zassert_equal(lorawan_services_class_c_start(), 1);
	zassert_equal(lorawan_services_class_c_start(), 2);

	zassert_equal(lorawan_services_class_c_stop(), 1);
	zassert_equal(get_class(), LORAWAN_CLASS_C);

	zassert_equal(lorawan_services_class_c_stop(), 0);
	zassert_equal(get_class(), LORAWAN_CLASS_A);
}

ZTEST_SUITE(services_class, NULL, NULL, NULL, NULL, NULL);
