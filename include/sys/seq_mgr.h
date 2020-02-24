/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_SYS_SEQ_H_
#define ZEPHYR_INCLUDE_SYS_SYS_SEQ_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/notify.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct sys_seq_mgr;
struct sys_seq;
union sys_seq_action;

/**
 * @defgroup sys_seq_apis Asynchronous sequence APIs
 * @ingroup kernel_apis
 * @{
 */

/** @internal */
#define SYS_SEQ_NUM_CHUNKS_BITS 13
#define SYS_SEQ_MAX_CHUNKS (BIT(SYS_SEQ_NUM_CHUNKS_BITS) - 1)

/** @brief Prototype of action processing function.
 *
 * If successfully started, process completion is notified by calling
 * sys_seq_finalize().
 *
 * Note that manager assumes that processing functions are asynchronous
 * and they are called from the context in which previous action
 * completes. It can also be interrupt context. If processing functions are
 * synchronous, potential stack usage increase must be considered.
 *
 * @param mgr Pointer to the manager.
 * @param action_data Action data to process.
 *
 * @return non-negative value on success, negative value otherwise.
 */
typedef int (*sys_seq_action_process)(struct sys_seq_mgr *mgr,
				    void *action);

/** @brief Action structure if custom processing function is used.
 *
 * Action consists of processing function followed by the data.
 */
struct sys_seq_func_action {
	/** @brief Processing function. */
	sys_seq_action_process func;

	/** @brief Data passed to the processing function on action execution.*/
	uint8_t data[0];
};

/** @brief Action representation.
 *
 * Sequence manager supports to modes of operations:
 * - manager specific processing function which is same for each action in any
 *   sequence. In that case, action contains only specific data.
 * - action specific processing function where each action consists of function
 *   pointer and the data. This mode is selected by not providing manager
 *   processing function.
 */
union sys_seq_action {
	struct sys_seq_func_action *custom;
	void *generic;
};

/**
 * @brief Sequence descriptor.
 *
 * @param paused optional paused handler. Used if sequence contains pauses.
 *
 * @param actions the pointer to array with actions.
 *
 * @param num_actions number of actions in the sequence.
 */
struct sys_seq {
	const union sys_seq_action *actions;
	uint16_t num_actions;
};

#define Z_ACTION_NAME(seq_name, idx) _name##_action_##idx
#define _Z_SYS_SEQ_ACTION_DECLARE(name, _qualifier, _custom, \
				handler, type, init_val)\
	_qualifier struct { \
		IF_ENABLED(_custom, (sys_seq_action_process func;)) \
		type data; \
	} name = { \
		IF_ENABLED(_custom, (.func = handler,)) \
		.data = __DEBRACKET init_val \
	}

#define Z_SYS_SEQ_ACTION_DECLARE(...) \
	_Z_SYS_SEQ_ACTION_DECLARE(__VA_ARGS__)

