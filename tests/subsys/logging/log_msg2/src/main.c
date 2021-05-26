/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <logging/log_msg2.h>
#include <logging/log_core.h>
#include <logging/log_ctrl.h>
#include <logging/log_instance.h>

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <sys/cbprintf.h>

#if defined(__sparc__) || defined(CONFIG_ARCH_POSIX)
/* On some platforms all strings are considered RW, that impacts size of the
 * package.
 */
#define TEST_LOG_MSG2_RW_STRINGS 1
#else
#define TEST_LOG_MSG2_RW_STRINGS 0
#endif

#if CONFIG_NO_OPTIMIZATIONS && CONFIG_ARM
#define EXP_MODE(name) Z_LOG_MSG2_MODE_RUNTIME
#else
#define EXP_MODE(name) Z_LOG_MSG2_MODE_##name
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
	z_log_msg2_init();
	log_set_timestamp_func(get_timestamp, 0);
}

void print_msg(struct log_msg2 *msg)
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

static void basic_validate(struct log_msg2 *msg,
			   const struct log_source_const_data *source,
			   uint8_t domain, uint8_t level, log_timestamp_t t,
			   const void *data, size_t data_len, char *str)
{
	int rv;
	uint8_t *d;
	size_t len = 0;
	char buf[256];
	struct test_buf tbuf = { .buf = buf, .idx = 0 };

	zassert_equal(log_msg2_get_source(msg), (void *)source, NULL);
	zassert_equal(log_msg2_get_domain(msg), domain, NULL);
	zassert_equal(log_msg2_get_level(msg), level, NULL);
	zassert_equal(log_msg2_get_timestamp(msg), t, NULL);

	d = log_msg2_get_data(msg, &len);
	zassert_equal(len, data_len, NULL);
	if (len) {
		rv = memcmp(d, data, data_len);
		zassert_equal(rv, 0, NULL);
	}

	d = log_msg2_get_package(msg, &len);
	if (str) {
		rv = cbpprintf(out, &tbuf, d);
		zassert_true(rv > 0, NULL);
		buf[rv] = '\0';

		rv = strncmp(buf, str, sizeof(buf));
		zassert_equal(rv, 0, "expected:\n%s,\ngot:\n%s", str, buf);
	}
}

union log_msg2_generic *msg_copy_and_free(union log_msg2_generic *msg,
					  uint8_t *buf, size_t buf_len)
{
	size_t len = sizeof(int) *
		     log_msg2_generic_get_wlen((union mpsc_pbuf_generic *)msg);

	zassert_true(len < buf_len, NULL);

	memcpy(buf, msg, len);

	z_log_msg2_free(msg);

	return (union log_msg2_generic *)buf;
}

void validate_base_message_set(const struct log_source_const_data *source,
				uint8_t domain, uint8_t level,
				log_timestamp_t t, const void *data,
				size_t data_len, char *str)
{
	uint8_t __aligned(Z_LOG_MSG2_ALIGNMENT) buf0[256];
	uint8_t __aligned(Z_LOG_MSG2_ALIGNMENT) buf1[256];
	uint8_t __aligned(Z_LOG_MSG2_ALIGNMENT) buf2[256];
	size_t len0, len1, len2;
	union log_msg2_generic *msg0, *msg1, *msg2;

	msg0 = z_log_msg2_claim();
	zassert_true(msg0, "Unexpected null message");
	len0 = log_msg2_generic_get_wlen((union mpsc_pbuf_generic *)msg0);
	msg0 = msg_copy_and_free(msg0, buf0, sizeof(buf0));

	msg1 = z_log_msg2_claim();
	zassert_true(msg1, "Unexpected null message");
	len1 = log_msg2_generic_get_wlen((union mpsc_pbuf_generic *)msg1);
	msg1 = msg_copy_and_free(msg1, buf1, sizeof(buf1));

	msg2 = z_log_msg2_claim();
	zassert_true(msg2, "Unexpected null message");
	len2 = log_msg2_generic_get_wlen((union mpsc_pbuf_generic *)msg2);
	msg2 = msg_copy_and_free(msg2, buf2, sizeof(buf2));

	print_msg(&msg0->log);
	print_msg(&msg1->log);
	print_msg(&msg2->log);

	/* Messages created with static packaging should have same output.
	 * Runtime created message (msg2) may have strings copied in and thus
	 * different length.
	 */
	zassert_equal(len0, len1, NULL);

	int rv = memcmp(msg0, msg1, sizeof(int) * len0);

	zassert_equal(rv, 0, "Unxecpted memcmp result: %d", rv);

	/* msg1 is not validated because it should be the same as msg0. */
	basic_validate(&msg0->log, source, domain, level,
			t, data, data_len, str);
	basic_validate(&msg2->log, source, domain, level,
			t, data, data_len, str);
}

