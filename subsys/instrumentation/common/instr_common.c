/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/instrumentation/instrumentation.h>
#include <zephyr/instrumentation/instrumentation_ringbuffer.h>
#include <zephyr/instrumentation/instrumentation_timestamp.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/retention/retention.h>
#include <zephyr/sys/reboot.h>

#include <kernel_internal.h>
#include <ksched.h>

/*
 * Memory buffer to store instrumentation event records has the following modes:
 *
 * Callgraph (tracing) ring buffer (default): Replace oldest entries when buffer
 * is full.
 *
 * Callgraph (tracing) fixed buffer: Stop buffering events when the buffer is
 * full, ensuring we have a callgraph from reset point or from wherever the
 * trigger function was called for the first time.
 *
 * Statistical (profiling): Buffer functions until out of memory.
 *
 */

const struct device *retention0 = DEVICE_DT_GET(DT_NODELABEL(retention0));

static bool _instr_initialized;
static bool _instr_enabled;
static bool _instr_on;
static bool _instr_tracing_disabled;
static bool _instr_profiling_disabled;
static bool _instr_tracing_supported = IS_ENABLED(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH);
static bool _instr_profiling_supported = IS_ENABLED(CONFIG_INSTRUMENTATION_MODE_STATISTICAL);

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
/*
 * Entry for discovered functions. The functions are added to 'disco_func' array
 * as they are called in the execution flow, hence "discovered" functions. Once
 * NUM_DISCO_FUNCS is reached, additional new executed functions are ignored and
 * no profiling information is collected for them.
 */

#define MAX_CALL_DEPTH CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_CALL_DEPTH
struct disco_func_entry {
	timing_t entry_timestamp;		/* Timestamp at function entry */
	uint64_t delta_t;			/* Accumulated (per function) delta time */
	void *addr;				/* Function address/ID */
	uint16_t call_depth;			/* Call depth */
};

#define MAX_NUM_DISCO_FUNC CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_NUM_FUNC
static int num_disco_func;
struct disco_func_entry disco_func[MAX_NUM_DISCO_FUNC] = { 0 };

/* To track the number of unbalanced/spurious entry/exist pairs, for debugging */
int unbalanced;
#endif

/* See instr_fundamentals_initialized() */
uint16_t magic = 0xABBA;

/* Defined in Kconfig */
extern int INSTR_TRIGGER_FUNCTION(void);
extern int INSTR_STOPPER_FUNCTION(void);

/* Default trigger and stopper addresses, from Kconfig */
void *k_trigger_callee = INSTR_TRIGGER_FUNCTION;
void *k_stopper_callee = INSTR_STOPPER_FUNCTION;

/* Current (live) trigger and stopper addresses */
void *trigger_callee;
void *stopper_callee;

bool instr_tracing_supported(void)
{
	return _instr_tracing_supported;
}

bool instr_profiling_supported(void)
{
	return _instr_profiling_supported;
}

__no_instrumentation__
int instr_init(void)
{
	/*
	 * This function can never be called before RAM is properly initialized. See comment in
	 * instr_fundamentals_initialized() for more details.
	 */
	assert(magic == 0xABBA);

	/*
	 * This flag needs to be set before calling any other function, otherwise it will cause an
	 * infinite recursion in the handler since instr_initialized() will return 0 and
	 * instr_init() will be called again.
	 */
	_instr_initialized = 1;

	if (retention_is_valid(retention0)) {
		/* Retained mem is already initialized, load trigger and stopper addresses */
		retention_read(retention0, 0, (uint8_t *)&trigger_callee,
				sizeof(trigger_callee));
		retention_read(retention0, sizeof(trigger_callee), (uint8_t *)&stopper_callee,
				sizeof(stopper_callee));
	} else {
		/* Retained mem not initialized, so write defaults */
		trigger_callee = k_trigger_callee;
		retention_write(retention0, 0, (const uint8_t *)&trigger_callee,
				sizeof(trigger_callee));
		stopper_callee = k_stopper_callee;
		retention_write(retention0, sizeof(trigger_callee),
				(const uint8_t *)&stopper_callee, sizeof(stopper_callee));
	}

#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	/* Initialize ring buffer */
	instr_rb_init();
#endif

	/* Init and start counters for timestamping */
	instr_timestamp_init();

	/*
	 * Enable instrumentation. When instrumentation is enabled it means it can be turned on and
	 * off. It will first be turned on when the trigger function is first called and turned off
	 * when stopper function exits. This two step mechanism allow disabling instrumentation at
	 * runtime in critical sections inside the instrumentation code by calling instr_disable()/
	 * instr_enable() at runtime, hence avoiding infinite loop and having to exclude a function
	 * at compile time. In other words, it allows a better granularity for enabling/disabling
	 * the instrumentation.
	 */
	instr_enable();

	return 0;
}

__no_instrumentation__
bool instr_initialized(void)
{
	return _instr_initialized;
}

