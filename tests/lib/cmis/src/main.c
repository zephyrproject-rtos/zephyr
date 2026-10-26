/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/cmis/cmis.h>

/*
 * Page 00h of a fictional CMIS 5.0 QSFP-DD 400G single-mode module, rebuilt
 * before every test so a test may mutate it. The flat image holds the 128-byte
 * Lower Memory followed by the 128-byte Upper Page 00h.
 */
static uint8_t page00[CMIS_PAGE_00H_SIZE];

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

static void build_page(void)
{
	(void)memset(page00, 0, sizeof(page00));

	/* Lower Memory: identity, state and module monitors. */
	page00[CMIS_LOWER_IDENTIFIER] = CMIS_ID_QSFP_DD;
	page00[CMIS_LOWER_REVISION] = 0x50U;              /* CMIS 5.0 */
	page00[CMIS_LOWER_MEMORY_MODEL] = 0x00U;          /* paged memory */
	page00[CMIS_LOWER_MODULE_STATE] = CMIS_MODULE_STATE_READY << 1U;
	page00[CMIS_LOWER_MODULE_FLAGS] = BIT(0) | BIT(4); /* temp high alarm, vcc high alarm */
	set_be16(page00, CMIS_LOWER_TEMPERATURE, 0x2D80U); /* +45.5 C */
	set_be16(page00, CMIS_LOWER_VCC, 33000U);         /* 3.300 V */
	page00[CMIS_LOWER_MEDIA_TYPE] = CMIS_MEDIA_OPTICAL_SMF;

	/* Upper Page 00h: administrative information. */
	set_ascii(page00, CMIS_PAGE00_VENDOR_NAME, "ACME PHOTONICS", CMIS_VENDOR_NAME_LEN);
	page00[CMIS_PAGE00_VENDOR_OUI + 0U] = 0x00U;
	page00[CMIS_PAGE00_VENDOR_OUI + 1U] = 0x90U;
	page00[CMIS_PAGE00_VENDOR_OUI + 2U] = 0x65U;
	set_ascii(page00, CMIS_PAGE00_VENDOR_PN, "QSFP-DD-400G", CMIS_VENDOR_PN_LEN);
	set_ascii(page00, CMIS_PAGE00_VENDOR_REV, "B2", CMIS_VENDOR_REV_LEN);
	set_ascii(page00, CMIS_PAGE00_VENDOR_SN, "QD2400001234", CMIS_VENDOR_SN_LEN);
	set_ascii(page00, CMIS_PAGE00_DATE_CODE, "240620", CMIS_DATE_CODE_LEN);
	page00[CMIS_PAGE00_POWER_CLASS] = 4U << 5U;               /* power class 4 */
	page00[CMIS_PAGE00_MAX_POWER] = 56U;                      /* 56 * 0.25 W == 14 W */
}

static void cmis_before(void *fixture)
{
	ARG_UNUSED(fixture);
	build_page();
}

ZTEST_SUITE(cmis, NULL, NULL, cmis_before, NULL, NULL);

ZTEST(cmis, test_parse_id)
{
	struct cmis_id id;

	zassert_ok(cmis_parse_id(page00, &id));

	zassert_equal(id.identifier, CMIS_ID_QSFP_DD);
	zassert_equal(id.cmis_major, 5U);
	zassert_equal(id.cmis_minor, 0U);
	zassert_false(id.flat_memory);
	zassert_equal(id.module_state, CMIS_MODULE_STATE_READY);
	zassert_equal(id.media_type, CMIS_MEDIA_OPTICAL_SMF);
	zassert_equal(id.power_class, 4U);
	zassert_equal(id.max_power_mw, 14000U, "got %u", id.max_power_mw);
	zassert_equal(id.vendor_oui[0], 0x00U);
	zassert_equal(id.vendor_oui[1], 0x90U);
	zassert_equal(id.vendor_oui[2], 0x65U);
	zassert_equal(strcmp(id.vendor_name, "ACME PHOTONICS"), 0, "got '%s'", id.vendor_name);
	zassert_equal(strcmp(id.vendor_pn, "QSFP-DD-400G"), 0, "got '%s'", id.vendor_pn);
	zassert_equal(strcmp(id.vendor_rev, "B2"), 0, "got '%s'", id.vendor_rev);
	zassert_equal(strcmp(id.vendor_sn, "QD2400001234"), 0, "got '%s'", id.vendor_sn);
	zassert_equal(strcmp(id.date_code, "240620"), 0, "got '%s'", id.date_code);
}

ZTEST(cmis, test_flat_memory)
{
	struct cmis_id id;

	page00[CMIS_LOWER_MEMORY_MODEL] |= CMIS_MEMORY_MODEL_FLAT_MEM;

	zassert_ok(cmis_parse_id(page00, &id));
	zassert_true(id.flat_memory);
}

ZTEST(cmis, test_parse_monitors)
{
	struct cmis_monitors mon;

	zassert_ok(cmis_parse_monitors(page00, &mon));

	zassert_equal(mon.temp_mdegc, 45500, "got %d", mon.temp_mdegc);
	zassert_equal(mon.vcc_uv, 3300000U, "got %u", mon.vcc_uv);
	zassert_equal(mon.flags, CMIS_FLAG_TEMP_HIGH_ALARM | CMIS_FLAG_VCC_HIGH_ALARM,
		      "got 0x%02x", mon.flags);
}

ZTEST(cmis, test_negative_temperature)
{
	struct cmis_monitors mon;

	/* -10.0 C == -2560 in 1/256 C units (0xF600). */
	set_be16(page00, CMIS_LOWER_TEMPERATURE, 0xF600U);

	zassert_ok(cmis_parse_monitors(page00, &mon));
	zassert_equal(mon.temp_mdegc, -10000, "got %d", mon.temp_mdegc);
}

ZTEST(cmis, test_null_arguments)
{
	struct cmis_id id;
	struct cmis_monitors mon;

	zassert_equal(cmis_parse_id(NULL, &id), -EINVAL);
	zassert_equal(cmis_parse_id(page00, NULL), -EINVAL);
	zassert_equal(cmis_parse_monitors(NULL, &mon), -EINVAL);
	zassert_equal(cmis_parse_monitors(page00, NULL), -EINVAL);
}

ZTEST(cmis, test_name_lookup)
{
	zassert_equal(strcmp(cmis_identifier_name(CMIS_ID_QSFP_DD), "QSFP-DD"), 0);
	zassert_equal(strcmp(cmis_module_state_name(CMIS_MODULE_STATE_READY), "Ready"), 0);
	zassert_equal(strcmp(cmis_media_type_name(CMIS_MEDIA_OPTICAL_SMF), "Single-mode fiber"), 0);

	/* Unknown values still return a non-NULL, printable string. */
	zassert_not_null(cmis_identifier_name(0xAAU));
	zassert_not_null(cmis_module_state_name(0xAAU));
	zassert_not_null(cmis_media_type_name(0xAAU));
}
