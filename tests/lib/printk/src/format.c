/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/printk.h>
#include <zephyr/tc_capture.h>

#define BUF_SZ 1024

#if defined(CONFIG_PICOLIBC)

#define ZEPHYR_PICOLIBC_VERSION (__PICOLIBC__ * 10000 + \
				 __PICOLIBC_MINOR__ * 100 + \
				 __PICOLIBC_PATCHLEVEL__)

#ifdef CONFIG_PICOLIBC_IO_MINIMAL
/*
 * If picolibc is >= 1.8.4, then minimal printf is available. Otherwise,
 * we're going to get the floating point version when the minimal one is
 * selected.
 */
#if ZEPHYR_PICOLIBC_VERSION >= 10804
#define HAS_PICOLIBC_IO_MINIMAL
#else
#define HAS_PICOLIBC_IO_FLOAT
#endif
#endif

#ifdef CONFIG_PICOLIBC_IO_LONG_LONG
/*
 * If picolibc is >= 1.8.5, then long long printf is available. Otherwise,
 * we're going to get the floating point version when the long long one is
 * selected.
 */
#if ZEPHYR_PICOLIBC_VERSION >= 10805
#define HAS_PICOLIBC_IO_LONG_LONG
#else
#define HAS_PICOLIBC_IO_FLOAT
#endif
#endif

#ifdef CONFIG_PICOLIBC_IO_FLOAT
#define HAS_PICOLIBC_IO_FLOAT
#endif

/*
 * Picolibc long long support is present if Zephyr configuration has
 * enabled long long or floating point support.
 */

static char expected_32[] = "22 113 10000 32768 40000 22\n"
	"p 112 -10000 -32768 -40000 -22\n"
#if defined(HAS_PICOLIBC_IO_MINIMAL)
	"0x1 0x1 0x1 0x1 0x1\n"
	"0x1 0x1 0x1 0x1\n"
	"42 42 42 42\n"
	"-42 -42 -42 -42\n"
	"42 42 42 42\n"
	"42 42 42 42\n"
	"25542abcdef  42\n"
#if defined(_WANT_MINIMAL_IO_LONG_LONG) || defined(__IO_MINIMAL_LONG_LONG)
	"68719476735 -1 18446744073709551615 ffffffffffffffff\n"
#else
	"-1 -1 4294967295 ffffffff\n"
#endif
#else
	"0x1 0x01 0x0001 0x00000001 0x0000000000000001\n"
	"0x1 0x 1 0x   1 0x       1\n"
	"42 42 0042 00000042\n"
	"-42 -42 -042 -0000042\n"
	"42 42   42       42\n"
	"42 42 0042 00000042\n"
	"255     42    abcdef        42\n"
#if defined(HAS_PICOLIBC_IO_LONG_LONG) || defined(HAS_PICOLIBC_IO_FLOAT)
	"68719476735 -1 18446744073709551615 ffffffffffffffff\n"
#else
	"-1 -1 4294967295 ffffffff\n"
#endif
#endif
	"0xcafebabe 0xbeef 0x2a\n"
;

static char expected_64[] = "22 113 10000 32768 40000 22\n"
	"p 112 -10000 -32768 -40000 -22\n"
#if defined(HAS_PICOLIBC_IO_MINIMAL)
	"0x1 0x1 0x1 0x1 0x1\n"
	"0x1 0x1 0x1 0x1\n"
	"42 42 42 42\n"
	"-42 -42 -42 -42\n"
	"42 42 42 42\n"
	"42 42 42 42\n"
	"25542abcdef  42\n"
#else
	"0x1 0x01 0x0001 0x00000001 0x0000000000000001\n"
	"0x1 0x 1 0x   1 0x       1\n"
	"42 42 0042 00000042\n"
	"-42 -42 -042 -0000042\n"
	"42 42   42       42\n"
	"42 42 0042 00000042\n"
	"255     42    abcdef        42\n"
#endif
	"68719476735 -1 18446744073709551615 ffffffffffffffff\n"
	"0xcafebabe 0xbeef 0x2a\n"
;
static char *expected = (sizeof(long) == sizeof(long long)) ? expected_64 : expected_32;
#else
#if defined(CONFIG_CBPRINTF_FULL_INTEGRAL)
static char *expected = "22 113 10000 32768 40000 22\n"
			"p 112 -10000 -32768 -40000 -22\n"
			"0x1 0x01 0x0001 0x00000001 0x0000000000000001\n"
			"0x1 0x 1 0x   1 0x       1\n"
			"42 42 0042 00000042\n"
			"-42 -42 -042 -0000042\n"
			"42 42   42       42\n"
			"42 42 0042 00000042\n"
			"255     42    abcdef        42\n"
			"68719476735 -1 18446744073709551615 ffffffffffffffff\n"
			"0xcafebabe 0xbeef 0x2a\n"
;
#elif defined(CONFIG_CBPRINTF_COMPLETE)
static char *expected = "22 113 10000 32768 40000 %llu\n"
			"p 112 -10000 -32768 -40000 %lld\n"
			"0x1 0x01 0x0001 0x00000001 0x0000000000000001\n"
			"0x1 0x 1 0x   1 0x       1\n"
			"42 42 0042 00000042\n"
			"-42 -42 -042 -0000042\n"
			"42 42   42       42\n"
			"42 42 0042 00000042\n"
			"255     42    abcdef        42\n"
			"%lld %lld %llu %llx\n"
			"0xcafebabe 0xbeef 0x2a\n"
