/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test microkernel target pipe APIs
 *
 * This modules tests the following target pipe routines:
 *
 *    task_pipe_put()
 *    task_pipe_get()
 *
 * The following target pipe routine does not yet have a test case:
 *    task_pipe_block_put()
 */

#include <zephyr.h>
#include <tc_util.h>
#include <misc/util.h>

#define  ONE_SECOND     (sys_clock_ticks_per_sec)

#define  IRQ_PRIORITY   3

#define  PIPE_SIZE  256    /* This must match the value in the MDEF file */

typedef struct {
	int  size;                 /* number of bytes to send/receive */
	K_PIPE_OPTION  options;    /* options for task_pipe_XXX() APIs */
	int  sent;                 /* expected # of bytes sent */
	int  rcode;                /* expected return code */
} SIZE_EXPECT;

static char txBuffer[PIPE_SIZE + 32];
static char rxBuffer[PIPE_SIZE + 32];

static SIZE_EXPECT  all_N[] = {
		{0, _ALL_N, 0, RC_FAIL},
		{1, _ALL_N, 1, RC_OK},
		{PIPE_SIZE - 1, _ALL_N, PIPE_SIZE - 1, RC_OK},
		{PIPE_SIZE, _ALL_N, PIPE_SIZE, RC_OK},
		{PIPE_SIZE + 1, _ALL_N, 0, RC_FAIL}
	};

static SIZE_EXPECT  many_all_N[] = {
		{PIPE_SIZE / 3, _ALL_N, PIPE_SIZE / 3, RC_OK,},
		{PIPE_SIZE / 3, _ALL_N, PIPE_SIZE / 3, RC_OK,},
		{PIPE_SIZE / 3, _ALL_N, PIPE_SIZE / 3, RC_OK,},
		{PIPE_SIZE / 3, _ALL_N, 0, RC_FAIL}
	};

static SIZE_EXPECT  one_to_N[] = {
		{0, _1_TO_N, 0, RC_FAIL},
		{1, _1_TO_N, 1, RC_OK},
		{PIPE_SIZE - 1, _1_TO_N, PIPE_SIZE - 1, RC_OK},
		{PIPE_SIZE, _1_TO_N, PIPE_SIZE, RC_OK},
		{PIPE_SIZE + 1, _1_TO_N, PIPE_SIZE, RC_OK}
	};

static SIZE_EXPECT  many_one_to_N[] = {
		{PIPE_SIZE / 3, _1_TO_N, PIPE_SIZE / 3, RC_OK},
		{PIPE_SIZE / 3, _1_TO_N, PIPE_SIZE / 3, RC_OK},
		{PIPE_SIZE / 3, _1_TO_N, PIPE_SIZE / 3, RC_OK},
		{PIPE_SIZE / 3, _1_TO_N, 1, RC_OK},
		{PIPE_SIZE / 3, _1_TO_N, 0, RC_FAIL}
	};

static SIZE_EXPECT  zero_to_N[] = {
		{0, _0_TO_N, 0, RC_FAIL},
		{1, _0_TO_N, 1, RC_OK},
		{PIPE_SIZE - 1, _0_TO_N, PIPE_SIZE - 1, RC_OK},
		{PIPE_SIZE, _0_TO_N, PIPE_SIZE, RC_OK},
		{PIPE_SIZE + 1, _0_TO_N, PIPE_SIZE, RC_OK}
	};

static SIZE_EXPECT  many_zero_to_N[] = {
		{PIPE_SIZE / 3, _0_TO_N, PIPE_SIZE / 3, RC_OK},
		{PIPE_SIZE / 3, _0_TO_N, PIPE_SIZE / 3, RC_OK},
		{PIPE_SIZE / 3, _0_TO_N, PIPE_SIZE / 3, RC_OK},
		{PIPE_SIZE / 3, _0_TO_N, 1, RC_OK},
		{PIPE_SIZE / 3, _0_TO_N, 0, RC_OK}
	};

