/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/hl78xx_apis.h>
#include <zephyr/ztest.h>

/** @brief Revisions at or above 5.7.4.0 support NB-NTN, with or without a model prefix. */
ZTEST(hl78xx_fw_version, test_supported)
{
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.5.7.4.0"));
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.5.7.4.1"));
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.5.8.0.0"));
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.6.0.0.0"));
	zassert_true(hl78xx_fw_version_supports_ntn("5.7.4.0"));
}

/** @brief Revisions below 5.7.4.0 do not support NB-NTN. */
ZTEST(hl78xx_fw_version, test_unsupported)
{
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812.5.7.3.9"));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812.4.9.9.9"));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7800.4.7.1.0"));
}

/** @brief Missing components compare as zero and trailing text is ignored. */
ZTEST(hl78xx_fw_version, test_short_and_suffixed)
{
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.5.7.4"));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812.5.7"));
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.5.7.4.0-rc1"));
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.5.7.4.0.9"));
}

/** @brief NULL, empty and non-numeric revisions compare as unsupported. */
ZTEST(hl78xx_fw_version, test_malformed)
{
	zassert_false(hl78xx_fw_version_supports_ntn(NULL));
	zassert_false(hl78xx_fw_version_supports_ntn(""));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812"));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812."));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812.R6"));
}

/** @brief A component that does not fit in 32 bits is rejected instead of wrapping. */
ZTEST(hl78xx_fw_version, test_component_overflow)
{
	zassert_false(hl78xx_fw_version_supports_ntn("4294967301.7.4.0"));
	zassert_false(hl78xx_fw_version_supports_ntn("HL7812.4294967296.0.0.0"));
	zassert_true(hl78xx_fw_version_supports_ntn("HL7812.4294967295.0.0.0"));
}

ZTEST_SUITE(hl78xx_fw_version, NULL, NULL, NULL, NULL, NULL);
