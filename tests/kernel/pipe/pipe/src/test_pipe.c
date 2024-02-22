/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>

/**
 * @brief Define and initialize test_pipe at compile time
 */
K_PIPE_DEFINE(test_pipe, 256, 4);
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PIPE_SIZE (256)

K_PIPE_DEFINE(small_pipe, 10, 4);

K_THREAD_STACK_DEFINE(stack_1, STACK_SIZE);

K_SEM_DEFINE(get_sem, 0, 1);
K_SEM_DEFINE(put_sem, 1, 1);
K_SEM_DEFINE(sync_sem, 0, 1);
K_SEM_DEFINE(multiple_send_sem, 0, 1);

ZTEST_BMEM uint8_t tx_buffer[PIPE_SIZE + 1];
ZTEST_BMEM uint8_t rx_buffer[PIPE_SIZE + 1];

#define TOTAL_ELEMENTS (sizeof(single_elements) / sizeof(struct pipe_sequence))
#define TOTAL_WAIT_ELEMENTS (sizeof(wait_elements) / \
			     sizeof(struct pipe_sequence))
#define TOTAL_TIMEOUT_ELEMENTS (sizeof(timeout_elements) / \
				sizeof(struct pipe_sequence))

/* Minimum tx/rx size*/
/* the pipe will always pass */
#define NO_CONSTRAINT (0U)

/* Pipe will at least put one byte */
#define ATLEAST_1 (1U)

/* Pipe must put all data on the buffer */
#define ALL_BYTES (sizeof(tx_buffer))

#define RETURN_SUCCESS  (0)
#define TIMEOUT_VAL (K_MSEC(10))
#define TIMEOUT_200MSEC (K_MSEC(200))

/* encompassing structs */
struct pipe_sequence {
	uint32_t size;
	uint32_t min_size;
	uint32_t sent_bytes;
	int return_value;
};

static const struct pipe_sequence single_elements[] = {
	{ 0, ALL_BYTES, 0, 0 },
	{ 1, ALL_BYTES, 1, RETURN_SUCCESS },
	{ PIPE_SIZE - 1, ALL_BYTES, PIPE_SIZE - 1, RETURN_SUCCESS },
	{ PIPE_SIZE, ALL_BYTES, PIPE_SIZE, RETURN_SUCCESS },
	{ PIPE_SIZE + 1, ALL_BYTES, 0, -EIO },
	/* minimum 1 byte */
	/* {0, ATLEAST_1, 0, -EIO}, */
	{ 1, ATLEAST_1, 1, RETURN_SUCCESS },
	{ PIPE_SIZE - 1, ATLEAST_1, PIPE_SIZE - 1, RETURN_SUCCESS },
	{ PIPE_SIZE, ATLEAST_1, PIPE_SIZE, RETURN_SUCCESS },
	{ PIPE_SIZE + 1, ATLEAST_1, PIPE_SIZE, RETURN_SUCCESS },
	/* /\* any number of bytes *\/ */
	{ 0, NO_CONSTRAINT, 0, 0 },
	{ 1, NO_CONSTRAINT, 1, RETURN_SUCCESS },
	{ PIPE_SIZE - 1, NO_CONSTRAINT, PIPE_SIZE - 1, RETURN_SUCCESS },
	{ PIPE_SIZE, NO_CONSTRAINT, PIPE_SIZE, RETURN_SUCCESS },
	{ PIPE_SIZE + 1, NO_CONSTRAINT, PIPE_SIZE, RETURN_SUCCESS }
};

static const struct pipe_sequence multiple_elements[] = {
	{ PIPE_SIZE / 3, ALL_BYTES, PIPE_SIZE / 3, RETURN_SUCCESS, },
	{ PIPE_SIZE / 3, ALL_BYTES, PIPE_SIZE / 3, RETURN_SUCCESS, },
	{ PIPE_SIZE / 3, ALL_BYTES, PIPE_SIZE / 3, RETURN_SUCCESS, },
	{ PIPE_SIZE / 3, ALL_BYTES, 0, -EIO },

	{ PIPE_SIZE / 3, ATLEAST_1, PIPE_SIZE / 3, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, ATLEAST_1, PIPE_SIZE / 3, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, ATLEAST_1, PIPE_SIZE / 3, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, ATLEAST_1, 1, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, ATLEAST_1, 0, -EIO },

	{ PIPE_SIZE / 3, NO_CONSTRAINT, PIPE_SIZE / 3, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, NO_CONSTRAINT, PIPE_SIZE / 3, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, NO_CONSTRAINT, PIPE_SIZE / 3, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, NO_CONSTRAINT, 1, RETURN_SUCCESS },
	{ PIPE_SIZE / 3, NO_CONSTRAINT, 0, RETURN_SUCCESS }
};