/* Create and initialise single action */
#define Z_SYS_SEQ_ACTION_CREATE(idx, param, _name) \
	Z_SYS_SEQ_ACTION_DECLARE(_name##_action_##idx, __DEBRACKET param)

#define Z_SYS_SEQ_ACTIONS_CREATE(_name, ...) \
	FOR_EACH_IDX_FIXED_ARG(Z_SYS_SEQ_ACTION_CREATE, (;), \
				_name, __VA_ARGS__)

#define Z_SEQ_ACTION_PTR(idx, param, _name) \
	{ (void *)&_name##_action_##idx }

/* Create array of pointers to actions and the sequence. */
#define Z_SYS_SEQ_INIT(_qualifier, _name, ...) \
	_qualifier union sys_seq_action _name##_actions[] = { \
		FOR_EACH_IDX_FIXED_ARG(Z_SEQ_ACTION_PTR, (,), \
				       _name, __VA_ARGS__) \
	}; \
	_qualifier struct sys_seq _name = { \
		.actions = _name##_actions, \
		.num_actions = NUM_VA_ARGS_LESS_1(__VA_ARGS__, /* empty */) \
	}

/* Declare and initialize actions. Create sequence with array of pointers to
 * just created actions. Use SYS_SEQ_ACTION or derivatives for actions.
 */
#define Z_SYS_SEQ_DEFINE(_qualifier, _name, ...) \
	Z_SYS_SEQ_ACTIONS_CREATE(_name, __VA_ARGS__); \
	Z_SYS_SEQ_INIT(_qualifier, _name, __VA_ARGS__)

/** @brief Create custom (with action specific handler) sequence.
 *
 * @param _qualifier Action type qualifier (e.g. static const). Can be empty.
 * @param _name  Sequence name.
 * @param ...    List of actions. See SYS_SEQ_ACTION for action content.
 */
#define SYS_SEQ_DEFINE(_qualifier, _name, ...) \
	Z_SYS_SEQ_DEFINE(_qualifier, _name, __VA_ARGS__)


#define Z_SYS_SEQ_ACTION /* empty */

/** @brief Sequence action initializer.
 *
 * @param _qualifier Action type qualifier (e.g. static const). Can be empty.
 * @param _type Action data type (e.g. struct foo).
 * @param _init_data Initialization data in parentheses (e.g. ({ .x = 5})).
 */
#define SYS_SEQ_ACTION(_qualifier, _type, _init_data) \
	Z_SYS_SEQ_ACTION(_qualifier, 0, 0, _type, _init_data)

/** @brief Action initializer when custom handler for each action is used.
 *
 * @param _handler Action processing function.
 * @param _qualifier Action type qualifier (e.g. static const). Can be empty.
 * @param _type Action data type (e.g. struct foo).
 * @param _init_data Initialization data in parentheses (e.g. ({ .x = 5})).
 */
#define SYS_SEQ_CUSTOM_ACTION(_handler, _qualifier, _type, _init_data) \
	Z_SYS_SEQ_ACTION(_qualifier, 1, _handler, _type, _init_data)

/** @brief Create pause action.
 *
 * When pause action is executed in the sequence then provided @p _handler is
 * called with @p _context as the data argument. Sequence is paused until
 * sys_seq_finalize is called.
 *
 * This is one of a generic actions provided with the sequence manager and it
 * can only be used in custom mode where generic processing function is not
 * provided.
 *
 * @param _handler Function called when pause action is processed.
 * @param _context Context passed to an action processing function as the data
 *		   argument.
 */
#define SYS_SEQ_ACTION_PAUSE(_handler, _context) \
	SYS_SEQ_CUSTOM_ACTION(_handler, static const, \
			      void *, ((void *)_context))


/** @brief Create delay action.
 *
 * When delay action is executed in the sequence then execution of the sequence
 * is paused and resumed after @p _timeout (in milliseconds).
 *
 * Action is using optional timer in the sequence manager thus it is available
 * only if timer instance is provided during the initialization.
 *
 * This is one of a generic actions provided with the sequence manager and it
 * can only be used in custom mode where generic processing function is not
 * provided.
 *
 * @param _timeout Timeout in milliseconds.
 */
#define SYS_SEQ_ACTION_MS_DELAY(_timeout) \
	SYS_SEQ_CUSTOM_ACTION(sys_seq_delay, static const, uint32_t,\
			    (USEC_PER_MSEC * _timeout))

/** @brief Create delay action.
 *
 * See SYS_SEQ_ACTION_MS_DELAY.
 *
 * @param _timeout Timeout in microseconds.
 */
#define SYS_SEQ_ACTION_US_DELAY(_timeout) \
	SYS_SEQ_CUSTOM_ACTION(sys_seq_delay, static const, uint32_t, (_timeout))

/** @brief Functions used by the manager to implement the service. */
struct sys_seq_functions {
	/**
	 * @brief Called on sequence start.
	 *
	 * Function is asynchronous. Sequence can proceed once this setup
	 * successfully returns and sys_seq_finalize() is called with
	 * positive result. Function is optional.
	 *
	 * @param mgr the asynchronous sequence manager.
	 *
	 * @param seq the initiated sequence.
	 *
	 * @return non-negative value on success, negative value otherwise.
	 */
	int (*setup)(struct sys_seq_mgr *mgr,
			const struct sys_seq *seq);

	/**
	 * @brief Called after last action in the sequence.
	 *
	 * Function is asynchronous. It receives the result of sequence
	 * execution. If result is negative then it should be propagated to
	 * the finalization. Completion is notified using
	 * sys_seq_finalize(). Function is optional.
	 *
	 * @return non-negative value on success, negative value otherwise.
	 */
	int (*teardown)(struct sys_seq_mgr *mgr,
			const struct sys_seq *seq, int actions,
			int res);

	/**
	 * @brief Process single action in the sequence.
	 *
	 * If null then action is casted to sys_seq_func_action which has
	 * pointer to the custom process function. This approach allows to have
	 * custom process function for each action.
	 */
	sys_seq_action_process action_process;

	/**
	 * @brief Function to transform a generic notification
	 * callback to its service-specific form.
	 *
	 * The implementation should cast @p cb to the proper signature
	 * for the service, and invoke the cast pointer with the
	 * appropriate arguments.
	 *
	 * @param mgr the service that supports queued operations.
	 *
	 * @param op the operation that has been completed.
	 *
	 * @param cb the generic callback to invoke.
	 */
	void (*callback)(struct sys_seq_mgr *mgr,
			 struct sys_seq *seq,
			 int res,
			 sys_notify_generic_callback cb);
};

/**
 * @brief State associated with sequence manager.
 */
struct sys_seq_mgr {
	/* Pointer to the functions that support the manager. */
	const struct sys_seq_functions *vtable;

	/* Active sequence. */
	struct sys_seq *current_seq;

	/* Notification object for current sequence. */
	struct sys_notify *current_notify;

	/* Currently executed action in the sequence. */
	uint16_t current_idx: SYS_SEQ_NUM_CHUNKS_BITS;
	uint16_t in_setup: 1;
	uint16_t in_teardown: 1;
	uint16_t abort: 1;

	/* optional timer for delay operation. */
	struct k_timer *timer;
};

/** @brief Initialise sequence manager.
 *
 * @param mgr the sequence manager.
 * @param vtable set of functions.
 * @param timer optional timer to be used to perform generic delay action.
 *
 * @return negative value on error, 0 or positive on success.
 */
int sys_seq_mgr_init(struct sys_seq_mgr *mgr,
		     const struct sys_seq_functions *vtable,
		     struct k_timer *timer);

/**
 * @brief Asynchronously process sequence.
 *
 * If successfully started, completion of sequence is notified asynchronously.
 *
 * @param mgr the sequence manager.
 *
 * @param seq the sequence to execute.
 *
 * @param notify completion notification struct.
 *
 * @retval -EBUSY if manager is busy already executing a sequence.
 *
 * @retval -EINVAL if number of actions in the sequence is invalid.
 *
 * @return A negative value if starting the sequence is rejected by the service.
 * A non-negative value indicates that sequence has been started and completion
 * notification will be provided.
 */
int sys_seq_process(struct sys_seq_mgr *mgr,
		      const struct sys_seq *seq, struct sys_notify *notify);

/**
 * @brief Send the completion notification for an operation.
 *
 * This function must be invoked by services that support asynchronous
 * sequences on completion of an operation: setup, teardown or action
 * processing.
 *
 * @param mgr the asynchronous sequence manager.
 *
 * @param res the result of the operation.
 *
 * @param incr_offset indicates which action will be executed next. If 0 then
 * next action is executed. Negative number moves back in sequence (e.g. -1
 * reruns current action). Positive moves forward (e.g. 1 skips one action).
 */
void sys_seq_finalize(struct sys_seq_mgr *mgr, int res, int incr_offset);

/**
 * @brief Resume paused sequence.
 *
 * @param mgr the sequence manager.
 *
 * @retval -EINVAL if manager is not processing any sequence or it is not
 * @retval -EINVAL if manager is not processing any sequence.
 * @retval 0 if successfully initiated abort.
 */
int sys_seq_abort(struct sys_seq_mgr *mgr);

/** @brief Get data for current action.
 *
 * @param mgr the sequence manager.
 *
 * @return Pointer to the current action data.
 */
void *sys_seq_get_current_data(struct sys_seq_mgr *mgr);

/** @internal Used for delay action. */
int sys_seq_delay(struct sys_seq_mgr *mgr, void *action_data);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_SYS_SEQ_H_ */
