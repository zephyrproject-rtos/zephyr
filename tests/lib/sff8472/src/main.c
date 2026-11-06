/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/sff/sff8472.h>

/*
 * Management pages of a fictional 10G 850 nm SFP+ with internally calibrated
 * digital diagnostics, rebuilt before every test so a test may mutate them.
 */
static uint8_t page_a0[SFF8472_PAGE_SIZE];
static uint8_t page_a2[SFF8472_PAGE_SIZE];

static void set_ascii(uint8_t *buf, size_t off, const char *str, size_t field_len)
{
	size_t len = strlen(str);

	for (size_t i = 0U; i < field_len; i++) {
		buf[off + i] = (i < len) ? (uint8_t)str[i] : (uint8_t)' ';
	}
}

static void set_be16(uint8_t *buf, size_t off, uint16_t val)
{
	buf[off] = (uint8_t)(val >> 8);
	buf[off + 1U] = (uint8_t)(val & 0xFFU);
}

static void build_pages(void)
{
	uint8_t cc;

	(void)memset(page_a0, 0, sizeof(page_a0));
	(void)memset(page_a2, 0, sizeof(page_a2));

	/* Serial ID page (A0h). */
	page_a0[SFF8472_A0_IDENTIFIER] = SFF8472_ID_SFP;
	page_a0[SFF8472_A0_CONNECTOR] = 0x07U; /* LC */
	page_a0[SFF8472_A0_BR_NOMINAL] = 103U; /* 10300 MBd */
	set_ascii(page_a0, SFF8472_A0_VENDOR_NAME, "ACME PHOTONICS", SFF8472_VENDOR_NAME_LEN);
	page_a0[SFF8472_A0_VENDOR_OUI + 0U] = 0x00U;
	page_a0[SFF8472_A0_VENDOR_OUI + 1U] = 0x90U;
	page_a0[SFF8472_A0_VENDOR_OUI + 2U] = 0x65U;
	set_ascii(page_a0, SFF8472_A0_VENDOR_PN, "AFBR-709SMZ", SFF8472_VENDOR_PN_LEN);
	set_ascii(page_a0, SFF8472_A0_VENDOR_REV, "A1", SFF8472_VENDOR_REV_LEN);
	set_be16(page_a0, SFF8472_A0_WAVELENGTH, 850U);
	set_ascii(page_a0, SFF8472_A0_VENDOR_SN, "SN12345678", SFF8472_VENDOR_SN_LEN);
	set_ascii(page_a0, SFF8472_A0_DATE_CODE, "230115", SFF8472_DATE_CODE_LEN);
	page_a0[SFF8472_A0_DIAG_MON_TYPE] =
		SFF8472_DIAG_MON_DDM_IMPLEMENTED | SFF8472_DIAG_MON_INT_CAL;

	cc = 0U;
	for (size_t i = 0U; i < SFF8472_A0_CC_BASE; i++) {
		cc += page_a0[i];
	}
	page_a0[SFF8472_A0_CC_BASE] = cc;

	cc = 0U;
	for (size_t i = 64U; i < SFF8472_A0_CC_EXT; i++) {
		cc += page_a0[i];
	}
	page_a0[SFF8472_A0_CC_EXT] = cc;

	/* Diagnostics page (A2h). */
	set_be16(page_a2, SFF8472_A2_TEMP, 0x2D80U);   /* +45.5 C */
	set_be16(page_a2, SFF8472_A2_VCC, 33000U);     /* 3.300 V */
	set_be16(page_a2, SFF8472_A2_TX_BIAS, 3250U);  /* 6.5 mA */
	set_be16(page_a2, SFF8472_A2_TX_POWER, 5000U); /* 500 uW */
	set_be16(page_a2, SFF8472_A2_RX_POWER, 4000U); /* 400 uW */
	page_a2[SFF8472_A2_ALARM_FLAGS] = BIT(7) | BIT(3);     /* temp high, tx bias high */
	page_a2[SFF8472_A2_ALARM_FLAGS + 1U] = BIT(6);         /* rx power low */
	page_a2[SFF8472_A2_WARNING_FLAGS] = BIT(4);            /* vcc low */
	page_a2[SFF8472_A2_WARNING_FLAGS + 1U] = 0U;
}

