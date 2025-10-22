/*
 * Copyright (c) 2025 Arm Limited (or its affiliates).
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM Neon/SVE2 SIMD context switch stress test
 *
 * This test validates the correctness and resilience of SIMD register
 * save/restore during frequent thread preemptions in Zephyr RTOS.
 *
 * The test performs mixed workloads (F32 matrix x vector multiplication and
 * complex number multiplication) across multiple threads, while a
 * high-priority preemptor thread intentionally clobbers SIMD registers to
 * simulate context-switch interference.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <tracing_user.h>

#include <stdio.h>
#include <math.h>
#include <float.h>

/* use asm reference variant to prevent compiler vectorization */
#define USE_ASM_SCALAR_AS_REF 1

#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#endif
#include <arm_neon.h>

/* amount of SIMD threads */
#define SIMD_THREAD_CNT		8

/* thread stack sizes */
#define WORKER_STACK		4096
#define PREEMPTOR_STACK		4096

#define SIMD_TASK_PRIO		2	/* priority for SIMD test threads */
#define PREEMPTOR_PRIO		0	/* higher priority to force preemptions */

#define F32REL_THRSH		1e-1f	/* SIMD test precision tolerance */

/* test duration = max high priority iterations */
#define TEST_DURATION_MSEC	10000

/* periodicity of context switches statistics dumps inside the high prio thread*/
#define STAT_UPD_PERIOD_MSEC	2501


/* matrix by vector mult fixed size */
#define TEST_MATRIX_ROWS	64
#define TEST_MATRIX_COLS	127

/* complx vector mult fixed size */
#define TEST_CMPLX_MULT_SZ	2048

/* 50% matrix mult / 50% complex mult threads */
#define MATRIX_TEST_CTX		(SIMD_THREAD_CNT/2)
#define CMPLX_MUL_TEST_CTX	(SIMD_THREAD_CNT/2)

/* SIMD tests storage */
typedef float float32_t;
typedef struct cfloat32_t {
	float32_t re, im;
} cfloat32_t;

__aligned(16) float32_t test_matrix[MATRIX_TEST_CTX][TEST_MATRIX_ROWS * TEST_MATRIX_COLS];
__aligned(16) float32_t test_vector[MATRIX_TEST_CTX][TEST_MATRIX_COLS];

__aligned(16) float32_t test_out_simd[MATRIX_TEST_CTX][TEST_MATRIX_COLS];
__aligned(16) float32_t test_out_scalar[MATRIX_TEST_CTX][TEST_MATRIX_COLS];

__aligned(16) cfloat32_t test_cmplx_mult_a[CMPLX_MUL_TEST_CTX][TEST_CMPLX_MULT_SZ];
__aligned(16) cfloat32_t test_cmplx_mult_b[CMPLX_MUL_TEST_CTX][TEST_CMPLX_MULT_SZ];

__aligned(16) cfloat32_t test_cmplx_mult_out_simd[CMPLX_MUL_TEST_CTX][TEST_CMPLX_MULT_SZ];
__aligned(16) cfloat32_t test_cmplx_mult_out_scalar[CMPLX_MUL_TEST_CTX][TEST_CMPLX_MULT_SZ];

