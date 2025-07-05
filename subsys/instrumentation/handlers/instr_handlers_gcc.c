/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/instrumentation/instrumentation.h>
#include <instr_timestamp.h>

__no_instrumentation__
void __cyg_profile_func_enter(void *callee, void *caller)
{
	/*
	 * On early boot it's not always possible to initialize the
	 * instrumentation. Only when instr_fundamentals_initialized returns
	 * true is when it's possible to initialize and enable the
	 * instrumentation subsys.
	 */
	if (!instr_fundamentals_initialized()) {
		return;
	}

	/* It's now possible to initialize and enable the instrumentation */
	if (!instr_initialized()) {
		instr_init();
	}

	/* Turn on instrumentation if trigger is called */
	if (callee == instr_get_trigger_func() && !instr_turned_on()) {
		instr_turn_on();
	}

	if (!instr_enabled()) {
		return;
	}

	instr_event_handler(INSTR_EVENT_ENTRY, callee, caller);
}

__no_instrumentation__
void __cyg_profile_func_exit(void *callee, void *caller)
{
	if (!instr_fundamentals_initialized()) {
		return;
	}

	if (!instr_enabled()) {
		return;
	}

	instr_event_handler(INSTR_EVENT_EXIT, callee, caller);

	/* Turn off instrumentation if stopper returns */
	if (callee == instr_get_stop_func() && instr_turned_on()) {
		instr_turn_off();
	}
}
