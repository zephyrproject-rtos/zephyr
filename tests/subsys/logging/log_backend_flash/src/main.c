/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The tests cover two aspects:
 *
 * - the backend-specific functions e.g. log_backend_flash_query();
 * - how the flash logging backend behaves over reboot.
 *
 * To emulate reboot, the reboot() function drops everything the backend
 * remembers and makes it read the flash again. It relies on the backend's
 * init() to do exactly that.
 *
 * To simplify the tests, they rely on the specifics of the layout used by the
 * flash backend.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_flash.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "test_build_id.h"

LOG_MODULE_REGISTER(test, LOG_LEVEL_INF);

static const struct flash_area *const fa = PARTITION_BY_NODE(DT_CHOSEN(zephyr_log_partition));

/* What the backend writes, as far as the staging helpers below need to know.
 *
 * NOTE: if the layout changes, tests should be added to validate backward
 * compatibility.
 */
struct part_hdr {
	uint32_t magic;
	uint16_t version;
	uint16_t reserved;
	char build_id[LOG_BACKEND_FLASH_BUILD_ID_SIZE];
} __packed;

struct rec_frame {
	uint16_t len;
	uint16_t crc;
} __packed;

/*
 * One scenario builds with CONFIG_LOG_BACKEND_FLASH_BUILD_ID=NULL, where the
 * backend stores no build id and makes no check. The Kconfig value reaches C as
 * the text of the expression, which is enough to tell the two apart.
 */
static bool firmware_has_a_build_id(void)
{
	return strcmp(CONFIG_LOG_BACKEND_FLASH_BUILD_ID, "NULL") != 0;
}

/** What the partition header should carry for a log this firmware wrote. */
static const char *own_build_id(void)
{
	return firmware_has_a_build_id() ? TEST_BUILD_ID : "";
}

/** Enough for every record any test here stores. */
#define READ_MAX 2048

static uint8_t read_buf[READ_MAX];

/**
 * Put the backend back in the state it would be in after a reboot: it forgets
 * everything and works the partition out again from what is on the medium.
 */
static void reboot(void)
{
	const struct log_backend *backend = log_backend_get_by_name("log_backend_flash");

	zassert_not_null(backend, "flash backend not registered");
	log_backend_init(backend);
}

static void partition_fill(uint8_t val)
{
	uint8_t chunk[64];

	memset(chunk, val, sizeof(chunk));

	for (off_t off = 0; off < (off_t)fa->fa_size; off += sizeof(chunk)) {
		zassert_ok(flash_area_write(fa, off, chunk, sizeof(chunk)));
	}
}

static void partition_wipe(void)
{
	zassert_ok(flash_area_flatten(fa, 0, fa->fa_size));
}

/** Bytes the log occupies, which is also where the next record goes. */
static size_t query_used(void)
{
	struct log_backend_flash_info info;

	log_flush();
	zassert_ok(log_backend_flash_query(&info));

	return info.used;
}

/**
 * Overwrite bytes anywhere in the partition, through the write blocks they sit
 * in. That is how damage is staged: nothing protects the framing in front of a
 * record, and the CRC that covers the record itself is only checked, never
 * relied on to stay right.
 */
static void poke(off_t off, const void *data, size_t len)
{
	size_t align = flash_area_align(fa);
	uint8_t block[64];
	off_t block_off;
	size_t chunk;

	block_off = (off_t)ROUND_DOWN((size_t)off, align);
	chunk = ROUND_UP((size_t)(off - block_off) + len, align);
	zassert_true(chunk <= sizeof(block), "damage spans more than the test buffer");

	zassert_ok(flash_area_read(fa, block_off, block, chunk));
	memcpy(&block[off - block_off], data, len);
	zassert_ok(flash_area_write(fa, block_off, block, chunk));
}

/** Stamp the log with a build id, which is how a foreign log is staged. */
static void restamp_build_id(const char *id)
{
	struct part_hdr hdr;

	zassert_ok(flash_area_read(fa, 0, &hdr, sizeof(hdr)));

	memset(hdr.build_id, 0, sizeof(hdr.build_id));
	strncpy(hdr.build_id, id, sizeof(hdr.build_id));

	/* RAM-like memory, so the header can simply be written over. */
	zassert_ok(flash_area_write(fa, 0, &hdr, sizeof(hdr)));
}

