/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>

#define TIMEOUT K_MSEC(100)
#define PIPE_LEN 8

static ZTEST_DMEM unsigned char __aligned(4) data[] = "abcd1234";

struct k_pipe put_get_pipe;


static void put_fail(struct k_pipe *p)
{
	size_t wt_byte = 0;

	zassert_false(k_pipe_put(p, data, PIPE_LEN, &wt_byte,
				 1, K_FOREVER), NULL);
	/**TESTPOINT: pipe put returns -EIO*/
	zassert_equal(k_pipe_put(p, data, PIPE_LEN, &wt_byte,
				 1, K_NO_WAIT), -EIO, NULL);
	zassert_false(wt_byte, NULL);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	zassert_equal(k_pipe_put(p, data, PIPE_LEN, &wt_byte,
				 1, TIMEOUT), -EAGAIN, NULL);
	zassert_true(wt_byte < 1, NULL);
	zassert_equal(k_pipe_put(p, data, PIPE_LEN, &wt_byte,
				 PIPE_LEN + 1, TIMEOUT), -EINVAL, NULL);

}

/**
 * @brief Test pipe put failure scenario
 * @ingroup kernel_pipe_tests
 * @see k_pipe_init(), k_pipe_put()
 */
void test_pipe_put_fail(void)
{
	k_pipe_init(&put_get_pipe, data, PIPE_LEN);

	put_fail(&put_get_pipe);
}
/**
 * @brief Test pipe put by a user thread
 * @ingroup kernel_pipe_tests
 * @see k_pipe_put()
 */
#ifdef CONFIG_USERSPACE
void test_pipe_user_put_fail(void)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL, NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN), NULL);
	/* check the number of bytes that may be read from pipe. */
	zassert_equal(k_pipe_read_avail(p), 0, NULL);
	/* check the number of bytes that may be written to pipe.*/
	zassert_equal(k_pipe_write_avail(p), PIPE_LEN, NULL);

	put_fail(p);
}
#endif

static void get_fail(struct k_pipe *p)
{
	unsigned char rx_data[PIPE_LEN];
	size_t rd_byte = 0;

	/**TESTPOINT: pipe put returns -EIO*/
	zassert_equal(k_pipe_get(p, rx_data, PIPE_LEN, &rd_byte, 1,
				 K_NO_WAIT), -EIO, NULL);
	zassert_false(rd_byte, NULL);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	zassert_equal(k_pipe_get(p, rx_data, PIPE_LEN, &rd_byte, 1,
				 TIMEOUT), -EAGAIN, NULL);
	zassert_true(rd_byte < 1, NULL);
	zassert_equal(k_pipe_get(p, rx_data, PIPE_LEN, &rd_byte, 1,
				 TIMEOUT), -EAGAIN, NULL);
}

/**
 * @brief Test pipe get failure scenario
 * @ingroup kernel_pipe_tests
 * @see k_pipe_init(), k_pipe_get()
 */
void test_pipe_get_fail(void)
{
	k_pipe_init(&put_get_pipe, data, PIPE_LEN);

	get_fail(&put_get_pipe);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test pipe get by a user thread
 * @ingroup kernel_pipe_tests
 * @see k_pipe_alloc_init()
 */
void test_pipe_user_get_fail(void)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL, NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN), NULL);

	get_fail(p);
}
#endif
