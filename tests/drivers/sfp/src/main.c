/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sfp.h>
#include <zephyr/sff/sff8472.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "emul_sff8472.h"

#define SFP_NODE DT_NODELABEL(sfp0)

static const struct device *const sfp_dev = DEVICE_DT_GET(SFP_NODE);
static const struct emul *const module_emul = EMUL_DT_GET(DT_NODELABEL(sfp_module));
static const struct device *const presence_port = DEVICE_DT_GET(DT_NODELABEL(sfp_gpio));
static const gpio_pin_t presence_pin = DT_GPIO_PIN(SFP_NODE, mod_def0_gpios);

/* MOD_DEF0 is grounded by an inserted module (active low). */
static void set_module_present(bool present)
{
	zassert_ok(gpio_emul_input_set(presence_port, presence_pin, present ? 0 : 1));
}

static void set_diag_mon_type(uint8_t value)
{
	zassert_ok(emul_sff8472_set_byte(module_emul, false, SFF8472_A0_DIAG_MON_TYPE, value));
}

static void *sfp_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	if (IS_ENABLED(CONFIG_USERSPACE)) {
		k_object_access_all_grant(sfp_dev);
	}

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "dummy shell backend not ready");

	return NULL;
}

static void sfp_after(void *fixture)
{
	ARG_UNUSED(fixture);

	set_module_present(true);
	emul_sff8472_reset(module_emul);
}

ZTEST(sfp, test_device_ready)
{
	zassert_true(device_is_ready(sfp_dev));
}

ZTEST_USER(sfp, test_read_id)
{
	struct sff8472_id id;

	zassert_ok(sfp_read_id(sfp_dev, &id));
	zassert_equal(id.identifier, SFF8472_ID_SFP);
	zassert_equal(id.connector, 0x07U);
	zassert_equal(id.wavelength_nm, 850U);
	zassert_equal(id.br_nominal_mbd, 10300U);
	zassert_equal(strcmp(id.vendor_name, "ACME PHOTONICS"), 0, "got '%s'", id.vendor_name);
	zassert_equal(strcmp(id.vendor_pn, "ACME-10G-SR"), 0, "got '%s'", id.vendor_pn);
	zassert_equal(strcmp(id.vendor_sn, "SN12345678"), 0, "got '%s'", id.vendor_sn);
	zassert_true(id.ddm_supported);
	zassert_true(id.ddm_internally_calibrated);
}

ZTEST_USER(sfp, test_read_diagnostics)
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

ZTEST(sfp, test_empty_cage)
{
	struct sff8472_id id;
	struct sff8472_diagnostics diag;

	set_module_present(false);
	zassert_equal(sfp_read_id(sfp_dev, &id), -ENODEV);
	zassert_equal(sfp_read_diagnostics(sfp_dev, &diag), -ENODEV);

	set_module_present(true);
	zassert_ok(sfp_read_id(sfp_dev, &id));
}

ZTEST(sfp, test_unsupported_diagnostics)
{
	struct sff8472_diagnostics diag;
	struct sff8472_id id;

	/* externally calibrated: raw values would look valid but are not */
	set_diag_mon_type(SFF8472_DIAG_MON_DDM_IMPLEMENTED | SFF8472_DIAG_MON_EXT_CAL);
	zassert_equal(sfp_read_diagnostics(sfp_dev, &diag), -ENOTSUP);
	zassert_ok(sfp_read_id(sfp_dev, &id));
	zassert_true(id.ddm_externally_calibrated);

	/* A2h only reachable after an address change sequence */
	set_diag_mon_type(SFF8472_DIAG_MON_DDM_IMPLEMENTED | SFF8472_DIAG_MON_INT_CAL |
			  SFF8472_DIAG_MON_ADDR_CHANGE);
	zassert_equal(sfp_read_diagnostics(sfp_dev, &diag), -ENOTSUP);

	/* no diagnostics at all */
	set_diag_mon_type(0U);
	zassert_equal(sfp_read_diagnostics(sfp_dev, &diag), -ENOTSUP);
}

ZTEST(sfp, test_shell_keeps_sign_below_zero)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *out;
	size_t size;

	/* -0.5 C is 0xFF80 in 1/256 C */
	zassert_ok(emul_sff8472_set_byte(module_emul, true, SFF8472_A2_TEMP, 0xFFU));
	zassert_ok(emul_sff8472_set_byte(module_emul, true, SFF8472_A2_TEMP + 1U, 0x80U));

	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(sh, "sfp ddm " DEVICE_DT_NAME(SFP_NODE)));
	out = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(out, "Temperature: -0.500 C"), "output:\n%s", out);
	zassert_not_null(strstr(out, "TX power   : 0.500 mW"), "output:\n%s", out);
	zassert_not_null(strstr(out, "TX bias    : 6.500 mA"), "output:\n%s", out);
}

ZTEST_SUITE(sfp, NULL, sfp_setup, NULL, sfp_after, NULL);