/*
 * Instrumentation can only be used when RAM is correctly initialized in early
 * boot stages and so variables in memory -- for example, _instr_initialized --
 * are correctly initialized. To ensure such a condition, a given variable,
 * 'magic', has its value in memory checked against a constant (a magic number)
 * that is kept in code (flash). Once the value matches the constant it means
 * the RAM is correctly initialized and so instrumentation can be initialized
 * properly and enabled for use.
 */
__no_instrumentation__
bool instr_fundamentals_initialized(void)
{
	if (magic == 0xABBA) {
		return true;
	} else {
		return false;
	}
}

__no_instrumentation__
int instr_enable(void)
{
	_instr_enabled = true;

	return 0;
}

__no_instrumentation__
int instr_disable(void)
{
	_instr_enabled = false;

	return 0;
}

__no_instrumentation__
bool instr_enabled(void)
{
	return _instr_enabled;
}

__no_instrumentation__
int instr_turn_on(void)
{
	_instr_on = true;

	return 0;
}

__no_instrumentation__
int instr_turn_off(void)
{
	_instr_on = false;

	return 0;
}

__no_instrumentation__
bool instr_turned_on(void)
{
	if (!_instr_enabled) {
		/* If instrumentation is disabled always return off state */
		return false;
	} else {
		return _instr_on;
	}
}

__no_instrumentation__
bool instr_trace_enabled(void)
{
	return !_instr_tracing_disabled;
}

__no_instrumentation__
bool instr_profile_enabled(void)
{
	return !_instr_profiling_disabled;
}

__no_instrumentation__
void instr_set_trigger_func(void *callee)
{
	/* Update trigger_callee before updating retained mem */
	trigger_callee = callee;

	retention_write(retention0, 0, (const uint8_t *)&trigger_callee,
			sizeof(trigger_callee));
}

__no_instrumentation__
void instr_set_stop_func(void *callee)
{
	/* Update stopper_callee before updatting retained mem */
	stopper_callee = callee;

	retention_write(retention0, sizeof(trigger_callee), (const uint8_t *)&stopper_callee,
			sizeof(stopper_callee));
}

__no_instrumentation__
void *instr_get_trigger_func(void)
{
	return trigger_callee;
}

__no_instrumentation__
void *instr_get_stop_func(void)
{
	return stopper_callee;
}

__no_instrumentation__
void instr_dump_buffer_uart(void)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	static const struct device *const uart_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	/* Event record */
	struct instr_record record;

	/* Make sure instrumentation is disable. */
	instr_disable();

	/* Initiator mark */
	printk("-*-#");

	/* TODO(gromero): Use zero-copy to copy from buffer directly to the
	 * UART FIFO instead of copying first from buffer to 'record'.
	 */
	while (instr_rb_get_item(&record) != NULL) {
		for (int i = 0; i < instr_rb_get_item_size(record.header.type); i++) {
			uart_poll_out(uart_dev, *((uint8_t *)&record + i));
		}
	}

	/* Terminator mark */
	printk("-*-!\n");
#endif
}

__no_instrumentation__
void instr_dump_deltas_uart(void)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
	static const struct device *const uart_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	instr_disable();

	/* Initiator mark */
	printk("-*-#");

	for (int i = 0; i < num_disco_func; i++) {
		uart_poll_out(uart_dev, INSTR_EVENT_PROFILE);
		for (int j = 0; j < sizeof(disco_func[i].addr); j++) {
			uart_poll_out(uart_dev, *((uint8_t *)&disco_func[i].addr + j));
		}
		for (int k = 0; k < sizeof(disco_func[i].delta_t); k++) {
			uart_poll_out(uart_dev, *((uint8_t *)&disco_func[i].delta_t + k));
		}
	}

	/* Terminator mark */
	printk("-*-!\n");
#endif
}

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
__no_instrumentation__
void push_callee_timestamp(void *callee)
{
	bool found = false;
	int curr_func;

	/* Find callee in the discovered function array */
	for (curr_func = 0; curr_func < num_disco_func; curr_func++) {
		if (disco_func[curr_func].addr == callee) {
			found = 1;
			break;
		}
	}

	if (!found) { /* New function discovered */
		if (num_disco_func >= MAX_NUM_DISCO_FUNC)
			/* No more space to add another function */
			return;


		disco_func[curr_func].delta_t = 0;
		disco_func[curr_func].call_depth = 0;
		disco_func[curr_func].addr = callee;

		num_disco_func++;
	}

	/* New function or no other instance of function active (called): record timestamp */
	if (disco_func[curr_func].call_depth == 0) {
		disco_func[curr_func].entry_timestamp = instr_timestamp_ns();
	}

	/* Update call depth if not reached out maximum call depth */
	if (disco_func[curr_func].call_depth < MAX_CALL_DEPTH) {
		disco_func[curr_func].call_depth++;
	}
}