/**
 * Stamp the log with a format version the backend does not know, which is how a
 * log written by a later layout is staged. Which version it becomes does not
 * matter, only that it is not the one in the partition now.
 */
static void restamp_unknown_version(void)
{
	struct part_hdr hdr;

	zassert_ok(flash_area_read(fa, 0, &hdr, sizeof(hdr)));
	hdr.version++;
	zassert_ok(flash_area_write(fa, 0, &hdr, sizeof(hdr)));
}

/** Snapshot of the start of the partition, enough to cover a few records. */
#define SNAPSHOT_MAX 512

static void partition_snapshot(uint8_t *buf)
{
	zassert_ok(flash_area_read(fa, 0, buf, SNAPSHOT_MAX));
}

static int char_out(int c, void *ctx)
{
	char **p = ctx;

	*(*p)++ = c;

	return c;
}

/*
 * To validate correct logging of messages, TEST_LOG_INF builds up an array of
 * expected strings, and expect_log() reads out the log_dict stream, checks the
 * metadata, and converts the message to a string to compare against the
 * expected string.
 *
 * expect_reset() clears the stored expectations.
 */
#define EXPECTED_MAX      10
#define EXPECTED_TEXT_MAX 100

static char expected_text[EXPECTED_MAX][EXPECTED_TEXT_MAX];
static const void *expected_data[EXPECTED_MAX];
static size_t expected_data_len[EXPECTED_MAX];
static size_t expected_cnt;

static void expect_reset(void)
{
	/* TEST_LOG_INF() records no payload, so the payload of a record has to
	 * start out empty rather than be cleared per message.
	 */
	memset(expected_data, 0, sizeof(expected_data));
	memset(expected_data_len, 0, sizeof(expected_data_len));
	expected_cnt = 0;
}

/**
 * Log a message, wait for it to reach the backend, and record what it should
 * read back as.
 *
 * Flushing here rather than at the call sites is what keeps a test from being
 * written that reboots before its messages were processed. Messages logged with
 * plain LOG_INF() still need a flush, which expect_log() does.
 */
#define TEST_LOG_INF(...)                                                                          \
	do {                                                                                       \
		int printed;                                                                       \
		zassert_true(expected_cnt < EXPECTED_MAX,                                          \
			     "more messages logged than the test records");                        \
		LOG_INF(__VA_ARGS__);                                                              \
		log_flush();                                                                       \
		printed = snprintf(expected_text[expected_cnt], EXPECTED_TEXT_MAX, __VA_ARGS__);   \
		zassert_true(printed < EXPECTED_TEXT_MAX, "text too large for " __VA_ARGS__);      \
		expected_cnt++;                                                                    \
	} while (0)

static ssize_t read_all(uint32_t *cnt)
{
	ssize_t len = log_backend_flash_read(0, 0, read_buf, sizeof(read_buf), cnt);

	zassert_true(len >= 0, "read failed: %zd", len);

	return len;
}

/**
 * Check the stored log against what TEST_LOG_INF() recorded, and the rest of
 * what query() reports against what the caller expects.
 *
 * What log_backend_flash_read() returns is a dictionary log stream, the same
 * thing log_parser.py is fed, so every record carries the lengths that say
 * where the next one starts.
 *
 * Returns the bytes read, which are left in read_buf.
 */
