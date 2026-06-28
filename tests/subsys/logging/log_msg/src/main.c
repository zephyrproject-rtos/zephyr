/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/cbprintf.h>

#if CONFIG_NO_OPTIMIZATIONS
#define EXP_MODE(name) Z_LOG_MSG_MODE_RUNTIME
#else
#define EXP_MODE(name) Z_LOG_MSG_MODE_##name
#endif

#define TEST_TIMESTAMP_INIT_VALUE \
	COND_CODE_1(CONFIG_LOG_TIMESTAMP_64BIT, (0x1234123412), (0x11223344))

static log_timestamp_t timestamp;

log_timestamp_t get_timestamp(void)
{
	return timestamp;
}

static void test_init(void)
{
	timestamp = TEST_TIMESTAMP_INIT_VALUE;
	z_log_msg_init();
	log_set_timestamp_func(get_timestamp, 0);
}

void print_msg(struct log_msg *msg)
{
	printk("-----------------------printing message--------------------\n");
	printk("message %p\n", msg);
	printk("package len: %d, data len: %d\n",
			msg->hdr.desc.package_len,
			msg->hdr.desc.data_len);
	for (int i = 0; i < msg->hdr.desc.package_len; i++) {
		printk("%02x ", msg->data[i]);
	}
	printk("\n");
	printk("source: %p\n", msg->hdr.source);
	printk("timestamp: %d\n", (int)msg->hdr.timestamp);
	printk("-------------------end of printing message-----------------\n");
}

struct test_buf {
	char *buf;
	int idx;
};

int out(int c, void *ctx)
{
	struct test_buf *t = ctx;

	t->buf[t->idx++] = c;

	return c;
}

static void basic_validate(struct log_msg *msg,
			   const struct log_source_const_data *source,
			   uint8_t domain, uint8_t level, log_timestamp_t t,
			   const void *data, size_t data_len, char *str)
{
	int rv;
	uint8_t *d;
	size_t len = 0;
	char buf[256];
	struct test_buf tbuf = { .buf = buf, .idx = 0 };

	zassert_equal(log_msg_get_source(msg), (void *)source);
	zassert_equal(log_msg_get_domain(msg), domain);
	zassert_equal(log_msg_get_level(msg), level);
	zassert_equal(log_msg_get_timestamp(msg), t);

	d = log_msg_get_data(msg, &len);
	zassert_equal(len, data_len);
	if (len) {
		rv = memcmp(d, data, data_len);
		zassert_equal(rv, 0);
	}

	d = log_msg_get_package(msg, &len);
	if (str) {
		rv = cbpprintf(out, &tbuf, d);
		zassert_true(rv > 0);
		buf[rv] = '\0';

		rv = strncmp(buf, str, sizeof(buf));
		zassert_equal(rv, 0, "expected:\n%s,\ngot:\n%s", str, buf);
	}
}

union log_msg_generic *msg_copy_and_free(union log_msg_generic *msg,
					  uint8_t *buf, size_t buf_len)
{
	size_t len = sizeof(int) *
		     log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg);

	zassert_true(len < buf_len);

	memcpy(buf, msg, len);

	z_log_msg_free(msg);

	return (union log_msg_generic *)buf;
}

void clear_pkg_flags(struct log_msg *msg)
{
#ifdef CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS
	/*
	 * The various tests create cbprintf packages differently
	 * for the same log message. This results in different
	 * package flags stored in those packages. These package
	 * flags can be ignored as we only want to make sure
	 * the remaining header bits, the format string, and
	 * the format arguments are all the same.
	 */

	uint8_t *d;
	size_t len;

	d = log_msg_get_package(msg, &len);
	if (len > 0) {
		union cbprintf_package_hdr *hdr = (void *)d;

		hdr->desc.pkg_flags = 0U;
	}
#else
	ARG_UNUSED(msg);
#endif
}