/*
 * With the following 'wait' cases, the pipe buffer may be bypassed.  It is
 * thus possible to transmit and receive via the pipe more bytes than its
 * buffer would allow.
 */

static SIZE_EXPECT  wait_all_N[] = {
		{0, _ALL_N, 0, RC_FAIL},
		{1, _ALL_N, 1, RC_OK},
		{PIPE_SIZE - 1, _ALL_N, PIPE_SIZE - 1, RC_OK},
		{PIPE_SIZE, _ALL_N, PIPE_SIZE, RC_OK},
		{PIPE_SIZE + 1, _ALL_N, PIPE_SIZE + 1, RC_OK}
	};

static SIZE_EXPECT  wait_one_to_N[] = {
		{0, _1_TO_N, 0, RC_FAIL},
		{1, _1_TO_N, 1, RC_OK},
		{PIPE_SIZE - 1, _1_TO_N, PIPE_SIZE - 1, RC_OK},
		{PIPE_SIZE, _1_TO_N, PIPE_SIZE, RC_OK},
		{PIPE_SIZE + 1, _1_TO_N, PIPE_SIZE + 1, RC_OK}
	};

static SIZE_EXPECT  timeout_cases[] = {
		{0, _ALL_N, 0, RC_FAIL},
		{1, _ALL_N, 0, RC_TIME},
		{PIPE_SIZE - 1, _ALL_N, 0 , RC_TIME},
		{PIPE_SIZE, _ALL_N, 0, RC_TIME},
		{PIPE_SIZE + 1, _ALL_N, 0, RC_TIME},
		{0, _1_TO_N, 0, RC_FAIL},
		{1, _1_TO_N, 0, RC_TIME},
		{PIPE_SIZE - 1, _1_TO_N, 0, RC_TIME},
		{PIPE_SIZE, _1_TO_N, 0, RC_TIME},
		{PIPE_SIZE + 1, _1_TO_N, 0, RC_TIME},
		{0, _0_TO_N, 0, RC_FAIL},
		{1, _0_TO_N, 0, RC_FAIL},
		{PIPE_SIZE - 1, _0_TO_N, 0, RC_FAIL},
		{PIPE_SIZE, _0_TO_N, 0, RC_FAIL},
		{PIPE_SIZE + 1, _0_TO_N, 0, RC_FAIL}
	};

extern ksem_t regSem;
extern ksem_t altSem;
extern ksem_t counterSem;

extern kpipe_t pipeId _GENERIC_SECTION(_k_pipe_ptr);

/**
 *
 * @brief Initialize objects used in this microkernel test suite
 *
 * @return N/A
 */

void microObjectsInit(void)
{
	int  i;    /* loop counter */

	for (i = 0; i < sizeof(rxBuffer); i++) {
		txBuffer[i] = (char) i;
	}
}

/**
 *
 * @brief Check the contents of the receive buffer
 *
 * @param buffer    pointer to buffer to check
 * @param size      number of bytes to check
 *
 * @return <size> on success, index of wrong character on failure
 */

int receiveBufferCheck(char *buffer, int size)
{
	int  j;    /* loop counter */

	for (j = 0; j < size; j++) {
		if (buffer[j] != (char) j) {
			return j;
		}
	}

	return size;
}