void test_log_msg2_0_args_msg(void)
{
#undef TEST_MSG
#define TEST_MSG "0 args"
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;

	test_init();
	printk("Test string:%s\n", TEST_MSG);

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level,
			  NULL, 0, TEST_MSG);
	zassert_equal(mode, EXP_MODE(ZERO_COPY), NULL);

	Z_LOG_MSG2_CREATE2(0, mode, 0, domain, source, level,
			  NULL, 0, TEST_MSG);
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	z_log_msg2_runtime_create(domain, source,
				  level, NULL, 0, TEST_MSG);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   NULL, 0, TEST_MSG);
}

void test_log_msg2_various_args(void)
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

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, s8, u, lld, (void *)str, lld, (void *)iarray);
	zassert_equal(mode, EXP_MODE(ZERO_COPY), NULL);

	Z_LOG_MSG2_CREATE2(0, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, s8, u, lld, (void *)str, lld, (void *)iarray);
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	z_log_msg2_runtime_create(domain, (void *)source, level, NULL,
				  0, TEST_MSG, s8, u, lld, str, lld, iarray);
	snprintfcb(str, sizeof(str), TEST_MSG, s8, u, lld, str, lld, iarray);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   NULL, 0, str);
}

void test_log_msg2_only_data(void)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;
	static const uint8_t array[] = {1, 2, 3, 4};

	test_init();

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level, array,
			   sizeof(array));
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	Z_LOG_MSG2_CREATE2(0, mode, 0, domain, source, level, array,
			   sizeof(array));
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	z_log_msg2_runtime_create(domain, (void *)source, level, array,
				  sizeof(array), NULL);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   array, sizeof(array), NULL);
}

void test_log_msg2_string_and_data(void)
{
#undef TEST_MSG
#define TEST_MSG "test"

	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	int mode;
	static const uint8_t array[] = {1, 2, 3, 4};

	test_init();

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level, array,
			   sizeof(array), TEST_MSG);
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	Z_LOG_MSG2_CREATE2(0, mode, 0, domain, source, level, array,
			   sizeof(array), TEST_MSG);
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	z_log_msg2_runtime_create(domain, (void *)source, level, array,
				  sizeof(array), TEST_MSG);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   array, sizeof(array), TEST_MSG);
}

void test_log_msg2_fp(void)
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
	float f = 1.234;
	double d = 11.3434;
	char str[256];
	int i = -100;

	test_init();

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, i, lli, f, &i, d, source);
	zassert_equal(mode, EXP_MODE(ZERO_COPY), NULL);

	Z_LOG_MSG2_CREATE2(0, mode, 0, domain, source, level, NULL, 0,
			TEST_MSG, i, lli, f, &i, d, source);
	zassert_equal(mode, EXP_MODE(FROM_STACK), NULL);

	z_log_msg2_runtime_create(domain, (void *)source, level, NULL, 0,
				  TEST_MSG, i, lli, f, &i, d, source);
	snprintfcb(str, sizeof(str), TEST_MSG, i, lli, f, &i, d, source);

	validate_base_message_set(source, domain, level,
				   TEST_TIMESTAMP_INIT_VALUE,
				   NULL, 0, str);
}

static void get_msg_validate_length(uint32_t exp_len)
{
	uint32_t len;
	union log_msg2_generic *msg;

	msg = z_log_msg2_claim();
	len = log_msg2_generic_get_wlen((union mpsc_pbuf_generic *)msg);

	zassert_equal(len, exp_len, "Unexpected message length %d (exp:%d)",
			len, exp_len);

	z_log_msg2_free(msg);
}

void test_mode_size_plain_string(void)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level, NULL, 0,
			"test str");
	zassert_equal(mode, EXP_MODE(ZERO_COPY),
			"Unexpected creation mode");

	Z_LOG_MSG2_CREATE2(0, mode, 0, domain, source, level, NULL, 0,
			"test str");
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - package of plain string header + string pointer
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = sizeof(struct log_msg2_hdr) +
			 /* package */2 * sizeof(const char *);
	if (mode == Z_LOG_MSG2_MODE_RUNTIME && TEST_LOG_MSG2_RW_STRINGS) {
		exp_len += 2 + strlen("test str");
	}

	exp_len = ROUND_UP(exp_len, Z_LOG_MSG2_ALIGNMENT) / sizeof(int);
	get_msg_validate_length(exp_len);
	get_msg_validate_length(exp_len);
}

void test_mode_size_data_only(void)
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

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level,
			   data, sizeof(data));
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - data
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = sizeof(struct log_msg2_hdr) + sizeof(data);
	exp_len = ROUND_UP(exp_len, Z_LOG_MSG2_ALIGNMENT) / sizeof(int);
	get_msg_validate_length(exp_len);
}

void test_mode_size_plain_str_data(void)
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

	Z_LOG_MSG2_CREATE2(1, mode, 0, domain, source, level,
			   data, sizeof(data), "test");
	zassert_equal(mode, EXP_MODE(FROM_STACK),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - data
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = sizeof(struct log_msg2_hdr) + sizeof(data) +
		  /* package */2 * sizeof(char *);
	if (mode == Z_LOG_MSG2_MODE_RUNTIME && TEST_LOG_MSG2_RW_STRINGS) {
		exp_len += 2 + strlen("test str");
	}
	exp_len = ROUND_UP(exp_len, Z_LOG_MSG2_ALIGNMENT) / sizeof(int);
	get_msg_validate_length(exp_len);
}