static ssize_t expect_log_full(const char *build_id, bool build_id_matches, uint32_t dropped)
{
	static uint8_t __aligned(Z_LOG_MSG_ALIGNMENT) pkg[128];
	struct log_backend_flash_info info;
	size_t off = 0;
	uint32_t cnt;
	ssize_t len;

	/* Whatever was logged has to have reached the backend before any of it
	 * can be read back, and putting that here is what keeps every test from
	 * having to remember it.
	 */
	log_flush();

	zassert_ok(log_backend_flash_query(&info));
	zassert_equal(info.record_cnt, expected_cnt, "%u records stored, %zu logged",
		      info.record_cnt, expected_cnt);
	zassert_equal(info.dropped_cnt, dropped, "%u messages dropped, expected %u",
		      info.dropped_cnt, dropped);
	zassert_equal(info.build_id_matches, build_id_matches);
	zassert_str_equal(info.build_id, build_id);
	zassert_true(info.size < fa->fa_size, "the partition needs room for a header");
	if (info.record_cnt > 0) {
		zassert_true(info.used > 0);
	} else {
		zassert_equal(info.used, 0);
	}

	len = read_all(&cnt);
	zassert_equal(cnt, expected_cnt, "%u records read, %zu logged", cnt, expected_cnt);

	for (size_t i = 0; i < expected_cnt; i++) {
		struct log_dict_output_normal_msg_hdr_t hdr;
		char text[EXPECTED_TEXT_MAX];
		char *p = text;
		int slen;

		zassert_true((off + sizeof(hdr)) <= (size_t)len, "stream ended at record %zu", i);
		memcpy(&hdr, &read_buf[off], sizeof(hdr));

		zassert_equal(hdr.type, MSG_NORMAL);
		zassert_equal(hdr.domain, 0);
		zassert_equal(hdr.level, LOG_LEVEL_INF);
		zassert_str_equal(log_source_name_get(0, (uint32_t)hdr.source), "test");

		zassert_true(hdr.package_len <= sizeof(pkg), "package too big for the test");
		memcpy(pkg, &read_buf[off + sizeof(hdr)], hdr.package_len);

		slen = cbpprintf(char_out, &p, pkg);
		zassert_true(slen >= 0, "cbpprintf() failed: %d", slen);
		text[slen] = '\0';
		zassert_str_equal(text, expected_text[i], "record %zu", i);

		zassert_equal(hdr.data_len, expected_data_len[i], "record %zu payload length", i);
		if (hdr.data_len > 0U) {
			zassert_mem_equal(&read_buf[off + sizeof(hdr) + hdr.package_len],
					  expected_data[i], hdr.data_len, "record %zu payload", i);
		}

		off += sizeof(hdr) + hdr.package_len + hdr.data_len;
	}

	zassert_equal(off, (size_t)len, "the stream holds more than the %zu records logged",
		      expected_cnt);

	return len;
}

/** The usual case: a log this firmware wrote, whole, with nothing dropped. */
static ssize_t expect_log(void)
{
	return expect_log_full(own_build_id(), true, 0);
}

/**
 * Erase the log, which also discards the recorded expectations: what is no
 * longer stored cannot be expected back.
 */
static void erase_log(void)
{
	zassert_ok(log_backend_flash_erase());
	expect_reset();
}

static void *suite_setup(void)
{
	zassert_true(flash_area_device_is_ready(fa), "log partition device not ready");
	zassert_equal(fa->fa_size % 64, 0, "test assumes a partition it can fill in 64B chunks");

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* All tests start with empty flash and freshly initialized log backend. */
	partition_wipe();
	reboot();
	expect_reset();
}

ZTEST_SUITE(log_backend_flash, NULL, suite_setup, test_before, NULL, NULL);

ZTEST(log_backend_flash, test_empty_flash_has_an_empty_log)
{
	expect_log();
}

ZTEST(log_backend_flash, test_erase_leaves_an_empty_log)
{
	LOG_INF("first");
	LOG_INF("second %d", 42);
	log_flush();
	erase_log();
	expect_log();
}

ZTEST(log_backend_flash, test_stored_records_are_dictionary_records)
{
	TEST_LOG_INF("first");
	TEST_LOG_INF("second %d", 42);

	expect_log();
}

ZTEST(log_backend_flash, test_log_survives_a_reboot)
{
	static const uint8_t blob[] = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01, 0x02, 0x03, 0x04};
	static uint8_t before[READ_MAX];
	struct log_backend_flash_info info;
	ssize_t len;
	size_t used;

	TEST_LOG_INF("one");
	TEST_LOG_INF("string %s", "42");
	TEST_LOG_INF("many format arguments %lld %d %d %d %d %d %d %d %d %d", 42LL, 0, 1, 2, 3, 4,
		     5, 6, 7, 8);
	/* Open-coded TEST_LOG_INF that does hexdump instead of string formatting */
	LOG_HEXDUMP_INF(blob, sizeof(blob), "blob");
	zassert_true(expected_cnt < EXPECTED_MAX, "more messages logged than the test records");
	snprintf(expected_text[expected_cnt], EXPECTED_TEXT_MAX, "blob");
	expected_data[expected_cnt] = blob;
	expected_data_len[expected_cnt] = sizeof(blob);
	expected_cnt++;
	log_flush();

	len = expect_log();
	memcpy(before, read_buf, len);

	zassert_ok(log_backend_flash_query(&info));
	used = info.used;

	reboot();

	zassert_ok(log_backend_flash_query(&info));
	zassert_equal(info.used, used);

	zassert_equal(expect_log(), len);
	zassert_mem_equal(read_buf, before, len);
}