static void sff8472_before(void *fixture)
{
	ARG_UNUSED(fixture);
	build_pages();
}

ZTEST_SUITE(sff8472, NULL, NULL, sff8472_before, NULL, NULL);

ZTEST(sff8472, test_parse_id)
{
	struct sff8472_id id;

	zassert_ok(sff8472_parse_id(page_a0, &id));

	zassert_equal(id.identifier, SFF8472_ID_SFP);
	zassert_equal(id.connector, 0x07U);
	zassert_equal(id.br_nominal_mbd, 10300U);
	zassert_equal(id.wavelength_nm, 850U);
	zassert_equal(id.vendor_oui[0], 0x00U);
	zassert_equal(id.vendor_oui[1], 0x90U);
	zassert_equal(id.vendor_oui[2], 0x65U);
	zassert_equal(strcmp(id.vendor_name, "ACME PHOTONICS"), 0, "got '%s'", id.vendor_name);
	zassert_equal(strcmp(id.vendor_pn, "AFBR-709SMZ"), 0, "got '%s'", id.vendor_pn);
	zassert_equal(strcmp(id.vendor_rev, "A1"), 0, "got '%s'", id.vendor_rev);
	zassert_equal(strcmp(id.vendor_sn, "SN12345678"), 0, "got '%s'", id.vendor_sn);
	zassert_equal(strcmp(id.date_code, "230115"), 0, "got '%s'", id.date_code);
	zassert_true(id.ddm_supported);
	zassert_true(id.ddm_internally_calibrated);
	zassert_false(id.ddm_externally_calibrated);
}

ZTEST(sff8472, test_checksums)
{
	zassert_true(sff8472_base_checksum_valid(page_a0));
	zassert_true(sff8472_ext_checksum_valid(page_a0));

	/* Corrupt a byte covered by each check code. */
	page_a0[SFF8472_A0_VENDOR_NAME] ^= 0xFFU;
	zassert_false(sff8472_base_checksum_valid(page_a0));

	page_a0[SFF8472_A0_VENDOR_SN] ^= 0xFFU;
	zassert_false(sff8472_ext_checksum_valid(page_a0));
}

ZTEST(sff8472, test_parse_diagnostics)
{
	struct sff8472_diagnostics diag;

	zassert_ok(sff8472_parse_diagnostics(page_a2, &diag));

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

ZTEST(sff8472, test_negative_temperature)
{
	struct sff8472_diagnostics diag;

	/* -10.0 C == -2560 in 1/256 C units (0xF600). */
	set_be16(page_a2, SFF8472_A2_TEMP, 0xF600U);

	zassert_ok(sff8472_parse_diagnostics(page_a2, &diag));
	zassert_equal(diag.temp_mdegc, -10000, "got %d", diag.temp_mdegc);
}

ZTEST(sff8472, test_null_arguments)
{
	struct sff8472_id id;
	struct sff8472_diagnostics diag;

	zassert_equal(sff8472_parse_id(NULL, &id), -EINVAL);
	zassert_equal(sff8472_parse_id(page_a0, NULL), -EINVAL);
	zassert_equal(sff8472_parse_diagnostics(NULL, &diag), -EINVAL);
	zassert_equal(sff8472_parse_diagnostics(page_a2, NULL), -EINVAL);
	zassert_false(sff8472_base_checksum_valid(NULL));
	zassert_false(sff8472_ext_checksum_valid(NULL));
}

ZTEST(sff8472, test_name_lookup)
{
	zassert_equal(strcmp(sff8472_identifier_name(SFF8472_ID_SFP), "SFP/SFP+/SFP28"), 0);
	zassert_equal(strcmp(sff8472_connector_name(0x07U), "LC"), 0);

	/* Unknown values still return a non-NULL, printable string. */
	zassert_not_null(sff8472_identifier_name(0xAAU));
	zassert_not_null(sff8472_connector_name(0xAAU));
}
