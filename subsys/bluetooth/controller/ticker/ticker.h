/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TICKER_H_
#define _TICKER_H_

/** \brief Macro to translate microseconds to tick units.
*
* \note This returns the floor value.
*/
#define TICKER_US_TO_TICKS(x) \
	( \
		((u32_t)(((u64_t) (x) * 1000000000UL) / 30517578125UL)) \
		& 0x00FFFFFF \
	)

/** \brief Macro returning remainder in nanoseconds over-and-above a tick unit.
*/
#define TICKER_REMAINDER(x) \
	( \
		( \
			((u64_t) (x) * 1000000000UL) \
			- ((u64_t) TICKER_US_TO_TICKS(x) * 30517578125UL) \
		) \
		/ 1000UL \
	)

/** \brief Macro to translate tick units to microseconds.
*/
#define TICKER_TICKS_TO_US(x) \
	((u32_t)(((u64_t) (x) * 30517578125UL) / 1000000000UL))

/** \defgroup Timer API return codes.
*
* @{
*/
#define TICKER_STATUS_SUCCESS 0	/**< Success. */
#define TICKER_STATUS_FAILURE 1	/**< Failure. */
#define TICKER_STATUS_BUSY    2	/**< Busy, requested feature will
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
#define TICKER_NULL		((u8_t)((u8_t)0 - 1))
#define TICKER_NULL_REMAINDER	0
#define TICKER_NULL_PERIOD	0
#define TICKER_NULL_SLOT	0
#define TICKER_NULL_LAZY	0
/**
* @}
*/

/** \brief Timer node type size.
*/
#define TICKER_NODE_T_SIZE	36

/** \brief Timer user type size.
*/
#define TICKER_USER_T_SIZE	8

/** \brief Timer user operation type size.
*/
#define TICKER_USER_OP_T_SIZE	44

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
		  u8_t count_user, void *user, u8_t count_op, void *user_op);
bool ticker_is_initialized(u8_t instance_index);
void ticker_trigger(u8_t instance_index);
u32_t ticker_start(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		   u32_t ticks_anchor, u32_t ticks_first, u32_t ticks_periodic,
		   u32_t remainder_periodic, u16_t lazy, u16_t ticks_slot,
		   ticker_timeout_func ticker_timeout_func, void *context,
		   ticker_op_func fp_op_func, void *op_context);
u32_t ticker_update(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		    u16_t ticks_drift_plus, u16_t ticks_drift_minus,
		    u16_t ticks_slot_plus, u16_t ticks_slot_minus, u16_t lazy,
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

#endif
