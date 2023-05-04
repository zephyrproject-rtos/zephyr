/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include "mock_frontend.h"
#include "test_module.h"
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/cbprintf.h>

#ifndef CONFIG_LOG_BUFFER_SIZE
#define CONFIG_LOG_BUFFER_SIZE 4
#endif

#ifndef NO_BACKENDS
#define NO_BACKENDS 0
#endif

LOG_MODULE_REGISTER(test, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

#ifdef CONFIG_LOG_USE_TAGGED_ARGUMENTS
/* The extra sizeof(int) is the end of arguments tag. */
#define LOG_SIMPLE_MSG_LEN \
	ROUND_UP(sizeof(struct log_msg) + \
		 sizeof(struct cbprintf_package_hdr_ext) + \
		 sizeof(int), CBPRINTF_PACKAGE_ALIGNMENT)
#else
#define LOG_SIMPLE_MSG_LEN \
	ROUND_UP(sizeof(struct log_msg) + \
		 sizeof(struct cbprintf_package_hdr_ext), CBPRINTF_PACKAGE_ALIGNMENT)
#endif

#ifdef CONFIG_LOG_TIMESTAMP_64BIT
#define TIMESTAMP_INIT_VAL 0x100000000
#else
#define TIMESTAMP_INIT_VAL 0
#endif

#if NO_BACKENDS
static struct log_backend backend1;
static struct log_backend backend2;
#else
MOCK_LOG_BACKEND_DEFINE(backend1, false);
MOCK_LOG_BACKEND_DEFINE(backend2, false);
#endif

static log_timestamp_t stamp;
static bool in_panic;
static int16_t test_source_id;
static int16_t test2_source_id;

static log_timestamp_t timestamp_get(void)
{
	return NO_BACKENDS ? (log_timestamp_t)UINT32_MAX : stamp++;
}

/**
 * @brief Flush logs.
 *
 * If processing thread is enabled keep sleeping until there are no pending messages
 * else process logs here.
 */
static void flush_log(void)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		while (log_data_pending()) {
			k_msleep(10);
		}
		k_msleep(100);
	} else {
		while (LOG_PROCESS()) {
		}
	}
}

static void log_setup(bool backend2_enable)
{
	stamp = TIMESTAMP_INIT_VAL;
	zassert_false(in_panic, "Logger in panic state.");

	log_core_init();
	if (!IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		log_init();
	}

	zassert_equal(0, log_set_timestamp_func(timestamp_get, 0),
		      "Expects successful timestamp function setting.");

	mock_log_frontend_reset();

	test_source_id = log_source_id_get(STRINGIFY(test));
	test2_source_id = log_source_id_get(STRINGIFY(test2));

	if (NO_BACKENDS) {
		return;
	}

	mock_log_backend_reset(&backend1);
	mock_log_backend_reset(&backend2);

	log_backend_enable(&backend1, backend1.cb->ctx, LOG_LEVEL_DBG);

	if (backend2_enable) {
		log_backend_enable(&backend2, backend2.cb->ctx, LOG_LEVEL_DBG);
	} else {
		log_backend_disable(&backend2);
	}

}

/**
 * @brief Process and validate that backends got expected content.
 *
 * @param backend2_enable If true backend2 is also validated.
 * @param panic True means that logging is in panic mode, flushing is skipped.
 */
static void process_and_validate(bool backend2_enable, bool panic)
{
	if (!panic) {
		flush_log();
	}

	mock_log_frontend_validate(panic);

	if (NO_BACKENDS) {
		int cnt;

		STRUCT_SECTION_COUNT(log_backend, &cnt);
		zassert_equal(cnt, 0);
		return;
	}

	if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY)) {
		return;
	}

	mock_log_backend_validate(&backend1, panic);

	if (backend2_enable) {
		mock_log_backend_validate(&backend2, panic);
	}
}

static bool dbg_enabled(void)
{
	return IS_ENABLED(CONFIG_SAMPLE_MODULE_LOG_LEVEL_DBG) || (CONFIG_LOG_OVERRIDE_LEVEL == 4);
}

