/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when net_buf_alloc() is called
 *
 *  Expected behaviour:
 *   - net_buf_alloc() to be called once with :
 *       - correct memory allocation pool
 */
void expect_single_call_net_buf_alloc(struct net_buf_pool *pool, k_timeout_t *timeout);

/*
 *  Validate expected behaviour when net_buf_alloc() isn't called
 *
 *  Expected behaviour:
 *   - net_buf_alloc() isn't called at all
 */
void expect_not_called_net_buf_alloc(void);

/*
 *  Validate expected behaviour when net_buf_reserve() is called
 *
 *  Expected behaviour:
 *   - net_buf_reserve() to be called once with :
 *       - correct reference value
 *       - 'reserve' argument set to 'BT_BUF_RESERVE' value
 */
void expect_single_call_net_buf_reserve(struct net_buf *buf);

/*
 *  Validate expected behaviour when net_buf_reserve() isn't called
 *
 *  Expected behaviour:
 *   - net_buf_reserve() isn't called at all
 */
void expect_not_called_net_buf_reserve(void);

/*
 *  Validate expected behaviour when net_buf_ref() is called
 *
 *  Expected behaviour:
 *   - net_buf_ref() to be called once with correct reference value
 */
void expect_single_call_net_buf_ref(struct net_buf *buf);

/*
 *  Validate expected behaviour when net_buf_ref() isn't called
 *
 *  Expected behaviour:
 *   - net_buf_ref() isn't called at all
 */
void expect_not_called_net_buf_ref(void);