static const struct pipe_sequence wait_elements[] = {
	{            1, ALL_BYTES,             1, RETURN_SUCCESS },
	{ PIPE_SIZE - 1, ALL_BYTES, PIPE_SIZE - 1, RETURN_SUCCESS },
	{    PIPE_SIZE, ALL_BYTES,     PIPE_SIZE, RETURN_SUCCESS },
	{ PIPE_SIZE + 1, ALL_BYTES, PIPE_SIZE + 1, RETURN_SUCCESS },

	{ PIPE_SIZE - 1, ATLEAST_1, PIPE_SIZE - 1, RETURN_SUCCESS },
};

static const struct pipe_sequence timeout_elements[] = {
	{            0, ALL_BYTES, 0, 0 },
	{            1, ALL_BYTES, 0, -EAGAIN },
	{ PIPE_SIZE - 1, ALL_BYTES, 0, -EAGAIN },
	{    PIPE_SIZE, ALL_BYTES, 0, -EAGAIN },
	{ PIPE_SIZE + 1, ALL_BYTES, 0, -EAGAIN },

	{            1, ATLEAST_1, 0, -EAGAIN },
	{ PIPE_SIZE - 1, ATLEAST_1, 0, -EAGAIN },
	{    PIPE_SIZE, ATLEAST_1, 0, -EAGAIN },
	{ PIPE_SIZE + 1, ATLEAST_1, 0, -EAGAIN }
};

struct k_thread get_single_tid;

/* Helper functions */

uint32_t rx_buffer_check(char *buffer, uint32_t size)
{
	uint32_t index;

	for (index = 0U; index < size; index++) {
		if (buffer[index] != (char) index) {
			printk("buffer[index] = %d index = %d\n",
			       buffer[index], (char) index);
			return index;
		}
	}

	return size;
}


/******************************************************************************/
void pipe_put_single(void)
{
	uint32_t index;
	size_t written;
	int return_value;
	size_t min_xfer;

	for (index = 0U; index < TOTAL_ELEMENTS; index++) {
		k_sem_take(&put_sem, K_FOREVER);

		min_xfer = (single_elements[index].min_size == ALL_BYTES ?
			    single_elements[index].size :
			    single_elements[index].min_size);

		return_value = k_pipe_put(&test_pipe, &tx_buffer,
					  single_elements[index].size, &written,
					  min_xfer, K_NO_WAIT);

		zassert_true((return_value ==
			      single_elements[index].return_value),
			     " Return value of k_pipe_put mismatch at index = %d expected =%d received = %d\n",
			     index,
			     single_elements[index].return_value, return_value);

		zassert_true((written == single_elements[index].sent_bytes),
			     "Bytes written mismatch written is %d but expected is %d index = %d\n",
			     written,
			     single_elements[index].sent_bytes, index);

		k_sem_give(&get_sem);
	}

}

void pipe_get_single(void *p1, void *p2, void *p3)
{
	uint32_t index;
	size_t read;
	int return_value;
	size_t min_xfer;

	for (index = 0U; index < TOTAL_ELEMENTS; index++) {
		k_sem_take(&get_sem, K_FOREVER);

		/* reset the rx buffer for the next interation */
		(void)memset(rx_buffer, 0, sizeof(rx_buffer));

		min_xfer = (single_elements[index].min_size == ALL_BYTES ?
			    single_elements[index].size :
			    single_elements[index].min_size);

		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  single_elements[index].size, &read,
					  min_xfer, K_NO_WAIT);


		zassert_true((return_value ==
			      single_elements[index].return_value),
			     "Return value of k_pipe_get mismatch at index = %d expected =%d received = %d\n",
			     index, single_elements[index].return_value,
			     return_value);

		zassert_true((read == single_elements[index].sent_bytes),
			     "Bytes read mismatch read is %d but expected is %d index = %d\n",
			     read, single_elements[index].sent_bytes, index);

		zassert_true(rx_buffer_check(rx_buffer, read) == read,
			     "Bytes read are not matching at index= %d\n expected =%d but received= %d",
			     index, read, rx_buffer_check(rx_buffer, read));
		k_sem_give(&put_sem);
	}
	k_sem_give(&sync_sem);
}

