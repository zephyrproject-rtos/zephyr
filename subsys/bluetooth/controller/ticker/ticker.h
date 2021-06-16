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
#define TICKER_NULL             ((uint8_t)((uint8_t)0 - 1))
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
#if defined(CONFIG_BT_TICKER_LOW_LAT)
#define TICKER_NODE_T_SIZE      40
#else
#if defined(CONFIG_BT_TICKER_EXT)
#define TICKER_NODE_T_SIZE      48
#else
#if defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
#define TICKER_NODE_T_SIZE      36
#else
#define TICKER_NODE_T_SIZE      44
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
#endif /* CONFIG_BT_TICKER_EXT */
#endif /* CONFIG_BT_TICKER_LOW_LAT */

/** \brief Timer user type size.
 */
#define TICKER_USER_T_SIZE      8

/** \brief Timer user operation type size.
 */
#if defined(CONFIG_BT_TICKER_EXT)
#define TICKER_USER_OP_T_SIZE   52
#else
#if defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
#define TICKER_USER_OP_T_SIZE   44
#else
#define TICKER_USER_OP_T_SIZE   48
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
#endif /* CONFIG_BT_TICKER_EXT */

#define TICKER_CALL_ID_NONE     0
#define TICKER_CALL_ID_ISR      1
#define TICKER_CALL_ID_TRIGGER  2
#define TICKER_CALL_ID_WORKER   3
#define TICKER_CALL_ID_JOB      4
#define TICKER_CALL_ID_PROGRAM  5

/* Use to ensure callback is invoked in all intervals, even when latencies
 * occur.
 */
#define TICKER_LAZY_MUST_EXPIRE      0xFFFF

/* Use in ticker_start to set lazy to 0 and do not change the must_expire
 * state.
 */
#define TICKER_LAZY_MUST_EXPIRE_KEEP 0xFFFE

/* Set this priority to ensure ticker node is always scheduled. Only one
 * ticker node can have priority TICKER_PRIORITY_CRITICAL at a time
 */
#define TICKER_PRIORITY_CRITICAL -128

typedef uint8_t (*ticker_caller_id_get_cb_t)(uint8_t user_id);
typedef void (*ticker_sched_cb_t)(uint8_t caller_id, uint8_t callee_id, uint8_t chain,
				  void *instance);
typedef void (*ticker_trigger_set_cb_t)(uint32_t value);

/** \brief Timer timeout function type.
 */
typedef void (*ticker_timeout_func) (uint32_t ticks_at_expire, uint32_t remainder,
				     uint16_t lazy, uint8_t force, void *context);

/** \brief Timer operation complete function type.
 */
typedef void (*ticker_op_func) (uint32_t status, void *op_context);

/** \brief Timer operation match callback function type.
 */
typedef bool (*ticker_op_match_func) (uint8_t ticker_id, uint32_t ticks_slot,
				      uint32_t ticks_to_expire, void *op_context);

#if defined(CONFIG_BT_TICKER_EXT)
struct ticker_ext {
	uint32_t ticks_slot_window;/* Window in which the slot
				    * reservation may be re-scheduled
				    * to avoid collision
				    */
	int32_t ticks_drift;	   /* Applied drift since last expiry */
	uint8_t reschedule_state;  /* State of re-scheduling of the
				    * node. See defines
				    * TICKER_RESCHEDULE_STATE_XXX
				    */
};
#endif /* CONFIG_BT_TICKER_EXT */

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
uint32_t ticker_init(uint8_t instance_index, uint8_t count_node, void *node,
		  uint8_t count_user, void *user, uint8_t count_op, void *user_op,
		  ticker_caller_id_get_cb_t caller_id_get_cb,
		  ticker_sched_cb_t sched_cb,
		  ticker_trigger_set_cb_t trigger_set_cb);
bool ticker_is_initialized(uint8_t instance_index);
void ticker_trigger(uint8_t instance_index);
void ticker_worker(void *param);
void ticker_job(void *param);
uint32_t ticker_start(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		   uint32_t ticks_anchor, uint32_t ticks_first, uint32_t ticks_periodic,
		   uint32_t remainder_periodic, uint16_t lazy, uint32_t ticks_slot,
		   ticker_timeout_func fp_timeout_func, void *context,
		   ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_update(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		    uint32_t ticks_drift_plus, uint32_t ticks_drift_minus,
		    uint32_t ticks_slot_plus, uint32_t ticks_slot_minus, uint16_t lazy,
		    uint8_t force, ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_yield_abs(uint8_t instance_index, uint8_t user_id,
			  uint8_t ticker_id, uint32_t ticks_at_yield,
			  ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_stop(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		  ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_stop_abs(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		      uint32_t ticks_at_stop, ticker_op_func fp_op_func,
		      void *op_context);
uint32_t ticker_next_slot_get(uint8_t instance_index, uint8_t user_id,
			      uint8_t *ticker_id, uint32_t *ticks_current,
			      uint32_t *ticks_to_expire,
			      ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_next_slot_get_ext(uint8_t instance_index, uint8_t user_id,
				  uint8_t *ticker_id, uint32_t *ticks_current,
				  uint32_t *ticks_to_expire, uint16_t *lazy,
				  ticker_op_match_func fp_op_match_func,
				  void *match_op_context,
				  ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_job_idle_get(uint8_t instance_index, uint8_t user_id,
			  ticker_op_func fp_op_func, void *op_context);
void ticker_job_sched(uint8_t instance_index, uint8_t user_id);
uint32_t ticker_ticks_now_get(void);
uint32_t ticker_ticks_diff_get(uint32_t ticks_now, uint32_t ticks_old);
#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
uint32_t ticker_priority_set(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
			  int8_t priority, ticker_op_func fp_op_func,
			  void *op_context);
#if defined(CONFIG_BT_TICKER_EXT)
uint32_t ticker_start_ext(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		       uint32_t ticks_anchor, uint32_t ticks_first,
		       uint32_t ticks_periodic, uint32_t remainder_periodic,
		       uint16_t lazy, uint32_t ticks_slot,
		       ticker_timeout_func fp_timeout_func, void *context,
		       ticker_op_func fp_op_func, void *op_context,
		       struct ticker_ext *ext_data);
uint32_t ticker_update_ext(uint8_t instance_index, uint8_t user_id,
			   uint8_t ticker_id, uint32_t ticks_drift_plus,
			   uint32_t ticks_drift_minus,
			   uint32_t ticks_slot_plus, uint32_t ticks_slot_minus,
			   uint16_t lazy, uint8_t force,
			   ticker_op_func fp_op_func, void *op_context,
			   uint8_t must_expire);
#endif /* CONFIG_BT_TICKER_EXT */
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/