ZTEST(test_log_api, test_log_various_messages)
{
	char str[128];
	char dstr[] = "abcd";
	int8_t i = -5;
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	unsigned long long ull = 0x1122334455667799;
	long long ll = -12313213214454545;

#define TEST_MSG_0 "%lld %llu %hhd"
#define TEST_MSG_0_PREFIX "%s: %lld %llu %hhd"
#define TEST_MSG_1 "%f %d %f"

	if (dbg_enabled()) {
		/* If prefix is enabled, add function name prefix */
		if (IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG)) {
			snprintk(str, sizeof(str),
				 TEST_MSG_0_PREFIX, __func__, ll, ull, i);
		} else {
			snprintk(str, sizeof(str), TEST_MSG_0, ll, ull, i);
		}

		mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_DBG, str);
		mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
					exp_timestamp++, str);
	}

	LOG_DBG(TEST_MSG_0, ll, ull, i);

#ifdef CONFIG_FPU
	float f = -1.2356;
	double d = -1.2356;

	snprintk(str, sizeof(str), TEST_MSG_1, f, 100,  d);
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, str);
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp++, str);

	LOG_INF(TEST_MSG_1, f, 100, d);
#endif /* CONFIG_FPU */

	snprintk(str, sizeof(str), "wrn %s", dstr);
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_WRN, str);
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_WRN,
				exp_timestamp++, str);

	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_ERR, "err");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_ERR,
				exp_timestamp++, "err");

	LOG_WRN("wrn %s", dstr);
	dstr[0] = '\0';

	LOG_ERR("err");

	process_and_validate(false, false);

#undef TEST_MSG_0
#undef TEST_MSG_0_PREFIX
#undef TEST_MSG_1
}

/*
 * Test is using 2 backends and runtime filtering is enabled. After first call
 * filtering for backend2 is reduced to warning. It is expected that next INFO
 * level log message will be passed only to backend1.
 */
ZTEST(test_log_api, test_log_backend_runtime_filtering)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		ztest_test_skip();
	}

	log_setup(true);

	if (dbg_enabled()) {
		char str[128];

		/* If prefix is enabled, add function name prefix */
		if (IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG)) {
			snprintk(str, sizeof(str),
				 "%s: test", __func__);
		} else {
			snprintk(str, sizeof(str), "test");
		}

		mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_DBG, str);
		mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
				exp_timestamp, str);
		mock_log_backend_record(&backend2, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
				exp_timestamp, str);
		exp_timestamp++;
	}

	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "test");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp, "test");
	mock_log_backend_record(&backend2, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp, "test");
	exp_timestamp++;

	LOG_DBG("test");
	LOG_INF("test");

	process_and_validate(true, false);


	log_filter_set(&backend2,
			Z_LOG_LOCAL_DOMAIN_ID,
			LOG_CURRENT_MODULE_ID(),
			LOG_LEVEL_WRN);

	uint8_t data[] = {1, 2, 4, 5, 6, 8};

	/* INF logs expected only on backend1 */
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "test");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp++, "test");

	mock_log_frontend_generic_record(LOG_CURRENT_MODULE_ID(), Z_LOG_LOCAL_DOMAIN_ID,
					 LOG_LEVEL_INF, "hexdump", data, sizeof(data));
	mock_log_backend_generic_record(&backend1, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID,
					LOG_LEVEL_INF,
					exp_timestamp++, "hexdump",
					data, sizeof(data));

	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_WRN, "test2");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_WRN,
				exp_timestamp, "test2");
	mock_log_backend_record(&backend2, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_WRN,
				exp_timestamp++, "test2");

	mock_log_frontend_generic_record(LOG_CURRENT_MODULE_ID(), Z_LOG_LOCAL_DOMAIN_ID,
					 LOG_LEVEL_WRN, "hexdump", data, sizeof(data));
	mock_log_backend_generic_record(&backend1, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID,
					LOG_LEVEL_WRN,
					exp_timestamp, "hexdump",
					data, sizeof(data));
	mock_log_backend_generic_record(&backend2, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID,
					LOG_LEVEL_WRN,
					exp_timestamp++, "hexdump",
					data, sizeof(data));

	LOG_INF("test");
	LOG_HEXDUMP_INF(data, sizeof(data), "hexdump");
	LOG_WRN("test2");
	LOG_HEXDUMP_WRN(data, sizeof(data), "hexdump");

	process_and_validate(true, false);

}

