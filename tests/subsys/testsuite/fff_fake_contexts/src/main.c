/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/code_under_test.h>


#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/fff_extensions.h>

#include "fakes/called_API.h"


#define RESET_HISTORY_AND_FAKES() \
	ZEPHYR_CALLED_API_FFF_FAKES_LIST(RESET_FAKE) \
	FFF_RESET_HISTORY()

DEFINE_FFF_GLOBALS;


/*
 * Custom Fakes:
 */

struct called_API_open_custom_fake_context {
	/* Written to code under test by custom fake */
	const struct called_API_info * const instance_out;

	int result;
};

int called_API_open_custom_fake(
	const struct called_API_info **instance_out)
{
	RETURN_HANDLED_CONTEXT(
		called_API_open,
		struct called_API_open_custom_fake_context,
		result,  /* return field name in _fake_context struct */
		context, /* Name of context ptr variable used below */
		{
			if (context != NULL) {
				if (context->result == 0) {
					if (instance_out != NULL) {
						*instance_out = context->instance_out;
					}
				}

				return context->result;
			}

			return called_API_open_fake.return_val;
		}
	);
}

/*
 * Tests
 */

ZTEST(fff_fake_contexts_tests, test_code_under_test)
{
	struct test_case {
		const char *description_oneliner;

		void **expected_call_history;

		/* Last FFF sequence entry is reused for excess calls.
		 * Have an extra entry that returns a distinct failure (-E2BIG)
		 *
		 * Expect one less call than _len, or 0 if sequence ptr is NULL
		 *
		 * Configure to return -E2BIG if excess calls.
		 */
		int    called_API_open_custom_fake_contexts_len;
		struct called_API_open_custom_fake_context *
		       called_API_open_custom_fake_contexts;

		int  called_API_close_fake_return_val_seq_len;
		int *called_API_close_fake_return_val_seq;

		int result_expected;
	}
	const test_cases[] = {
		{
			.description_oneliner = "First called_API_open() returns -EINVAL",
			.expected_call_history = (void * [])
			{
				called_API_open,
				NULL, /* mark end of array */
			},

			.called_API_open_custom_fake_contexts_len = 2,
			.called_API_open_custom_fake_contexts =
				(struct called_API_open_custom_fake_context [])
			{
				{
					.result = -EINVAL,
				},
				{
					.result = -E2BIG, /* for excessive calls */
				},
			},

			.called_API_close_fake_return_val_seq = NULL,

			.result_expected = -EINVAL,
		},
		{
			.description_oneliner = "First called_API_close() returns -EINVAL",
			.expected_call_history = (void * [])
			{
				called_API_open,
				called_API_close,
				NULL, /* mark end of array */
			},

			.called_API_open_custom_fake_contexts_len = 2,
			.called_API_open_custom_fake_contexts =
				(struct called_API_open_custom_fake_context [])
			{
				{
					.result = 0,
				},
				{
					.result = -E2BIG, /* for excessive calls */
				},
			},

			.called_API_close_fake_return_val_seq_len = 2,
			.called_API_close_fake_return_val_seq = (int [])
			{
				-EINVAL,
				-E2BIG, /* for excessive calls */
			},

			.result_expected = -EINVAL,
		},
		{
			.description_oneliner = "Second called_API_open() returns -EINVAL",
			.expected_call_history = (void * [])
			{
				called_API_open,
				called_API_close,
				called_API_open,
				NULL, /* mark end of array */
			},

			.called_API_open_custom_fake_contexts_len = 3,
			.called_API_open_custom_fake_contexts =
				(struct called_API_open_custom_fake_context [])
			{
				{
					.result = 0,
				},
				{
					.result = -EINVAL,
				},
				{
					.result = -E2BIG, /* for excessive calls */
				},
			},

			.called_API_close_fake_return_val_seq_len = 2,
			.called_API_close_fake_return_val_seq = (int [])
			{
				0,
				-E2BIG, /* for excessive calls */
			},

			.result_expected = -EINVAL,
		},
		{
			.description_oneliner = "Second called_API_close() returns -EINVAL",
			.expected_call_history = (void * [])
			{
				called_API_open,
				called_API_close,
				called_API_open,
				called_API_close,
				NULL, /* mark end of array */
			},

			.called_API_open_custom_fake_contexts_len = 3,
			.called_API_open_custom_fake_contexts =
				(struct called_API_open_custom_fake_context [])
			{
				{
					.result = 0,
				},
				{
					.result = 0,
				},
				{
					.result = -E2BIG, /* for excessive calls */
				},
			},

			.called_API_close_fake_return_val_seq_len = 3,
			.called_API_close_fake_return_val_seq = (int [])
			{
				0,
				-EINVAL,
				-E2BIG, /* for excessive calls */
			},

			.result_expected = -EINVAL,
		},
		{
			.description_oneliner = "All calls return no error",
			.expected_call_history = (void * [])
			{
				called_API_open,
				called_API_close,
				called_API_open,
				called_API_close,
				NULL, /* mark end of array */
			},

			.called_API_open_custom_fake_contexts_len = 3,
			.called_API_open_custom_fake_contexts =
				(struct called_API_open_custom_fake_context [])
			{
				{
					.result = 0,
				},
				{
					.result = 0,
				},
				{
					.result = -E2BIG, /* for excessive calls */
				},
			},

			.called_API_close_fake_return_val_seq_len = 3,
			.called_API_close_fake_return_val_seq = (int [])
			{
				0,
				0,
				-E2BIG, /* for excessive calls */
			},

			.result_expected = 0,
		},

	};

	for (int i = 0; i < ARRAY_SIZE(test_cases); ++i) {
		const struct test_case * const tc = &test_cases[i];


		printk("Checking test_cases[%i]: %s\n", i,
			(tc->description_oneliner != NULL) ? tc->description_oneliner : "");

		/*
		 * Set up pre-conditions
		 */
		RESET_HISTORY_AND_FAKES();

		/* NOTE: Point to the return type field in the first returns struct.
		 *       This custom_fake:
		 *	 - uses *_fake.return_val_seq and CONTAINER_OF()
		 *	     to determine the beginning of the array of structures.
		 *	 - uses *_fake.return_val_seq_id to index into
		 *	     the array of structures.
		 *       This overloading is to allow the return_val_seq to
		 *       also contain call-specific output parameters to be
		 *       applied by the custom_fake.
		 */
		called_API_open_fake.return_val = -E2BIG; /* for excessive calls */
		SET_RETURN_SEQ(called_API_open,
			&tc->called_API_open_custom_fake_contexts[0].result,
			tc->called_API_open_custom_fake_contexts_len);
		called_API_open_fake.custom_fake = called_API_open_custom_fake;

		/* NOTE: This uses the standard _fake without contexts */
		called_API_close_fake.return_val = -E2BIG; /* for excessive calls */
		SET_RETURN_SEQ(called_API_close,
			tc->called_API_close_fake_return_val_seq,
			tc->called_API_close_fake_return_val_seq_len);


		/*
		 * Call code_under_test
		 */
		int result = code_under_test();


		/*
		 * Verify expected behavior of code_under_test:
		 *   - call history, args per call
		 *   - results
		 *   - outputs
		 */
		if (tc->expected_call_history != NULL) {
			for (int j = 0; j < fff.call_history_idx; ++j) {
				zassert_equal(fff.call_history[j],
						tc->expected_call_history[j], NULL);
			}
			zassert_is_null(tc->expected_call_history[
						fff.call_history_idx], NULL);
		} else {
			zassert_equal(fff.call_history_idx, 0, NULL);
		}


		const int called_API_open_fake_call_count_expected =
			(tc->called_API_open_custom_fake_contexts == NULL) ? 0 :
			 tc->called_API_open_custom_fake_contexts_len - 1;

		zassert_equal(called_API_open_fake.call_count,
			      called_API_open_fake_call_count_expected, NULL);
		for (int j = 0; j < called_API_open_fake_call_count_expected; ++j) {
			zassert_not_null(called_API_open_fake.arg0_history[j], NULL);
		}


		const int called_API_close_fake_call_count_expected =
			(tc->called_API_close_fake_return_val_seq == NULL) ? 0 :
			 tc->called_API_close_fake_return_val_seq_len - 1;

		zassert_equal(called_API_close_fake.call_count,
			called_API_close_fake_call_count_expected, NULL);
		for (int j = 0; j < called_API_close_fake_call_count_expected; ++j) {
			/* Verify code_under_test returns value provided by open. */
			zassert_equal(called_API_close_fake.arg0_history[j],
				tc->called_API_open_custom_fake_contexts[j]
					.instance_out, NULL);
		}


		zassert_equal(result, tc->result_expected, NULL);
	}
}

ZTEST_SUITE(fff_fake_contexts_tests, NULL, NULL, NULL, NULL, NULL);
