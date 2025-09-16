/*
 * Minimal SNTP library test
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include <zephyr/ztest.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <string.h>
#include "../../../../../subsys/net/lib/sntp/sntp_pkt.h"

int32_t parse_response(uint8_t *data, uint16_t len, struct sntp_time *expected_orig_ts,
			 struct sntp_time *res);

ZTEST(sntp, test_sntp_init_null)
{
	int ret;

	/* Expect -EFAULT if ctx or addr is NULL */
	ret = sntp_init(NULL, NULL, 0);
	zassert_equal(ret, -EFAULT, "sntp_init should fail with -EFAULT when ctx and addr NULL");
}

static void fill_base_packet(struct sntp_pkt *pkt, struct sntp_time *expected)
{
	memset(pkt, 0, sizeof(*pkt));
	pkt->li = 0;
	pkt->vn = 3;
	pkt->mode = 4; /* server */
	pkt->stratum = 1; /* valid */
	pkt->tx_tm_s = htonl(expected->seconds);
	pkt->tx_tm_f = htonl(expected->fraction);
	pkt->orig_tm_s = htonl(expected->seconds);
	pkt->orig_tm_f = htonl(expected->fraction);
}

ZTEST(sntp, test_parse_response_ok_epoch_1900_based)
{
	struct sntp_time expected = { .seconds = 2208988800ULL + 100, .fraction = 123456789 };
	struct sntp_time out = {0};
	struct sntp_pkt pkt;
	int32_t ret;

	fill_base_packet(&pkt, &expected);
	ret = parse_response((uint8_t *)&pkt, sizeof(pkt), &expected, &out);
	zassert_equal(ret, 0, "parse_response should succeed");
	zassert_equal(out.seconds, 100, "Seconds not converted to Unix epoch correctly");
	zassert_equal(out.fraction, expected.fraction, "Fraction mismatch");
}

ZTEST(sntp, test_parse_response_ok_epoch_2036_based_wrap)
{
	struct sntp_time expected = { .seconds = 100, .fraction = 42 };
	struct sntp_time out = {0};
	struct sntp_pkt pkt;
	int32_t ret;

	fill_base_packet(&pkt, &expected);
	pkt.tx_tm_s = htonl(expected.seconds & 0x7FFFFFFF); /* ensure MSB clear */
	ret = parse_response((uint8_t *)&pkt, sizeof(pkt), &expected, &out);
	zassert_equal(ret, 0, "parse_response should succeed for wrap case");
	uint64_t expected_sec = expected.seconds + 0x100000000ULL - 2208988800ULL;
	zassert_equal(out.seconds, expected_sec, "Wrap conversion incorrect");
}

ZTEST(sntp, test_parse_response_orig_mismatch)
{
	struct sntp_time expected = { .seconds = 2208988800ULL + 10, .fraction = 1 };
	struct sntp_time out = {0};
	struct sntp_pkt pkt;
	int32_t ret;

	fill_base_packet(&pkt, &expected);
	pkt.orig_tm_s = htonl(expected.seconds + 1); /* mismatch */
	ret = parse_response((uint8_t *)&pkt, sizeof(pkt), &expected, &out);
	zassert_equal(ret, -ERANGE, "Expected -ERANGE for originate mismatch");
}

ZTEST(sntp, test_parse_response_wrong_mode)
{
	struct sntp_time expected = { .seconds = 2208988800ULL + 20, .fraction = 2 };
	struct sntp_time out = {0};
	struct sntp_pkt pkt;
	int32_t ret;

	fill_base_packet(&pkt, &expected);
	pkt.mode = 3; /* client */
	ret = parse_response((uint8_t *)&pkt, sizeof(pkt), &expected, &out);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for wrong mode");
}

ZTEST(sntp, test_parse_response_kiss_of_death)
{
	struct sntp_time expected = { .seconds = 2208988800ULL + 30, .fraction = 3 };
	struct sntp_time out = {0};
	struct sntp_pkt pkt;
	int32_t ret;

	fill_base_packet(&pkt, &expected);
	pkt.stratum = 0; /* KoD */
	ret = parse_response((uint8_t *)&pkt, sizeof(pkt), &expected, &out);
	zassert_equal(ret, -EBUSY, "Expected -EBUSY for KoD stratum");
}

ZTEST(sntp, test_parse_response_zero_transmit)
{
	struct sntp_time expected = { .seconds = 2208988800ULL + 40, .fraction = 4 };
	struct sntp_time out = {0};
	struct sntp_pkt pkt;
	int32_t ret;

	fill_base_packet(&pkt, &expected);
	pkt.tx_tm_s = htonl(0);
	pkt.tx_tm_f = htonl(0);
	ret = parse_response((uint8_t *)&pkt, sizeof(pkt), &expected, &out);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for zero transmit timestamp");
}

/* ---- Spec-based NTP <-> Unix conversion helpers (not using implementation) ---- */

#define NTP_UNIX_EPOCH_OFFSET 2208988800UL /* Seconds from 1900-01-01 to 1970-01-01 */