static size_t get_max_hexdump(void)
{
	return CONFIG_LOG_BUFFER_SIZE - sizeof(struct log_msg_hdr);
}

#if defined(CONFIG_ARCH_POSIX)
#define STR_SIZE(s) (strlen(s) + 2 * sizeof(char))
#else
#define STR_SIZE(s) 0
#endif

static size_t get_long_hexdump(void)
{
	size_t extra_msg_sz = 0;
	size_t extra_hexdump_sz = 0;

	if (IS_ENABLED(CONFIG_LOG_USE_TAGGED_ARGUMENTS)) {
		/* First message with 2 arguments => 2 tags */
		extra_msg_sz = 2 * sizeof(int);

		/*
		 * Hexdump with an implicit "%s" and the "hexdump" string
		 * as argument => 1 tag.
		 */
		extra_hexdump_sz = sizeof(int);
	}

	return CONFIG_LOG_BUFFER_SIZE -
		/* First message */
		ROUND_UP(LOG_SIMPLE_MSG_LEN + 2 * sizeof(int) + extra_msg_sz,
			 CBPRINTF_PACKAGE_ALIGNMENT) -
		/* Hexdump message excluding data */
		ROUND_UP(LOG_SIMPLE_MSG_LEN + STR_SIZE("hexdump") + extra_hexdump_sz,
			 CBPRINTF_PACKAGE_ALIGNMENT);
}

/*
 * When LOG_MODE_OVERFLOW is enabled, logger should discard oldest messages when
 * there is no room. However, if after discarding all messages there is still no
 * room then current log is discarded.
 */
static uint8_t data[CONFIG_LOG_BUFFER_SIZE];

ZTEST(test_log_api, test_log_overflow)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		ztest_test_skip();
	}

	if (!IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW)) {
		ztest_test_skip();
	}

	for (int i = 0; i < CONFIG_LOG_BUFFER_SIZE; i++) {
		data[i] = i;
	}

	uint32_t hexdump_len = get_long_hexdump();

	/* expect first message to be dropped */
	exp_timestamp++;
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "test 100 100");
	mock_log_frontend_generic_record(LOG_CURRENT_MODULE_ID(), Z_LOG_LOCAL_DOMAIN_ID,
					 LOG_LEVEL_INF, "hexdump", data, hexdump_len);
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "test2");
	mock_log_backend_generic_record(&backend1, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
					exp_timestamp++, "hexdump",
					data, hexdump_len);
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp++, "test2");
	mock_log_backend_drop_record(&backend1, 1);

	LOG_INF("test %d %d", 100, 100);
	LOG_HEXDUMP_INF(data, hexdump_len, "hexdump");
	LOG_INF("test2");

	process_and_validate(false, false);

	log_setup(false);

	exp_timestamp = TIMESTAMP_INIT_VAL;
	hexdump_len = get_max_hexdump();
	mock_log_backend_reset(&backend1);
	mock_log_frontend_reset();

	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "test");
	mock_log_frontend_generic_record(LOG_CURRENT_MODULE_ID(), Z_LOG_LOCAL_DOMAIN_ID,
					 LOG_LEVEL_INF, "test", data, hexdump_len + 1);
	/* Log2 allocation is not destructive if request exceeds the
	 * capacity.
	 */
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp, "test");
	mock_log_backend_drop_record(&backend1, 1);

	LOG_INF("test");
	LOG_HEXDUMP_INF(data, hexdump_len + 1, "test");

	process_and_validate(false, false);

}

/*
 * Test checks if arguments are correctly processed by the logger.
 *
 * Log messages with supported number of messages are called. Test backend and
 * frontend validate number of arguments and values.
 */
#define MOCK_LOG_FRONT_BACKEND_RECORD(str) do { \
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(), \
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF, \
				exp_timestamp++, str); \
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, str); \
} while (0)

