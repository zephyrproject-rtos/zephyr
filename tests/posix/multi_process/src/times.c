/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/times.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(posix_multi_process, test_times)
{
	static const struct {
		const char *name;
		size_t offset;
	} fields[] = {
		{
			.name = "utime",
			.offset = offsetof(struct tms, tms_utime),
		},
		{
			.name = "stime",
			.offset = offsetof(struct tms, tms_stime),
		},
		{
			.name = "cutime",
			.offset = offsetof(struct tms, tms_cutime),
		},
		{
			.name = "cstime",
			.offset = offsetof(struct tms, tms_cstime),
		},
	};
	struct tms test_tms[2] = {};
	clock_t rtime[2];

	rtime[0] = times(&test_tms[0]);
	k_msleep(MSEC_PER_SEC);
	rtime[1] = times(&test_tms[1]);

	zexpect_not_equal(rtime[0], -1);
	zexpect_not_equal(rtime[1], -1);

	printk("t0: rtime: %ld utime: %ld stime: %ld cutime: %ld cstime: %ld\n", rtime[0],
	       test_tms[0].tms_utime, test_tms[0].tms_stime, test_tms[0].tms_cutime,
	       test_tms[0].tms_cstime);
	printk("t1: rtime: %ld utime: %ld stime: %ld cutime: %ld cstime: %ld\n", rtime[1],
	       test_tms[1].tms_utime, test_tms[1].tms_stime, test_tms[1].tms_cutime,
	       test_tms[1].tms_cstime);

	ARRAY_FOR_EACH(fields, i) {
		const char *name = fields[i].name;
		size_t offset = fields[i].offset;

		clock_t t0 = *(clock_t *)((uint8_t *)&test_tms[0] + offset);
		clock_t t1 = *(clock_t *)((uint8_t *)&test_tms[1] + offset);

		zexpect_true(t1 >= t0, "time moved backward for tms_%s: t0: %ld t1: %ld", name, t0,
			     t1);
	}
}