ZTEST(log_backend_flash, test_logging_continues_after_a_reboot)
{
	TEST_LOG_INF("before %d", 1);
	TEST_LOG_INF("before %d", 2);

	reboot();

	TEST_LOG_INF("after %d", 1);
	TEST_LOG_INF("after %d", 2);

	expect_log();
}

ZTEST(log_backend_flash, test_erase_survives_a_reboot)
{
	/* Do *not* add to expected. */
	LOG_INF("before the erase");
	log_flush();

	erase_log();
	reboot();

	expect_log();

	TEST_LOG_INF("after the erase");

	expect_log();
}

ZTEST(log_backend_flash, test_read_range)
{
	uint32_t cnt;
	ssize_t whole;
	ssize_t first;
	ssize_t len;

	TEST_LOG_INF("one");
	TEST_LOG_INF("two");
	TEST_LOG_INF("three");

	whole = read_all(&cnt);
	zassert_equal(cnt, 3);

	/* Skipping the first record works; it yields the last 2. */
	len = log_backend_flash_read(1, 0, read_buf, sizeof(read_buf), &cnt);
	zassert_equal(cnt, 2);

	/* Out-of-order reading works. */
	first = log_backend_flash_read(0, 1, read_buf, sizeof(read_buf), &cnt);
	zassert_equal(cnt, 1);
	zassert_true(first > 0);
	zassert_equal(len, whole - first);

	/* Reading from beyond the end of the log is not an error. */
	len = log_backend_flash_read(3, 0, read_buf, sizeof(read_buf), &cnt);
	zassert_equal(len, 0);
	zassert_equal(cnt, 0);

	/* Re-reading everything works. */
	expect_log();
}

ZTEST(log_backend_flash, test_read_stops_when_the_buffer_is_full)
{
	uint32_t cnt;
	ssize_t first;
	ssize_t len;

	TEST_LOG_INF("one");
	TEST_LOG_INF("two");

	first = log_backend_flash_read(0, 1, read_buf, sizeof(read_buf), &cnt);
	zassert_equal(cnt, 1);
	zassert_true(first > 0);

	/* A buffer that holds the first record and not the second. */
	len = log_backend_flash_read(0, 0, read_buf, first, &cnt);
	zassert_equal(len, first);
	zassert_equal(cnt, 1);

	/* A buffer that is still not large enough for the second log. */
	len = log_backend_flash_read(
		0, 0, read_buf, first + sizeof(struct log_dict_output_normal_msg_hdr_t) + 3, &cnt);
	zassert_equal(len, first);
	zassert_equal(cnt, 1);

	/* A buffer too small for even the first record is an error rather than
	 * an empty read: there is a record to hand back and no room to say so.
	 */
	zassert_equal(log_backend_flash_read(0, 0, read_buf, first - 1, &cnt), -ENOSPC);

	/* All of the above doesn't affect the log itself. */
	expect_log();
}

ZTEST(log_backend_flash, test_read_rejects_a_buffer_too_small_for_the_record_asked_for)
{
	uint32_t cnt;
	ssize_t second;

	TEST_LOG_INF("one");
	TEST_LOG_INF("two");
	TEST_LOG_INF("three");

	/* How much the record the next reads start at takes on its own. */
	second = log_backend_flash_read(1, 1, read_buf, sizeof(read_buf), &cnt);
	zassert_equal(cnt, 1);
	zassert_true(second > 0);

	/* One byte short of it. The record that has to fit is the first one
	 * asked for, not the first one in the log: the records before it are
	 * skipped rather than copied, so they ask nothing of the buffer.
	 */
	zassert_equal(log_backend_flash_read(1, 0, read_buf, second - 1, &cnt), -ENOSPC);

	/* Exactly enough for it is not too small. */
	zassert_equal(log_backend_flash_read(1, 1, read_buf, second, &cnt), second);
	zassert_equal(cnt, 1);

	/* All of the above doesn't affect the log itself. */
	expect_log();
}