/******************************************************************************/
void pipe_put_multiple(void)
{
	uint32_t index;
	size_t written;
	int return_value;
	size_t min_xfer;

	for (index = 0U; index < TOTAL_ELEMENTS; index++) {

		min_xfer = (multiple_elements[index].min_size == ALL_BYTES ?
			    multiple_elements[index].size :
			    multiple_elements[index].min_size);

		return_value = k_pipe_put(&test_pipe, &tx_buffer,
					  multiple_elements[index].size,
					  &written,
					  min_xfer, K_NO_WAIT);

		zassert_true((return_value ==
			      multiple_elements[index].return_value),
			     "Return value of k_pipe_put mismatch at index = %d expected =%d received = %d\n",
			     index,
			     multiple_elements[index].return_value,
			     return_value);

		zassert_true((written == multiple_elements[index].sent_bytes),
			     "Bytes written mismatch written is %d but expected is %d index = %d\n",
			     written,
			     multiple_elements[index].sent_bytes, index);
		if (return_value != RETURN_SUCCESS) {
			k_sem_take(&multiple_send_sem, K_FOREVER);
		}

	}

}

void pipe_get_multiple(void *p1, void *p2, void *p3)
{
	uint32_t index;
	size_t read;
	int return_value;
	size_t min_xfer;

	for (index = 0U; index < TOTAL_ELEMENTS; index++) {


		/* reset the rx buffer for the next interation */
		(void)memset(rx_buffer, 0, sizeof(rx_buffer));

		min_xfer = (multiple_elements[index].min_size == ALL_BYTES ?
			    multiple_elements[index].size :
			    multiple_elements[index].min_size);

		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  multiple_elements[index].size, &read,
					  min_xfer, K_NO_WAIT);


		zassert_true((return_value ==
			      multiple_elements[index].return_value),
			     "Return value of k_pipe_get mismatch at index = %d expected =%d received = %d\n",
			     index, multiple_elements[index].return_value,
			     return_value);

		zassert_true((read == multiple_elements[index].sent_bytes),
			     "Bytes read mismatch read is %d but expected is %d index = %d\n",
			     read, multiple_elements[index].sent_bytes, index);

		zassert_true(rx_buffer_check(rx_buffer, read) == read,
			     "Bytes read are not matching at index= %d\n expected =%d but received= %d",
			     index, read, rx_buffer_check(rx_buffer, read));

		if (return_value != RETURN_SUCCESS) {
			k_sem_give(&multiple_send_sem);
		}

	}
	k_sem_give(&sync_sem);
}

/******************************************************************************/

void pipe_put_forever_wait(void)
{
	size_t written;
	int return_value;

	/* 1. fill the pipe. */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  PIPE_SIZE, K_FOREVER);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_put failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(written == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);


	/* wake up the get task */
	k_sem_give(&get_sem);
	/* 2. k_pipe_put() will force a context switch to the other thread. */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  PIPE_SIZE, K_FOREVER);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_put failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(written == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);

	/* 3. k_pipe_put() will force a context switch to the other thread. */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  ATLEAST_1, K_FOREVER);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_put failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(written == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);

}


void pipe_get_forever_wait(void *pi, void *p2, void *p3)
{
	size_t read;
	int return_value;

	/* get blocked until put forces the execution to come here */
	k_sem_take(&get_sem, K_FOREVER);

	/* k_pipe_get will force a context switch to the put function. */
	return_value = k_pipe_get(&test_pipe, &rx_buffer,
				  PIPE_SIZE, &read,
				  PIPE_SIZE, K_FOREVER);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_get failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(read == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, read);

	/* k_pipe_get will force a context switch to the other thread. */
	return_value = k_pipe_get(&test_pipe, &rx_buffer,
				  PIPE_SIZE, &read,
				  ATLEAST_1, K_FOREVER);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_get failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(read == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, read);

	/*3. last read to clear the pipe */
	return_value = k_pipe_get(&test_pipe, &rx_buffer,
				  PIPE_SIZE, &read,
				  ATLEAST_1, K_FOREVER);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_get failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(read == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, read);


	k_sem_give(&sync_sem);

}