#define dump_buf(a, buf_sz, wrap, format)				\
{									\
	printf("%s:\n", #a);						\
	for (int i = 0; i < buf_sz; i++) {				\
		printf(i % wrap == wrap - 1 ? format",\n" : format", ",	\
		   (double)a[i]);					\
	}								\
	printf("\n");							\
}

#define F32_MIN		(-FLT_MAX)

__attribute__ ((noinline))
float32_t max_abs_diff_f32(const float32_t *src_A, const float32_t *src_B,
			   uint32_t block_size)
{
	uint32_t blk_cnt = block_size >> 2;  /* Process 4 elements per loop */
	uint32_t rem_cnt = block_size & 3;   /* Remaining elements */
	float32x4_t max_vec = vdupq_n_f32(-FLT_MAX);

	const float32_t *p_A = src_A;
	const float32_t *p_B = src_B;

	while (blk_cnt > 0) {
		float32x4_t in_vec_A = vld1q_f32(p_A);
		float32x4_t in_vec_B = vld1q_f32(p_B);

		in_vec_A = vabsq_f32(in_vec_A - in_vec_B);
		max_vec = vmaxq_f32(max_vec, in_vec_A);
		p_A += 4;
		p_B += 4;
		blk_cnt--;
	}

	/* Reduce 4-wide vector to one float */
	float32x2_t tmp = vpmax_f32(vget_low_f32(max_vec), vget_high_f32(max_vec));
	float32_t max_value = fmaxf(vget_lane_f32(tmp, 0), vget_lane_f32(tmp, 1));

	/* Handle remaining elements (less than 4) */
	while (rem_cnt > 0) {
		float32_t new_val = fabsf(*p_A++ - *p_B++);

		if (new_val > max_value) {
			max_value = new_val;
		}
		rem_cnt--;
	}

	return max_value;
}

/**
 * verify reference vs SIMD out within range tolerance
 */

bool vec_within_threshold_f32(const float32_t *__restrict ref,
			      const float32_t *__restrict vec2,
			      int length, float32_t threshold)
{
	return (max_abs_diff_f32(ref, vec2, length) <= threshold);
}

/**
 * f32_mat_x_vec_ref
 * Reference row-major matrixvector multiply:
 *   y[i] = sum_j A[i*m + j] * x[j], for i in [0..n)
 * Arguments:
 *   A  : n×m matrix (row-major)
 *   x  : length m vector
 *   y  : length n output vector
 *   n,m: sizes in rows/cols
 * Scalar reference used for correctness checks.
 */

__attribute__ ((noinline))
void f32_mat_x_vec_ref(const float *__restrict A,
		       const float *__restrict x, float *__restrict y,
		       uint64_t n, uint64_t m)
{
	for (uint64_t i = 0; i < n; i++) {
		float sum = 0;
#ifdef USE_ASM_SCALAR_AS_REF
		const float32_t *p_M = &A[i * m];
		const float32_t *p_V = x;
		uint64_t cnt = m;

		__asm__ volatile (
			"1:	ldr     s0, [%[p_M]], #4\n"
			"	ldr     s1, [%[p_V]], #4\n"
			"	fmadd   %s[sum], s0, s1, %s[sum]\n"
			"	subs    %[cnt], %[cnt], #1\n"
			"	b.ne    1b\n"
			: [p_M] "+&r"(p_M), [p_V] "+&r"(p_V),
			  [sum] "+w"(sum), [cnt] "+&r"(cnt)
			:
			: "s0", "s1", "memory", "cc");
#else
		for (uint64_t j = 0; j < m; ++j) {
			sum += A[i * m + j] * x[j];
		}
#endif
		y[i] = sum;
	}
}

/**
 * f32_mat_x_vec_simd
 * SIMD-accelerated matrix vector multiply.
 */

#if defined(__ARM_FEATURE_SVE)

__attribute__ ((noinline))
void f32_mat_x_vec_simd(const float *A, const float *x, float *y,
			uint64_t rows, uint64_t cols)
{
	for (uint64_t i = 0; i < rows; i++) {
		svfloat32_t acc = svdup_f32(0.0f);

		for (uint64_t j = 0; j < cols; j += svcntw()) {
			svbool_t pg = svwhilelt_b32(j, cols);
			svfloat32_t a_vec = svld1(pg, &A[i * cols + j]);
			svfloat32_t x_vec = svld1(pg, &x[j]);

			acc = svmla_m(pg, acc, a_vec, x_vec);
		}

		/* Reduce across vector lanes into a scalar */
		float sum = svaddv(svptrue_b32(), acc);

		y[i] = sum;
	}
}

#else

__attribute__ ((noinline))
void f32_mat_x_vec_simd(const float *__restrict A, const float *__restrict x,
			float *__restrict y, uint64_t n, uint64_t m)
{
	for (uint64_t i = 0; i < n; i++) {
		float32x4_t acc = vdupq_n_f32(0.0f);
		uint64_t j = 0;

		for (; j + 4 <= m; j += 4) {
			float32x4_t a = vld1q_f32(&A[i * m + j]);
			float32x4_t xv = vld1q_f32(&x[j]);

			acc = vmlaq_f32(acc, a, xv);
		}

		/* Horizontal reduction of 4 lanes */
		float sum;
#if defined(__aarch64__)
		sum = vaddvq_f32(acc);  /* AArch64: fast across-lanes sum */
#else
		/* Portable AArch32 reduction */
		float32x2_t low = vget_low_f32(acc);
		float32x2_t high = vget_high_f32(acc);
		float32x2_t s2 = vadd_f32(low, high);   /* (a0+a2, a1+a3) */

		s2 = vpadd_f32(s2, s2); /* (a0+a1+a2+a3, ...) */
		sum = vget_lane_f32(s2, 0);
#endif

		/* Scalar tail */
		for (; j < m; ++j) {
			sum += A[i * m + j] * x[j];
		}
		y[i] = sum;
	}
}

#endif

/**
 * f32_cmplx_mult_ref
 * Element-wise complex multiply reference
 * dst[i] = src_A[i] * src_B[i]
 * Complex numbers stored as {re, im} interleaved.
 */

void f32_cmplx_mult_ref(const cfloat32_t *src_A, const cfloat32_t *src_B,
			cfloat32_t *dst, uint64_t size)
{
#ifdef USE_ASM_SCALAR_AS_REF
	__asm__ volatile (
		"1:	subs    %[size], %[size], #1\n"
		"	ldp     s0, s1, [%[src_A]], #8\n"
		"	ldp     s2, s3, [%[src_B]], #8\n"
		"	fmul    s4, s3, s1\n"
		"	fmul    s1, s1, s2\n"
		"	fnmsub  s2, s2, s0, s4\n"
		"	fmadd   s1, s3, s0, s1\n"
		"	stp     s2, s1, [%[dst]], #8\n"
		"	b.ne    1b\n"
		: [src_A] "+&r"(src_A), [src_B] "+&r"(src_B),
		  [dst] "+&r"(dst), [size] "+&r"(size)
		:
		: "s0", "s1", "s2", "s3", "s4", "memory", "cc");
#else
	uint64_t i;

	for (i = 0; i < size; i++) {
		dst[i].re = (src_A[i].re * src_B[i].re) - (src_A[i].im * src_B[i].im);
		dst[i].im = (src_A[i].re * src_B[i].im) + (src_A[i].im * src_B[i].re);
	}
#endif
}

/**
 * f32_cmplx_mult_simd
 * Element-wise complex multiply SIMD
 * dst[i] = src_A[i] * src_B[i]
 * Complex numbers stored as {re, im} interleaved.
 */

#if defined(__ARM_FEATURE_SVE)

__attribute__ ((noinline))
void f32_cmplx_mult_simd(const cfloat32_t *src_A, const cfloat32_t *src_B,
			 cfloat32_t *dst, uint64_t size)
{
	uint64_t count = 0, count_1 = 0;
	uint64_t a1 = 0, b1 = 0, c1 = 0;

	__asm__ volatile (
		/* Check if there are any elements to process before start of loop */
		"	ptrue p2.s\n"
		"	whilelt p0.d, %[count], %[size]\n"
		"	b.none  3f\n"
		"	cntd    %[count_1]\n"
		"	whilelt p1.d, %[count_1], %[size]\n"
		"	b.none  2f\n"
		"	addvl   %[a1], %[src_A], #1\n"
		"	addvl   %[b1], %[src_B], #1\n"
		"	addvl   %[c1], %[dst], #1\n"
		"1:	dup     z0.s, #0\n"
		"	dup     z1.s, #0\n"
		/* Load complex elements from a and b arrays */
		"	ld1d    {z10.d}, p0/z, [%[src_A], %[count], lsl #3]\n"
		"	ld1d    {z12.d}, p0/z, [%[src_B], %[count], lsl #3]\n"
		"	ld1d    {z11.d}, p1/z, [%[a1], %[count], lsl #3]\n"
		"	ld1d    {z13.d}, p1/z, [%[b1], %[count], lsl #3]\n"
		/* Integer Complex multiplication */
		"	fcmla    z0.s, p2/m, z10.s, z12.s, #0\n"
		"	fcmla    z0.s, p2/m, z10.s, z12.s, #90\n"
		"	fcmla    z1.s, p2/m, z11.s, z13.s, #0\n"
		"	fcmla    z1.s, p2/m, z11.s, z13.s, #90\n"
		/* Store result to c array */
		"	st1d    {z0.d}, p0, [%[dst], %[count], lsl #3]\n"
		"	st1d    {z1.d}, p1, [%[c1], %[count], lsl #3]\n"
		"	incd    %[count], all, mul #2\n"
		"	whilelt p1.d, %[count], %[size]\n"
		"	b.first 1b\n"
		"	decd    %[count]\n"
		"	whilelt p0.d, %[count], %[size]\n"
		"	b.none  3f\n"
		"2:	dup     z0.s, #0\n"
		"	ld1d    {z10.d}, p0/z, [%[src_A], %[count], lsl #3]\n"
		"	ld1d    {z12.d}, p0/z, [%[src_B], %[count], lsl #3]\n"
		"	fcmla    z0.s, p2/m, z10.s, z12.s, #0\n"
		"	fcmla    z0.s, p2/m, z10.s, z12.s, #90\n"
		"	st1d    {z0.d}, p0, [%[dst], %[count], lsl #3]\n"
		/* End of operation */
		"3:\n"
		: [a1] "+&r"(a1), [b1] "+&r"(b1),
		  [c1] "+&r"(c1), [count] "+&r"(count), [count_1] "+&r"(count_1)
		: [src_A] "r"(src_A), [src_B] "r"(src_B),
		  [dst] "r"(dst), [size] "r"(size)
		: "z0", "z1", "z10", "z11", "z12", "z13", "p0", "p1", "p2", "memory", "cc");
}

#else

__attribute__ ((noinline))
void f32_cmplx_mult_simd(const cfloat32_t *src_A, const cfloat32_t *src_B,
			 cfloat32_t *dst, uint64_t size)
{
	float reg0 = 0, reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0;

	__asm__ volatile (
		/* Check whether there are >=4 elements to process before */
		/* starting loop. If not, proceed to loop tail. */
		"	cbz     %[size], 3f\n"
		"	cmp     %[size], #4\n"
		"	blt     2f\n"
		/* Loop begins here: */
		"1:	movi    v0.4s, #0x0\n"
		"	movi    v1.4s, #0x0\n"
		/* Load elements from a & b arrays such that real and */
		/* imaginary parts are de-interleaved */
		"	ld2     {v10.4s,v11.4s}, [%[src_A]], #32\n"
		"	ld2     {v20.4s,v21.4s}, [%[src_B]], #32\n"
		/* Perform complex multiplication */
		"	fmla     v0.4s, v10.4s, v20.4s\n"
		"	fmls     v0.4s, v11.4s, v21.4s\n"
		"	fmla     v1.4s, v10.4s, v21.4s\n"
		"	fmla     v1.4s, v11.4s, v20.4s\n"
		/* Store the result to c array */
		"	st2     {v0.4s,v1.4s}, [%[dst]], #32\n"
		/* Compare whether are >=4 elements left to continue with */
		/* loop iterations */
		"	sub     %[size], %[size], #4\n"
		"	cmp     %[size], #4\n"
		"	bge     1b\n"
		/* Loop ends here: */
		/* Process loop tail if any */
		"2:	cbz     %[size], 3f\n"
		"	ldp     %s[reg0], %s[reg1], [%[src_A]], #8\n"
		"	ldp     %s[reg2], %s[reg3], [%[src_B]], #8\n"
		"	fmul    %s[reg4], %s[reg0], %s[reg2]\n"
		"	fmsub   %s[reg4], %s[reg1], %s[reg3], %s[reg4]\n"
		"	fmul    %s[reg5], %s[reg0], %s[reg3]\n"
		"	fmadd   %s[reg5], %s[reg1], %s[reg2], %s[reg5]\n"
		"	stp     %s[reg4], %s[reg5], [%[dst]], #8\n"
		"	sub     %[size], %[size], #1\n"
		"	cbnz    %[size], 2b\n"
		"3:\n"
		: [src_A] "+&r"(src_A), [src_B] "+&r"(src_B),
		  [dst] "+&r"(dst), [reg0] "+&w"(reg0), [reg1] "+&w"(reg1), [reg2] "+&w"(reg2),
		  [reg3] "+&w"(reg3), [reg4] "+&w"(reg4), [reg5] "+&w"(reg5), [size] "+&r"(size)
		:
		: "v0", "v1", "v10", "v11", "v20", "v21", "v0", "v1", "memory", "cc");
}

#endif

void gen_test_data_f32(float32_t *in_out, float32_t scale,
		       float32_t offs, uint64_t size)
{
	for (uint64_t i = 0; i < size; i++) {
		in_out[i] = (float32_t)i * scale + offs;
	}
}

/* context switch tracker */
atomic_t switches_cnt;

void sys_trace_thread_switched_in_user(void)
{
	__asm__ volatile ("hint #0x31");
}

void sys_trace_thread_switched_out_user(void)
{
	atomic_inc(&switches_cnt);
	__asm__ volatile ("hint #0x33");
}

static inline uint32_t preempt_snapshot(void)
{
	return (uint32_t)atomic_get(&switches_cnt);
}

static inline uint32_t preempt_cnt_delta(uint32_t snap)
{
	return (uint32_t)atomic_get(&switches_cnt) - snap;
}

/* Per-thread result flags */
struct worker_stats {
	volatile int failures;		/* count of mismatches */
	volatile int success;		/* how many checks performed */
	volatile int switch_during_simd;/* how many times preempted during simd */
	uint32_t id;			/* unique per worker */
} __aligned(64);

struct matrix_test_ctx {
	const float32_t *mat;
	const float32_t *vec;
	float32_t *out_simd;
	float32_t *out_scal;
} __aligned(64);

struct cplxmult_test_ctx {
	const cfloat32_t *vecA;
	const cfloat32_t *vecB;
	cfloat32_t *out_simd;
	cfloat32_t *out_scal;
} __aligned(64);

struct worker_stats wstats[SIMD_THREAD_CNT];
struct matrix_test_ctx mattst_ctx[MATRIX_TEST_CTX];
struct cplxmult_test_ctx cplxmultst_ctx[CMPLX_MUL_TEST_CTX];

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, SIMD_THREAD_CNT, WORKER_STACK);
struct k_thread worker_threads[SIMD_THREAD_CNT];

