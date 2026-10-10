/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/byteorder.h>

/* The advertising and scan response data carry the step as Manufacturer
 * Specific Data of the company ID reserved for tests, so that the scanner
 * can tell the steps and the two kinds of data apart.
 */
#define TEST_COMPANY_ID    0xFFFFU
#define TEST_DATA_ADV      0x01U
#define TEST_DATA_SCAN_RSP 0x02U
#define TEST_DATA_LEN      4U

/* The scanner scans with this identity, which the advertiser puts on its
 * Filter Accept List.
 */
#define SCANNER_ADDR "C0:00:00:00:00:02"

/* The advertiser moves to the next step once the scanner has seen the
 * current one.
 */
enum test_step {
	/* ADV_NONCONN_IND */
	STEP_NONCONN,
	/* ADV_SCAN_IND, answered with a SCAN_RSP */
	STEP_SCAN,
	/* The advertising and scan response data updated while advertising */
	STEP_SCAN_UPDATE,
	/* Scan requests filtered, from a scanner on the Filter Accept List */
	STEP_FAL_ACCEPT,
	/* Scan requests filtered, from a scanner not on the Filter Accept List */
	STEP_FAL_REJECT,
	STEP_COUNT,
};

static inline void test_data_init(uint8_t *data, uint8_t kind, uint8_t step)
{
	sys_put_le16(TEST_COMPANY_ID, data);
	data[2] = kind;
	data[3] = step;
}