ZTEST(test_log_api, test_log_arguments)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	MOCK_LOG_FRONT_BACKEND_RECORD("test");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5");

	LOG_INF("test");
	LOG_INF("test %d %d %d", 1, 2, 3);
	LOG_INF("test %d %d %d %d", 1, 2, 3, 4);
	LOG_INF("test %d %d %d %d %d", 1, 2, 3, 4, 5);

	process_and_validate(false, false);

	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8 9");

	LOG_INF("test %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6);
	LOG_INF("test %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7);
	LOG_INF("test %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8);
	LOG_INF("test %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9);

	process_and_validate(false, false);

	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8 9 10");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8 9 10 11");

	LOG_INF("test %d %d %d %d %d %d %d %d %d %d",
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
	LOG_INF("test %d %d %d %d %d %d %d %d %d %d %d",
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);

	process_and_validate(false, false);

	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8 9 10 11 12");
	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8 9 10 11 12 13");

	LOG_INF("test %d %d %d %d %d %d %d %d %d %d %d %d",
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);

	LOG_INF("test %d %d %d %d %d %d %d %d %d %d %d %d %d",
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);

	process_and_validate(false, false);

	MOCK_LOG_FRONT_BACKEND_RECORD("test 1 2 3 4 5 6 7 8 9 10 11 12 13 14");
	LOG_INF("test %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);

	process_and_validate(false, false);
}

/* Function comes from the file which is part of test module. It is
 * expected that logs coming from it will have same source_id as current
 * module (this file).
 */
ZTEST(test_log_api, test_log_from_declared_module)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	/* See test module for log message content. */
	if (dbg_enabled()) {
		char str[128];

		/* If prefix is enabled, add function name prefix */
		if (IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG)) {
			snprintk(str, sizeof(str),
				 "%s: " TEST_DBG_MSG, "test_func");
		} else {
			snprintk(str, sizeof(str), TEST_DBG_MSG);
		}

		mock_log_frontend_record(test_source_id, LOG_LEVEL_DBG, str);
		mock_log_backend_record(&backend1, test_source_id,
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
					exp_timestamp++, str);
	}

	mock_log_frontend_record(test_source_id, LOG_LEVEL_ERR, TEST_ERR_MSG);
	mock_log_backend_record(&backend1, test_source_id,
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_ERR,
				exp_timestamp++, TEST_ERR_MSG);

	test_func();

	if (dbg_enabled()) {
		char str[128];

		/* If prefix is enabled, add function name prefix */
		if (IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG)) {
			snprintk(str, sizeof(str),
				 "%s: " TEST_INLINE_DBG_MSG, "test_inline_func");
		} else {
			snprintk(str, sizeof(str), TEST_INLINE_DBG_MSG);
		}

		mock_log_frontend_record(test_source_id, LOG_LEVEL_DBG, str);
		mock_log_backend_record(&backend1, test_source_id,
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
					exp_timestamp++, str);
	}

	mock_log_frontend_record(test_source_id, LOG_LEVEL_ERR, TEST_INLINE_ERR_MSG);
	mock_log_backend_record(&backend1, test_source_id,
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_ERR,
				exp_timestamp, TEST_INLINE_ERR_MSG);

	test_inline_func();

	process_and_validate(false, false);

}

/** Calculate how many messages will fit in the buffer. Also calculate if
 * remaining free space is size of message or not. This impacts how many messages
 * are dropped. If free space is equal to message size then when buffer is full,
 * adding new message will lead to one message drop, otherwise 2 message will
 * be dropped.
 */
static size_t get_short_msg_capacity(void)
{
	return CONFIG_LOG_BUFFER_SIZE / LOG_SIMPLE_MSG_LEN;
}

static void log_n_messages(uint32_t n_msg, uint32_t exp_dropped)
{
	printk("ex dropped:%d\n", exp_dropped);
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	stamp = TIMESTAMP_INIT_VAL;

	for (uint32_t i = 0; i < n_msg; i++) {
		mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "dummy");
		if (i >= exp_dropped) {
			mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp, "dummy");
		}
		exp_timestamp++;
		LOG_INF("dummy");
	}

	mock_log_backend_drop_record(&backend1, exp_dropped);

	process_and_validate(false, false);
	mock_log_backend_reset(&backend1);
}

/*
 * Test checks if backend receives notification about dropped messages. It
 * first blocks threads to ensure full control of log processing time and
 * then logs certain log messages, expecting dropped notification.
 *
 * With multiple cpus you may not get consistent message dropping
 * as other core may process logs. Executing on 1 cpu only.
 */