void validate_base_message_set(const struct log_source_const_data *source,
				uint8_t domain, uint8_t level,
				log_timestamp_t t, const void *data,
				size_t data_len, char *str)
{
	uint8_t __aligned(Z_LOG_MSG_ALIGNMENT) buf0[256];
	uint8_t __aligned(Z_LOG_MSG_ALIGNMENT) buf1[256];
	uint8_t __aligned(Z_LOG_MSG_ALIGNMENT) buf2[256];
	size_t len0, len1, len2;
	union log_msg_generic *msg0, *msg1, *msg2;

	msg0 = z_log_msg_claim(NULL);
	zassert_true(msg0, "Unexpected null message");
	len0 = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg0);
	msg0 = msg_copy_and_free(msg0, buf0, sizeof(buf0));
	clear_pkg_flags(&msg0->log);

	msg1 = z_log_msg_claim(NULL);
	zassert_true(msg1, "Unexpected null message");
	len1 = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg1);
	msg1 = msg_copy_and_free(msg1, buf1, sizeof(buf1));
	clear_pkg_flags(&msg1->log);

	msg2 = z_log_msg_claim(NULL);
	zassert_true(msg2, "Unexpected null message");
	len2 = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg2);
	msg2 = msg_copy_and_free(msg2, buf2, sizeof(buf2));
	clear_pkg_flags(&msg2->log);

	print_msg(&msg0->log);
	print_msg(&msg1->log);
	print_msg(&msg2->log);

	/* Messages created with static packaging should have same output.
	 * Runtime created message (msg2) may have strings copied in and thus
	 * different length.
	 */
	zassert_equal(len0, len1);

	int rv = memcmp(msg0, msg1, sizeof(int) * len0);

	zassert_equal(rv, 0, "Unexpected memcmp result: %d", rv);

	/* msg1 is not validated because it should be the same as msg0. */
	basic_validate(&msg0->log, source, domain, level,
			t, data, data_len, str);
	basic_validate(&msg2->log, source, domain, level,
			t, data, data_len, str);
}

/**
 * @brief Verify creation and read-back of a log message with no format arguments.
 *
 * @details
 * Builds the same zero-argument log message three ways (zero-copy static, from-stack
 * static, and runtime), confirms each packaging path selects the expected creation
 * mode, then claims the queued messages and validates that the structured message
 * round-trips its source, domain, level, timestamp and rendered string. This exercises
 * the structured, machine-readable log message that underpins post-processing.
 *
 * Test steps:
 * - Initialize the log message subsystem and timestamp source.
 * - Create the "0 args" message via Z_LOG_MSG_CREATE3 (zero-copy and from-stack) and runtime.
 * - Assert the creation mode for each path.
 * - Claim and validate the resulting messages against the expected fields.
 *
 * Expected result:
 * - All three paths produce equivalent messages that decode to the original fields and text.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 */
ZTEST(log_msg, test_log_msg_0_args_msg)
{
#undef TEST_MSG
#define TEST_MSG "0 args"
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;

	test_init();
	printk("Test string:%s\n", TEST_MSG);

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level,
			  NULL, 0, TEST_MSG);
	zassert_equal(mode, EXP_MODE(ZERO_COPY));

	Z_LOG_MSG_CREATE3(0, mode, 0, domain, source, level,
			  NULL, 0, TEST_MSG);
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	z_log_msg_runtime_create(domain, source,
				  level, NULL, 0, 0, TEST_MSG);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   NULL, 0, TEST_MSG);
}

/**
 * @brief Verify packing and rendering of a log message carrying various argument types.
 *
 * @details
 * Creates a log message whose format string mixes signed/unsigned chars, long long and
 * pointer arguments, across all packaging paths, then claims the messages and renders the
 * stored cbprintf package back to a string. Confirms the packed arguments reproduce the
 * reference snprintf output, validating printf-style formatting/argument packing in the
 * structured message.
 *
 * Test steps:
 * - Initialize the subsystem and define a multi-argument format string.
 * - Create the message via zero-copy, from-stack and runtime packaging.
 * - Produce a reference rendering with snprintfcb.
 * - Claim and validate the messages render to the reference string.
 *
 * Expected result:
 * - The packed message renders to exactly the printf-formatted reference output.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_msg, test_log_msg_various_args)
{
#undef TEST_MSG
#define TEST_MSG "%d %d %lld %p %lld %p"
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;
	uint8_t u = 0x45;
	signed char s8 = -5;
	long long lld = 0x12341234563412;
	char str[256];
	static const int iarray[] = {1, 2, 3, 4};

	test_init();
	printk("Test string:%s\n", TEST_MSG);

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, s8, u, lld, (void *)str, lld, (void *)iarray);
	zassert_equal(mode, EXP_MODE(ZERO_COPY));

	Z_LOG_MSG_CREATE3(0, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, s8, u, lld, (void *)str, lld, (void *)iarray);
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	z_log_msg_runtime_create(domain, (void *)source, level, NULL,
				  0, 0, TEST_MSG, s8, u, lld, str, lld, iarray);
	snprintfcb(str, sizeof(str), TEST_MSG, s8, u, lld, str, lld, iarray);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   NULL, 0, str);
}

/**
 * @brief Verify a log message that carries only a binary data payload.
 *
 * @details
 * Creates a log message with a raw byte array and no format string, across the packaging
 * paths, then claims the messages and confirms the attached data payload round-trips
 * byte-for-byte. This covers logging of binary data carried by the structured message
 * (the basis for hexdump-style output).
 *
 * Test steps:
 * - Initialize the subsystem.
 * - Create the message with a byte array payload via static and runtime paths.
 * - Claim and validate the data payload matches the original array.
 *
 * Expected result:
 * - The binary payload is preserved and read back unchanged.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-12
 */