__no_instrumentation__
void pop_callee_timestamp(void *callee)
{
	uint64_t dt_ns;
	uint64_t entry_timestamp;
	uint64_t exit_timestamp;
	int curr_func;

	for (curr_func = 0; curr_func < num_disco_func; curr_func++) {
		if (disco_func[curr_func].addr == callee) {
			disco_func[curr_func].call_depth--;

			/* Last active function is returning */
			if (disco_func[curr_func].call_depth == 0) {
				entry_timestamp = disco_func[curr_func].entry_timestamp;
				exit_timestamp = instr_timestamp_ns(); /* Now */

				/* Compute delta T */
				dt_ns = exit_timestamp - entry_timestamp;

				/* Accummulate delta T */
				disco_func[curr_func].delta_t += dt_ns;

			}

			return;
		}
	}

	/* Track number of unbalanced/spurious function exits */
	unbalanced++;
}
#endif

__no_instrumentation__
void save_context(struct instr_record *record)
{
	k_tid_t curr_thread;

	curr_thread = k_current_get();

	record->context.cpu = arch_proc_id();
	record->context.thread_id = curr_thread;
	record->context.mode = curr_thread ? k_thread_priority_get(curr_thread) : 0;
#ifdef CONFIG_THREAD_NAME
	if (curr_thread) {
		k_thread_name_copy(curr_thread, record->context.thread_name, 20);
	} else { /* Not in a thread context */
		strncpy(record->context.thread_name, "thread-none", 12);
	}
#endif
}

#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
__no_instrumentation__
enum instr_event_types promote_event_type(enum instr_event_types type, void *callee,
		unsigned int *lock_key)
{
	/*
	 * Context switch events.
	 */

	/*
	 * Only when z_thread_mark_switched_in/out are entered a trace event is
	 * recorded, i.e. it doesn't matter when such a functions return. So
	 * scheduler INSTR_EVENT_EXIT events are discarded by promoting them to
	 * INSTR_EVENT_INVALID. Later, on the host side, a pair of in and out
	 * INSTR_EVENT_ENTRY events will be used to compose a single ftrace
	 * sched_switch event.
	 */
	if (callee == z_thread_mark_switched_in) {
		if (type == INSTR_EVENT_EXIT) {
			return INSTR_EVENT_INVALID;
		}
		*lock_key = irq_lock();
		type = INSTR_EVENT_SCHED_IN;
	}

	if (callee == z_thread_mark_switched_out) {
		if (type == INSTR_EVENT_EXIT) {
			return INSTR_EVENT_INVALID;
		}
		*lock_key = irq_lock();
		type = INSTR_EVENT_SCHED_OUT;
	}

	/*
	 * Other ENTRY and EXIT events are not promoted.
	 */

	/*
	 * Add other type promotions below.
	 * ...
	 */

	return type;
}
#endif

__no_instrumentation__
void instr_event_handler(enum instr_event_types type, void *callee, void *caller)
{
#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	/* For IRQ locking */
	unsigned int key = 0;
#endif

	/*
	 * Essentially, the instrumented code can only generate events when a
	 * function is called or returns. Event type promotion happens based on
	 * context, when entry and exit events are transformed to new ones based
	 * on the context (see promote_event_type).
	 */
	assert(type == INSTR_EVENT_ENTRY || type == INSTR_EVENT_EXIT);

	if (!instr_turned_on()) {
		return;
	}

	/* Enter critical section */
	instr_disable();

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
	/* Profiling */
	if (!_instr_profiling_disabled) {
		if (type == INSTR_EVENT_ENTRY) {
			/* Record current timestamp */
			push_callee_timestamp(callee);
		}

		if (type == INSTR_EVENT_EXIT) {
			/* Compute delta time for callee and accumulate it */
			pop_callee_timestamp(callee);
		}
	}
#endif

#if defined(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
	/* For tracing, promote type based on the context */
	type = promote_event_type(type, callee, &key);
	if (type == INSTR_EVENT_INVALID) {
		/* Don't trace invalid events */
		instr_enable();
		return;
	}

	/* Tracing */
	if (!_instr_tracing_disabled) {
		struct instr_record *record;    /* For zero-copy case */
		struct instr_record tmp_record; /* For non-zero-copy case */

		/*
		 * Check if it's possible to zero-copy (which implies no
		 * overwriting).
		 */
		record = instr_rb_put_item_claim(type);
		if (record != NULL) {
			record->header.type = type;
			record->callee = callee;
			record->caller = caller;
			record->timestamp = instr_timestamp_ns();

			save_context(record);

			instr_rb_put_item_finish(type);

		} else {
			if (IS_ENABLED(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_BUFFER_OVERWRITE)) {
				record = &tmp_record;

				/*
				 * No zero-copy way since buffer wraps.
				 *
				 * TODO(gromero): refactor below as a
				 * single helper function?
				 */
				record->header.type = type;
				record->callee = callee;
				record->caller = caller;
				record->timestamp = instr_timestamp_ns();

				save_context(record);

				/* No zero-copy. */
				instr_rb_put_item_wrapping(record);

			} else {/* No overwriting mode */
				/* Buffer is full, so disable tracing. */
				_instr_tracing_disabled = 1;
				return;
			}
		}

	}

	if (type == INSTR_EVENT_SCHED_IN ||
		type == INSTR_EVENT_SCHED_OUT) {
		irq_unlock(key);
	}
#endif

	instr_enable();
}