/******************************************************************************/
void pipe_put_timeout(void)
{
	size_t written;
	int return_value;

	/* 1. fill the pipe. */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  PIPE_SIZE, TIMEOUT_VAL);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_put failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(written == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);


	/* pipe put cant be satisfied and thus timeout */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  PIPE_SIZE, TIMEOUT_VAL);

	zassert_true(return_value == -EAGAIN,
		     "k_pipe_put failed expected = -EAGAIN received = %d\n",
		     return_value);

	zassert_true(written == 0,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);

	/* Try once more with 1 byte pipe put cant be satisfied and
	 * thus timeout.
	 */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  ATLEAST_1, TIMEOUT_VAL);

	zassert_true(return_value == -EAGAIN,
		     "k_pipe_put failed expected = -EAGAIN received = %d\n",
		     return_value);

	zassert_true(written == 0,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);

	k_sem_give(&get_sem);

	/* 2. pipe_get thread will now accept this data  */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  PIPE_SIZE, TIMEOUT_VAL);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_put failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(written == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);

	/* 3. pipe_get thread will now accept this data  */
	return_value = k_pipe_put(&test_pipe, &tx_buffer,
				  PIPE_SIZE, &written,
				  ATLEAST_1, TIMEOUT_VAL);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_put failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(written == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, written);
}


void pipe_get_timeout(void *pi, void *p2, void *p3)
{
	size_t read;
	int return_value;

	/* get blocked until put forces the execution to come here */
	k_sem_take(&get_sem, K_FOREVER);

	/* k_pipe_get will do a context switch to the put function. */
	return_value = k_pipe_get(&test_pipe, &rx_buffer,
				  PIPE_SIZE, &read,
				  PIPE_SIZE, TIMEOUT_VAL);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_get failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(read == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, read);

	/* k_pipe_get will do a context switch to the put function. */
	return_value = k_pipe_get(&test_pipe, &rx_buffer,
				  PIPE_SIZE, &read,
				  ATLEAST_1, TIMEOUT_VAL);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_get failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(read == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, read);

	/* cleanup the pipe */
	return_value = k_pipe_get(&test_pipe, &rx_buffer,
				  PIPE_SIZE, &read,
				  ATLEAST_1, TIMEOUT_VAL);

	zassert_true(return_value == RETURN_SUCCESS,
		     "k_pipe_get failed expected = 0 received = %d\n",
		     return_value);

	zassert_true(read == PIPE_SIZE,
		     "k_pipe_put written failed expected = %d received = %d\n",
		     PIPE_SIZE, read);

	k_sem_give(&sync_sem);
}


/******************************************************************************/
void pipe_get_on_empty_pipe(void)
{
	size_t read;
	int return_value;
	uint32_t read_size;
	uint32_t size_array[] = { 1, PIPE_SIZE - 1, PIPE_SIZE, PIPE_SIZE + 1 };

	for (int i = 0; i < 4; i++) {
		read_size = size_array[i];
		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  read_size, &read,
					  read_size, K_NO_WAIT);

		zassert_true(return_value == -EIO,
			     "k_pipe_get failed expected = -EIO received = %d\n",
			     return_value);

		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  read_size, &read,
					  ATLEAST_1, K_NO_WAIT);

		zassert_true(return_value == -EIO,
			     "k_pipe_get failed expected = -EIO received = %d\n",
			     return_value);

		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  read_size, &read,
					  NO_CONSTRAINT, K_NO_WAIT);

		zassert_true(return_value == RETURN_SUCCESS,
			     "k_pipe_get failed expected = 0 received = %d\n",
			     return_value);

		zassert_true(read == 0,
			     "k_pipe_put written failed expected = %d received = %d\n",
			     PIPE_SIZE, read);
	}

}


