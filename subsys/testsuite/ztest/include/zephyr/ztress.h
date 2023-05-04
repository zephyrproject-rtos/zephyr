/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework ztress macros
 */

#ifndef TESTSUITE_ZTEST_INCLUDE_ZTRESS_H__
#define TESTSUITE_ZTEST_INCLUDE_ZTRESS_H__

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @internal Internal ID's to distinguish context type. */
#define ZTRESS_ID_THREAD 0
#define ZTRESS_ID_K_TIMER 1

/**
 * @defgroup ztest_ztress Ztest ztress macros
 * @ingroup ztest
 *
 * This module provides test stress when using Ztest.
 *
 * @{
 */

/** @brief Descriptor of a k_timer handler execution context.
 *
 * The handler is executed in the k_timer handler context which typically means
 * interrupt context. This context will preempt any other used in the set.
 *
 * @note There can only be up to one k_timer context in the set and it must be the
 * first argument of @ref ZTRESS_EXECUTE.
 *
 * @param handler User handler of type @ref ztress_handler.
 *
 * @param user_data User data passed to the @p handler.
 *
 * @param exec_cnt Number of handler executions to complete the test. If 0 then
 * this is not included in completion criteria.
 *
 * @param init_timeout Initial backoff time base (given in @ref k_timeout_t). It
 * is adjusted during the test to optimize CPU load. The actual timeout used for
 * the timer is randomized.
 */
#define ZTRESS_TIMER(handler, user_data, exec_cnt, init_timeout) \
	(ZTRESS_ID_K_TIMER, handler, user_data, exec_cnt, 0, init_timeout)

/** @brief Descriptor of a thread execution context.
 *
 * The handler is executed in the thread context. The priority of the thread is
 * determined based on the order in which contexts are listed in @ref ZTRESS_EXECUTE.
 *
 * @note thread sleeps for random amount of time. Additionally, the thread busy-waits
 * for a random length of time to further increase randomization in the test.
 *
 * @param handler User handler of type @ref ztress_handler.
 *
 * @param user_data User data passed to the @p handler.
 *
 * @param exec_cnt Number of handler executions to complete the test. If 0 then
 * this is not included in completion criteria.
 *
 * @param preempt_cnt Number of preemptions of that context to complete the test.
 * If 0 then this is not included in completion criteria.
 *
 * @param init_timeout Initial backoff time base (given in @ref k_timeout_t). It
 * is adjusted during the test to optimize CPU load. The actual timeout used for
 * sleeping is randomized.
 */
#define ZTRESS_THREAD(handler, user_data, exec_cnt, preempt_cnt, init_timeout) \
	(ZTRESS_ID_THREAD, handler, user_data, exec_cnt, preempt_cnt, init_timeout)

/** @brief User handler called in one of the configured contexts.
 *
 * @param user_data User data provided in the context descriptor.
 *
 * @param cnt Current execution counter. Counted from 0.
 *
 * @param last Flag set to true indicates that it is the last execution because
 * completion criteria are met, test timed out or was aborted.
 *
 * @param prio Context priority counting from 0 which indicates the highest priority.
 *
 * @retval true continue test.
 * @retval false stop executing the current context.
 */
typedef bool (*ztress_handler)(void *user_data, uint32_t cnt, bool last, int prio);

/** @internal Context structure. */
struct ztress_context_data {
	/* Handler. */
	ztress_handler handler;

	/* User data */
	void *user_data;

	/* Minimum number of executions to complete the test. */
	uint32_t exec_cnt;

	/* Minimum number of preemptions to complete the test. Valid only for
	 * thread context.
	 */
	uint32_t preempt_cnt;

	/* Initial timeout. */
	k_timeout_t t;
};

/** @brief Initialize context structure.
 *
 * For argument types see @ref ztress_context_data. For more details see
 * @ref ZTRESS_THREAD.
 *
 * @param _handler Handler.
 * @param _user_data User data passed to the handler.
 * @param _exec_cnt Execution count limit.
 * @param _preempt_cnt Preemption count limit.
 * @param _t Initial timeout.
 */
#define ZTRESS_CONTEXT_INITIALIZER(_handler, _user_data, _exec_cnt, _preempt_cnt, _t) \
	{ \
		.handler = (_handler), \
		.user_data = (_user_data), \
		.exec_cnt = (_exec_cnt), \
		.preempt_cnt = (_preempt_cnt), \
		.t = (_t) \
	}

/** @internal Strip first argument (context type) and call struct initializer. */
#define Z_ZTRESS_GET_HANDLER_DATA2(_, ...) \
	ZTRESS_CONTEXT_INITIALIZER(__VA_ARGS__)