ZTEST(log_msg, test_log_msg_only_data)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;
	static const uint8_t array[] = {1, 2, 3, 4};

	test_init();

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level, array,
			   sizeof(array));
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	Z_LOG_MSG_CREATE3(0, mode, 0, domain, source, level, array,
			   sizeof(array));
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	z_log_msg_runtime_create(domain, (void *)source, level, array,
				  sizeof(array), 0, NULL);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   array, sizeof(array), NULL);
}

/**
 * @brief Verify a log message that carries both a format string and binary data.
 *
 * @details
 * Creates a log message combining a text format string with a binary byte array, across
 * the packaging paths, then claims the messages and confirms both the rendered string and
 * the attached data payload round-trip correctly. This validates that the structured
 * message can simultaneously carry formatted text and a binary (hexdump) payload.
 *
 * Test steps:
 * - Initialize the subsystem.
 * - Create the message with both a format string and a data array via static and runtime paths.
 * - Claim and validate both the text and the data payload.
 *
 * Expected result:
 * - Both the rendered string and binary payload are read back intact.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 * @verifies ZEP-SRS-11-12
 */
ZTEST(log_msg, test_log_msg_string_and_data)
{
#undef TEST_MSG
#define TEST_MSG "test"

	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;
	static const uint8_t array[] = {1, 2, 3, 4};

	test_init();

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level, array,
			   sizeof(array), TEST_MSG);
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	Z_LOG_MSG_CREATE3(0, mode, 0, domain, source, level, array,
			   sizeof(array), TEST_MSG);
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	z_log_msg_runtime_create(domain, (void *)source, level, array,
				  sizeof(array), 0, TEST_MSG);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   array, sizeof(array), TEST_MSG);
}

/**
 * @brief Verify packing and rendering of a log message with floating-point arguments.
 *
 * @details
 * When floating-point cbprintf support is available, creates a log message whose format
 * string includes float and double arguments alongside integers and pointers, across the
 * packaging paths, then claims and renders the stored package and compares against a
 * reference snprintf rendering. Validates printf-style formatting of floating-point
 * arguments in the structured message.
 *
 * Test steps:
 * - Skip when FP/cbprintf FP support is not enabled.
 * - Create the message with mixed integer/float/double/pointer arguments.
 * - Produce a reference rendering with snprintfcb.
 * - Claim and validate the messages render to the reference string.
 *
 * Expected result:
 * - The packed floating-point arguments render to the printf-formatted reference output.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_msg, test_log_msg_fp)
{
	if (!(IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT) && IS_ENABLED(CONFIG_FPU))) {
		return;
	}

#undef TEST_MSG
#define TEST_MSG "%d %lld %f %p %f %p"

	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;
	long long lli = 0x1122334455;
	float f = 1.234f;
	double d = 11.3434;
	char str[256];
	int i = -100;

	test_init();

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, i, lli, (double)f, &i, d, source);
	zassert_equal(mode, EXP_MODE(ZERO_COPY));

	Z_LOG_MSG_CREATE3(0, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, i, lli, (double)f, &i, d, source);
	zassert_equal(mode, EXP_MODE(FROM_STACK));

	z_log_msg_runtime_create(domain, (void *)source, level, NULL, 0, 0,
				  TEST_MSG, i, lli, (double)f, &i, d, source);
	snprintfcb(str, sizeof(str), TEST_MSG, i, lli, (double)f, &i, d, source);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   NULL, 0, str);
}

static void get_msg_validate_length(uint32_t exp_len)
{
	uint32_t len;
	union log_msg_generic *msg;

	msg = z_log_msg_claim(NULL);
	len = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg);

	zassert_equal(len, exp_len, "Unexpected message length %d (exp:%d)",
			len, exp_len);

	z_log_msg_free(msg);
}

/**
 * @brief Verify message length for a plain-string log message.
 *
 * @details
 * Creates a log message holding only a plain (no-argument) string and checks that the
 * claimed message occupies the precisely computed word length (header plus package
 * header, rounded to alignment). Confirms the structured message is laid out
 * deterministically so it can be stored and post-processed reliably.
 *
 * Test steps:
 * - Create a plain-string message via zero-copy and from-stack paths and assert the mode.
 * - Compute the expected aligned word length.
 * - Claim the messages and validate their measured length.
 *
 * Expected result:
 * - The message word length matches the computed expected length.
 *
 * @see log_msg_generic_get_wlen()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 */