/******************************************************************************/
void pipe_put_forever_timeout(void)
{
	uint32_t index;
	size_t written;
	int return_value;
	size_t min_xfer;

	/* using this to synchronize the 2 threads  */
	k_sem_take(&put_sem, K_FOREVER);

	for (index = 0U; index < TOTAL_WAIT_ELEMENTS; index++) {

		min_xfer = (wait_elements[index].min_size == ALL_BYTES ?
			    wait_elements[index].size :
			    wait_elements[index].min_size);

		return_value = k_pipe_put(&test_pipe, &tx_buffer,
					  wait_elements[index].size, &written,
					  min_xfer, K_FOREVER);

		zassert_true((return_value ==
			      wait_elements[index].return_value),
			     "Return value of k_pipe_put mismatch at index = %d expected =%d received = %d\n",
			     index, wait_elements[index].return_value,
			     return_value);

		zassert_true((written == wait_elements[index].sent_bytes),
			     "Bytes written mismatch written is %d but expected is %d index = %d\n",
			     written, wait_elements[index].sent_bytes, index);

	}

}

void pipe_get_forever_timeout(void *p1, void *p2, void *p3)
{
	uint32_t index;
	size_t read;
	int return_value;
	size_t min_xfer;

	/* using this to synchronize the 2 threads  */
	k_sem_give(&put_sem);
	for (index = 0U; index < TOTAL_WAIT_ELEMENTS; index++) {

		min_xfer = (wait_elements[index].min_size == ALL_BYTES ?
			    wait_elements[index].size :
			    wait_elements[index].min_size);

		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  wait_elements[index].size, &read,
					  min_xfer, K_FOREVER);


		zassert_true((return_value ==
			      wait_elements[index].return_value),
			     "Return value of k_pipe_get mismatch at index = %d expected =%d received = %d\n",
			     index, wait_elements[index].return_value,
			     return_value);

		zassert_true((read == wait_elements[index].sent_bytes),
			     "Bytes read mismatch read is %d but expected is %d index = %d\n",
			     read, wait_elements[index].sent_bytes, index);


	}
	k_sem_give(&sync_sem);
}

/******************************************************************************/
void pipe_put_get_timeout(void)
{
	uint32_t index;
	size_t read;
	int return_value;
	size_t min_xfer;

	for (index = 0U; index < TOTAL_TIMEOUT_ELEMENTS; index++) {

		min_xfer = (timeout_elements[index].min_size == ALL_BYTES ?
			    timeout_elements[index].size :
			    timeout_elements[index].min_size);

		return_value = k_pipe_get(&test_pipe, &rx_buffer,
					  timeout_elements[index].size, &read,
					  min_xfer, TIMEOUT_200MSEC);


		zassert_true((return_value ==
			      timeout_elements[index].return_value),
			     "Return value of k_pipe_get mismatch at index = %d expected =%d received = %d\n",
			     index, timeout_elements[index].return_value,
			     return_value);

		zassert_true((read == timeout_elements[index].sent_bytes),
			     "Bytes read mismatch read is %d but expected is %d index = %d\n",
			     read, timeout_elements[index].sent_bytes, index);


	}

}

/******************************************************************************/
ZTEST_BMEM bool valid_fault;
void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	if (valid_fault) {
		valid_fault = false; /* reset back to normal */
		ztest_test_pass();
	} else {
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}
/******************************************************************************/
/* Test case entry points */

/**
 * @brief Verify pipe with 1 element insert.
 *
 * @ingroup kernel_pipe_tests
 *
 * @details
 * Test Objective:
 * - transfer sequences of bytes of data in whole.
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TEST_USERSPACE.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define and initialize a pipe at compile time.
 * -# Using a sub thread to get pipe data in whole,
 * and check if the data is correct with expected.
 * -# Using main thread to put data in whole,
 * check if the return is correct with expected.
 *
 * Expected Test Result:
 * - The pipe put/get whole data is correct.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_pipe_put(), k_pipe_get()
 */
ZTEST_USER(pipe, test_pipe_on_single_elements)
{
	/* initialize the tx buffer */
	for (int i = 0; i < sizeof(tx_buffer); i++) {
		tx_buffer[i] = i;
	}

	k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			pipe_get_single, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			K_NO_WAIT);

	pipe_put_single();
	k_sem_take(&sync_sem, K_FOREVER);
	k_thread_abort(&get_single_tid);
	ztest_test_pass();
}

