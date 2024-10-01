/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/smf.h>

/*
 * Hierarchical 5 Ancestor State Test Transition:
 *
 *	P05_ENTRY --> P04_ENTRY --> P03_ENTRY --> P02_ENTRY ---------|
 *                                                                   |
 *      |------------------------------------------------------------|
 *      |
 *      |--> P01_ENTRY --> A_ENTRY --> A_RUN --> A_EXIT -------------|
 *                                                                   |
 *      |------------------------------------------------------------|
 *      |
 *      |--> B_ENTRY --> B_RUN --> P01_RUN --> P02_RUN --> P03_RUN --|
 *                                                                   |
 *      |------------------------------------------------------------|
 *      |
 *      |--> P04_RUN --> P05_RUN --> B_EXIT --> P01_EXIT ------------|
 *                                                                   |
 *      |------------------------------------------------------------|
 *      |
 *      |--> P02_EXIT --> P03_EXIT --> P04_EXIT --> P05_EXIT --------|
 *                                                                   |
 *      |------------------------------------------------------------|
 *      |
 *      |--> C_ENTRY --> C_RUN --> C_EXIT --> D_ENTRY
 */

#define TEST_OBJECT(o) ((struct test_object *)o)

#define SMF_RUN                         3

#define P05_ENTRY_BIT    (1 << 0)
#define P04_ENTRY_BIT    (1 << 1)
#define P03_ENTRY_BIT    (1 << 2)
#define P02_ENTRY_BIT    (1 << 3)
#define P01_ENTRY_BIT    (1 << 4)
#define A_ENTRY_BIT      (1 << 5)
#define A_RUN_BIT        (1 << 6)
#define A_EXIT_BIT       (1 << 7)
#define B_ENTRY_BIT      (1 << 8)
#define B_RUN_BIT        (1 << 9)
#define P01_RUN_BIT      (1 << 10)
#define P02_RUN_BIT      (1 << 11)
#define P03_RUN_BIT      (1 << 12)
#define P04_RUN_BIT      (1 << 13)
#define P05_RUN_BIT      (1 << 14)
#define B_EXIT_BIT       (1 << 15)
#define P01_EXIT_BIT     (1 << 16)
#define P02_EXIT_BIT     (1 << 17)
#define P03_EXIT_BIT     (1 << 18)
#define P04_EXIT_BIT     (1 << 19)
#define P05_EXIT_BIT     (1 << 20)
#define C_ENTRY_BIT      (1 << 21)
#define C_RUN_BIT        (1 << 22)
#define C_EXIT_BIT       (1 << 23)

#define TEST_VALUE_NUM              24
static uint32_t test_value[] = {
	0x00000000, /* P05_ENTRY */
	0x00000001, /* P04_ENTRY */
	0x00000003, /* P03_ENTRY */
	0x00000007, /* P02_ENTRY */
	0x0000000f, /* P01_ENTRY */
	0x0000001f, /*   A_ENTRY */
	0x0000003f, /*   A_RUN   */
	0x0000007f, /*   A_EXIT  */
	0x000000ff, /*   B_ENTRY */
	0x000001ff, /*   B_RUN   */
	0x000003ff, /* P01_RUN   */
	0x000007ff, /* P02_RUN   */
	0x00000fff, /* P03_RUN   */
	0x00001fff, /* P04_RUN   */
	0x00003fff, /* P05_RUN   */
	0x00007fff, /*   B_EXIT  */
	0x0000ffff, /* P01_EXIT  */
	0x0001ffff, /* P02_EXIT  */
	0x0003ffff, /* P03_EXIT  */
	0x0007ffff, /* P04_EXIT  */
	0x000fffff, /* P05_EXIT  */
	0x001fffff, /*   C_ENTRY */
	0x003fffff, /*   C_RUN   */
	0x007fffff, /*   C_EXIT  */
	0x00ffffff, /*   D_ENTRY */
};

/* Forward declaration of test_states */
static const struct smf_state test_states[];

/* List of all TypeC-level states */
enum test_state {
	P05,
	P04,
	P03,
	P02,
	P01,
	A,
	B,
	C,
	D,
};

static struct test_object {
	struct smf_ctx ctx;
	uint32_t transition_bits;
	uint32_t tv_idx;
} test_obj;

static void p05_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 05 entry failed");

	o->transition_bits |= P05_ENTRY_BIT;
}