/**
 *
 * @brief Helper routine to pipePutTest()
 *
 * @param singleItems    testcase list (one item in the pipe)
 * @param nSingles       number of items in testcase
 * @param manyItems      testcase list (many items in the pipe)
 * @param nMany          number of items in testcase
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutHelperWork(SIZE_EXPECT *singleItems, int nSingles,
					  SIZE_EXPECT *manyItems, int nMany)
{
	int  i;                      /* loop counter */
	int  j;                      /* loop counter */
	int  rv;                     /* value returned from task_pipe_get() */
	int  index;                  /* index to corrupted byte in buffer */
	int  bytesReceived;          /* number of bytes received */

	for (i = 0; i < nSingles; i++) {
		(void)task_sem_take(altSem, TICKS_UNLIMITED);
		for (j = 0; j < sizeof(rxBuffer); j++) {
			rxBuffer[j] = 0;
		}

		rv = task_pipe_get(pipeId, rxBuffer, singleItems[i].size,
						   &bytesReceived, singleItems[i].options,
						   TICKS_NONE);
		if (rv != singleItems[i].rcode) {
			TC_ERROR("task_pipe_get(%d bytes) : Expected %d not %d.\n"
					 "    bytesReceived = %d\n",
					 singleItems[i].size, singleItems[i].rcode,
					 rv, bytesReceived);
			return TC_FAIL;
		}

		if (bytesReceived != singleItems[i].sent) {
			TC_ERROR("task_pipe_get(%d) : "
					 "Expected %d bytes to be received, not %d\n",
					 singleItems[i].size, singleItems[i].sent, bytesReceived);
			return TC_FAIL;
		}

		index = receiveBufferCheck(rxBuffer, bytesReceived);
		if (index != bytesReceived) {
			TC_ERROR("pipePutHelper: rxBuffer[%d] is %d, not %d\n",
					 index, rxBuffer[index], index);
			return TC_FAIL;
		}

		task_sem_give(counterSem);
		task_sem_give(regSem);
	}

	/*
	 * Get items from the pipe.  There should be more than one item
	 * stored in it.
	 */

	(void)task_sem_take(altSem, TICKS_UNLIMITED);

	for (i = 0; i < nMany; i++) {
		for (j = 0; j < sizeof(rxBuffer); j++) {
			rxBuffer[j] = 0;
		}

		rv = task_pipe_get(pipeId, rxBuffer, manyItems[i].size,
						   &bytesReceived, manyItems[i].options,
						   TICKS_NONE);

		if (rv != manyItems[i].rcode) {
			TC_ERROR("task_pipe_get(%d bytes) : Expected %d not %d.\n"
					 "    bytesReceived = %d, iteration: %d\n",
					 manyItems[i].size, manyItems[i].rcode,
					 rv, bytesReceived, i+1);
			return TC_FAIL;
		}

		if (bytesReceived != manyItems[i].sent) {
			TC_ERROR("task_pipe_get(%d) : "
					 "Expected %d bytes to be received, not %d\n",
					 manyItems[i].size, manyItems[i].sent, bytesReceived);
			return TC_FAIL;
		}

		index = receiveBufferCheck(rxBuffer, bytesReceived);
		if (index != bytesReceived) {
			TC_ERROR("pipeGetHelper: rxBuffer[%d] is %d, not %d\n",
					 index, rxBuffer[index], index);
			return TC_FAIL;
		}

		task_sem_give(counterSem);
	}

	task_sem_give(regSem);   /* Wake the RegressionTask */

	return TC_PASS;
}

