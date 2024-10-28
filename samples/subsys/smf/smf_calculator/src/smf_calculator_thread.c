/*
 * Copyright (c) 2024 Glenn Andrews
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <stdbool.h>
#include "smf_calculator_thread.h"
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include <stdio.h>

LOG_MODULE_REGISTER(smf_thread, LOG_LEVEL_DBG);

K_MSGQ_DEFINE(event_msgq, sizeof(struct calculator_event), 8, 1);

/* In theory can still overflow. A double can be hundreds of characters long */
#define RESULT_STRING_LENGTH 64

enum display_mode {
	DISPLAY_OPERAND_1,
	DISPLAY_OPERAND_2,
	DISPLAY_RESULT,
	DISPLAY_ERROR,
};

struct operand {
	char string[CALCULATOR_STRING_LENGTH];
	int index;
};

/* User defined object */
struct s_object {
	/* This must be first */
	struct smf_ctx ctx;

	/* Other state specific data add here */
	struct calculator_event event;

	struct operand operand_1;
	struct operand operand_2;
	char operator_btn;
	struct operand result;
} s_obj;

static enum display_mode current_display_mode = DISPLAY_OPERAND_1;

static void set_display_mode(enum display_mode mode)
{
	current_display_mode = mode;
}

static void setup_operand(struct operand *op)
{
	/* indexes start at 1 because position 0 is the sign */
	op->index = 1;
	op->string[0] = ' '; /* space for sign */
	op->string[1] = '0'; /* A 0 indicator to be overwritten */
	op->string[2] = 0x00;
}

static int insert(struct operand *op, char digit)
{
	if (op->index >= (CALCULATOR_STRING_LENGTH - 1)) {
		/* Can't store an extra operand */
		return -ENOBUFS;
	}
	op->string[op->index++] = digit;
	op->string[op->index] = 0x00;

	return 0;
}

static void negate(struct operand *op)
{
	if (op->string[0] == ' ') {
		op->string[0] = '-';
	} else {
		op->string[0] = ' ';
	}
}

/**
 * @brief copies one operand struct to another
 * Assumes src operand is well-formed
 */
static void copy_operand(struct operand *dest, struct operand *src)
{
	strncpy(dest->string, src->string, CALCULATOR_STRING_LENGTH);
	dest->index = src->index;
}

static int calculate_result(struct s_object *s)
{
	double operand_1 = strtod(s->operand_1.string, NULL);
	double operand_2 = strtod(s->operand_2.string, NULL);
	double result;
	char result_string[RESULT_STRING_LENGTH];

	switch (s->operator_btn) {
	case '+':
		result = operand_1 + operand_2;
		break;
	case '-':
		result = operand_1 - operand_2;
		break;
	case '*':
		result = operand_1 * operand_2;
		break;
	case '/':
		if (operand_2 != 0.0) {
			result = operand_1 / operand_2;
		} else {
			/* division by zero */
			return -1;
		}
		break;
	default:
		/* unknown operator */
		return -1;
	}

	snprintf(result_string, RESULT_STRING_LENGTH, "% f", result);

	/* Strip off trailing zeroes from result */
	for (int i = strlen(result_string) - 1; i >= 0; i--) {
		if (result_string[i] != '0') {
			/* was it .000? */
			if (result_string[i] == '.') {
				result_string[i] = 0x00;
			} else {
				result_string[i + 1] = 0x00;
			}
			break;
		}
	}

	/* copy to the result operand */
	strncpy(s->result.string, result_string, CALCULATOR_STRING_LENGTH - 1);
	s->result.string[CALCULATOR_STRING_LENGTH - 1] = 0x00;
	s->result.index = strlen(s->result.string);

	return 0;
}

/* copy result into operand_1 and clear operand_2 to allow chaining calculations */
static void chain_calculations(struct s_object *s)
{
	copy_operand(&s->operand_1, &s->result);
	setup_operand(&s->operand_2);
}