ZTEST(test_log_api_1cpu, test_log_msg_dropped_notification)
{
	log_setup(false);

	if (IS_ENABLED(CONFIG_SMP)) {
		/* With smp you may not get consistent message dropping as other
		 * core may process logs.
		 */
		ztest_test_skip();
	}

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		ztest_test_skip();
	}

	if (!IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW)) {
		ztest_test_skip();
	}

	uint32_t capacity = get_short_msg_capacity();

	log_n_messages(capacity, 0);

	/* Expect messages dropped when logger more than buffer capacity. */
	log_n_messages(capacity + 1, 1);
	log_n_messages(capacity + 2, 2);
}

/* Test checks if panic is correctly executed. On panic logger should flush all
 * messages and process logs in place (not in deferred way).
 */
ZTEST(test_log_api, test_log_panic)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_WRN, "test");
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_WRN, "test");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_WRN,
				exp_timestamp++, "test");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_WRN,
				exp_timestamp++, "test");
	LOG_WRN("test");
	LOG_WRN("test");

	/* logs should be flushed in panic */
	log_panic();

	process_and_validate(false, true);

	/* messages processed where called */
	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_WRN, "test");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_WRN,
				exp_timestamp++, "test");
	LOG_WRN("test");

	process_and_validate(false, true);
}

ZTEST(test_log_api, test_log_printk)
{
	if (!IS_ENABLED(CONFIG_LOG_PRINTK)) {
		ztest_test_skip();
	}

	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	mock_log_backend_record(&backend1, 0,
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INTERNAL_RAW_STRING,
				exp_timestamp++, "test 100");
	printk("test %d", 100);

	log_panic();

	mock_log_backend_record(&backend1, 0,
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INTERNAL_RAW_STRING,
				exp_timestamp++, "test 101");
	printk("test %d", 101);

	process_and_validate(false, true);
}

ZTEST(test_log_api, test_log_printk_vs_raw)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	mock_log_frontend_record(0, LOG_LEVEL_INTERNAL_RAW_STRING, "test 100\n");
	mock_log_backend_record(&backend1, 0,
				CONFIG_LOG_DOMAIN_ID, LOG_LEVEL_INTERNAL_RAW_STRING,
				exp_timestamp++, "test 100\n");
	LOG_PRINTK("test %d\n", 100);


	mock_log_frontend_record(1, LOG_LEVEL_INTERNAL_RAW_STRING, "test 100\n");
	mock_log_backend_record(&backend1, 1,
				CONFIG_LOG_DOMAIN_ID, LOG_LEVEL_INTERNAL_RAW_STRING,
				exp_timestamp++, "test 100\n");
	LOG_RAW("test %d\n", 100);


	process_and_validate(false, false);
}

ZTEST(test_log_api, test_log_arg_evaluation)
{
	char str[128];
#define TEST_MSG_0 "%u %u"
#define TEST_MSG_0_PREFIX "%s: %u %u"
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;
	uint32_t cnt0 = 0;
	uint32_t cnt1 = 0;
	uint32_t exp0 = 1;
	uint32_t exp1 = 1;

	log_setup(false);

	if (dbg_enabled()) {
		/* Debug message arguments are only evaluated when this level
		 * is enabled.
		 */
		exp0++;
		exp1++;
	}

	mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_INF, "0 0");
	mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
				Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_INF,
				exp_timestamp++, "0 0");
	if (dbg_enabled()) {
		/* If prefix is enabled, add function name prefix */
		if (IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG)) {
			snprintk(str, sizeof(str),
				 TEST_MSG_0_PREFIX, __func__, 1, 1);
		} else {
			snprintk(str, sizeof(str), TEST_MSG_0, 1, 1);
		}
		mock_log_frontend_record(LOG_CURRENT_MODULE_ID(), LOG_LEVEL_DBG, str);
		mock_log_backend_record(&backend1, LOG_CURRENT_MODULE_ID(),
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
					exp_timestamp++, str);
	}
	/* Arguments used for logging shall be evaluated only once. They should
	 * be evaluated also when given log level is disabled.
	 */
	LOG_INF("%u %u", cnt0++, cnt1++);
	LOG_DBG("%u %u", cnt0++, cnt1++);

	zassert_equal(cnt0, exp0, "Got:%u, Expected:%u", cnt0, exp0);
	zassert_equal(cnt1, exp1, "Got:%u, Expected:%u", cnt1, exp1);

	process_and_validate(false, false);