/**
 * @brief Test when multiple items are present in the pipe
 * @details transfer sequences of bytes of data in part.
 * - Using a sub thread to get data part.
 * - Using main thread to put data part by part
 * @ingroup kernel_pipe_tests
 * @see k_pipe_put()
 */
ZTEST_USER(pipe, test_pipe_on_multiple_elements)
{
	k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			pipe_get_multiple, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			K_NO_WAIT);

	pipe_put_multiple();
	k_sem_take(&sync_sem, K_FOREVER);
	k_thread_abort(&get_single_tid);
	ztest_test_pass();
}

/**
 * @brief Test when multiple items are present with wait
 * @ingroup kernel_pipe_tests
 * @see k_pipe_put()
 */
ZTEST_USER(pipe, test_pipe_forever_wait)
{
	k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			pipe_get_forever_wait, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			K_NO_WAIT);

	pipe_put_forever_wait();
	k_sem_take(&sync_sem, K_FOREVER);
	k_thread_abort(&get_single_tid);
	ztest_test_pass();
}

/**
 * @brief Test pipes with timeout
 *
 * @ingroup kernel_pipe_tests
 *
 * @details
 * Test Objective:
 *	- Check if the kernel support supplying a timeout parameter
 * indicating the maximum amount of time a process will wait.
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TEST_USERSPACE.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Create a sub thread to get data from a pipe.
 * -# In the sub thread, Set K_MSEC(10) as timeout for k_pipe_get().
 * and check the data which get from pipe if correct.
 * -# In main thread, use k_pipe_put to put data.
 * and check the return of k_pipe_put.
 *
 * Expected Test Result:
 * - The pipe can set timeout and works well.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_pipe_put()
 */
ZTEST_USER(pipe, test_pipe_timeout)
{
	k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			pipe_get_timeout, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			K_NO_WAIT);

	pipe_put_timeout();
	k_sem_take(&sync_sem, K_FOREVER);
	k_thread_abort(&get_single_tid);
	ztest_test_pass();
}

/**
 * @brief Test pipe get from a empty pipe
 * @ingroup kernel_pipe_tests
 * @see k_pipe_get()
 */
ZTEST_USER(pipe, test_pipe_get_on_empty_pipe)
{
	pipe_get_on_empty_pipe();
	ztest_test_pass();
}

/**
 * @brief Test the pipe_get with K_FOREVER as timeout.
 * @details Testcase is similar to test_pipe_on_single_elements()
 * but with K_FOREVER as timeout.
 * @ingroup kernel_pipe_tests
 * @see k_pipe_put()
 */
ZTEST_USER(pipe, test_pipe_forever_timeout)
{
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(0));

	k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			pipe_get_forever_timeout, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			K_NO_WAIT);

	pipe_put_forever_timeout();
	k_sem_take(&sync_sem, K_FOREVER);
	ztest_test_pass();
}

/**
 * @brief k_pipe_get timeout test
 * @ingroup kernel_pipe_tests
 * @see k_pipe_get()
 */
ZTEST_USER(pipe, test_pipe_get_timeout)
{
	pipe_put_get_timeout();

	ztest_test_pass();
}

/**
 * @brief Test pipe get of invalid size
 * @ingroup kernel_pipe_tests
 * @see k_pipe_get()
 */
ZTEST_USER(pipe, test_pipe_get_invalid_size)
{
	size_t read;
	int ret;

	valid_fault = true;
	ret = k_pipe_get(&test_pipe, &rx_buffer,
		   0, &read,
		   1, TIMEOUT_200MSEC);

	zassert_equal(ret, -EINVAL,
		      "fault didn't occur for min_xfer <= bytes_to_read");
}

/**
 * @brief Test pipe get returns immediately if >= min_xfer is available
 * @ingroup kernel_pipe_tests
 * @see k_pipe_get()
 */