K_THREAD_STACK_DEFINE(preemptor_stack, PREEMPTOR_STACK);
struct k_thread preemptor_thread;

/* Test control */
volatile bool test_complete;

void matmul_loop(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg3);
	struct worker_stats *st = (struct worker_stats *)arg1;
	struct matrix_test_ctx *tst = (struct matrix_test_ctx *)arg2;

	/*
	 * unique per-thread seed for vector generation
	 */
	float32_t offs = 2.0f + (float32_t)st->id;

	offs = offs * 0.0123f;

	gen_test_data_f32((float32_t *)tst->mat, 0.125f, offs,
			  TEST_MATRIX_ROWS * TEST_MATRIX_COLS);
	gen_test_data_f32((float32_t *)tst->vec, 0.025f, offs, TEST_MATRIX_COLS);

	f32_mat_x_vec_ref(tst->mat, tst->vec, tst->out_scal, TEST_MATRIX_ROWS, TEST_MATRIX_COLS);

	while (!test_complete) {
		__asm__ volatile (
			"hint #9\n"
			"mov x0, %[id]\n"
			:
			: [id] "r"(st->id)
			: "x0");

		/*
		 * clear output
		 */
		for (uint64_t i = 0; i < TEST_MATRIX_ROWS; i++) {
			tst->out_simd[i] = 0.0f;
		}

		uint32_t snap = preempt_snapshot();

		f32_mat_x_vec_simd(tst->mat, tst->vec, tst->out_simd, TEST_MATRIX_ROWS,
				   TEST_MATRIX_COLS);

		uint32_t cnt = preempt_cnt_delta(snap);

		if (cnt) {
			st->switch_during_simd += cnt;
		}

		if (!vec_within_threshold_f32(tst->out_scal, tst->out_simd,
					  TEST_MATRIX_ROWS, F32REL_THRSH)) {
			printf("error in mat x vec test\n");
			dump_buf(tst->out_simd, TEST_MATRIX_ROWS, TEST_MATRIX_ROWS, "%f");
			dump_buf(tst->out_scal, TEST_MATRIX_ROWS, TEST_MATRIX_ROWS, "%f");
			st->failures++;

			break;
		}
		st->success++;

		__asm__ volatile ("hint #11");
	}
}