/**
 *
 * @brief Helper routine to pipePutTest()
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutHelper(void)
{
	int  rv;   /* return value from pipePutHelperWork() */

	rv = pipePutHelperWork(all_N, ARRAY_SIZE(all_N),
						   many_all_N, ARRAY_SIZE(many_all_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on all_N/many_all_N test\n");
		return TC_FAIL;
	}

	rv = pipePutHelperWork(one_to_N, ARRAY_SIZE(one_to_N),
						   many_one_to_N, ARRAY_SIZE(many_one_to_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _1_TO_N/many_1_TO_N test\n");
		return TC_FAIL;
	}

	rv = pipePutHelperWork(zero_to_N, ARRAY_SIZE(zero_to_N),
						   many_zero_to_N, ARRAY_SIZE(many_zero_to_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _0_TO_N/many_0_TO_N test\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_put(TICKS_NONE)
 *
 * This routine tests the task_pipe_put(TICKS_NONE) API.
 *
 * @param singleItems    testcase list (one item in the pipe)
 * @param nSingles       number of items in testcase
 * @param manyItems      testcase list (many items in the pipe)
 * @param nMany          number of items in testcase
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutTestWork(SIZE_EXPECT *singleItems, int nSingles,
					SIZE_EXPECT *manyItems, int nMany)
{
	int  rv;                     /* return code from task_pipe_put() */
	int  i;                      /* loop counter */
	int  bytesWritten;           /* # of bytes sent to pipe */
	int nitem;           /* expected item number processed by the other task */

	task_sem_reset(counterSem);

	for (i = 0; i < nSingles; i++) {
		rv = task_pipe_put(pipeId, txBuffer, singleItems[i].size,
						   &bytesWritten, singleItems[i].options,
						   TICKS_NONE);
		if (rv != singleItems[i].rcode) {
			TC_ERROR("task_pipe_put(%d) : Expected %d not %d.\n"
					 "    bytesWritten = %d, Iteration: %d\n",
					 singleItems[i].size, singleItems[i].rcode,
					 rv, bytesWritten, i + 1);
			return TC_FAIL;
		}

		if (bytesWritten != singleItems[i].sent) {
			TC_ERROR("task_pipe_put(%d) : "
					 "Expected %d bytes to be written, not %d\n",
					 singleItems[i].size, singleItems[i].sent, bytesWritten);
			return TC_FAIL;
		}

		task_sem_give(altSem);
		(void)task_sem_take(regSem, TICKS_UNLIMITED);

		nitem = task_sem_count_get(counterSem) - 1;
		if (nitem != i) {
			TC_ERROR("Expected item number is %d, not %d\n",
					 i, nitem);
			return TC_FAIL;
		}
	}

	/* This time, more than one item will be in the pipe at a time */

	task_sem_reset(counterSem);

	for (i = 0; i < nMany; i++) {
		rv = task_pipe_put(pipeId, txBuffer, manyItems[i].size,
						   &bytesWritten, manyItems[i].options, TICKS_NONE);
		if (rv != manyItems[i].rcode) {
			TC_ERROR("task_pipe_put(%d) : Expected %d not %d.\n"
					 "    bytesWritten = %d, iteration: %d\n",
					 manyItems[i].size, manyItems[i].rcode,
					 rv, bytesWritten, i + 1);
			return TC_FAIL;
		}

		if (bytesWritten != manyItems[i].sent) {
			TC_ERROR("task_pipe_put(%d) : "
					 "Expected %d bytes to be written, not %d\n",
					 manyItems[i].size, manyItems[i].sent, bytesWritten);
			return TC_FAIL;
		}
	}

	task_sem_give(altSem);   /* Wake the alternate task */
	/* wait for other task reading all the items from pipe */
	(void)task_sem_take(regSem, TICKS_UNLIMITED);

	if (task_sem_count_get(counterSem) != nMany) {
		TC_ERROR("Expected number of items %d, not %d\n",
				 nMany, task_sem_count_get(counterSem));
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_put(TICKS_NONE)
 *
 * This routine tests the task_pipe_put(TICKS_NONE) API.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutTest(void)
{
	int  rv;    /* return value from pipePutTestWork() */

	rv = pipePutTestWork(all_N, ARRAY_SIZE(all_N),
						 many_all_N, ARRAY_SIZE(many_all_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _ALL_N/many_ALL_N test\n");
		return TC_FAIL;
	}

	rv = pipePutTestWork(one_to_N, ARRAY_SIZE(one_to_N),
						 many_one_to_N, ARRAY_SIZE(many_one_to_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _1_TO_N/many_1_TO_N test\n");
		return TC_FAIL;
	}

	rv = pipePutTestWork(zero_to_N, ARRAY_SIZE(zero_to_N),
						 many_zero_to_N, ARRAY_SIZE(many_zero_to_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _0_TO_N/many_0_TO_N test\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Help test task_pipe_put(TICKS_UNLIMITED)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutWaitHelper(void)
{
	int  i;           /* loop counter */
	int  rv;          /* return value from task_pipe_get*/
	int  bytesRead;   /* # of bytes read from task_pipe_get() */

	/* Wait until test is ready */
	(void)task_sem_take(altSem, TICKS_UNLIMITED);

	/*
	 * 1. task_pipe_get(TICKS_UNLIMITED) will force a context
	 * switch to RegressionTask().
	 */
	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE,
						&bytesRead, _ALL_N, TICKS_UNLIMITED);
	if ((rv != RC_OK) || (bytesRead != PIPE_SIZE)) {
		TC_ERROR("Expected return code %d, not %d\n"
				 "Expected %d bytes to be read, not %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesRead);
		return TC_FAIL;
	}

	/*
	 * 2. task_pipe_get(TICKS_UNLIMITED) will force a context
	 * switch to RegressionTask().
	 */
	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE,
						&bytesRead, _1_TO_N, TICKS_UNLIMITED);
	if ((rv != RC_OK) || (bytesRead != PIPE_SIZE)) {
		TC_ERROR("Expected return code %d, not %d\n"
				 "Expected %d bytes to be read, not %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesRead);
		return TC_FAIL;
	}

	/*
	 * Before emptying the pipe, check that task_pipe_get(TICKS_UNLIMITED)
	 * fails when using the _0_TO_N option.
	 */

	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE / 2,
						&bytesRead, _0_TO_N, TICKS_UNLIMITED);
	if (rv != RC_FAIL) {
		TC_ERROR("Expected return code %d, not %d\n", RC_FAIL, rv);
		return TC_FAIL;
	}

	/* 3. Empty the pipe in two reads */
	for (i = 0; i < 2; i++) {
		rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE / 2,
						   &bytesRead, _0_TO_N, TICKS_NONE);
		if ((rv != RC_OK) || (bytesRead != PIPE_SIZE / 2)) {
			TC_ERROR("Expected return code %d, not %d\n"
					 "Expected %d bytes to be read, not %d\n",
					 RC_OK, rv, PIPE_SIZE / 2, bytesRead);
			return TC_FAIL;
		}
	}

	task_sem_give(regSem);

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_put(TICKS_UNLIMITED)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutWaitTest(void)
{
	int  rv;            /* return code from task_pipe_put() */
	int  bytesWritten;  /* # of bytes written to pipe */

	/* 1. Fill the pipe */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
							&bytesWritten, _ALL_N, TICKS_UNLIMITED);
	if ((rv != RC_OK) || (bytesWritten != PIPE_SIZE)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesWritten);
		return TC_FAIL;
	}

	task_sem_give(altSem);    /* Wake the alternate task */

	/* 2. task_pipe_put() will force a context switch to AlternateTask(). */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _ALL_N, TICKS_UNLIMITED);
	if ((rv != RC_OK) || (bytesWritten != PIPE_SIZE)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesWritten);
		return TC_FAIL;
	}

	/* 3. task_pipe_put() will force a context switch to AlternateTask(). */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _1_TO_N, TICKS_UNLIMITED);
	if ((rv != RC_OK) || (bytesWritten != PIPE_SIZE)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesWritten);
		return TC_FAIL;
	}

	/* This should return immediately as _0_TO_N with a wait is an error. */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _0_TO_N, TICKS_UNLIMITED);
	if ((rv != RC_FAIL) || (bytesWritten != 0)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_FAIL, rv, 0, bytesWritten);
		return TC_FAIL;
	}

	/* Wait for AlternateTask()'s pipePutWaitHelper() to finish */
	(void)task_sem_take(regSem, TICKS_UNLIMITED);

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_get(timeout)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutTimeoutHelper(void)
{
	int  i;         /* loop counter */
	int  rv;        /* return value from task_pipe_get() */
	int  bytesRead; /* # of bytes read from task_pipe_get() */

	/* Wait until test is ready */
	(void)task_sem_take(altSem, TICKS_UNLIMITED);

	/*
	 * 1. task_pipe_get(timeout) will force a context
	 * switch to RegressionTask()
	 */
	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE,
						&bytesRead, _ALL_N, ONE_SECOND);
	if ((rv != RC_OK) || (bytesRead != PIPE_SIZE)) {
		TC_ERROR("Expected return code %d, not %d\n"
				 "Expected %d bytes to be read, not %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesRead);
		return TC_FAIL;
	}

	/*
	 * 2. task_pipe_get(timeout) will force a context
	 * switch to RegressionTask().
	 */
	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE,
						&bytesRead, _1_TO_N, ONE_SECOND);
	if ((rv != RC_OK) || (bytesRead != PIPE_SIZE)) {
		TC_ERROR("Expected return code %d, not %d\n"
				 "Expected %d bytes to be read, not %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesRead);
		return TC_FAIL;
	}

	/*
	 * Before emptying the pipe, check that task_pipe_get(timeout) fails when
	 * using the _0_TO_N option.
	 */

	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE / 2,
						&bytesRead, _0_TO_N, ONE_SECOND);
	if (rv != RC_FAIL) {
		TC_ERROR("Expected return code %d, not %d\n", RC_FAIL, rv);
		return TC_FAIL;
	}

	/* 3. Empty the pipe in two reads */
	for (i = 0; i < 2; i++) {
		rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE / 2,
						   &bytesRead, _0_TO_N, TICKS_NONE);
		if ((rv != RC_OK) || (bytesRead != PIPE_SIZE / 2)) {
			TC_ERROR("Expected return code %d, not %d\n"
					 "Expected %d bytes to be read, not %d\n",
					 RC_OK, rv, PIPE_SIZE / 2, bytesRead);
			return TC_FAIL;
		}
	}

	task_sem_give(regSem);

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_put(timeout)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipePutTimeoutTest(void)
{
	int  rv;             /* return code from task_pipe_put() */
	int  bytesWritten;   /* # of bytes written to task_pipe_put() */

	/* 1. Fill the pipe */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _ALL_N, ONE_SECOND);
	if ((rv != RC_OK) || (bytesWritten != PIPE_SIZE)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesWritten);
		return TC_FAIL;
	}

	/* Timeout while waiting to put data into the pipe */

	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _ALL_N, ONE_SECOND);
	if ((rv != RC_TIME) || (bytesWritten != 0)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_TIME, rv, 0, bytesWritten);
		return TC_FAIL;
	}

	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _1_TO_N, ONE_SECOND);
	if ((rv != RC_TIME) || (bytesWritten != 0)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_TIME, rv, 0, bytesWritten);
		return TC_FAIL;
	}

	task_sem_give(altSem);    /* Wake the alternate task */

	/* 2. task_pipe_put() will force a context switch to AlternateTask(). */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _ALL_N, ONE_SECOND);
	if ((rv != RC_OK) || (bytesWritten != PIPE_SIZE)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesWritten);
		return TC_FAIL;
	}

	/* 3. task_pipe_put() will force a context switch to AlternateTask(). */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _1_TO_N, ONE_SECOND);
	if ((rv != RC_OK) || (bytesWritten != PIPE_SIZE)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_OK, rv, PIPE_SIZE, bytesWritten);
		return TC_FAIL;
	}

	/* This should return immediately as _0_TO_N with a wait is an error. */
	rv = task_pipe_put(pipeId, txBuffer, PIPE_SIZE,
						&bytesWritten, _0_TO_N, TICKS_UNLIMITED);
	if ((rv != RC_FAIL) || (bytesWritten != 0)) {
		TC_ERROR("Return code: expected %d, got %d\n"
				 "Bytes written: expected %d, got %d\n",
				 RC_FAIL, rv, 0, bytesWritten);
		return TC_FAIL;
	}

	/* Wait for AlternateTask()'s pipePutWaitHelper() to finish */
	(void)task_sem_take(regSem, TICKS_UNLIMITED);

	return TC_PASS;
}