ZTEST_USER(pipe, test_pipe_get_min_xfer)
{
	int res;
	size_t bytes_written = 0;
	size_t bytes_read = 0;
	char buf[8] = {};

	res = k_pipe_put(&test_pipe, "Hi!", 3, &bytes_written,
			 3 /* min_xfer */, K_FOREVER);
	zassert_equal(res, 0, "did not write entire message");
	zassert_equal(bytes_written, 3, "did not write entire message");

	res = k_pipe_get(&test_pipe, buf, sizeof(buf), &bytes_read,
			 1 /* min_xfer */, K_FOREVER);
	zassert_equal(res, 0, "did not read at least one byte");
	zassert_equal(bytes_read, 3, "did not read all bytes available");
}

/**
 * @brief Test pipe put returns immediately if >= min_xfer is available
 * @ingroup kernel_pipe_tests
 * @see k_pipe_put()
 */
ZTEST_USER(pipe, test_pipe_put_min_xfer)
{
	int res;
	size_t bytes_written = 0;

	/* write 6 bytes into the pipe, so that 2 bytes are still free */
	for (size_t i = 0; i < 2; ++i) {
		bytes_written = 0;
		res = k_pipe_put(&test_pipe, "Hi!", 3, &bytes_written,
				 3 /* min_xfer */, K_FOREVER);
		zassert_equal(res, 0, "did not write entire message");
		zassert_equal(bytes_written, 3, "did not write entire message");
	}

	/* attempt to write 3 bytes, but allow success if >= 1 byte */
	bytes_written = 0;
	res = k_pipe_put(&test_pipe, "Hi!", 3, &bytes_written,
			 1 /* min_xfer */, K_FOREVER);
	zassert_equal(res, 0, "did not write min_xfer");
	zassert_true(bytes_written >= 1, "did not write min_xfer");

	/* flush the pipe so other test can write to this pipe */
	k_pipe_flush(&test_pipe);
}

/**
 * @brief Test defining and initializing pipes at run time
 *
 * @ingroup kernel_pipe_tests
 *
 * @details
 * Test Objective:
 * - Check if the kernel provided a mechanism for defining and
 * initializing pipes at run time.
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TEST_USERSPACE.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define and initialize a pipe at run time
 * -# Using this pipe to transfer data.
 * -# Check the pipe get/put operation.
 *
 * Expected Test Result:
 * - Pipe can be defined and initialized at run time.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_pipe_init()
 */
ZTEST(pipe, test_pipe_define_at_runtime)
{
	unsigned char ring_buffer[PIPE_SIZE];
	struct k_pipe pipe;
	size_t written, read;

	/*Define and initialize pipe at run time*/
	k_pipe_init(&pipe, ring_buffer, sizeof(ring_buffer));

	/* initialize the tx buffer */
	for (int i = 0; i < sizeof(tx_buffer); i++) {
		tx_buffer[i] = i;
	}

	/*Using test_pipe which define and initialize at run time*/
	zassert_equal(k_pipe_put(&pipe, &tx_buffer,
			PIPE_SIZE, &written,
			PIPE_SIZE, K_NO_WAIT), RETURN_SUCCESS, NULL);

	/* Returned without waiting; zero data bytes were written. */
	zassert_equal(k_pipe_put(&pipe, &tx_buffer,
			PIPE_SIZE, &written,
			PIPE_SIZE, K_NO_WAIT), -EIO, NULL);

	/* Waiting period timed out. */
	zassert_equal(k_pipe_put(&pipe, &tx_buffer,
			PIPE_SIZE, &written,
			PIPE_SIZE, TIMEOUT_VAL), -EAGAIN, NULL);

	zassert_equal(k_pipe_get(&pipe, &rx_buffer,
			PIPE_SIZE, &read,
			PIPE_SIZE, K_NO_WAIT), RETURN_SUCCESS, NULL);

	zassert_true(rx_buffer_check(rx_buffer, read) == read,
			"Bytes read are not match.");

	/* Returned without waiting; zero data bytes were read. */
	zassert_equal(k_pipe_get(&pipe, &rx_buffer,
			PIPE_SIZE, &read,
			PIPE_SIZE, K_NO_WAIT), -EIO, NULL);
	/* Waiting period timed out. */
	zassert_equal(k_pipe_get(&pipe, &rx_buffer,
			PIPE_SIZE, &read,
			PIPE_SIZE, TIMEOUT_VAL), -EAGAIN, NULL);
}