void cmplxmul_loop(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg3);
	struct worker_stats *st = (struct worker_stats *)arg1;
	struct cplxmult_test_ctx *tst = (struct cplxmult_test_ctx *)arg2;

	/*
	 * unique per-thread seed
	 */
	float32_t offs = 2.0f + (float32_t)st->id;

	gen_test_data_f32((float32_t *)tst->vecA, 0.025f, offs,
			  TEST_CMPLX_MULT_SZ * 2);
	gen_test_data_f32((float32_t *)tst->vecB, 0.01234f, (0.234f * offs),
			  TEST_CMPLX_MULT_SZ * 2);

	f32_cmplx_mult_ref(tst->vecA, tst->vecB, tst->out_scal, TEST_CMPLX_MULT_SZ);

	while (!test_complete) {
		__asm__ volatile (
			"hint #0x21\n"
			"mov x0, %[id]\n"
			:
			: [id] "r"(st->id)
			: "x0");

		/*
		 * clear output
		 */
		for (uint64_t i = 0; i < TEST_CMPLX_MULT_SZ; i++) {
			tst->out_simd[i].re = 0.0f;
			tst->out_simd[i].im = 0.0f;
		}

		uint32_t snap = preempt_snapshot();

		f32_cmplx_mult_simd(tst->vecA, tst->vecB, tst->out_simd, TEST_CMPLX_MULT_SZ);

		st->switch_during_simd += preempt_cnt_delta(snap);

		float32_t *ptest_cmplx_mult_out_simd = (float32_t *)tst->out_simd;
		float32_t *ptest_cmplx_mult_out_scalar = (float32_t *)tst->out_scal;

		if (!vec_within_threshold_f32(ptest_cmplx_mult_out_scalar,
					  ptest_cmplx_mult_out_simd,
					  TEST_CMPLX_MULT_SZ * 2,
					  F32REL_THRSH)) {
			printf("error in cmplx  test\n");
			dump_buf(ptest_cmplx_mult_out_simd, TEST_CMPLX_MULT_SZ,
				 TEST_CMPLX_MULT_SZ, "%f");
			dump_buf(ptest_cmplx_mult_out_scalar, TEST_CMPLX_MULT_SZ,
				 TEST_CMPLX_MULT_SZ, "%f");
			st->failures++;

			break;
		}
		st->success++;

		__asm__ volatile ("hint #0x23");
	}
}