/**
 *
 * @brief Routine to test task_pipe_get(TICKS_NONE)
 *
 * This routine tests the task_pipe_get(TICKS_NONE) API.  Some of this
 * functionality has already been tested while testing task_pipe_put().  As
 * a result, the only remaining functionality that needs to be checked are
 * attempts to get data from an empty pipe.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipeGetTest(void)
{
	int  i;         /* loop counter */
	int  j;         /* loop counter */
	int  rv;        /* return code from task_pipe_get() */
	int  bytesRead; /* # of bytes read from task_pipe_get() */
	int  size[] ={1, PIPE_SIZE - 1, PIPE_SIZE, PIPE_SIZE + 1};
	K_PIPE_OPTION  options[] = {_ALL_N, _1_TO_N};

	for (j = 0; j < ARRAY_SIZE(options); j++) {
		for (i = 0; i < ARRAY_SIZE(size); i++) {
			rv = task_pipe_get(pipeId, rxBuffer, size[i],
							   &bytesRead, options[j], TICKS_NONE);
			if (rv != RC_FAIL) {
				TC_ERROR("Expected return code %d, not %d\n", RC_FAIL, rv);
				return TC_FAIL;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(size); i++) {
		rv = task_pipe_get(pipeId, rxBuffer, size[i],
						   &bytesRead, _0_TO_N, TICKS_NONE);
		if (rv != RC_OK) {
			TC_ERROR("Expected return code %d, not %d\n", RC_OK, rv);
			return TC_FAIL;
		}

		if (bytesRead != 0) {
			TC_ERROR("Expected <bytesRead> %d, not %d\n", 0, bytesRead);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_get(TICKS_UNLIMITED)
 *
 * @param items     testcase list for task_pipe_get(TICKS_UNLIMITED)
 * @param nItems    number of items in list
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipeGetWaitHelperWork(SIZE_EXPECT *items, int nItems)
{
	int  i;          /* loop counter */
	int  rv;         /* return value from task_pipe_put() */
	int  bytesSent;  /* # of bytes sent to task_pipe_put() */

	for (i = 0; i < nItems; i++) {
		/*
		 * Pipe should be empty.  Most calls to task_pipe_get(TICKS_UNLIMITED)
		 * should block until the call to task_pipe_put() is performed in the
		 * routine pipeGetWaitHelperWork().
		 */

		bytesSent = 0;
		rv = task_pipe_put(pipeId, rxBuffer, items[i].size,
						   &bytesSent, items[i].options,
						   TICKS_UNLIMITED);
		if ((rv != items[i].rcode) || (bytesSent != items[i].sent)) {
			TC_ERROR("Expected return value %d, got %d\n"
					 "Expected bytesSent = %d, got %d\n",
					 items[i].rcode, rv, 0, bytesSent);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_get(TICKS_UNLIMITED)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipeGetWaitHelper(void)
{
	int  rv;    /* return coded form pipeGetWaitHelperWork() */

	(void)task_sem_take(altSem, TICKS_UNLIMITED);

	rv = pipeGetWaitHelperWork(wait_all_N, ARRAY_SIZE(wait_all_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _ALL_N test\n");
		return TC_FAIL;
	}

	rv = pipeGetWaitHelperWork(wait_one_to_N, ARRAY_SIZE(wait_one_to_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _1_TO_N test\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_get(TICKS_UNLIMITED)
 *
 * @param items     testcase list for task_pipe_get(TICKS_UNLIMITED)
 * @param nItems    number of items in list
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipeGetWaitTestWork(SIZE_EXPECT *items, int nItems)
{
	int  i;         /* loop counter */
	int  rv;        /* return code from task_pipe_get() */
	int  bytesRead; /* # of bytes read from task_pipe_get() */

	for (i = 0; i < nItems; i++) {
		/*
		 * Pipe should be empty.  Most calls to task_pipe_get(TICKS_UNLIMITED)
		 * should block until the call to task_pipe_put() is performed in the
		 * routine pipeGetWaitHelperWork().
		 */

		rv = task_pipe_get(pipeId, rxBuffer, items[i].size,
						   &bytesRead, items[i].options,
						   TICKS_UNLIMITED);
		if ((rv != items[i].rcode) || (bytesRead != items[i].sent)) {
			TC_ERROR("Expected return value %d, got %d\n"
					 "Expected bytesRead = %d, got %d\n",
					 items[i].rcode, rv, 0, bytesRead);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test task_pipe_get(TICKS_UNLIMITED)
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipeGetWaitTest(void)
{
	int  rv;         /* return code from pipeGetWaitTestWork() */
	int  bytesRead;  /* # of bytes read from task_pipe_get_waitait() */

	task_sem_give(altSem);   /* Wake AlternateTask */

	rv = pipeGetWaitTestWork(wait_all_N, ARRAY_SIZE(wait_all_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _ALL_N test\n");
		return TC_FAIL;
	}

	rv = pipeGetWaitTestWork(wait_one_to_N, ARRAY_SIZE(wait_one_to_N));
	if (rv != TC_PASS) {
		TC_ERROR("Failed on _1_TO_N test\n");
		return TC_FAIL;
	}

	rv = task_pipe_get(pipeId, rxBuffer, PIPE_SIZE,
						&bytesRead, _0_TO_N, TICKS_UNLIMITED);
	if (rv != RC_FAIL) {
		TC_ERROR("Expected return code of %d, not %d\n", RC_FAIL, rv);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test remaining task_pipe_get(timeout) functionality
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int pipeGetTimeoutTest(void)
{
	int  i;          /* loop counter */
	int  rv;         /* return value from task_pipe_get() */
	int  bytesRead;  /* # of bytes read from task_pipe_get() */

	for (i = 0; i < ARRAY_SIZE(timeout_cases); i++) {
		rv = task_pipe_get(pipeId, rxBuffer, timeout_cases[i].size,
						   &bytesRead, timeout_cases[i].options,
						   ONE_SECOND);
		if ((rv != timeout_cases[i].rcode) ||
			(bytesRead != timeout_cases[i].sent)) {
			TC_ERROR("Expected return code %d, got %d\n"
					 "Expected <bytesRead> %d, got %d\n"
					 "Iteration %d\n",
					 timeout_cases[i].rcode, rv, timeout_cases[i].sent,
					 bytesRead, i + 1);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Alternate task in the test suite
 *
 * This routine runs at a lower priority than RegressionTask().
 *
 * @return TC_PASS or TC_FAIL
 */

int AlternateTask(void)
{
	int rv;   /* return code from each set of test cases */

	rv = pipePutHelper();
	if (rv != TC_PASS) {
		return TC_FAIL;
	}

	rv = pipePutWaitHelper();
	if (rv != TC_PASS) {
		return TC_FAIL;
	}

	rv = pipePutTimeoutHelper();
	if (rv != TC_PASS) {
		return TC_FAIL;
	}

	/*
	 * There is no pipeGetHelper() as the task_pipe_get() checks have
	 * either been done in pipePutHelper(), or pipeGetTest().
	 */

	rv = pipeGetWaitHelper();
	if (rv != TC_PASS) {
		return TC_FAIL;
	}

	/*
	 * There is no pipeGetTimeoutHelper() as the task_pipe_get(timeout) checks
	 * have either been done in pipePutTimeoutHelper() or
	 * pipeGetTimeoutTest().
	 */

	return TC_PASS;
}

/**
 *
 * @brief Main task in the test suite
 *
 * This is the entry point to the pipe test suite.
 *
 * @return TC_PASS or TC_FAIL
 */

int RegressionTask(void)
{
	int  tcRC;    /* test case return code */

	microObjectsInit();

	TC_PRINT("Testing task_pipe_put(TICKS_NONE) ...\n");
	tcRC = pipePutTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing task_pipe_put(TICKS_UNLIMITED) ...\n");
	tcRC = pipePutWaitTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing task_pipe_put(timeout) ...\n");
	tcRC = pipePutTimeoutTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing task_pipe_get(TICKS_NONE) ...\n");
	tcRC = pipeGetTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing task_pipe_get(TICKS_UNLIMITED) ...\n");
	tcRC = pipeGetWaitTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing task_pipe_get(timeout) ...\n");
	tcRC = pipeGetTimeoutTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	return TC_PASS;
}
