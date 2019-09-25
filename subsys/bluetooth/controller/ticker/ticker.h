/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** \defgroup Timer API return codes.
 *
 * @{
 */
#define TICKER_STATUS_SUCCESS 0 /**< Success. */
#define TICKER_STATUS_FAILURE 1 /**< Failure. */
#define TICKER_STATUS_BUSY    2 /**< Busy, requested feature will
				  * complete later in time as job is
				  * disabled or at lower execution
				  * priority than the caller.
				  */
/**
 * @}
 */

/** \defgroup Timer API common defaults parameter values.
 *
 * @{
 */
#define TICKER_NULL             ((u8_t)((u8_t)0 - 1))
#define TICKER_NULL_REMAINDER   0
#define TICKER_NULL_PERIOD      0
#define TICKER_NULL_SLOT        0
#define TICKER_NULL_LAZY        0
#define TICKER_NULL_MUST_EXPIRE 0
/**
 * @}
 */

/** \brief Timer node type size.
 */
#if defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
#define TICKER_NODE_T_SIZE      40
#else
#define TICKER_NODE_T_SIZE      44
#endif

/** \brief Timer user type size.
 */
#define TICKER_USER_T_SIZE      8

/** \brief Timer user operation type size.
 */
#define TICKER_USER_OP_T_SIZE   48

#define TICKER_CALL_ID_NONE     0
#define TICKER_CALL_ID_ISR      1
#define TICKER_CALL_ID_TRIGGER  2
#define TICKER_CALL_ID_WORKER   3
#define TICKER_CALL_ID_JOB      4
#define TICKER_CALL_ID_PROGRAM  5

/* Use to ensure callback is invoked in all intervals, even when latencies
 * occur
 */
#define TICKER_LAZY_MUST_EXPIRE 0xFFFF

/* Set this priority to ensure ticker node is always scheduled. Only one
 * ticker node can have priority TICKER_PRIORITY_CRITICAL at a time
 */
#define TICKER_PRIORITY_CRITICAL -128

typedef u8_t (*ticker_caller_id_get_cb_t)(u8_t user_id);
typedef void (*ticker_sched_cb_t)(u8_t caller_id, u8_t callee_id, u8_t chain,
				  void *instance);
typedef void (*ticker_trigger_set_cb_t)(u32_t value);

/** \brief Timer timeout function type.
 */
typedef void (*ticker_timeout_func) (u32_t ticks_at_expire, u32_t remainder,
				     u16_t lazy, void *context);

/** \brief Timer operation complete function type.
 */
typedef void (*ticker_op_func) (u32_t status, void *op_context);

/** \brief Timer module initialization.
 *
 * \param[in]  instance_index  Timer mode instance 0 or 1 (uses RTC0 CMP0 or
 *				CMP1 respectively).
 * \param[in]  count_node      Max. no. of ticker nodes to initialize.
 * \param[in]  node
 * \param[in]  count_user
 * \param[in]  user
 * \param[in]  count_op
 * \param[in]  user_op
 */
u32_t ticker_init(u8_t instance_index, u8_t count_node, void *node,
		  u8_t count_user, void *user, u8_t count_op, void *user_op,
		  ticker_caller_id_get_cb_t caller_id_get_cb,
		  ticker_sched_cb_t sched_cb,
		  ticker_trigger_set_cb_t trigger_set_cb);
bool ticker_is_initialized(u8_t instance_index);
void ticker_trigger(u8_t instance_index);
void ticker_worker(void *param);
void ticker_job(void *param);
u32_t ticker_start(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		   u32_t ticks_anchor, u32_t ticks_first, u32_t ticks_periodic,
		   u32_t remainder_periodic, u16_t lazy, u32_t ticks_slot,
		   ticker_timeout_func fp_timeout_func, void *context,
		   ticker_op_func fp_op_func, void *op_context);
u32_t ticker_update(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		    u32_t ticks_drift_plus, u32_t ticks_drift_minus,
		    u32_t ticks_slot_plus, u32_t ticks_slot_minus, u16_t lazy,
		    u8_t force, ticker_op_func fp_op_func, void *op_context);
u32_t ticker_stop(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		  ticker_op_func fp_op_func, void *op_context);
u32_t ticker_next_slot_get(u8_t instance_index, u8_t user_id,
			   u8_t *ticker_id_head, u32_t *ticks_current,
			   u32_t *ticks_to_expire,
			   ticker_op_func fp_op_func, void *op_context);
u32_t ticker_job_idle_get(u8_t instance_index, u8_t user_id,
			  ticker_op_func fp_op_func, void *op_context);
void ticker_job_sched(u8_t instance_index, u8_t user_id);
u32_t ticker_ticks_now_get(void);
u32_t ticker_ticks_diff_get(u32_t ticks_now, u32_t ticks_old);
#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
u32_t ticker_priority_set(u8_t instance_index, u8_t user_id, u8_t ticker_id,
			  s8_t priority, ticker_op_func fp_op_func,
			  void *op_context);
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