/**
 * simd_reg_clobber
 * Simulate register clobbering between context switches
 */
void simd_reg_clobber(void)
{
#if defined(__ARM_FEATURE_SVE)
	__asm__ volatile (
		"	dup     z1.s, #1\n"
		"	dup     z2.s, #2\n"
		"	dup     z3.s, #3\n"
		"	dup     z4.s, #4\n"
		"	dup     z5.s, #5\n"
		"	dup     z6.s, #6\n"
		"	dup     z7.s, #7\n"
		"	dup     z8.s, #8\n"
		"	dup     z9.s, #9\n"
		"	dup     z10.s, #10\n"
		"	dup     z11.s, #11\n"
		"	dup     z12.s, #12\n"
		"	dup     z13.s, #13\n"
		"	dup     z14.s, #14\n"
		"	dup     z15.s, #15\n"
		"	dup     z16.s, #(16+0)\n"
		"	dup     z17.s, #(16+1)\n"
		"	dup     z18.s, #(16+2)\n"
		"	dup     z19.s, #(16+3)\n"
		"	dup     z20.s, #(16+4)\n"
		"	dup     z21.s, #(16+5)\n"
		"	dup     z22.s, #(16+6)\n"
		"	dup     z23.s, #(16+7)\n"
		"	dup     z24.s, #(16+8)\n"
		"	dup     z25.s, #(16+9)\n"
		"	dup     z26.s, #(16+10)\n"
		"	dup     z27.s, #(16+11)\n"
		"	dup     z28.s, #(16+12)\n"
		"	dup     z29.s, #(16+13)\n"
		"	dup     z30.s, #(16+14)\n"
		"	dup     z31.s, #(16+15)\n"
		"	whilelo p0.s,%x[s] ,%x[e]\n"
		"	whilelo p1.s,%x[s] ,%x[e]\n"
		"	whilelo p2.s,%x[s] ,%x[e]\n"
		"	whilelo p3.s,%x[s] ,%x[e]\n"
		"	whilelo p4.s,%x[s] ,%x[e]\n"
		"	whilelo p5.s,%x[s] ,%x[e]\n"
		"	whilelo p6.s,%x[s] ,%x[e]\n"
		"	whilelo p7.s,%x[s] ,%x[e]\n"
		"	whilelo p8.s,%x[s] ,%x[e]\n"
		"	whilelo p9.s,%x[s] ,%x[e]\n"
		"	whilelo p10.s,%x[s] ,%x[e]\n"
		"	whilelo p11.s,%x[s] ,%x[e]\n"
		"	whilelo p12.s,%x[s] ,%x[e]\n"
		"	whilelo p13.s,%x[s] ,%x[e]\n"
		"	whilelo p14.s,%x[s] ,%x[e]\n"
		"	whilelo p15.s,%x[s] ,%x[e]\n"
		:
		: [s] "r"(0), [e] "r"(3)
		: "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7",
		  "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15",
		  "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
		  "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
		  "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
		  "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31",
		  "memory", "cc");
#else
	__asm__ volatile (
		"	movi    v0.4s, #0\n"
		"	movi    v1.4s, #1\n"
		"	movi    v2.4s, #2\n"
		"	movi    v3.4s, #3\n"
		"	movi    v4.4s, #4\n"
		"	movi    v5.4s, #5\n"
		"	movi    v6.4s, #6\n"
		"	movi    v7.4s, #7\n"
		"	movi    v8.4s, #8\n"
		"	movi    v9.4s, #9\n"
		"	movi    v10.4s, #10\n"
		"	movi    v11.4s, #11\n"
		"	movi    v12.4s, #12\n"
		"	movi    v13.4s, #13\n"
		"	movi    v14.4s, #14\n"
		"	movi    v15.4s, #15\n"
		"	movi    v16.4s, #(16+0)\n"
		"	movi    v17.4s, #(16+1)\n"
		"	movi    v18.4s, #(16+2)\n"
		"	movi    v19.4s, #(16+3)\n"
		"	movi    v20.4s, #(16+4)\n"
		"	movi    v21.4s, #(16+5)\n"
		"	movi    v22.4s, #(16+6)\n"
		"	movi    v23.4s, #(16+7)\n"
		"	movi    v24.4s, #(16+8)\n"
		"	movi    v25.4s, #(16+9)\n"
		"	movi    v26.4s, #(16+10)\n"
		"	movi    v27.4s, #(16+11)\n"
		"	movi    v28.4s, #(16+12)\n"
		"	movi    v29.4s, #(16+13)\n"
		"	movi    v30.4s, #(16+14)\n"
		"	movi    v31.4s, #(16+15)\n"
		:
		:
		: "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
		  "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
		  "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
		  "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31",
		  "memory", "cc");
#endif
}

