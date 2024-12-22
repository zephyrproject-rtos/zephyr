/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

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
	zassert_false(wt_byte);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	zassert_equal(k_pipe_put(p, data, PIPE_LEN, &wt_byte,
				 1, TIMEOUT), -EAGAIN, NULL);
	zassert_true(wt_byte < 1);
	zassert_equal(k_pipe_put(p, data, PIPE_LEN, &wt_byte,
				 PIPE_LEN + 1, TIMEOUT), -EINVAL, NULL);

}

/**
 * @brief Test pipe put failure scenario
 * @ingroup kernel_pipe_tests
 * @see k_pipe_init(), k_pipe_put()
 */
ZTEST(pipe_api_1cpu, test_pipe_put_fail)
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
ZTEST_USER(pipe_api_1cpu, test_pipe_user_put_fail)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));
	/* check the number of bytes that may be read from pipe. */
	zassert_equal(k_pipe_read_avail(p), 0);
	/* check the number of bytes that may be written to pipe.*/
	zassert_equal(k_pipe_write_avail(p), PIPE_LEN);

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
	zassert_false(rd_byte);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	zassert_equal(k_pipe_get(p, rx_data, PIPE_LEN, &rd_byte, 1,
				 TIMEOUT), -EAGAIN, NULL);
	zassert_true(rd_byte < 1);
	zassert_equal(k_pipe_get(p, rx_data, PIPE_LEN, &rd_byte, 1,
				 TIMEOUT), -EAGAIN, NULL);
}

/**
 * @brief Test pipe get failure scenario
 * @ingroup kernel_pipe_tests
 * @see k_pipe_init(), k_pipe_get()
 */
ZTEST(pipe_api, test_pipe_get_fail)
{
	k_pipe_init(&put_get_pipe, data, PIPE_LEN);

	get_fail(&put_get_pipe);
}

#ifdef CONFIG_USERSPACE
static unsigned char user_unreach[PIPE_LEN];
static size_t unreach_byte;

/**
 * @brief Test pipe get by a user thread
 * @ingroup kernel_pipe_tests
 * @see k_pipe_alloc_init()
 */
ZTEST_USER(pipe_api, test_pipe_user_get_fail)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));

	get_fail(p);
}

/**
 * @brief Test k_pipe_alloc_init() failure scenario
 *
 * @details See what will happen if an uninitialized
 * k_pipe is passed to k_pipe_alloc_init().
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_alloc_init()
 */
ZTEST_USER(pipe_api, test_pipe_alloc_not_init)
{
	struct k_pipe pipe;

	ztest_set_fault_valid(true);
	k_pipe_alloc_init(&pipe, PIPE_LEN);
}

/**
 * @brief Test k_pipe_get() failure scenario
 *
 * @details See what will happen if an uninitialized
 * k_pipe is passed to k_pipe_get().
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_get()
 */
ZTEST_USER(pipe_api, test_pipe_get_null)
{
	unsigned char rx_data[PIPE_LEN];
	size_t rd_byte = 0;

	ztest_set_fault_valid(true);
	k_pipe_get(NULL, rx_data, PIPE_LEN,
	&rd_byte, 1, TIMEOUT);
}

/**
 * @brief Test k_pipe_get() failure scenario
 *
 * @details See what will happen if the parameter
 * address is accessed deny to test k_pipe_get
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_get()
 */
ZTEST_USER(pipe_api, test_pipe_get_unreach_data)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);
	size_t rd_byte = 0;

	zassert_true(p != NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));

	ztest_set_fault_valid(true);
	k_pipe_get(p, user_unreach, PIPE_LEN,
	&rd_byte, 1, TIMEOUT);

}

/**
 * @brief Test k_pipe_get() failure scenario
 *
 * @details See what will happen if the parameter
 * address is accessed deny to test k_pipe_get
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_get()
 */
ZTEST_USER(pipe_api, test_pipe_get_unreach_size)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);
	unsigned char rx_data[PIPE_LEN];

	zassert_true(p != NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));

	ztest_set_fault_valid(true);
	k_pipe_get(p, rx_data, PIPE_LEN,
	&unreach_byte, 1, TIMEOUT);

}

/**
 * @brief Test k_pipe_put() failure scenario
 *
 * @details See what will happen if a null pointer
 * is passed into the k_pipe_put as a parameter
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_put()
 */
ZTEST_USER(pipe_api, test_pipe_put_null)
{
	unsigned char tx_data = 0xa;
	size_t to_wt = 0, wt_byte = 0;

	ztest_set_fault_valid(true);
	k_pipe_put(NULL, &tx_data, to_wt,
		&wt_byte, 1, TIMEOUT);
}

/**
 * @brief Test k_pipe_put() failure scenario
 *
 * @details See what will happen if the parameter
 * address is accessed deny to test k_pipe_put
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_put()
 */
ZTEST_USER(pipe_api, test_pipe_put_unreach_data)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);
	size_t to_wt = 0, wt_byte = 0;

	zassert_true(p != NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));

	ztest_set_fault_valid(true);
	k_pipe_put(p, &user_unreach[0], to_wt,
		&wt_byte, 1, TIMEOUT);

}

/**
 * @brief Test k_pipe_put() failure scenario
 *
 * @details See what will happen if the parameter
 * address is accessed deny to test k_pipe_put
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_put()
 */
ZTEST_USER(pipe_api, test_pipe_put_unreach_size)
{
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);
	unsigned char tx_data = 0xa;
	size_t to_wt = 0;

	zassert_true(p != NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));

	ztest_set_fault_valid(true);
	k_pipe_put(p, &tx_data, to_wt,
		&unreach_byte, 1, TIMEOUT);
}

/**
 * @brief Test k_pipe_read_avail() failure scenario
 *
 * @details See what will happen if a null pointer
 * is passed into the k_pipe_read_avail as a parameter
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_read_avail()
 */
ZTEST_USER(pipe_api, test_pipe_read_avail_null)
{
	ztest_set_fault_valid(true);
	k_pipe_read_avail(NULL);
}

/**
 * @brief Test k_pipe_write_avail() failure scenario
 *
 * @details See what will happen if a null pointer
 * is passed into the k_pipe_write_avail as a parameter
 *
 * @ingroup kernel_pipe_tests
 *
 * @see k_pipe_write_avail()
 */
ZTEST_USER(pipe_api, test_pipe_write_avail_null)
{
	ztest_set_fault_valid(true);
	k_pipe_write_avail(NULL);
}
#endif