ZTEST(log_msg, test_mode_size_plain_string)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level, NULL, 0,
			"test str");
	zassert_equal(mode, EXP_MODE(ZERO_COPY),
			"Unexpected creation mode");

	Z_LOG_MSG_CREATE3(0, mode, 0, domain, source, level, NULL, 0,
			"test str");
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - package of plain string header + string pointer
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = offsetof(struct log_msg, data) +
			 /* package */sizeof(struct cbprintf_package_hdr_ext);

	exp_len = ROUND_UP(exp_len, Z_LOG_MSG_ALIGNMENT) / sizeof(int);
	get_msg_validate_length(exp_len);
	get_msg_validate_length(exp_len);
}

/**
 * @brief Verify message length for a data-only log message.
 *
 * @details
 * Creates a log message carrying only a binary data array and checks that the claimed
 * message occupies the computed word length (header plus data, rounded to alignment) and
 * that the presence of data forces from-stack creation. Confirms deterministic layout of
 * a structured message carrying a binary payload.
 *
 * Test steps:
 * - Create a data-only message and assert it uses the from-stack mode.
 * - Compute the expected aligned word length.
 * - Claim the message and validate its measured length.
 *
 * Expected result:
 * - The message word length matches the computed expected length.
 *
 * @see log_msg_generic_get_wlen()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-12
 */
ZTEST(log_msg, test_mode_size_data_only)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;
	/* If data is present then message is created from stack, even though
	 * _from_stack is 0.
	 */
	uint8_t data[] = {1, 2, 3};

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level,
			   data, sizeof(data));
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - data
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = offsetof(struct log_msg, data) + sizeof(data);
	exp_len = ROUND_UP(exp_len, Z_LOG_MSG_ALIGNMENT) / sizeof(int);
	get_msg_validate_length(exp_len);
}

/**
 * @brief Verify message length for a log message with a plain string and data.
 *
 * @details
 * Creates a log message combining a plain string and a binary data array and checks that
 * the claimed message occupies the computed word length (header plus data plus package
 * header, rounded to alignment). Confirms deterministic layout when a structured message
 * carries both text and a binary payload.
 *
 * Test steps:
 * - Create a string+data message and assert it uses the from-stack mode.
 * - Compute the expected aligned word length.
 * - Claim the message and validate its measured length.
 *
 * Expected result:
 * - The message word length matches the computed expected length.
 *
 * @see log_msg_generic_get_wlen()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-12
 */
ZTEST(log_msg, test_mode_size_plain_str_data)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;
	/* If data is present then message is created from stack, even though
	 * _from_stack is 0.
	 */
	uint8_t data[] = {1, 2, 3};

	Z_LOG_MSG_CREATE3(1, mode, 0, domain, source, level,
			   data, sizeof(data), "test");
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - data
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = offsetof(struct log_msg, data) + sizeof(data) +
		  /* package */sizeof(struct cbprintf_package_hdr_ext);
	exp_len = ROUND_UP(exp_len, Z_LOG_MSG_ALIGNMENT) / sizeof(int);
	get_msg_validate_length(exp_len);
}

/**
 * @brief Verify message length for a log message with a format string and one string argument.
 *
 * @details
 * Creates a log message with a format string and a single string-pointer argument and
 * checks that the claimed message occupies the computed word length (header plus package
 * header plus one pointer, rounded to alignment). Confirms deterministic packing of a
 * formatted message with string arguments.
 *
 * Test steps:
 * - Create the message accepting one string pointer via zero-copy and from-stack paths and assert the mode.
 * - Compute the expected aligned word length.
 * - Claim the messages and validate their measured length.
 *
 * Expected result:
 * - The message word length matches the computed expected length.
 *
 * @see log_msg_generic_get_wlen()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_msg, test_mode_size_str_with_strings)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;
	static const char *prefix = "prefix";

	Z_LOG_MSG_CREATE3(1, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, "test %s", prefix);
	zassert_equal(mode, EXP_MODE(ZERO_COPY),
			"Unexpected creation mode");
	Z_LOG_MSG_CREATE3(0, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, "test %s", prefix);
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - package: header + fmt pointer + pointer
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = offsetof(struct log_msg, data) +
			 /* package */sizeof(struct cbprintf_package_hdr_ext) +
				      sizeof(const char *);
	exp_len = ROUND_UP(exp_len, Z_LOG_MSG_ALIGNMENT) / sizeof(int);

	get_msg_validate_length(exp_len);
	get_msg_validate_length(exp_len);
}