/* Declaration of possible states */
enum demo_states {
	STATE_ON,
	STATE_READY,
	STATE_RESULT,
	STATE_BEGIN,
	STATE_NEGATED_1,
	STATE_OPERAND_1,
	STATE_ZERO_1,
	STATE_INT_1,
	STATE_FRAC_1,
	STATE_NEGATED_2,
	STATE_OPERAND_2,
	STATE_ZERO_2,
	STATE_INT_2,
	STATE_FRAC_2,
	STATE_OP_ENTERED,
	STATE_OP_CHAINED,
	STATE_OP_NORMAL,
	STATE_ERROR,
};

/* Forward declaration of state table */
static const struct smf_state calculator_states[];

static void on_entry(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	setup_operand(&s->operand_1);
	setup_operand(&s->operand_2);
	setup_operand(&s->result);
	s->operator_btn = 0x00;
}

static void on_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case CANCEL_BUTTON:
		smf_set_state(&s->ctx, &calculator_states[STATE_ON]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void ready_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DECIMAL_POINT:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_1]);
		break;
	case DIGIT_1_9:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_INT_1]);
		break;
	case DIGIT_0:
		/* Don't insert the leading zero */
		smf_set_state(&s->ctx, &calculator_states[STATE_ZERO_1]);
		break;
	case OPERATOR:
		s->operator_btn = s->event.operand;
		smf_set_state(&s->ctx, &calculator_states[STATE_OP_CHAINED]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}
static void result_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_RESULT);
}

static void result_run(void *obj)
{
	LOG_DBG("");
	/* Ready state handles the exits from this state */
}

static void begin_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_OPERAND_1);
}

static void begin_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case OPERATOR:
		/* We only care about negative */
		if (s->event.operand == '-') {
			smf_set_state(&s->ctx, &calculator_states[STATE_NEGATED_1]);
		}
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void negated_1_entry(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	negate(&s->operand_1);
}

static void negated_1_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DECIMAL_POINT:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_1]);
		break;
	case DIGIT_1_9:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_INT_1]);
		break;
	case DIGIT_0:
		/* Don't need to insert leading zeroes */
		smf_set_state(&s->ctx, &calculator_states[STATE_ZERO_1]);
		break;
	case OPERATOR:
		/* We only care about ignoring the negative key */
		if (s->event.operand == '-') {
			smf_set_handled(&s->ctx);
		}
		break;
	case CANCEL_ENTRY:
		setup_operand(&s->operand_1);
		smf_set_state(&s->ctx, &calculator_states[STATE_BEGIN]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void operand_1_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_OPERAND_1);
}

static void operand_1_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case OPERATOR:
		s->operator_btn = s->event.operand;
		smf_set_state(&s->ctx, &calculator_states[STATE_OP_ENTERED]);
		break;
	case CANCEL_ENTRY:
		setup_operand(&s->operand_1);
		smf_set_state(&s->ctx, &calculator_states[STATE_READY]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void zero_1_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
		/* We ignore leading zeroes */
		smf_set_handled(&s->ctx);
		break;
	case DIGIT_1_9:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_INT_1]);
		break;
	case DECIMAL_POINT:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_1]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void int_1_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
	case DIGIT_1_9:
		insert(&s->operand_1, s->event.operand);
		smf_set_handled(&s->ctx);
		break;
	case DECIMAL_POINT:
		insert(&s->operand_1, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_1]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void frac_1_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
	case DIGIT_1_9:
		insert(&s->operand_1, s->event.operand);
		smf_set_handled(&s->ctx);
		break;
	case DECIMAL_POINT:
		/* Ignore further decimal points */
		smf_set_handled(&s->ctx);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void negated_2_entry(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	negate(&s->operand_2);
}

static void negated_2_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DECIMAL_POINT:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_2]);
		break;
	case DIGIT_1_9:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_INT_2]);
		break;
	case DIGIT_0:
		/* Don't insert the leading zero */
		smf_set_state(&s->ctx, &calculator_states[STATE_ZERO_2]);
		break;
	case OPERATOR:
		/* We only care about ignoring the negative key */
		if (s->event.operand == '-') {
			smf_set_handled(&s->ctx);
		}
		break;
	case CANCEL_ENTRY:
		setup_operand(&s->operand_2);
		smf_set_state(&s->ctx, &calculator_states[STATE_OP_ENTERED]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void operand_2_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_OPERAND_2);
}

