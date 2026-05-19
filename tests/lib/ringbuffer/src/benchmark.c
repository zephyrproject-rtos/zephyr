#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>

#define BENCH_BUF_SIZE 128
#define BENCH_OP_SIZE  16

static uint8_t bench_storage[BENCH_BUF_SIZE];
static struct ring_buf bench_rb;
static uint8_t bench_payload[BENCH_OP_SIZE];
static uint8_t *bench_ptr;

static void suite_setup(void)
{
	ring_buf_init(&bench_rb, BENCH_BUF_SIZE, bench_storage);
}

ZTEST_BENCHMARK_SUITE(ring_buffer, suite_setup, NULL);

static void setup_empty(void)
{
	ring_buf_reset(&bench_rb);
}

static void setup_full(void)
{
	ring_buf_reset(&bench_rb);
	ring_buf_put(&bench_rb, bench_payload, BENCH_OP_SIZE);
}

static void setup_pending_put_ptr(void)
{
	setup_empty();
	ring_buf_put_ptr(&bench_rb, &bench_ptr);
}

static void setup_pending_get_ptr(void)
{
	setup_full();
	ring_buf_get_ptr(&bench_rb, &bench_ptr);
}

static void advance_heads(uint32_t bytes)
{
	while (bytes) {
		uint32_t step = MIN(bytes, BENCH_OP_SIZE);

		ring_buf_put(&bench_rb, bench_payload, step);
		ring_buf_get(&bench_rb, bench_payload, step);
		bytes -= step;
	}
}

static void setup_pre_wrap_empty(void)
{
	setup_empty();
	advance_heads(BENCH_BUF_SIZE - BENCH_OP_SIZE / 2);
}

static void setup_pre_wrap_full(void)
{
	setup_pre_wrap_empty();
	ring_buf_put(&bench_rb, bench_payload, BENCH_OP_SIZE);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, put, 100, setup_empty, NULL)
{
	ring_buf_put(&bench_rb, bench_payload, BENCH_OP_SIZE);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, get, 100, setup_full, NULL)
{
	ring_buf_get(&bench_rb, bench_payload, BENCH_OP_SIZE);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, put_ptr, 100, setup_empty, NULL)
{
	ring_buf_put_ptr(&bench_rb, &bench_ptr);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, commit, 100, setup_pending_put_ptr, NULL)
{
	ring_buf_commit(&bench_rb, BENCH_OP_SIZE);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, get_ptr, 100, setup_full, NULL)
{
	ring_buf_get_ptr(&bench_rb, &bench_ptr);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, consume, 100, setup_pending_get_ptr, NULL)
{
	ring_buf_consume(&bench_rb, BENCH_OP_SIZE);
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, put_ptr_commit_wrap, 100, setup_pre_wrap_empty, NULL)
{
	uint8_t *p;
	uint32_t off = 0;

	while (off < BENCH_OP_SIZE) {
		uint32_t n = MIN(ring_buf_put_ptr(&bench_rb, &p), BENCH_OP_SIZE - off);

		memcpy(p, bench_payload + off, n);
		ring_buf_commit(&bench_rb, n);
		off += n;
	}
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(ring_buffer, get_ptr_consume_wrap, 100, setup_pre_wrap_full, NULL)
{
	uint8_t out[BENCH_OP_SIZE];
	uint8_t *p;
	uint32_t off = 0;

	while (off < BENCH_OP_SIZE) {
		uint32_t n = MIN(ring_buf_get_ptr(&bench_rb, &p), BENCH_OP_SIZE - off);

		memcpy(out + off, p, n);
		ring_buf_consume(&bench_rb, n);
		off += n;
	}
}