static void p05_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 05 run failed");

	o->transition_bits |= P05_RUN_BIT;

	smf_set_state(SMF_CTX(obj), &test_states[C]);
}

static void p05_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 05 exit failed");

	o->transition_bits |= P05_EXIT_BIT;
}

static void p04_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 04 entry failed");

	o->transition_bits |= P04_ENTRY_BIT;
}

static void p04_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 04 run failed");

	o->transition_bits |= P04_RUN_BIT;
}

static void p04_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 04 exit failed");

	o->transition_bits |= P04_EXIT_BIT;
}

static void p03_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 03 entry failed");

	o->transition_bits |= P03_ENTRY_BIT;
}

static void p03_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 03 run failed");

	o->transition_bits |= P03_RUN_BIT;
}

static void p03_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 03 exit failed");

	o->transition_bits |= P03_EXIT_BIT;
}

static void p02_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 02 entry failed");

	o->transition_bits |= P02_ENTRY_BIT;
}

static void p02_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 02 run failed");

	o->transition_bits |= P02_RUN_BIT;
}

static void p02_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 02 exit failed");

	o->transition_bits |= P02_EXIT_BIT;
}

static void p01_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 01 entry failed");

	o->transition_bits |= P01_ENTRY_BIT;
}

static void p01_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 01 run failed");

	o->transition_bits |= P01_RUN_BIT;
}

static void p01_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test Parent 01 exit failed");

	o->transition_bits |= P01_EXIT_BIT;
}

static void a_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State A entry failed");

	o->transition_bits |= A_ENTRY_BIT;
}

static void a_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State A run failed");

	o->transition_bits |= A_RUN_BIT;

	smf_set_state(SMF_CTX(obj), &test_states[B]);
}

static void a_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State A exit failed");

	o->transition_bits |= A_EXIT_BIT;
}

static void b_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State B entry failed");

	o->transition_bits |= B_ENTRY_BIT;
}

static void b_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State B run failed");

	o->transition_bits |= B_RUN_BIT;
}

static void b_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State B exit failed");

	o->transition_bits |= B_EXIT_BIT;
}

static void c_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State C entry failed");

	o->transition_bits |= C_ENTRY_BIT;
}

static void c_run(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State C run failed");
	o->transition_bits |= C_RUN_BIT;

	smf_set_state(SMF_CTX(obj), &test_states[D]);
}

static void c_exit(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;

	zassert_equal(o->transition_bits, test_value[o->tv_idx],
		      "Test State C exit failed");

	o->transition_bits |= C_EXIT_BIT;
}

static void d_entry(void *obj)
{
	struct test_object *o = TEST_OBJECT(obj);

	o->tv_idx++;
}

static const struct smf_state test_states[] = {
	[P05] SMF_CREATE_STATE(p05_entry, p05_run, p05_exit, NULL, NULL),
	[P04] SMF_CREATE_STATE(p04_entry, p04_run, p04_exit, &test_states[P05], NULL),
	[P03] SMF_CREATE_STATE(p03_entry, p03_run, p03_exit, &test_states[P04], NULL),
	[P02] SMF_CREATE_STATE(p02_entry, p02_run, p02_exit, &test_states[P03], NULL),
	[P01] SMF_CREATE_STATE(p01_entry, p01_run, p01_exit, &test_states[P02], NULL),
	[A] = SMF_CREATE_STATE(a_entry, a_run, a_exit, &test_states[P01], NULL),
	[B] = SMF_CREATE_STATE(b_entry, b_run, b_exit, &test_states[P01], NULL),
	[C] = SMF_CREATE_STATE(c_entry, c_run, c_exit, NULL, NULL),
	[D] = SMF_CREATE_STATE(d_entry, NULL, NULL, NULL, NULL),
};

ZTEST(smf_tests, test_smf_hierarchical_5_ancestors)
{
	test_obj.tv_idx = 0;
	test_obj.transition_bits = 0;
	smf_set_initial((struct smf_ctx *)&test_obj, &test_states[A]);

	for (int i = 0; i < SMF_RUN; i++) {
		if (smf_run_state((struct smf_ctx *)&test_obj) < 0) {
			break;
		}
	}

	zassert_equal(TEST_VALUE_NUM, test_obj.tv_idx,
		      "Incorrect test value index");
	zassert_equal(test_obj.transition_bits, test_value[test_obj.tv_idx],
		      "Final state not reached");
}