/**
 * @brief Helper thread to test k_pipe_flush() and k_pipe_buffer_flush()
 *
 * This helper thread attempts to write 50 bytes to the pipe identified by
 * [p1], which has an internal buffer size of 10. This helper thread is
 * expected to fill the internal buffer, and then block until it can complete
 * the write.
 */
void test_pipe_flush_helper(void *p1, void *p2, void *p3)
{
	struct k_pipe *pipe = (struct k_pipe *)p1;
	char    buffer[50];
	size_t  i;
	size_t  bytes_written;
	int     rv;

	for (i = 0; i < sizeof(buffer); i++) {
		buffer[i] = i;
	}

	rv = k_pipe_put(pipe, buffer, sizeof(buffer), &bytes_written,
			sizeof(buffer), K_FOREVER);

	zassert_true(rv == 0, "k_pipe_put() failed with %d", rv);
	zassert_true(bytes_written == sizeof(buffer),
		     "Expected %zu bytes written, not %zu",
		     sizeof(buffer), bytes_written);
}

/**
 * @brief Test flushing a pipe
 *
 * @ingroup kernel_pipe_tests
 *
 * @details
 * Test Objective:
 * - Check if the kernel flushes a pipe properly at runtime.
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TEST_USERSPACE.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define and initialize a pipe at run time
 * -# Use this pipe to transfer data.
 * -# Have a thread fill and block on writing to the pipe
 * -# Flush the pipe and check that the pipe is completely empty
 * -# Have a thread fill and block on writing to the pipe
 * -# Flush only the pipe's buffer and check the results
 *
 * Expected Test Result:
 * - Reading from the pipe after k_pipe_flush() results in no data to read.
 * - Reading from the pipe after k_pipe_buffer_flush() results in read data,
 *   but missing the original data that was in the buffer prior to the flush.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise
 * failure.
 *
 * Assumptions and Constraints:
 * - N/A
 */
ZTEST(pipe, test_pipe_flush)
{
	unsigned char  results_buffer[50];
	size_t  bytes_read;
	size_t  i;
	int     rv;

	memset(results_buffer, 0, sizeof(results_buffer));

	(void)k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			      test_pipe_flush_helper, &small_pipe, NULL, NULL,
			      K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			      K_NO_WAIT);

	k_sleep(K_MSEC(50));        /* give helper thread time to execute */

	/* Completely flush the pipe */

	k_pipe_flush(&small_pipe);

	rv = k_pipe_get(&small_pipe, results_buffer, sizeof(results_buffer),
			&bytes_read, 0, K_MSEC(100));

	zassert_true(rv == 0, "k_pipe_get() failed with %d\n", rv);
	zassert_true(bytes_read == 0,
		     "k_pipe_get() unexpectedly read %zu bytes\n", bytes_read);

	rv = k_thread_join(&get_single_tid, K_MSEC(50));
	zassert_true(rv == 0, "Wait for helper thread failed (%d)", rv);

	(void)k_thread_create(&get_single_tid, stack_1, STACK_SIZE,
			      test_pipe_flush_helper, &small_pipe, NULL, NULL,
			      K_PRIO_PREEMPT(0), K_INHERIT_PERMS | K_USER,
			      K_NO_WAIT);

	k_sleep(K_MSEC(50));        /* give helper thread time to execute */

	/*
	 * Only flush the pipe's buffer. This is expected to leave 40 bytes
	 * left to receive.
	 */

	k_pipe_buffer_flush(&small_pipe);

	rv = k_pipe_get(&small_pipe, results_buffer, sizeof(results_buffer),
			&bytes_read, 0, K_MSEC(100));

	zassert_true(rv == 0, "k_pipe_get() failed with %d\n", rv);
	zassert_true(bytes_read == 40,
		     "k_pipe_get() unexpectedly read %zu bytes\n", bytes_read);
	for (i = 0; i < 40; i++) {
		zassert_true(results_buffer[i] == (unsigned char)(i + 10),
			     "At offset %zd, expected byte %02x, not %02x\n",
			     i, (i + 10), results_buffer[i]);
	}

	rv = k_thread_join(&get_single_tid, K_MSEC(50));
	zassert_true(rv == 0, "Wait for helper thread failed (%d)", rv);
}