ZTEST(log_backend_flash, test_read_into_a_buffer_of_no_size)
{
	uint32_t cnt;

	/* Nothing stored, so nothing fails to fit. */
	zassert_equal(log_backend_flash_read(0, 0, read_buf, 0, &cnt), 0);
	zassert_equal(cnt, 0);

	TEST_LOG_INF("one");

	/* A record has to fit now, and no record fits in no space. */
	zassert_equal(log_backend_flash_read(0, 0, read_buf, 0, &cnt), -ENOSPC);

	/* Past the end of the log there is again nothing that has to fit. */
	zassert_equal(log_backend_flash_read(1, 0, read_buf, 0, &cnt), 0);
	zassert_equal(cnt, 0);

	/* All of the above doesn't affect the log itself. */
	expect_log();
}

ZTEST(log_backend_flash, test_damaged_record_ends_the_log)
{
	uint8_t byte;
	off_t at;

	/* Logged with TEST_LOG_INF(), so expected back. */
	TEST_LOG_INF("before the damage %d", 1);
	TEST_LOG_INF("before the damage %d", 2);

	/* The body of the record after those, a few bytes into its dictionary
	 * header. Logged with LOG_INF(), so not expected back.
	 */
	at = (off_t)(sizeof(struct part_hdr) + query_used() + sizeof(struct rec_frame) + 4U);

	LOG_INF("damaged");
	LOG_INF("and the one after it");
	log_flush();

	zassert_ok(flash_area_read(fa, at, &byte, sizeof(byte)));
	byte ^= 0xff;
	poke(at, &byte, sizeof(byte));
	reboot();

	/* A record that fails its CRC is the end of the log, like anything else
	 * that is not a record. The record after the damaged one is
	 * unreachable: the length in front of the damaged one is the only thing
	 * that says where it begins, and that is not to be trusted.
	 */
	expect_log();

	/* And the next record is written over the damaged one. */
	TEST_LOG_INF("after the damage");
	expect_log();
}

ZTEST(log_backend_flash, test_unwritten_partition_is_adopted)
{
	partition_wipe();
	reboot();

	expect_log();

	TEST_LOG_INF("hello");
	reboot();

	expect_log();
}

ZTEST(log_backend_flash, test_unknown_content_is_reformatted)
{
	partition_fill(0x5a);
	reboot();

	expect_log();

	TEST_LOG_INF("hello");
	reboot();

	expect_log();
}

ZTEST(log_backend_flash, test_damaged_frame_length_ends_the_log)
{
	uint16_t len;
	off_t at;

	TEST_LOG_INF("before the damage %d", 1);
	TEST_LOG_INF("before the damage %d", 2);

	/* The length in front of the record after those. */
	at = (off_t)(sizeof(struct part_hdr) + query_used() + offsetof(struct rec_frame, len));

	LOG_INF("damaged");
	LOG_INF("and the one after it");
	log_flush();

	/* A length that cannot belong to a record the backend wrote. */
	len = 0xfffe;
	poke(at, &len, sizeof(len));
	reboot();

	/* Where the next record begins is gone, so the log ends here. */
	expect_log();

	TEST_LOG_INF("after the damage");
	expect_log();
}

ZTEST(log_backend_flash, test_plausible_frame_length_still_ends_the_log)
{
	uint16_t len;
	off_t at;

	TEST_LOG_INF("before the damage %d", 1);
	TEST_LOG_INF("before the damage %d", 2);

	at = (off_t)(sizeof(struct part_hdr) + query_used() + offsetof(struct rec_frame, len));

	LOG_INF("damaged");
	LOG_INF("and the one after it");
	log_flush();

	/* This length has no cbprintf payload so is shorter than the actual
	 * length. The CRC should capture the incorrect length.
	 */
	len = sizeof(struct log_dict_output_normal_msg_hdr_t);
	poke(at, &len, sizeof(len));
	reboot();

	expect_log();

	TEST_LOG_INF("after the damage");
	expect_log();
}