#undef TEST_MSG_0
#undef TEST_MSG_0_PREFIX
}

ZTEST(test_log_api, test_log_override_level)
{
	log_timestamp_t exp_timestamp = TIMESTAMP_INIT_VAL;

	log_setup(false);

	if (CONFIG_LOG_OVERRIDE_LEVEL == 4) {
		char str[128];

		/* If prefix is enabled, add function name prefix */
		if (IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG)) {
			snprintk(str, sizeof(str),
				 "%s: " TEST_DBG_MSG, "test_func");
		} else {
			snprintk(str, sizeof(str), TEST_DBG_MSG);
		}

		mock_log_frontend_record(test2_source_id, LOG_LEVEL_DBG, str);
		mock_log_backend_record(&backend1, test2_source_id,
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_DBG,
					exp_timestamp++, str);

		mock_log_frontend_record(test2_source_id, LOG_LEVEL_ERR, TEST_ERR_MSG);
		mock_log_backend_record(&backend1, test2_source_id,
					Z_LOG_LOCAL_DOMAIN_ID, LOG_LEVEL_ERR,
					exp_timestamp++, TEST_ERR_MSG);
	} else if (CONFIG_LOG_OVERRIDE_LEVEL != 0) {
		zassert_true(false, "Unexpected configuration.");
	}

	test_func2();

	process_and_validate(false, false);
}

/* Disable backends because same suite may be executed again but compiled by C++ */
static void log_api_suite_teardown(void *data)
{
	ARG_UNUSED(data);

	if (NO_BACKENDS) {
		return;
	}

	log_backend_disable(&backend1);
	log_backend_disable(&backend2);
}

static void *log_api_suite_setup(void)
{
	PRINT("Configuration:\n");
	PRINT("\t Mode: %s\n",
	      IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY) ? "Frontend only" :
	      (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? "Immediate" : "Deferred"));
	PRINT("\t Frontend: %s\n",
	      IS_ENABLED(CONFIG_LOG_FRONTEND) ? "Yes" : "No");
	PRINT("\t Runtime filtering: %s\n",
	      IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ? "yes" : "no");
	PRINT("\t Overwrite: %s\n",
	      IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW) ? "yes" : "no");
	if (NO_BACKENDS) {
		PRINT("\t No backends\n");
	}
#if __cplusplus
	PRINT("\t C++: yes\n");
#endif
	flush_log();

	return NULL;
}

static void log_api_suite_before(void *data)
{
	ARG_UNUSED(data);

	if (NO_BACKENDS) {
		return;
	}

	/* Flush logs and enable test backends. */
	flush_log();

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		mock_log_frontend_check_enable();
	}
	mock_log_backend_check_enable(&backend1);
	mock_log_backend_check_enable(&backend2);
}

static void log_api_suite_before_1cpu(void *data)
{
	ztest_simple_1cpu_before(data);
	log_api_suite_before(data);
}

static void log_api_suite_after(void *data)
{
	ARG_UNUSED(data);
	if (NO_BACKENDS) {
		return;
	}

	/* Disable testing backends after the test. Otherwise test may fail due
	 * to unexpected log message.
	 */
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		mock_log_frontend_check_disable();
	}
	mock_log_backend_check_disable(&backend1);
	mock_log_backend_check_disable(&backend2);
}

static void log_api_suite_after_1cpu(void *data)
{
	log_api_suite_after(data);
	ztest_simple_1cpu_after(data);
}

ZTEST_SUITE(test_log_api, NULL, log_api_suite_setup,
	    log_api_suite_before, log_api_suite_after, log_api_suite_teardown);

/* Suite dedicated for tests that need to run on 1 cpu only. */
ZTEST_SUITE(test_log_api_1cpu, NULL, log_api_suite_setup,
	    log_api_suite_before_1cpu, log_api_suite_after_1cpu, log_api_suite_teardown);