static void spec_unix_to_ntp(uint64_t unix_sec, uint32_t unix_frac_us,
							 uint32_t *ntp_sec, uint32_t *ntp_frac)
{
	/* Era 0 only (before wrap) */
	*ntp_sec = (uint32_t)(unix_sec + NTP_UNIX_EPOCH_OFFSET);
	/* Convert microseconds fraction to 32-bit fractional field */
	uint64_t frac64 = ((uint64_t)unix_frac_us << 32) / 1000000ULL; /* scale to 2^-32 */
	*ntp_frac = (uint32_t)frac64;
}

static void spec_ntp_to_unix(uint32_t ntp_sec, uint32_t ntp_frac,
							 uint64_t *unix_sec, uint32_t *unix_frac_us)
{
	if (ntp_sec >= NTP_UNIX_EPOCH_OFFSET) {
		*unix_sec = (uint64_t)ntp_sec - NTP_UNIX_EPOCH_OFFSET; /* Era 0 */
	} else {
		/* Post-2036 era wrap heuristic: add 2^32 then subtract offset */
		*unix_sec = 0x100000000ULL + (uint64_t)ntp_sec - NTP_UNIX_EPOCH_OFFSET;
	}
	/* Convert fractional 32-bit field back to microseconds with rounding */
	uint64_t us = ((uint64_t)ntp_frac * 1000000ULL + (1ULL << 31)) >> 32;
	*unix_frac_us = (uint32_t)us;
}

ZTEST(sntp, test_spec_ntp_unix_round_trip_prewrap)
{
	uint64_t unix_sec_in = 1704067200ULL; /* 2024-01-01 00:00:00 UTC */
	uint32_t unix_us_in = 987654; /* fractional microseconds */
	uint32_t ntp_sec, ntp_frac;
	uint64_t unix_sec_out; uint32_t unix_us_out;

	spec_unix_to_ntp(unix_sec_in, unix_us_in, &ntp_sec, &ntp_frac);
	/* Validate forward conversion basic properties */
	zassert_true(ntp_sec >= NTP_UNIX_EPOCH_OFFSET, "NTP seconds should exceed offset for pre-wrap date");
	spec_ntp_to_unix(ntp_sec, ntp_frac, &unix_sec_out, &unix_us_out);
	zassert_equal(unix_sec_out, unix_sec_in, "Unix seconds mismatch after round-trip");
	/* Allow 1 microsecond tolerance due to rounding */
	zassert_true((int32_t)unix_us_out - (int32_t)unix_us_in <= 1 && (int32_t)unix_us_in - (int32_t)unix_us_out <= 1,
				 "Unix fractional microseconds mismatch (%u vs %u)", unix_us_in, unix_us_out);
}

ZTEST(sntp, test_spec_ntp_unix_round_trip_postwrap)
{
	/* Simulate a timestamp shortly after the 2036 era wrap: choose NTP seconds small (MSB clear) */
	uint32_t ntp_sec = 100; /* Represents 2036-02-07 06:28:16 + 100 seconds */
	uint32_t ntp_frac = 0x40000000; /* 0.25 seconds */
	uint64_t unix_sec; uint32_t unix_us;
	uint32_t back_ntp_sec; uint32_t back_ntp_frac;

	spec_ntp_to_unix(ntp_sec, ntp_frac, &unix_sec, &unix_us);
	/* Convert back: need unix fraction scaled */
	spec_unix_to_ntp(unix_sec, unix_us, &back_ntp_sec, &back_ntp_frac);
	/* After re-encoding we should get a value with MSB set (era 0 representation) OR same small one? */
	/* We only assert that decoding then encoding yields an NTP second whose interpreted unix time matches */
	uint64_t unix_sec_check; uint32_t unix_us_check;
	spec_ntp_to_unix(back_ntp_sec, back_ntp_frac, &unix_sec_check, &unix_us_check);
	zassert_equal(unix_sec_check, unix_sec, "Post-wrap unix seconds not preserved");
	zassert_true((int32_t)unix_us_check - (int32_t)unix_us <= 1 && (int32_t)unix_us - (int32_t)unix_us_check <= 1,
				 "Post-wrap unix fractional microseconds mismatch");
}

ZTEST(sntp, test_spec_ntp_unix_epoch_zero)
{
	uint64_t unix_sec_in = 0ULL; /* 1970-01-01 */
	uint32_t unix_us_in = 0;
	uint32_t ntp_sec, ntp_frac;
	uint64_t unix_sec_out; uint32_t unix_us_out;

	spec_unix_to_ntp(unix_sec_in, unix_us_in, &ntp_sec, &ntp_frac);
	zassert_equal(ntp_sec, NTP_UNIX_EPOCH_OFFSET, "NTP seconds for Unix epoch incorrect");
	zassert_equal(ntp_frac, 0U, "NTP fraction for zero should be zero");
	spec_ntp_to_unix(ntp_sec, ntp_frac, &unix_sec_out, &unix_us_out);
	zassert_equal(unix_sec_out, unix_sec_in, "Unix epoch seconds mismatch");
	zassert_equal(unix_us_out, unix_us_in, "Unix epoch fraction mismatch");
}

ZTEST_SUITE(sntp, NULL, NULL, NULL, NULL, NULL);