ZTEST(log_backend_flash, test_erase_clears_the_whole_partition)
{
	uint8_t erased = flash_area_erased_val(fa);
	struct log_backend_flash_info info;
	uint8_t chunk[64];
	uint8_t junk[64];
	size_t hdr_size;
	off_t beyond;

	for (int i = 0; i < 6; i++) {
		TEST_LOG_INF("message %d", i);
	}

	zassert_ok(log_backend_flash_query(&info));
	hdr_size = fa->fa_size - info.size;
	zassert_true(info.used > 0);

	/* Something well past where the log reached, which is what an erase
	 * that stopped at the end of the log would leave behind.
	 */
	beyond = (off_t)ROUND_UP(hdr_size + info.used + sizeof(junk), sizeof(junk));
	memset(junk, 0x5a, sizeof(junk));
	zassert_ok(flash_area_write(fa, beyond, junk, sizeof(junk)));

	erase_log();

	/*
	 * Every byte past the header goes, not only the ones the log reached.
	 * What terminates the walk as the log grows back is the two zero bytes
	 * written after each record, and those do not land atomically with it:
	 * a record ending on or one byte short of a write block boundary puts
	 * them in the next block, a separate flash write. Lose power in between
	 * and the record sits there without them, and anything left further up
	 * would be read as a record that was never logged.
	 */
	for (off_t off = (off_t)hdr_size; off < (off_t)fa->fa_size; off += sizeof(chunk)) {
		zassert_ok(flash_area_read(fa, off, chunk, sizeof(chunk)));

		for (size_t i = 0; i < sizeof(chunk); i++) {
			zassert_equal(chunk[i], erased, "byte %ld survived the erase",
				      (long)(off + (off_t)i));
		}
	}
}

ZTEST(log_backend_flash, test_records_left_by_a_partial_erase_are_not_adopted)
{
	static uint8_t before[SNAPSHOT_MAX];
	static uint8_t image[SNAPSHOT_MAX];
	struct log_backend_flash_info info;
	size_t align = flash_area_align(fa);
	size_t hdr_size;
	size_t first;
	size_t used;

	/* Records of the same size, so that one written later ends exactly
	 * where the one after it began.
	 */
	TEST_LOG_INF("message %d", 0);
	first = query_used();

	TEST_LOG_INF("message %d", 1);
	TEST_LOG_INF("message %d", 2);
	used = query_used();

	zassert_ok(log_backend_flash_query(&info));
	hdr_size = fa->fa_size - info.size;
	zassert_true((hdr_size + used) <= SNAPSHOT_MAX, "log bigger than the test snapshot");

	partition_snapshot(before);

	/*
	 * An erase that did not clear everything: the header is fresh and the
	 * log reads as empty, but the records past the first one are still on
	 * the medium. That is what is left behind when a walk stopped early and
	 * the erase only cleared as far as the walk got.
	 */
	erase_log();

	zassert_ok(flash_area_read(fa, 0, image, SNAPSHOT_MAX));
	memcpy(&image[hdr_size + first], &before[hdr_size + first], used - first);
	zassert_ok(flash_area_write(fa, 0, image, ROUND_UP(hdr_size + used, align)));

	reboot();
	expect_log();

	/* A record the size of the one that was cleared, so that it ends
	 * exactly where the stale second record begins.
	 */
	TEST_LOG_INF("message %d", 0);

	zassert_ok(log_backend_flash_query(&info));
	zassert_equal(info.record_cnt, 1, "%u records stored: the stale ones were adopted",
		      info.record_cnt);
	zassert_equal(info.used, first, "the log reaches past the record just written");
}

ZTEST(log_backend_flash, test_unknown_format_version_stops_logging)
{
	static uint8_t before[SNAPSHOT_MAX];
	static uint8_t after[SNAPSHOT_MAX];
	struct log_backend_flash_info info;

	TEST_LOG_INF("written in a layout this firmware knows");

	restamp_unknown_version();
	reboot();

	/* Nothing in a layout this firmware does not know can be located, so
	 * asking about the log and reading it say so rather than guess.
	 */
	zassert_equal(log_backend_flash_query(&info), -ENOTSUP);
	zassert_equal(log_backend_flash_read(0, 0, read_buf, sizeof(read_buf), NULL), -ENOTSUP);

	/* And nothing is added to it: the partition is left exactly as it was,
	 * which is the only thing that can be checked while it cannot be read.
	 */
	partition_snapshot(before);
	LOG_INF("must not be stored");
	log_flush();
	partition_snapshot(after);
	zassert_mem_equal(after, before, SNAPSHOT_MAX, "a log in an unknown layout was written to");

	/* An erase is what recovers, and it works whatever is in there. */
	erase_log();
	expect_log();

	TEST_LOG_INF("stored again");
	expect_log();
}

