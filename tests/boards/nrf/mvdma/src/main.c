/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <mvdma.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_uarte.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_mvdma.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/toolchain.h>

static struct k_sem done;
static struct k_sem done2;
static void *exp_user_data;
uint32_t t_delta;

#define SLOW_PERIPH_NODE DT_CHOSEN(zephyr_console)
#define SLOW_PERIPH_MEMORY_SECTION()						\
	COND_CODE_1(DT_NODE_HAS_PROP(SLOW_PERIPH_NODE, memory_regions),		\
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(		\
			DT_PHANDLE(SLOW_PERIPH_NODE, memory_regions)))))),	\
		())

#define BUF_LEN 128
#define REAL_BUF_LEN ROUND_UP(BUF_LEN, CONFIG_DCACHE_LINE_SIZE)

static uint8_t ram3_buffer1[REAL_BUF_LEN] SLOW_PERIPH_MEMORY_SECTION();
static uint8_t ram3_buffer2[REAL_BUF_LEN] SLOW_PERIPH_MEMORY_SECTION();
static uint8_t ram3_buffer3[REAL_BUF_LEN] SLOW_PERIPH_MEMORY_SECTION();
static uint8_t ram3_buffer4[REAL_BUF_LEN] SLOW_PERIPH_MEMORY_SECTION();
static uint8_t buffer1[512] __aligned(CONFIG_DCACHE_LINE_SIZE);
static uint8_t buffer2[512] __aligned(CONFIG_DCACHE_LINE_SIZE);
static uint8_t buffer3[512] __aligned(CONFIG_DCACHE_LINE_SIZE);

static uint32_t get_ts(void)
{
	nrf_timer_task_trigger(NRF_TIMER120, NRF_TIMER_TASK_CAPTURE0);
	return nrf_timer_cc_get(NRF_TIMER120, NRF_TIMER_CC_CHANNEL0);
}

static void mvdma_handler(void *user_data, int status)
{
	uint32_t *ts = user_data;

	if (ts) {
		*ts = get_ts();
	}
	zassert_equal(user_data, exp_user_data);
	k_sem_give(&done);
}

static void mvdma_handler2(void *user_data, int status)
{
	struct k_sem *sem = user_data;

	k_sem_give(sem);
}

#define IS_ALIGNED32(x) IS_ALIGNED(x, sizeof(uint32_t))
static void opt_memcpy(void *dst, void *src, size_t len)
{
	/* If length and addresses are word aligned do the optimized copying. */
	if (IS_ALIGNED32(len) && IS_ALIGNED32(dst) && IS_ALIGNED32(src)) {
		for (uint32_t i = 0; i < len / sizeof(uint32_t); i++) {
			((uint32_t *)dst)[i] = ((uint32_t *)src)[i];
		}
		return;
	}

	memcpy(dst, src, len);
}

static void dma_run(uint32_t *src_desc, size_t src_len,
		    uint32_t *sink_desc, size_t sink_len, bool blocking)
{
	int rv;
	uint32_t t_start, t, t_int, t_dma_setup, t_end;
	struct mvdma_jobs_desc job = {
		.source = src_desc,
		.source_desc_size = src_len,
		.sink = sink_desc,
		.sink_desc_size = sink_len,
	};
	struct mvdma_ctrl ctrl = NRF_MVDMA_CTRL_INIT(blocking ? NULL : mvdma_handler, &t_int);

	exp_user_data = &t_int;

	t_start = get_ts();
	rv = mvdma_xfer(&ctrl, &job, true);
	t_dma_setup = get_ts() - t_start - t_delta;
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);

	if (blocking) {
		while (mvdma_xfer_check(&ctrl) == -EBUSY) {
		}
	} else {
		rv = k_sem_take(&done, K_MSEC(100));
	}
	t_end = get_ts();
	t = t_end - t_start - t_delta;
	zassert_equal(rv, 0);
	TC_PRINT("DMA setup took %3.2fus\n", (double)t_dma_setup / 320);
	if (blocking) {
		TC_PRINT("DMA transfer (blocking) %3.2fus\n", (double)t / 320);
	} else {
		t_int = t_int - t_start - t_delta;
		TC_PRINT("DMA transfer (non-blocking) to IRQ:%3.2fus, to thread:%3.2f\n",
				(double)t_int / 320, (double)t_int / 320);
	}
}

