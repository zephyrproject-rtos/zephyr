/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sfp.h>
#include <zephyr/sff/sff8472.h>
#include <zephyr/ztest.h>

static const struct device *const sfp_dev = DEVICE_DT_GET_ANY(sff_sff8472);

ZTEST(sfp, test_device_ready)
{
	zassert_not_null(sfp_dev, "no sff,sff8472 device in devicetree");
	zassert_true(device_is_ready(sfp_dev));
}

ZTEST(sfp, test_read_id)
{
	struct sff8472_id id;

	zassert_ok(sfp_read_id(sfp_dev, &id));
	zassert_equal(id.identifier, SFF8472_ID_SFP);
	zassert_equal(id.connector, 0x07U);
	zassert_equal(id.wavelength_nm, 850U);
	zassert_equal(id.br_nominal_mbd, 10300U);
	zassert_equal(strcmp(id.vendor_name, "ACME PHOTONICS"), 0, "got '%s'", id.vendor_name);
	zassert_equal(strcmp(id.vendor_pn, "AFBR-709SMZ"), 0, "got '%s'", id.vendor_pn);
	zassert_equal(strcmp(id.vendor_sn, "SN12345678"), 0, "got '%s'", id.vendor_sn);
	zassert_true(id.ddm_supported);
	zassert_true(id.ddm_internally_calibrated);
}

ZTEST(sfp, test_read_diagnostics)
{
	struct sff8472_diagnostics diag;

	zassert_ok(sfp_read_diagnostics(sfp_dev, &diag));
	zassert_equal(diag.temp_mdegc, 45500, "got %d", diag.temp_mdegc);
	zassert_equal(diag.vcc_uv, 3300000U, "got %u", diag.vcc_uv);
	zassert_equal(diag.tx_bias_ua, 6500U, "got %u", diag.tx_bias_ua);
	zassert_equal(diag.tx_power_nw, 500000U, "got %u", diag.tx_power_nw);
	zassert_equal(diag.rx_power_nw, 400000U, "got %u", diag.rx_power_nw);
	zassert_equal(diag.alarm_flags,
		      SFF8472_FLAG_TEMP_HIGH | SFF8472_FLAG_TX_BIAS_HIGH |
		      SFF8472_FLAG_RX_POWER_LOW,
		      "got 0x%04x", diag.alarm_flags);
	zassert_equal(diag.warning_flags, SFF8472_FLAG_VCC_LOW, "got 0x%04x", diag.warning_flags);
}

ZTEST_SUITE(sfp, NULL, NULL, NULL, NULL, NULL);