ZTEST(log_backend_flash, test_foreign_build_id_stops_logging)
{
	if (!firmware_has_a_build_id()) {
		ztest_test_skip();
	}

	TEST_LOG_INF("written by the firmware that owns this log");

	/* Restamping leaves the records alone, so they are still expected. */
	restamp_build_id(TEST_FOREIGN_BUILD_ID);
	reboot();

	/* A foreign log reads back whole; it is only writing that is refused. */
	expect_log_full(TEST_FOREIGN_BUILD_ID, false, 0);

	LOG_INF("must not be stored");
	expect_log_full(TEST_FOREIGN_BUILD_ID, false, 1);

	/* An explicit erase is what gets logging going again. */
	erase_log();
	expect_log();

	TEST_LOG_INF("stored again");
	expect_log();
}

ZTEST(log_backend_flash, test_a_log_carrying_no_build_id_is_not_claimed)
{
	if (!firmware_has_a_build_id()) {
		ztest_test_skip();
	}

	TEST_LOG_INF("written by firmware that carried no build id");

	/* A log written by firmware built with the build id set to NULL. Its
	 * records are no more decodable against this firmware than any other
	 * build's, so it is somebody else's log too.
	 */
	restamp_build_id("");
	reboot();

	expect_log_full("", false, 0);

	LOG_INF("must not be stored");
	expect_log_full("", false, 1);

	/* And the header is left as it was: claiming the log by stamping this
	 * firmware's build id on it would make the records already in it look
	 * decodable against this build, which they are not.
	 */
	erase_log();
	expect_log();

	TEST_LOG_INF("stored again");
	expect_log();
}

ZTEST(log_backend_flash, test_without_a_build_id_any_log_is_appended_to)
{
	if (firmware_has_a_build_id()) {
		ztest_test_skip();
	}

	TEST_LOG_INF("hello");
	reboot();

	/* own_build_id() is empty in this build, so expect_log() is already
	 * saying that no build id was stored and that nothing mismatches.
	 */
	expect_log();

	/* A log stamped by other firmware is added to rather than refused,
	 * which is the whole of what carrying no build id means.
	 */
	restamp_build_id(TEST_FOREIGN_BUILD_ID);
	reboot();

	expect_log_full(TEST_FOREIGN_BUILD_ID, true, 0);

	TEST_LOG_INF("and another");
	expect_log_full(TEST_FOREIGN_BUILD_ID, true, 0);
}

ZTEST(log_backend_flash, test_full_partition_stops_logging)
{
	struct log_backend_flash_info info;
	uint32_t stored;
	size_t used;

	/* Far more than the partition holds. Flushing as we go keeps the core
	 * from dropping the messages before the backend ever sees them, so that
	 * what is counted here is the partition filling up.
	 */
	for (int i = 0; i < 1000; i++) {
		LOG_INF("filling the log with message %d", i);
		if ((i % 16) == 0) {
			log_flush();
		}
	}
	log_flush();

	zassert_ok(log_backend_flash_query(&info));
	zassert_true(info.record_cnt > 0);
	zassert_true(info.used <= info.size, "wrote past the end of the partition");
	zassert_true(info.dropped_cnt > 0, "dropped messages were not counted");
	stored = info.record_cnt;
	used = info.used;

	reboot();

	zassert_ok(log_backend_flash_query(&info));
	zassert_equal(info.record_cnt, stored, "%u records after the reboot, %u before",
		      info.record_cnt, stored);
	zassert_equal(info.used, used);
	zassert_equal(info.dropped_cnt, 0, "the dropped count survived the reboot");

	/* Still full after the reboot, so a record of the size that stopped
	 * fitting still does not fit. It has to be the same message: the log
	 * stops at the first record too big for what is left, not at a mark,
	 * so a shorter one could still go in.
	 */
	LOG_INF("filling the log with message %d", 1000);
	log_flush();

	zassert_ok(log_backend_flash_query(&info));
	zassert_equal(info.record_cnt, stored);
	zassert_equal(info.used, used);

	/* After erase we can log again and dropped count is zero. */
	erase_log();
	expect_log();
	TEST_LOG_INF("Space after erase");
	expect_log();
	reboot();
	expect_log();
}