static void operand_2_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case CANCEL_ENTRY:
		setup_operand(&s->operand_2);
		smf_set_state(&s->ctx, &calculator_states[STATE_OP_ENTERED]);
		break;
	case OPERATOR:
		if (calculate_result(s) == 0) {
			/* move result into operand_1 to allow chaining */
			chain_calculations(s);
			/* Copy in new operand */
			s->operator_btn = s->event.operand;
			smf_set_state(&s->ctx, &calculator_states[STATE_OP_CHAINED]);
		} else {
			smf_set_state(&s->ctx, &calculator_states[STATE_ERROR]);
		}
		break;
	case EQUALS:
		if (calculate_result(s) == 0) {
			/* move result into operand_1 to allow chaining */
			chain_calculations(s);
			smf_set_state(&s->ctx, &calculator_states[STATE_RESULT]);
		} else {
			smf_set_state(&s->ctx, &calculator_states[STATE_ERROR]);
		}
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void zero_2_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
		/* We ignore leading zeroes */
		smf_set_handled(&s->ctx);
		break;
	case DIGIT_1_9:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_INT_2]);
		break;
	case DECIMAL_POINT:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_2]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void int_2_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
	case DIGIT_1_9:
		insert(&s->operand_2, s->event.operand);
		smf_set_handled(&s->ctx);
		break;
	case DECIMAL_POINT:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_2]);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void frac_2_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
	case DIGIT_1_9:
		insert(&s->operand_2, s->event.operand);
		smf_set_handled(&s->ctx);
		break;
	case DECIMAL_POINT:
		/* Ignore further decimal points */
		smf_set_handled(&s->ctx);
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void op_entered_run(void *obj)
{
	struct s_object *s = (struct s_object *)obj;

	LOG_DBG("");

	switch (s->event.event_id) {
	case DIGIT_0:
		/* Don't insert the leading zero */
		smf_set_state(&s->ctx, &calculator_states[STATE_ZERO_2]);
		break;
	case DIGIT_1_9:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_INT_2]);
		break;
	case DECIMAL_POINT:
		insert(&s->operand_2, s->event.operand);
		smf_set_state(&s->ctx, &calculator_states[STATE_FRAC_2]);
		break;
	case OPERATOR:
		/* We only care about negative */
		if (s->event.operand == '-') {
			smf_set_state(&s->ctx, &calculator_states[STATE_NEGATED_2]);
		}
		break;
	default:
		/* Let parent state handle it: shuts up compiler warning */
		break;
	}
}

static void op_chained_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_OPERAND_1);
}

static void op_normal_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_OPERAND_2);
}

static void error_entry(void *obj)
{
	LOG_DBG("");
	set_display_mode(DISPLAY_ERROR);
}