/** @internal Macro for initializing context data. */
#define Z_ZTRESS_GET_HANDLER_DATA(data) \
	Z_ZTRESS_GET_HANDLER_DATA2 data

/** @internal Macro for checking if provided context is a timer context. */
#define Z_ZTRESS_HAS_TIMER(data, ...) \
	GET_ARG_N(1, __DEBRACKET data)

/** @internal If context descriptor is @ref ZTRESS_TIMER, it returns index of that
 * descriptor in the list of arguments.
 */
#define Z_ZTRESS_TIMER_IDX(idx, data) \
	((GET_ARG_N(1, __DEBRACKET data)) == ZTRESS_ID_K_TIMER ? idx : 0)

/** @intenal Macro validates that @ref ZTRESS_TIMER context is not used except for
 * the first item in the list of contexts.
 */
#define Z_ZTRESS_TIMER_CONTEXT_VALIDATE(...) \
	BUILD_ASSERT((FOR_EACH_IDX(Z_ZTRESS_TIMER_IDX, (+), __VA_ARGS__)) == 0, \
			"There can only be up to one ZTRESS_TIMER context and it must " \
			"be the first in the list")

/** @brief Setup and run stress test.
 *
 * It initialises all contexts and calls @ref ztress_execute.
 *
 * @param ... List of contexts. Contexts are configured using @ref ZTRESS_TIMER
 * and @ref ZTRESS_THREAD macros. @ref ZTRESS_TIMER must be the first argument if
 * used. Each thread context has an assigned priority. The priority is assigned in
 * a descending order (first listed thread context has the highest priority). The
 * number of supported thread contexts is configurable in Kconfig.
 */
#define ZTRESS_EXECUTE(...) do {							\
	Z_ZTRESS_TIMER_CONTEXT_VALIDATE(__VA_ARGS__);					\
	int has_timer = Z_ZTRESS_HAS_TIMER(__VA_ARGS__);				\
	struct ztress_context_data data1[] = {						\
		FOR_EACH(Z_ZTRESS_GET_HANDLER_DATA, (,), __VA_ARGS__)			\
	};										\
	size_t cnt = ARRAY_SIZE(data1) - has_timer;					\
	static struct ztress_context_data data[ARRAY_SIZE(data1)];                      \
	for (size_t i = 0; i < ARRAY_SIZE(data1); i++) {                                \
		data[i] = data1[i];                                                     \
	}	                                                                        \
	int err = ztress_execute(has_timer ? &data[0] : NULL, &data[has_timer], cnt);	\
											\
	zassert_equal(err, 0, "ztress_execute failed (err: %d)", err);			\
} while (0)

/** Execute contexts.
 *
 * The test runs until all completion requirements are met or until the test times out
 * (use @ref ztress_set_timeout to configure timeout) or until the test is aborted
 * (@ref ztress_abort).
 *
 * on test completion a report is printed (@ref ztress_report is called internally).
 *
 * @param timer_data Timer context. NULL if timer context is not used.
 * @param thread_data List of thread contexts descriptors in priority descending order.
 * @param cnt Number of thread contexts.
 *
 * @retval -EINVAL If configuration is invalid.
 * @retval 0 if test is successfully performed.
 */
int ztress_execute(struct ztress_context_data *timer_data,
		     struct ztress_context_data *thread_data,
		     size_t cnt);

/** @brief Abort ongoing stress test. */
void ztress_abort(void);

/** @brief Set test timeout.
 *
 * Test is terminated after timeout disregarding completion criteria. Setting
 * is persistent between executions.
 *
 * @param t Timeout.
 */
void ztress_set_timeout(k_timeout_t t);

/** @brief Print last test report.
 *
 * Report contains number of executions and preemptions for each context, initial
 * and adjusted timeouts and CPU load during the test.
 */
void ztress_report(void);

/** @brief Get number of executions of a given context in the last test.
 *
 * @param id Context id. 0 means the highest priority.
 *
 * @return Number of executions.
 */
int ztress_exec_count(uint32_t id);

/** @brief Get number of preemptions of a given context in the last test.
 *
 * @param id Context id. 0 means the highest priority.
 *
 * @return Number of preemptions.
 */
int ztress_preempt_count(uint32_t id);

/** @brief Get optimized timeout base of a given context in the last test.
 *
 * Optimized value can be used to update initial value. It will improve the test
 * since optimal CPU load will be reach immediately.
 *
 * @param id Context id. 0 means the highest priority.
 *
 * @return Optimized timeout base.
 */
uint32_t ztress_optimized_ticks(uint32_t id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* TESTSUITE_ZTEST_INCLUDE_ZTRESS_H__ */
