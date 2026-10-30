/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>
#include "fcb_test.h"

#define TORN_LEN 16

/* Data of TORN_LEN bytes whose CRC-8, as FCB computes it over the length and
 * the data, is the erased value of the flash: the end marker such an entry
 * would get without CONFIG_FCB_ENDMARKER_NEVER_ERASED.
 */
static void data_with_erased_crc(struct fcb *_fcb, uint8_t *data)
{
	uint8_t len_buf[2];
	uint8_t crc8;
	int cnt;
	int i;

	cnt = fcb_put_len(_fcb, len_buf, TORN_LEN);
	zassert_true(cnt > 0, "fcb_put_len call failure");
	for (i = 0; i < TORN_LEN; i++) {
		data[i] = (uint8_t)(i * 37 + 11);
	}
	for (i = 0; i < 256; i++) {
		data[TORN_LEN - 1] = (uint8_t)i;
		crc8 = crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, len_buf, cnt);
		crc8 = crc8_ccitt(crc8, data, TORN_LEN);
		if (crc8 == _fcb->f_erase_value) {
			return;
		}
	}
	zassert_unreachable("no data with an erased CRC-8");
}

/* An entry whose data was written but not its end marker, as a power loss
 * leaves it, is not valid, even when its CRC-8 equals the erased value.
 */
ZTEST(fcb_test_with_2sectors_set, test_fcb_entry_without_endmarker)
{
	struct fcb_entry loc;
	uint8_t data[TORN_LEN];
	int rc;

	if (!IS_ENABLED(CONFIG_FCB_ENDMARKER_NEVER_ERASED)) {
		ztest_test_skip();
	}
	data_with_erased_crc(&test_fcb, data);
	rc = fcb_append(&test_fcb, TORN_LEN, &loc);
	zassert_true(rc == 0, "fcb_append call failure");
	rc = flash_area_write(test_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), data, TORN_LEN);
	zassert_true(rc == 0, "flash_area_write call failure");
	/* no fcb_append_finish(): the power went before the end marker */
	rc = fcb_elem_info(&test_fcb, &loc);
	zassert_equal(rc, -EBADMSG, "an entry without its end marker was taken as valid");
}

/* The same data, finished, is a valid entry, and its end marker is not the
 * erased value.
 */
ZTEST(fcb_test_with_2sectors_set, test_fcb_entry_with_erased_crc)
{
	struct fcb_entry loc;
	uint8_t data[TORN_LEN];
	uint8_t em;
	int rc;

	if (!IS_ENABLED(CONFIG_FCB_ENDMARKER_NEVER_ERASED)) {
		ztest_test_skip();
	}
	data_with_erased_crc(&test_fcb, data);
	rc = fcb_append(&test_fcb, TORN_LEN, &loc);
	zassert_true(rc == 0, "fcb_append call failure");
	rc = flash_area_write(test_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), data, TORN_LEN);
	zassert_true(rc == 0, "flash_area_write call failure");
	rc = fcb_append_finish(&test_fcb, &loc);
	zassert_true(rc == 0, "fcb_append_finish call failure");
	rc = fcb_elem_info(&test_fcb, &loc);
	zassert_true(rc == 0, "a finished entry was not valid");
	rc = flash_area_read(test_fcb.fap,
			     loc.fe_sector->fs_off + loc.fe_data_off +
				     fcb_len_in_flash(&test_fcb, loc.fe_data_len),
			     &em, sizeof(em));
	zassert_true(rc == 0, "flash_area_read call failure");
	zassert_not_equal(em, test_fcb.f_erase_value, "the end marker is the erased value");
}
