/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TICKER_H_
#define _TICKER_H_

/** \brief Macro to translate microseconds to tick units.
*
* \note This returns the floor value.
*/
#define TICKER_US_TO_TICKS(x) \
	( \
		((uint32_t)(((uint64_t) (x) * 1000000000UL) / 30517578125UL)) \
		& 0x00FFFFFF \
	)

/** \brief Macro returning remainder in nanoseconds over-and-above a tick unit.
*/
#define TICKER_REMAINDER(x) \
	( \
		( \
			((uint64_t) (x) * 1000000000UL) \
			- ((uint64_t) TICKER_US_TO_TICKS(x) * 30517578125UL) \
		) \
		/ 1000UL \
	)

/** \brief Macro to translate tick units to microseconds.
*/
#define TICKER_TICKS_TO_US(x) \
	((uint32_t)(((uint64_t) (x) * 30517578125UL) / 1000000000UL))

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
#define TICKER_NULL		((uint8_t)((uint8_t)0 - 1))
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
typedef void (*ticker_timeout_func) (uint32_t ticks_at_expire,
				      uint32_t remainder, uint16_t lazy,
				      void *context);

/** \brief Timer operation complete function type.
*/
typedef void (*ticker_op_func) (uint32_t status, void *op_context);

/** \brief Timer module initialization.
*
* \param[in]  instance_index  Timer mode instance 0 or 1 (uses RTC0 CMP0 or
*				CMP1 respectively).
* \param[in]  count_node      Max. no. of ticker nodes to initialise.
* \param[in]  node
* \param[in]  count_user
* \param[in]  user
* \param[in]  count_op
* \param[in]  user_op
*/
uint32_t ticker_init(uint8_t instance_index, uint8_t count_node, void *node,
		    uint8_t count_user, void *user, uint8_t count_op,
		    void *user_op);
void ticker_trigger(uint8_t instance_index);
uint32_t ticker_start(uint8_t instance_index, uint8_t user_id,
		     uint8_t ticker_id, uint32_t ticks_anchor,
		     uint32_t ticks_first, uint32_t ticks_periodic,
		     uint32_t remainder_periodic, uint16_t lazy,
		     uint16_t ticks_slot,
		     ticker_timeout_func ticker_timeout_func, void *context,
		     ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_update(uint8_t instance_index, uint8_t user_id,
		      uint8_t ticker_id, uint16_t ticks_drift_plus,
		      uint16_t ticks_drift_minus, uint16_t ticks_slot_plus,
		      uint16_t ticks_slot_minus, uint16_t lazy, uint8_t force,
		      ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_stop(uint8_t instance_index, uint8_t user_id,
		    uint8_t ticker_id, ticker_op_func fp_op_func,
		    void *op_context);
uint32_t ticker_next_slot_get(uint8_t instance_index, uint8_t user_id,
			     uint8_t *ticker_id_head,
			     uint32_t *ticks_current,
			     uint32_t *ticks_to_expire,
			     ticker_op_func fp_op_func, void *op_context);
uint32_t ticker_job_idle_get(uint8_t instance_index, uint8_t user_id,
			    ticker_op_func fp_op_func, void *op_context);
void ticker_job_sched(uint8_t instance_index);
uint32_t ticker_ticks_now_get(void);
uint32_t ticker_ticks_diff_get(uint32_t ticks_now, uint32_t ticks_old);

#endif
