/*
 * Copyright(c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework FFF extension macros
 */

#ifndef ZEPHYR_SUBSYS_TESTSUITE_INCLUDE_ZEPHYR_FFF_EXTENSIONS_H_
#define ZEPHYR_SUBSYS_TESTSUITE_INCLUDE_ZEPHYR_FFF_EXTENSIONS_H_

#include <zephyr/fff.h>
#include <zephyr/sys/util.h> /* for CONTAINER_OF */

/**
 * @defgroup fff_extensions FFF extensions
 * @ingroup all_tests
 *
 * This module provides extensions to FFF for simplifying the
 * configuration and usage of fakes.
 *
 * @{
 */


/**
 * @brief Wrap custom fake body to extract defined context struct
 *
 * Add extension macro for simplified creation of
 * fake functions needing call-specific context data.
 *
 * This macro enables a fake to be implemented as follows and
 * requires no familiarity with the inner workings of FFF.
 *
 * @code
 * struct FUNCNAME##_custom_fake_context
 * {
 *     struct instance * const instance;
 *     int result;
 * };
 *
 * int FUNCNAME##_custom_fake(
 *     const struct instance **instance_out)
 * {
 *     RETURN_HANDLED_CONTEXT(
 *         FUNCNAME,
 *         struct FUNCNAME##_custom_fake_context,
 *         result,
 *         context,
 *         {
 *             if (context != NULL)
 *             {
 *                 if (context->result == 0)
 *                 {
 *                     if (instance_out != NULL)
 *                     {
 *                         *instance_out = context->instance;
 *                     }
 *                 }
 *                 return context->result;
 *             }
 *             return FUNCNAME##_fake.return_val;
 *         }
 *     );
 * }
 * @endcode
 *
 * @param FUNCNAME Name of function being faked
 * @param CONTEXTTYPE type of custom defined fake context struct
 * @param RESULTFIELD name of field holding the return type & value
 * @param CONTEXTPTRNAME expected name of pointer to custom defined fake context struct
 * @param HANDLERBODY in-line custom fake handling logic
 */

#define RETURN_HANDLED_CONTEXT(FUNCNAME,                                      \
		CONTEXTTYPE, RESULTFIELD, CONTEXTPTRNAME, HANDLERBODY)        \
	if (FUNCNAME##_fake.return_val_seq_len) {                             \
		CONTEXTTYPE * const contexts =                                \
		CONTAINER_OF(FUNCNAME##_fake.return_val_seq,                  \
				CONTEXTTYPE, RESULTFIELD);                    \
		size_t const seq_idx = (FUNCNAME##_fake.return_val_seq_idx <  \
					FUNCNAME##_fake.return_val_seq_len) ? \
					FUNCNAME##_fake.return_val_seq_idx++ :\
					FUNCNAME##_fake.return_val_seq_idx - 1;\
		CONTEXTTYPE * const CONTEXTPTRNAME = &contexts[seq_idx];      \
		HANDLERBODY;                                                  \
	}                                                                     \
	return FUNCNAME##_fake.return_val

/**
 * @}
 */

#endif /* ZEPHYR_SUBSYS_TESTSUITE_INCLUDE_ZEPHYR_FFF_EXTENSIONS_H_ */
