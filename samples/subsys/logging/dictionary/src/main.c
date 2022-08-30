/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <zephyr/zephyr.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hello_world, 4);

static const char *hexdump_msg = "HEXDUMP! HEXDUMP@ HEXDUMP#";

void main(void)
{
	int8_t i8 = 1;
	uint8_t u8 = 2;
	int16_t i16 = 16;
	uint16_t u16 = 17;
	int32_t i32 = 32;
	uint32_t u32 = 33;
	int64_t i64 = 64;
	uint64_t u64 = 65;
	char c = '!';
	char *s = "static str";
	char *s1 = "c str";
	char vs0[32];
	char vs1[32];
	void *p = s;

	printk("Hello World! %s\n", CONFIG_BOARD);

	LOG_ERR("error string");
	LOG_DBG("debug string");
	LOG_INF("info string");

	LOG_DBG("int8_t %" PRId8 ", uint8_t %" PRIu8, i8, u8);
	LOG_DBG("int16_t %" PRId16 ", uint16_t %" PRIu16, i16, u16);
	LOG_DBG("int32_t %" PRId32 ", uint32_t %" PRIu32, i32, u32);
	LOG_DBG("int64_t %" PRId64 ", uint64_t %" PRIu64, i64, u64);

	memset(vs0, 0, sizeof(vs0));
	snprintk(&vs0[0], sizeof(vs0), "%s", "dynamic str");

	memset(vs1, 0, sizeof(vs1));
	snprintk(&vs1[0], sizeof(vs1), "%s", "another dynamic str");

	LOG_DBG("char %c", c);
	LOG_DBG("s str %s %s", s, s1);
	LOG_DBG("d str %s", vs0);
	LOG_DBG("mixed str %s %s %s %s %s %s %s", vs0, "---", vs0, "---", vs1, "---", vs1);
	LOG_DBG("mixed c/s %c %s %s %s %c", c, s, vs0, s, c);

	LOG_DBG("pointer %p", p);

	LOG_HEXDUMP_DBG(hexdump_msg, strlen(hexdump_msg), "For HeXdUmP!");

#ifdef CONFIG_FPU
	float f = 66.67;
	double d = 68.69;

	LOG_DBG("float %f, double %f", (double)f, d);
#ifdef CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE
	long double ld = 70.71;

	LOG_DBG("long double %Lf", ld);
#endif
#endif
}