;
#elif defined(CONFIG_CBPRINTF_NANO)
static char *expected = "22 113 10000 32768 40000 22\n"
			"p 112 -10000 -32768 -40000 -22\n"
			"0x1 0x01 0x0001 0x00000001 0x0000000000000001\n"
			"0x1 0x 1 0x   1 0x       1\n"
			"42 42 0042 00000042\n"
			"-42 -42 -042 -0000042\n"
			"42 42   42       42\n"
			"42 42 0042 00000042\n"
			"255     42    abcdef        42\n"
			"ERR -1 ERR ERR\n"
			"0xcafebabe 0xbeef 0x2a\n"
;
#endif
#endif

static size_t stv = 22;
static unsigned char uc = 'q';
static unsigned short int usi = 10000U;
static unsigned int ui = 32768U;
static unsigned long ul = 40000;

/* FIXME
 * we know printk doesn't have full support for 64-bit values.
 * at least show it can print uint64_t values less than 32-bits wide
 */
static unsigned long long ull = 22;

static char c = 'p';
static signed short int ssi = -10000;
static signed int si = -32768;
static signed long sl = -40000;
static signed long long sll = -22;

static uint32_t hex = 0xCAFEBABE;

static void *ptr = (void *)0xBEEF;

/**
 * @addtogroup lib_printk_tests
 * @{
 */

/**
 * @brief Verify printk() formats a wide range of conversions correctly.
 *
 * @details
 * Validates that the console output path produces byte-for-byte the
 * expected text for integer width/sign/padding specifiers, hexadecimal,
 * pointers and 64-bit values. The expected output accounts for the
 * configured formatting engine (cbprintf complete/nano/full-integral or
 * the picolibc printf variants).
 *
 * Test steps:
 * - Clear the capture buffer.
 * - Emit a fixed series of printk() format strings covering
 *   size/char/short/int/long/long-long, hex, width, zero/space padding,
 *   left-justify and pointer conversions.
 * - Compare the captured output to the expected string.
 *
 * Expected result:
 * - The captured printk output equals the expected string.
 *
 * @see printk()
 */
ZTEST(lib_printk, test_printk_format_specifiers)
{
	static char captured[BUF_SZ];

	tc_capture_clear();

	printk("%zu %hhu %hu %u %lu %llu\n", stv, uc, usi, ui, ul, ull);
	printk("%c %hhd %hd %d %ld %lld\n", c, c, ssi, si, sl, sll);
	printk("0x%x 0x%02x 0x%04x 0x%08x 0x%016x\n", 1, 1, 1, 1, 1);
	printk("0x%x 0x%2x 0x%4x 0x%8x\n", 1, 1, 1, 1);
	printk("%d %02d %04d %08d\n", 42, 42, 42, 42);
	printk("%d %02d %04d %08d\n", -42, -42, -42, -42);
	printk("%u %2u %4u %8u\n", 42, 42, 42, 42);
	printk("%u %02u %04u %08u\n", 42, 42, 42, 42);
	printk("%-8u%-6d%-4x  %8d\n", 0xFF, 42, 0xABCDEF, 42);
	printk("%lld %lld %llu %llx\n", 0xFFFFFFFFFULL, -1LL, -1ULL, -1ULL);
	printk("0x%x %p %-2p\n", hex, ptr, (char *)42);

	(void)tc_capture_get(captured, sizeof(captured));
	zassert_str_equal(captured, expected, "printk failed");
}

/**
 * @brief Verify snprintk() formats a wide range of conversions correctly.
 *
 * @details
 * Renders the same conversion series as the printk() formatting test
 * into a buffer with snprintk() and verifies the result matches the
 * expected string, proving the buffered formatting entry point behaves
 * identically to the console output path.
 *
 * Test steps:
 * - Render a fixed series of format strings into a buffer with
 *   snprintk(), accumulating the returned lengths.
 * - Compare the buffer contents to the expected string.
 *
 * Expected result:
 * - The snprintk() output equals the expected string.
 *
 * @see snprintk()
 */
ZTEST(lib_printk, test_printk_snprintk_format_specifiers)
{
	static char buf[BUF_SZ];
	int count = 0;

	count += snprintk(buf + count, sizeof(buf) - count,
			  "%zu %hhu %hu %u %lu %llu\n",
			  stv, uc, usi, ui, ul, ull);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%c %hhd %hd %d %ld %lld\n", c, c, ssi, si, sl, sll);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "0x%x 0x%02x 0x%04x 0x%08x 0x%016x\n", 1, 1, 1, 1, 1);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "0x%x 0x%2x 0x%4x 0x%8x\n", 1, 1, 1, 1);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%d %02d %04d %08d\n", 42, 42, 42, 42);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%d %02d %04d %08d\n", -42, -42, -42, -42);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%u %2u %4u %8u\n", 42, 42, 42, 42);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%u %02u %04u %08u\n", 42, 42, 42, 42);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%-8u%-6d%-4x  %8d\n",
			  0xFF, 42, 0xABCDEF, 42);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "%lld %lld %llu %llx\n",
			  0xFFFFFFFFFULL, -1LL, -1ULL, -1ULL);
	count += snprintk(buf + count, sizeof(buf) - count,
			  "0x%x %p %-2p\n", hex, ptr, (char *)42);
	buf[count] = '\0';
	zassert_str_equal(buf, expected, "snprintk failed");
}

/**
 * @}
 */
