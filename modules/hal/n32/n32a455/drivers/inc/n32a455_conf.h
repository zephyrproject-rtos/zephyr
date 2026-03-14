/*
 * N32A455 Configuration Header
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __N32A455_CONF_H__
#define __N32A455_CONF_H__

/* Disable assertions to avoid warnings */
#define USE_FULL_ASSERT 0

/* Assert parameter stub */
#if !defined(USE_FULL_ASSERT) || (USE_FULL_ASSERT == 0)
#define assert_param(expr) ((void)0)
#endif

#endif /* __N32A455_CONF_H__ */
