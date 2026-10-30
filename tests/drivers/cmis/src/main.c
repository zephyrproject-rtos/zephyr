/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/cmis/cmis.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cmis.h>
#include <zephyr/ztest.h>

static const struct device *const cmis_dev = DEVICE_DT_GET_ANY(sff_cmis);

ZTEST(cmis_driver, test_device_ready)
{
	zassert_not_null(cmis_dev, "no sff,cmis device in devicetree");
	zassert_true(device_is_ready(cmis_dev));
}

ZTEST(cmis_driver, test_read_id)
{
	struct cmis_id id;

	zassert_ok(cmis_read_id(cmis_dev, &id));
	zassert_equal(id.identifier, CMIS_ID_QSFP_DD);
	zassert_equal(id.cmis_major, 5U);
	zassert_equal(id.cmis_minor, 0U);
	zassert_equal(id.module_state, CMIS_MODULE_STATE_READY);
	zassert_equal(id.media_type, CMIS_MEDIA_OPTICAL_SMF);
	zassert_equal(id.power_class, 4U);
	zassert_equal(id.max_power_mw, 14000U, "got %u", id.max_power_mw);
	zassert_equal(strcmp(id.vendor_name, "ACME PHOTONICS"), 0, "got '%s'", id.vendor_name);
	zassert_equal(strcmp(id.vendor_pn, "QSFP-DD-400G"), 0, "got '%s'", id.vendor_pn);
	zassert_equal(strcmp(id.vendor_sn, "QD2400001234"), 0, "got '%s'", id.vendor_sn);
}

ZTEST(cmis_driver, test_read_monitors)
{
	struct cmis_monitors mon;

	zassert_ok(cmis_read_monitors(cmis_dev, &mon));
	zassert_equal(mon.temp_mdegc, 45500, "got %d", mon.temp_mdegc);
	zassert_equal(mon.vcc_uv, 3300000U, "got %u", mon.vcc_uv);
	zassert_equal(mon.flags, CMIS_FLAG_TEMP_HIGH_ALARM | CMIS_FLAG_VCC_HIGH_ALARM,
		      "got 0x%02x", mon.flags);
}

ZTEST_SUITE(cmis_driver, NULL, NULL, NULL, NULL, NULL);