static void test_memcpy(void *dst, void *src, size_t len, bool frag_dst, bool frag_src, bool blk)
{
	int rv;
	int cache_err = IS_ENABLED(CONFIG_DCACHE) ? 0 : -ENOTSUP;
	uint32_t t;

	t = get_ts();
	opt_memcpy(dst, src, len);
	t = get_ts() - t - t_delta;
	TC_PRINT("\nDMA transfer for dst:%p%s src:%p%s length:%d\n",
		 dst, frag_dst ? "(fragmented)" : "", src, frag_src ? "(fragmented)" : "",
		 len);
	TC_PRINT("CPU copy took %3.2fus\n", (double)t / 320);

	memset(dst, 0, len);
	for (size_t i = 0; i < len; i++) {
		((uint8_t *)src)[i] = (uint8_t)i;
	}

	uint32_t source_job[] = {
		NRF_MVDMA_JOB_DESC(src, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t source_job_frag[] = {
		NRF_MVDMA_JOB_DESC(src, len / 2, NRF_MVDMA_ATTR_DEFAULT, 0),
		/* empty transfer in the middle. */
		NRF_MVDMA_JOB_DESC(1/*dummy addr*/, 0, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(&((uint8_t *)src)[len / 2], len / 2, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job[] = {
		NRF_MVDMA_JOB_DESC(dst, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job_frag[] = {
		NRF_MVDMA_JOB_DESC(dst, len / 2, NRF_MVDMA_ATTR_DEFAULT, 0),
		/* empty tranwfer in the middle. */
		NRF_MVDMA_JOB_DESC(1/*dummy addr*/, 0, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(&((uint8_t *)dst)[len / 2], len / 2, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	rv = sys_cache_data_flush_range(src, len);
	zassert_equal(rv, cache_err);

	dma_run(frag_src ? source_job_frag : source_job,
		frag_src ? sizeof(source_job_frag) : sizeof(source_job),
		frag_dst ? sink_job_frag : sink_job,
		frag_dst ? sizeof(sink_job_frag) : sizeof(sink_job), blk);

	rv = sys_cache_data_invd_range(dst, len);
	zassert_equal(rv, cache_err);

	zassert_equal(memcmp(src, dst, len), 0);
}

static void test_unaligned(uint8_t *dst, uint8_t *src, size_t len,
			   size_t total_dst, size_t offset_dst)
{
	int rv;
	int cache_err = IS_ENABLED(CONFIG_DCACHE) ? 0 : -ENOTSUP;

	memset(dst, 0, total_dst);
	for (size_t i = 0; i < len; i++) {
		((uint8_t *)src)[i] = (uint8_t)i;
	}

	uint32_t source_job[] = {
		NRF_MVDMA_JOB_DESC(src, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job[] = {
		NRF_MVDMA_JOB_DESC(&dst[offset_dst], len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	struct mvdma_jobs_desc job = {
		.source = source_job,
		.source_desc_size = sizeof(source_job),
		.sink = sink_job,
		.sink_desc_size = sizeof(sink_job),
	};
	struct mvdma_ctrl ctrl = NRF_MVDMA_CTRL_INIT(mvdma_handler, NULL);

	exp_user_data = NULL;

	rv = sys_cache_data_flush_range(src, len);
	zassert_equal(rv, cache_err);
	rv = sys_cache_data_flush_range(dst, total_dst);
	zassert_equal(rv, cache_err);

	rv = mvdma_xfer(&ctrl, &job, true);
	zassert_equal(rv, 0);

	rv = k_sem_take(&done, K_MSEC(100));
	zassert_equal(rv, 0);

	rv = sys_cache_data_invd_range(dst, total_dst);
	zassert_equal(rv, cache_err);

	zassert_equal(memcmp(src, &dst[offset_dst], len), 0);
	for (size_t i = 0; i < offset_dst; i++) {
		zassert_equal(dst[i], 0);
	}
	for (size_t i = offset_dst + len; i < total_dst; i++) {
		zassert_equal(dst[i], 0);
	}
}

ZTEST(mvdma, test_copy_unaligned)
{
	uint8_t src[4] __aligned(CONFIG_DCACHE_LINE_SIZE) = {0xaa, 0xbb, 0xcc, 0xdd};
	uint8_t dst[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);

	for (int i = 1; i < 4; i++) {
		for (int j = 1; j < 4; j++) {
			test_unaligned(dst, src, i, sizeof(dst), j);
		}
	}
}

static void copy_from_slow_periph_mem(bool blocking)
{
	test_memcpy(buffer1, ram3_buffer1, BUF_LEN, false, false, blocking);
	test_memcpy(buffer1, ram3_buffer1, BUF_LEN, true, false, blocking);
	test_memcpy(buffer1, ram3_buffer1, BUF_LEN, false, true, blocking);
	test_memcpy(buffer1, ram3_buffer1, BUF_LEN, true, true, blocking);
	test_memcpy(buffer1, ram3_buffer1, 16, false, false, blocking);
}

ZTEST(mvdma, test_copy_from_slow_periph_mem_blocking)
{
	copy_from_slow_periph_mem(true);
}

ZTEST(mvdma, test_copy_from_slow_periph_mem_nonblocking)
{
	copy_from_slow_periph_mem(false);
}

static void copy_to_slow_periph_mem(bool blocking)
{
	test_memcpy(ram3_buffer1, buffer1, BUF_LEN, false, false, blocking);
	test_memcpy(ram3_buffer1, buffer1, 16, false, false, blocking);
}

ZTEST(mvdma, test_copy_to_slow_periph_mem_blocking)
{
	copy_to_slow_periph_mem(true);
}

ZTEST(mvdma, test_copy_to_slow_periph_mem_nonblocking)
{
	copy_to_slow_periph_mem(false);
}

ZTEST(mvdma, test_memory_copy)
{
	test_memcpy(buffer1, buffer2, sizeof(buffer1), false, false, true);
	test_memcpy(buffer1, buffer2, sizeof(buffer1), false, false, false);
}

static void test_memcmp(uint8_t *buf1, uint8_t *buf2, size_t len, int line)
{
	if (memcmp(buf1, buf2, len) != 0) {
		for (int i = 0; i < len; i++) {
			if (buf1[i] != buf2[i]) {
				zassert_false(true);
				return;
			}
		}
	}
}

#define APP_RAM0_REGION										\
	__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(app_ram_region)))))	\
	__aligned(32)

static void concurrent_jobs(bool blocking)
{
#define buf1_src buffer1
#define buf1_dst ram3_buffer4
#define buf2_src buffer2
#define buf2_dst buffer3
	int cache_err = IS_ENABLED(CONFIG_DCACHE) ? 0 : -ENOTSUP;
	int rv;

	memset(buf1_dst, 0, BUF_LEN);
	memset(buf2_dst, 0, BUF_LEN);
	for (size_t i = 0; i < BUF_LEN; i++) {
		buf1_src[i] = (uint8_t)i;
		buf2_src[i] = (uint8_t)i + 100;
	}
	bool buf1_src_cached = true;
	bool buf1_dst_cached = false;
	bool buf2_src_cached = true;
	bool buf2_dst_cached = true;

	static uint32_t source_job[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
		NRF_MVDMA_JOB_DESC(buf2_src, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	static uint32_t sink_job[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
		NRF_MVDMA_JOB_DESC(buf2_dst, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	static uint32_t source_job_periph_ram[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
		NRF_MVDMA_JOB_DESC(buf1_src, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	static uint32_t sink_job_periph_ram[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
		NRF_MVDMA_JOB_DESC(buf1_dst, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	struct mvdma_jobs_desc job = {
		.source = source_job_periph_ram,
		.source_desc_size = sizeof(source_job_periph_ram),
		.sink = sink_job_periph_ram,
		.sink_desc_size = sizeof(sink_job_periph_ram),
	};
	struct mvdma_jobs_desc job2 = {
		.source = source_job,
		.source_desc_size = sizeof(source_job),
		.sink = sink_job,
		.sink_desc_size = sizeof(sink_job),
	};
	static struct mvdma_ctrl ctrl;
	static struct mvdma_ctrl ctrl2;
	int rv1 = 0;
	int rv2 = 0;

	if (blocking) {
		ctrl.handler = NULL;
		ctrl2.handler = NULL;
	} else {
		ctrl.handler = mvdma_handler2;
		ctrl.user_data = &done;
		ctrl2.handler = mvdma_handler2;
		ctrl2.user_data = &done2;
	}

	k_sem_init(&done, 0, 1);
	k_sem_init(&done2, 0, 1);

	if (buf1_src_cached) {
		rv = sys_cache_data_flush_range(buf1_src, BUF_LEN);
		zassert_equal(rv, cache_err);
	}

	if (buf1_dst_cached) {
		rv = sys_cache_data_flush_range(buf1_dst, BUF_LEN);
		zassert_equal(rv, cache_err);
	}

	if (buf2_src_cached) {
		rv = sys_cache_data_flush_range(buf2_src, BUF_LEN);
		zassert_equal(rv, cache_err);
	}

	if (buf2_dst_cached) {
		rv = sys_cache_data_flush_range(buf2_dst, BUF_LEN);
		zassert_equal(rv, cache_err);
	}

	uint32_t t, t2 = 0, t3 = 0, t4 = 0, t5 = 0;

	t = get_ts();
	rv = mvdma_xfer(&ctrl, &job, true);
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);

	t4 = get_ts();

	rv = mvdma_xfer(&ctrl2, &job2, true);
	zassert_true(rv >= 0, "Unexpected rv:%d", rv);

	t5 = get_ts();

	if (blocking) {
		while ((rv1 = mvdma_xfer_check(&ctrl)) == -EBUSY) {
		}
		t2 = get_ts();
		nrf_mvdma_task_trigger(NRF_MVDMA, NRF_MVDMA_TASK_PAUSE);
		while ((rv2 = mvdma_xfer_check(&ctrl2)) == -EBUSY) {
		}
		t3 = get_ts();
	} else {
		rv = k_sem_take(&done, K_MSEC(100));
		zassert_equal(rv, 0);

		rv = k_sem_take(&done2, K_MSEC(100));
		zassert_equal(rv, 0);
	}

	if (buf1_dst_cached) {
		rv = sys_cache_data_invd_range(buf1_dst, BUF_LEN);
		zassert_equal(rv, cache_err);
	}

	if (buf2_dst_cached) {
		rv = sys_cache_data_invd_range(buf2_dst, BUF_LEN);
		zassert_equal(rv, cache_err);
	}

	TC_PRINT("%sblocking transfers t1_setup:%d t2_setup:%d t2:%d t3:%d\n",
			blocking ? "" : "non", t4 - t, t5 - t, t2 - t, t3 - t);
	TC_PRINT("buf1_src:%p buf1_dst:%p buf2_src:%p buf2_dst:%p\n",
		buf1_src, buf1_dst, buf2_src, buf2_dst);
	TC_PRINT("job1 src:%p sink:%p job2 src:%p sink:%p\n",
		source_job_periph_ram, sink_job_periph_ram, source_job, sink_job);
	test_memcmp(buf1_src, buf1_dst, BUF_LEN, __LINE__);
	test_memcmp(buf2_src, buf2_dst, BUF_LEN, __LINE__);
}

ZTEST(mvdma, test_concurrent_jobs)
{
	concurrent_jobs(true);
	concurrent_jobs(false);
}

static void concurrent_jobs_check(bool job1_blocking, bool job2_blocking, bool job3_blocking,
		bool timing)
{
	int rv;
	uint32_t ts1, ts2, ts3, t_memcpy;

	TC_PRINT("mode %s %s\n", !job1_blocking ? "job1 nonblocking" :
			!job2_blocking ? "job2 nonblocking" : !job3_blocking ? "job3 nonblocking" :
			"all blocking", timing ? "+timing measurement" : "");
	if (timing) {
		ts1 = get_ts();
		opt_memcpy(ram3_buffer1, buffer1, BUF_LEN);
		opt_memcpy(ram3_buffer2, buffer2, BUF_LEN);
		ts2 = get_ts();
		t_memcpy = ts2 - ts1 - t_delta;
		TC_PRINT("Memcpy time %d (copying %d to RAM3)\n", t_memcpy, 2 * BUF_LEN);
	}

	memset(ram3_buffer1, 0, BUF_LEN);
	memset(ram3_buffer2, 0, BUF_LEN);
	memset(ram3_buffer3, 0, BUF_LEN);
	for (size_t i = 0; i < BUF_LEN; i++) {
		buffer1[i] = (uint8_t)i;
		buffer2[i] = (uint8_t)i + 100;
		buffer3[i] = (uint8_t)i + 200;
	}
	rv = sys_cache_data_flush_range(buffer1, BUF_LEN);
	zassert_equal(rv, 0);
	rv = sys_cache_data_flush_range(buffer2, BUF_LEN);
	zassert_equal(rv, 0);
	rv = sys_cache_data_flush_range(buffer3, BUF_LEN);
	zassert_equal(rv, 0);

	uint32_t src_job1[] = {
		NRF_MVDMA_JOB_DESC(buffer1, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job1[] = {
		NRF_MVDMA_JOB_DESC(ram3_buffer1, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};

	uint32_t src_job2[] = {
		NRF_MVDMA_JOB_DESC(buffer2, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job2[] = {
		NRF_MVDMA_JOB_DESC(ram3_buffer2, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};

	uint32_t src_job3[] = {
		NRF_MVDMA_JOB_DESC(buffer3, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job3[] = {
		NRF_MVDMA_JOB_DESC(ram3_buffer3, BUF_LEN, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	struct k_sem sem;
	struct mvdma_jobs_desc job1 = {
		.source = src_job1,
		.source_desc_size = sizeof(src_job1),
		.sink = sink_job1,
		.sink_desc_size = sizeof(sink_job1),
	};
	struct mvdma_jobs_desc job2 = {
		.source = src_job2,
		.source_desc_size = sizeof(src_job2),
		.sink = sink_job2,
		.sink_desc_size = sizeof(sink_job2),
	};
	struct mvdma_jobs_desc job3 = {
		.source = src_job3,
		.source_desc_size = sizeof(src_job3),
		.sink = sink_job3,
		.sink_desc_size = sizeof(sink_job3),
	};
	struct mvdma_ctrl ctrl1;
	struct mvdma_ctrl ctrl2;
	struct mvdma_ctrl ctrl3;

	ctrl1.handler = job1_blocking ? NULL : mvdma_handler2;
	ctrl2.handler = job2_blocking ? NULL : mvdma_handler2;
	ctrl3.handler = job3_blocking ? NULL : mvdma_handler2;
	ctrl1.user_data = &sem;
	ctrl2.user_data = &sem;
	ctrl3.user_data = &sem;

	k_sem_init(&sem, 0, 1);

	ts1 = get_ts();

	rv = mvdma_xfer(&ctrl1, &job1, true);
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);

	rv = mvdma_xfer(&ctrl2, &job2, true);
	zassert_true(rv >= 0, "Unexpected rv:%d", rv);

	ts2 = get_ts();

	if (timing) {
		bool eq;

		if (job2_blocking) {
			while ((rv = mvdma_xfer_check(&ctrl2)) == -EBUSY) {
			}
			eq = buffer2[BUF_LEN - 1] ==  ram3_buffer2[BUF_LEN - 1];
			ts3 = get_ts();
		} else {
			rv = k_sem_take(&sem, K_MSEC(100));
			eq = buffer2[BUF_LEN - 1] ==  ram3_buffer2[BUF_LEN - 1];
			ts3 = get_ts();
			zassert_ok(rv);
		}
		zassert_true(eq,
			"If copying finished (%d), last byte should be there. %02x (exp:%02x)",
				rv, ram3_buffer2[BUF_LEN - 1], buffer2[BUF_LEN - 1]);
		zassert_true(job1_blocking);
		zassert_true(mvdma_xfer_check(&ctrl1) >= 0);
		TC_PRINT("Two jobs setup time: %d, from start to finish:%d (%sblocking)\n",
			ts2 - ts1 - t_delta, ts3 - ts1 - 2 * t_delta,
			job2_blocking ? "" : "non-");
	} else {
		rv = job1_blocking ? mvdma_xfer_check(&ctrl1) : k_sem_take(&sem, K_NO_WAIT);
		if (rv != -EBUSY) {

			TC_PRINT("t:%d ctrl1:%p ctrl2:%p", ts2 - ts1, ctrl1.handler, ctrl2.handler);
		}
		zassert_equal(rv, -EBUSY, "Unexpected err:%d", rv);
		rv = job2_blocking ? mvdma_xfer_check(&ctrl2) : k_sem_take(&sem, K_NO_WAIT);
		zassert_equal(rv, -EBUSY, "Unexpected err:%d", rv);

		k_busy_wait(10000);
	}

	test_memcmp(ram3_buffer1, buffer1, BUF_LEN, __LINE__);
	test_memcmp(ram3_buffer2, buffer2, BUF_LEN, __LINE__);

	rv = mvdma_xfer(&ctrl3, &job3, true);
	zassert_equal(rv, 0, "Unexpected rv:%d", rv);

	if (!timing) {
		rv = job1_blocking ? mvdma_xfer_check(&ctrl1) : k_sem_take(&sem, K_NO_WAIT);
		zassert_true(rv >= 0, "Unexpected rv:%d", rv);
		rv = job2_blocking ? mvdma_xfer_check(&ctrl2) : k_sem_take(&sem, K_NO_WAIT);
		zassert_true(rv >= 0, "Unexpected rv:%d", rv);
	}

	rv = job3_blocking ? mvdma_xfer_check(&ctrl3) : k_sem_take(&sem, K_NO_WAIT);
	zassert_equal(rv, -EBUSY);

	k_busy_wait(10000);
	rv = job3_blocking ? mvdma_xfer_check(&ctrl3) : k_sem_take(&sem, K_NO_WAIT);
	zassert_true(rv >= 0);

	test_memcmp(ram3_buffer3, buffer3, BUF_LEN, __LINE__);
}

ZTEST(mvdma, test_concurrent_jobs_check)
{
	concurrent_jobs_check(true, true, true, false);
	concurrent_jobs_check(false, true, true, false);
	concurrent_jobs_check(true, false, true, false);
	concurrent_jobs_check(true, true, false, false);

	concurrent_jobs_check(true, true, true, true);
	concurrent_jobs_check(true, false, true, true);
}

#if DT_SAME_NODE(DT_CHOSEN(zephyr_console), DT_NODELABEL(uart135))
#define p_reg NRF_UARTE135
#elif DT_SAME_NODE(DT_CHOSEN(zephyr_console), DT_NODELABEL(uart136))
#define p_reg NRF_UARTE136
#else
#error "Not supported"
#endif

static void peripheral_operation(bool blocking)
{
	static const uint8_t zero;
	static const uint32_t evt_err = (uint32_t)&p_reg->EVENTS_ERROR;
	static const uint32_t evt_rxto = (uint32_t)&p_reg->EVENTS_RXTO;
	static const uint32_t evt_endrx = (uint32_t)&p_reg->EVENTS_DMA.RX.END;
	static const uint32_t evt_rxstarted = (uint32_t)&p_reg->EVENTS_DMA.RX.READY;
	static const uint32_t evt_txstopped = (uint32_t)&p_reg->EVENTS_TXSTOPPED;
	static uint32_t evts[8] __aligned(CONFIG_DCACHE_LINE_SIZE);

	static const int len = 4;
	uint32_t source_job_periph_ram[] __aligned(CONFIG_DCACHE_LINE_SIZE)  = {
		NRF_MVDMA_JOB_DESC(evt_err, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&zero, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_endrx, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&zero, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_rxto, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&zero, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_rxstarted, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&zero, len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_txstopped, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job_periph_ram[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
		NRF_MVDMA_JOB_DESC(&evts[0], len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_err, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&evts[1], len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_endrx, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&evts[2], len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_rxto, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&evts[3], len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(evt_rxstarted, len,
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC(&evts[4], len, NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};

	TC_PRINT("Reading and clearing UARTE events (9 peripheral op).");
	dma_run(source_job_periph_ram, sizeof(source_job_periph_ram),
		sink_job_periph_ram, sizeof(sink_job_periph_ram), blocking);
	for (int i = 0; i < 8; i++) {
		TC_PRINT("evt%d:%d ", i, evts[i]);
	}
	TC_PRINT("\n");
}

ZTEST(mvdma, test_peripheral_operation_blocking)
{
	peripheral_operation(true);
}

ZTEST(mvdma, test_peripheral_operation_nonblocking)
{
	peripheral_operation(false);
}

static void mix_periph_slow_ram(bool blocking)
{
	int rv;
	uint32_t t1;
	static uint8_t tx_buffer[] __aligned(4) = "tst buf which contain 32bytes\r\n";
	static uint8_t tx_buffer_ram3[40] SLOW_PERIPH_MEMORY_SECTION();
	static uint32_t xfer_data[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
		(uint32_t)tx_buffer_ram3,
		sizeof(tx_buffer),
		1
	};
	uint32_t source_job[] __aligned(CONFIG_DCACHE_LINE_SIZE)  = {
		NRF_MVDMA_JOB_DESC(tx_buffer, sizeof(tx_buffer), NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC(xfer_data, sizeof(xfer_data), NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_TERMINATE
	};
	uint32_t sink_job[] __aligned(CONFIG_DCACHE_LINE_SIZE)  = {
		NRF_MVDMA_JOB_DESC(tx_buffer_ram3, sizeof(tx_buffer), NRF_MVDMA_ATTR_DEFAULT, 0),
		NRF_MVDMA_JOB_DESC((uint32_t)&p_reg->DMA.TX.PTR, 2 * sizeof(uint32_t),
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_DESC((uint32_t)&p_reg->TASKS_DMA.TX.START, sizeof(uint32_t),
				NRF_MVDMA_ATTR_DEFAULT, NRF_MVDMA_EXT_ATTR_PERIPH),
		NRF_MVDMA_JOB_TERMINATE
	};

	memset(tx_buffer_ram3, 'a', sizeof(tx_buffer_ram3));

	TC_PRINT("MVDMA buffer copy and transfer trigger. RAM3 buffer:%p RAM0 buffer:%p\n",
			tx_buffer_ram3, tx_buffer);
	rv = sys_cache_data_flush_range(tx_buffer, sizeof(tx_buffer));
	zassert_equal(rv, 0);
	dma_run(source_job, sizeof(source_job), sink_job, sizeof(sink_job), blocking);

	k_msleep(10);
	TC_PRINT("Manual operation test\n");
	memset(tx_buffer_ram3, 'a', sizeof(tx_buffer_ram3));
	k_msleep(10);

	t1 = get_ts();
	opt_memcpy(tx_buffer_ram3, tx_buffer, sizeof(tx_buffer));
	p_reg->DMA.TX.PTR = (uint32_t)tx_buffer_ram3;
	p_reg->DMA.TX.MAXCNT = sizeof(tx_buffer);
	p_reg->TASKS_DMA.TX.START = 1;
	t1 = get_ts() - t1 - t_delta;
	k_msleep(10);
	TC_PRINT("Manual operation took:%3.2f us\n", (double)t1 / 320);
}

ZTEST(mvdma, test_mix_periph_slow_ram_blocking)
{
	mix_periph_slow_ram(true);
}

ZTEST(mvdma, test_mix_periph_slow_ram_nonblocking)
{
	mix_periph_slow_ram(false);
}

ZTEST(mvdma, test_simple_xfer)
{
	struct mvdma_basic_desc desc __aligned(CONFIG_DCACHE_LINE_SIZE) =
		NRF_MVDMA_BASIC_MEMCPY_INIT(buffer2, buffer1, BUF_LEN);
	int rv;
	uint32_t t;

	/* Run twice to get the timing result when code is cached. Timing of the first run
	 * depends on previous test cases.
	 */
	for (int i = 0; i < 2; i++) {
		struct mvdma_ctrl ctrl = NRF_MVDMA_CTRL_INIT(NULL, NULL);

		zassert_true(IS_ALIGNED(&desc, CONFIG_DCACHE_LINE_SIZE));
		memset(buffer1, 0xaa, BUF_LEN);
		memset(buffer2, 0xbb, BUF_LEN);
		sys_cache_data_flush_range(buffer1, BUF_LEN);
		sys_cache_data_flush_range(buffer2, BUF_LEN);

		t = get_ts();
		rv = mvdma_basic_xfer(&ctrl, &desc, false);
		t = get_ts() - t;
		zassert_ok(rv);

		k_busy_wait(1000);
		rv = mvdma_xfer_check(&ctrl);
		zassert_true(rv >= 0);

		sys_cache_data_invd_range(buffer2, BUF_LEN);
		test_memcmp(buffer1, buffer2, BUF_LEN, __LINE__);
	}

	TC_PRINT("MVDMA memcpy setup (code cached) took:%d (%3.2fus)\n", t, (double)t / 320);
}

ZTEST(mvdma, test_simple_zero_fill)
{
	struct mvdma_basic_desc desc __aligned(CONFIG_DCACHE_LINE_SIZE) =
		NRF_MVDMA_BASIC_ZERO_INIT(buffer1, BUF_LEN);
	struct mvdma_ctrl ctrl = NRF_MVDMA_CTRL_INIT(NULL, NULL);
	int rv;

	memset(buffer1, 0xaa, BUF_LEN);
	sys_cache_data_flush_range(buffer1, BUF_LEN);
	rv = mvdma_basic_xfer(&ctrl, &desc, false);
	zassert_ok(rv);

	k_busy_wait(1000);
	rv = mvdma_xfer_check(&ctrl);
	zassert_true(rv >= 0);

	/* DMA shall fill the buffer with 0's. */
	sys_cache_data_invd_range(buffer1, BUF_LEN);
	for (int i = 0; i < BUF_LEN; i++) {
		zassert_equal(buffer1[i], 0);
	}
}

static void before(void *unused)
{
	uint32_t t_delta2;

	nrf_timer_bit_width_set(NRF_TIMER120, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_prescaler_set(NRF_TIMER120, 0);
	nrf_timer_task_trigger(NRF_TIMER120, NRF_TIMER_TASK_START);

	t_delta = get_ts();
	t_delta = get_ts() - t_delta;

	t_delta2 = get_ts();
	t_delta2 = get_ts() - t_delta2;

	t_delta = MIN(t_delta2, t_delta);

	nrf_gpio_cfg_output(9*32);
	nrf_gpio_cfg_output(9*32+1);
	nrf_gpio_cfg_output(9*32+2);
	nrf_gpio_cfg_output(9*32+3);
	k_sem_init(&done, 0, 1);
	k_sem_init(&done2, 0, 1);
}

ZTEST_SUITE(mvdma, NULL, NULL, before, NULL, NULL);