/**
 * @brief Verify message length for a log message with two string arguments.
 *
 * @details
 * Creates a log message with a format string and two string-pointer arguments (one
 * accepted by reference, one copied in) and checks that the claimed message occupies the
 * computed word length accounting for the copied string. Confirms deterministic packing
 * when a formatted message embeds string arguments.
 *
 * Test steps:
 * - Create the message with two string arguments via zero-copy and from-stack paths and assert the mode.
 * - Compute the expected aligned word length including the copied string.
 * - Claim the messages and validate their measured length.
 *
 * Expected result:
 * - The message word length matches the computed expected length.
 *
 * @see log_msg_generic_get_wlen()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_msg, test_mode_size_str_with_2strings)
{
#undef TEST_STR
#define TEST_STR "%s test %s"

	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;
	static const char *prefix = "prefix";
	char suffix[] = "suffix";

	Z_LOG_MSG_CREATE3(1, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, TEST_STR, prefix, suffix);
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");
	Z_LOG_MSG_CREATE3(0, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, TEST_STR, prefix, suffix);
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - package: header + fmt pointer + 2 pointers
	 * - index location of read only string
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = offsetof(struct log_msg, data) +
			 /* package */sizeof(struct cbprintf_package_hdr_ext) +
				      2 * sizeof(const char *) + 2 + strlen(suffix);

	exp_len = ROUND_UP(exp_len, Z_LOG_MSG_ALIGNMENT) / sizeof(int);

	get_msg_validate_length(exp_len);
	get_msg_validate_length(exp_len);
}

static log_timestamp_t timestamp_get_inc(void)
{
	return timestamp++;
}

/**
 * @brief Verify message buffer saturation and message dropping when overflow is disabled.
 *
 * @details
 * With overflow mode disabled, fills the log message buffer to capacity, then confirms
 * that further messages are dropped and counted, and that the buffered messages can still
 * be claimed in order by their timestamps until the buffer is empty. This validates the
 * deferred-logging buffering behavior: messages are queued and surplus ones are dropped
 * rather than overwriting existing entries.
 *
 * Test steps:
 * - Skip when overflow mode is enabled.
 * - Fill the buffer to its computed capacity and assert no drops yet.
 * - Create additional messages and assert the dropped count increments accordingly.
 * - Claim all buffered messages in order, then assert the buffer is empty.
 *
 * Expected result:
 * - Surplus messages are dropped and counted; buffered messages are claimed in order.
 *
 * @see z_log_dropped_read_and_clear()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-6
 */
ZTEST(log_msg, test_saturate)
{
	if (IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW)) {
		return;
	}

	uint32_t exp_len =
		ROUND_UP(offsetof(struct log_msg, data) + 2 * sizeof(void *),
			 Z_LOG_MSG_ALIGNMENT);
	uint32_t exp_capacity = CONFIG_LOG_BUFFER_SIZE / exp_len;
	int mode;
	union log_msg_generic *msg;

	test_init();
	timestamp = 0;
	log_set_timestamp_func(timestamp_get_inc, 0);

	for (int i = 0; i < exp_capacity; i++) {
		Z_LOG_MSG_CREATE3(1, mode, 0, 0, (void *)1, 2, NULL, 0, "test");
	}

	zassert_equal(z_log_dropped_read_and_clear(), 0, "No dropped messages.");

	/* Message should not fit in and be dropped. */
	Z_LOG_MSG_CREATE3(1, mode, 0, 0, (void *)1, 2, NULL, 0, "test");
	Z_LOG_MSG_CREATE3(0, mode, 0, 0, (void *)1, 2, NULL, 0, "test");
	z_log_msg_runtime_create(0, (void *)1, 2, NULL, 0, 0, "test");

	zassert_equal(z_log_dropped_read_and_clear(), 3, "No dropped messages.");

	for (int i = 0; i < exp_capacity; i++) {
		msg = z_log_msg_claim(NULL);
		zassert_equal(log_msg_get_timestamp(&msg->log), i,
				"Unexpected timestamp used for message id");
		z_log_msg_free(msg);
	}

	msg = z_log_msg_claim(NULL);

	zassert_equal(msg, NULL, "Expected no pending messages");
}

/*test case main entry*/
ZTEST_SUITE(log_msg, NULL, NULL, NULL, NULL, NULL);