/* high-priority preemptor: force preemption + SIMD register clobber */
void preemptor_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	int periodic_cnt = 0;

	while (periodic_cnt < TEST_DURATION_MSEC) {
		__asm__ volatile ("hint #13");

		simd_reg_clobber();

		if (++periodic_cnt % STAT_UPD_PERIOD_MSEC == 0) {
			for (int i = 0; i < SIMD_THREAD_CNT; ++i) {
				TC_PRINT("task %d succ %d fail %d switch_during_simd %d\n",
					 wstats[i].id, wstats[i].success,
					 wstats[i].failures, wstats[i].switch_during_simd);
			}
		}

		k_msleep(1);
	}
}

void sve_stress_init(void)
{
	/* Reset state */
	test_complete = false;
	atomic_set(&switches_cnt, 0);

	for (int i = 0; i < SIMD_THREAD_CNT; ++i) {
		wstats[i].id = i;
		wstats[i].success = 0;
		wstats[i].failures = 0;
		wstats[i].switch_during_simd = 0;
	}
}

ZTEST(arm64_sve_stress, test_simd_context_switch_stress)
{
	TC_PRINT("=== ARM Neon/SVE2 SIMD context switch stress test ===\n");

#if defined(__ARM_FEATURE_SVE)
	TC_PRINT("Using SVE instructions\n");
#else
	TC_PRINT("Using Neon instructions\n");
#endif

	sve_stress_init();

	/*
	 * Spawn workers at equal priority, preemptive
	 */
	for (int i = 0; i < SIMD_THREAD_CNT; ++i) {
		char thrd_name[16];

		if (i < MATRIX_TEST_CTX) {
			mattst_ctx[i].mat = test_matrix[i];
			mattst_ctx[i].vec = test_vector[i];
			mattst_ctx[i].out_simd = test_out_simd[i];
			mattst_ctx[i].out_scal = test_out_scalar[i];

			snprintk(thrd_name, sizeof(thrd_name), "matmul%d", wstats[i].id);
			k_thread_create(&worker_threads[i],
					worker_stacks[i], WORKER_STACK,
					matmul_loop, &wstats[i], &mattst_ctx[i], NULL,
					SIMD_TASK_PRIO, 0, K_NO_WAIT);
		} else {
			int idx = i - MATRIX_TEST_CTX;

			cplxmultst_ctx[idx].vecA = test_cmplx_mult_a[idx];
			cplxmultst_ctx[idx].vecB = test_cmplx_mult_b[idx];
			cplxmultst_ctx[idx].out_simd = test_cmplx_mult_out_simd[idx];
			cplxmultst_ctx[idx].out_scal = test_cmplx_mult_out_scalar[idx];

			snprintk(thrd_name, sizeof(thrd_name), "cmplxmul%d", wstats[i].id);
			k_thread_create(&worker_threads[i],
					worker_stacks[i], WORKER_STACK,
					cmplxmul_loop, &wstats[i], &cplxmultst_ctx[idx], NULL,
					SIMD_TASK_PRIO, 0, K_NO_WAIT);
		}
		k_thread_name_set(&worker_threads[i], thrd_name);
	}

	/*
	 * High-priority preemptor to increase preemption points, SIMD vector content change
	 * and stats display
	 */
	k_thread_create(&preemptor_thread, preemptor_stack, PREEMPTOR_STACK,
			preemptor_entry, NULL, NULL, NULL, PREEMPTOR_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&preemptor_thread, "simdpreemptor");

	/* Wait for test to complete */
	k_thread_join(&preemptor_thread, K_FOREVER);

	/* Signal threads to stop and wait for them */
	test_complete = true;

	for (int i = 0; i < SIMD_THREAD_CNT; ++i) {
		k_thread_join(&worker_threads[i], K_FOREVER);
	}

	/* Print final statistics */
	TC_PRINT("\n=== Final Test Results ===\n");
	int total_failures = 0;
	int total_success = 0;
	int total_switches = 0;

	for (int i = 0; i < SIMD_THREAD_CNT; ++i) {
		TC_PRINT("task %d: success=%d failures=%d switches_during_simd=%d\n",
			 wstats[i].id, wstats[i].success, wstats[i].failures,
			 wstats[i].switch_during_simd);
		total_failures += wstats[i].failures;
		total_success += wstats[i].success;
		total_switches += wstats[i].switch_during_simd;
	}

	TC_PRINT("Total: success=%d failures=%d switches=%d\n",
		 total_success, total_failures, total_switches);

	/* Verify test passed */
	zassert_equal(total_failures, 0,
		  "SIMD context switch stress test failed with %d errors", total_failures);
	zassert_true(total_success > 0, "No successful SIMD operations completed");
	zassert_true(total_switches > 0, "No context switches occurred during SIMD operations");
}

ZTEST_SUITE(arm64_sve_stress, NULL, NULL, NULL, NULL, NULL);