/* State storage: handler functions, parent states and initial transition states */
/* clang-format off */
static const struct smf_state calculator_states[] = {
	[STATE_ON] =         SMF_CREATE_STATE(on_entry, on_run, NULL,
						NULL, &calculator_states[STATE_READY]),
	[STATE_READY] =      SMF_CREATE_STATE(NULL, ready_run, NULL,
						&calculator_states[STATE_ON],
						&calculator_states[STATE_BEGIN]),
	[STATE_RESULT] =     SMF_CREATE_STATE(result_entry, result_run,
						NULL, &calculator_states[STATE_READY], NULL),
	[STATE_BEGIN] =      SMF_CREATE_STATE(begin_entry, begin_run, NULL,
						&calculator_states[STATE_READY], NULL),
	[STATE_NEGATED_1] =  SMF_CREATE_STATE(negated_1_entry, negated_1_run, NULL,
						&calculator_states[STATE_ON], NULL),
	[STATE_OPERAND_1] =  SMF_CREATE_STATE(operand_1_entry, operand_1_run, NULL,
						&calculator_states[STATE_ON], NULL),
	[STATE_ZERO_1] =     SMF_CREATE_STATE(NULL, zero_1_run, NULL,
						&calculator_states[STATE_OPERAND_1], NULL),
	[STATE_INT_1] =      SMF_CREATE_STATE(NULL, int_1_run, NULL,
						&calculator_states[STATE_OPERAND_1], NULL),
	[STATE_FRAC_1] =     SMF_CREATE_STATE(NULL, frac_1_run, NULL,
						&calculator_states[STATE_OPERAND_1], NULL),
	[STATE_NEGATED_2] =  SMF_CREATE_STATE(negated_2_entry, negated_2_run, NULL,
						&calculator_states[STATE_ON], NULL),
	[STATE_OPERAND_2] =  SMF_CREATE_STATE(operand_2_entry, operand_2_run, NULL,
						&calculator_states[STATE_ON], NULL),
	[STATE_ZERO_2] =     SMF_CREATE_STATE(NULL, zero_2_run, NULL,
						&calculator_states[STATE_OPERAND_2], NULL),
	[STATE_INT_2] =      SMF_CREATE_STATE(NULL, int_2_run, NULL,
						&calculator_states[STATE_OPERAND_2], NULL),
	[STATE_FRAC_2] =     SMF_CREATE_STATE(NULL, frac_2_run, NULL,
						&calculator_states[STATE_OPERAND_2], NULL),
	[STATE_OP_ENTERED] = SMF_CREATE_STATE(NULL, op_entered_run, NULL,
						&calculator_states[STATE_ON],
						&calculator_states[STATE_OP_NORMAL]),
	[STATE_OP_CHAINED] = SMF_CREATE_STATE(op_chained_entry, NULL, NULL,
						&calculator_states[STATE_OP_ENTERED], NULL),
	[STATE_OP_NORMAL] =  SMF_CREATE_STATE(op_normal_entry, NULL, NULL,
						&calculator_states[STATE_OP_ENTERED], NULL),
	[STATE_ERROR] =      SMF_CREATE_STATE(error_entry, NULL, NULL,
						&calculator_states[STATE_ON], NULL),
};
/* clang-format on */

int post_calculator_event(struct calculator_event *event, k_timeout_t timeout)
{
	return k_msgq_put(&event_msgq, event, timeout);
}

static void output_display(void)
{
	char *output;

	switch (current_display_mode) {
	case DISPLAY_OPERAND_1:
		output = s_obj.operand_1.string;
		break;
	case DISPLAY_OPERAND_2:
		output = s_obj.operand_2.string;
		break;
	case DISPLAY_RESULT:
		output = s_obj.result.string;
		break;
	case DISPLAY_ERROR:
		output = "ERROR";
		break;
	default:
		output = "";
	}
	update_display(output);
}

static void smf_calculator_thread(void *arg1, void *arg2, void *arg3)
{
	smf_set_initial(SMF_CTX(&s_obj), &calculator_states[STATE_ON]);
	while (1) {
		int rc;

		rc = k_msgq_get(&event_msgq, &s_obj.event, K_FOREVER);
		if (rc != 0) {
			continue;
		}
		/* run state machine with given message */

		LOG_INF("Received %c from GUI", s_obj.event.operand);
		int ret = smf_run_state(SMF_CTX(&s_obj));

		if (ret) {
			/* State machine was terminated if a non-zero value is returned */
			break;
		}

		output_display();
		LOG_INF("op1=%s, op=%c op2=%s res=%s", s_obj.operand_1.string, s_obj.operator_btn,
			s_obj.operand_2.string, s_obj.result.string);
	}
}

K_THREAD_DEFINE(smf_calculator, SMF_THREAD_STACK_SIZE, smf_calculator_thread, NULL, NULL, NULL,
		SMF_THREAD_PRIORITY, 0, 0);