void test_mode_size_str_with_strings(void)
{
	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;
	static const char *prefix = "prefix";

	Z_LOG_MSG2_CREATE2(1, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, "test %s", prefix);
	zassert_equal(mode, EXP_MODE(ZERO_COPY),
			"Unexpected creation mode");
	Z_LOG_MSG2_CREATE2(0, mode,
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
	exp_len = sizeof(struct log_msg2_hdr) +
			 /* package */3 * sizeof(const char *);
	exp_len = ROUND_UP(exp_len, Z_LOG_MSG2_ALIGNMENT) / sizeof(int);

	get_msg_validate_length(exp_len);
	get_msg_validate_length(exp_len);
}

void test_mode_size_str_with_2strings(void)
{
#undef TEST_STR
#define TEST_STR "%s test %s"

	static const uint8_t domain = 3;
	static const uint8_t level = 2;
	const void *source = (const void *)123;
	uint32_t exp_len;
	int mode;
	static const char *prefix = "prefix";

	Z_LOG_MSG2_CREATE2(1, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, TEST_STR, prefix, "sufix");
	zassert_equal(mode, EXP_MODE(RUNTIME),
			"Unexpected creation mode");
	Z_LOG_MSG2_CREATE2(0, mode,
			   1 /* accept one string pointer*/,
			   domain, source, level,
			   NULL, 0, TEST_STR, prefix, "sufix");
	zassert_equal(mode, EXP_MODE(RUNTIME),
			"Unexpected creation mode");

	/* Calculate expected message length. Message consists of:
	 * - header
	 * - package: header + fmt pointer + 2 pointers (on some platforms
	 *   strings are included in the package)
	 *
	 * Message size is rounded up to the required alignment.
	 */
	exp_len = sizeof(struct log_msg2_hdr) +
			 /* package */4 * sizeof(const char *);
	if (TEST_LOG_MSG2_RW_STRINGS) {
		exp_len += strlen("sufix") + 2 /* null + header */ +
			  strlen(prefix) + 2 /* null + header */+
			  strlen(TEST_STR) + 2 /* null + header */;
	}

	exp_len = ROUND_UP(exp_len, Z_LOG_MSG2_ALIGNMENT) / sizeof(int);

	get_msg_validate_length(exp_len);
	get_msg_validate_length(exp_len);
}

static log_timestamp_t timestamp_get_inc(void)
{
	return timestamp++;
}

void test_saturate(void)
{
	if (IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW)) {
		return;
	}

	uint32_t exp_len =
		ROUND_UP(sizeof(struct log_msg2_hdr) + 2 * sizeof(void *),
			 Z_LOG_MSG2_ALIGNMENT);
	uint32_t exp_capacity = (CONFIG_LOG_BUFFER_SIZE - 1) / exp_len;
	int mode;
	union log_msg2_generic *msg;

	test_init();
	timestamp = 0;
	log_set_timestamp_func(timestamp_get_inc, 0);

	for (int i = 0; i < exp_capacity; i++) {
		Z_LOG_MSG2_CREATE2(1, mode, 0, 0, (void *)1, 2, NULL, 0, "test");
	}

	zassert_equal(z_log_dropped_read_and_clear(), 0, "No dropped messages.");

	/* Message should not fit in and be dropped. */
	Z_LOG_MSG2_CREATE2(1, mode, 0, 0, (void *)1, 2, NULL, 0, "test");
	Z_LOG_MSG2_CREATE2(0, mode, 0, 0, (void *)1, 2, NULL, 0, "test");
	z_log_msg2_runtime_create(0, (void *)1, 2, NULL, 0, "test");

	zassert_equal(z_log_dropped_read_and_clear(), 3, "No dropped messages.");

	for (int i = 0; i < exp_capacity; i++) {
		msg = z_log_msg2_claim();
		zassert_equal(log_msg2_get_timestamp(&msg->log), i,
				"Unexpected timestamp used for message id");
	}

	msg = z_log_msg2_claim();
	zassert_equal(msg, NULL, "Expected no pending messages");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_msg2,
		ztest_unit_test(test_log_msg2_0_args_msg),
		ztest_unit_test(test_log_msg2_various_args),
		ztest_unit_test(test_log_msg2_only_data),
		ztest_unit_test(test_log_msg2_string_and_data),
		ztest_unit_test(test_log_msg2_fp),
		ztest_unit_test(test_mode_size_plain_string),
		ztest_unit_test(test_mode_size_data_only),
		ztest_unit_test(test_mode_size_plain_str_data),
		ztest_unit_test(test_mode_size_str_with_2strings),
		ztest_unit_test(test_saturate)
		);
	ztest_run_test_suite(test_log_msg2);
}
