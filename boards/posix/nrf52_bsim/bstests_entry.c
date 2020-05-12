/*
 * Copyright (c) 2017-2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "init.h"
#include <stdint.h>
#include <string.h>
#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

/*
 * Result of the testcase execution.
 * Note that the executable will return the maximum of bst_result and
 * {the HW model return code} to the shell and that
 * {the HW model return code} will be 0 unless it fails or it is
 * configured illegally
 */
enum bst_result_t bst_result;

static struct bst_test_instance *current_test;
static struct bst_test_list *test_list_top;

__attribute__((weak)) bst_test_install_t test_installers[] = { NULL };

struct bst_test_list *bst_add_tests(struct bst_test_list *tests,
				const struct bst_test_instance *test_def)
{
	int idx = 0;
	struct bst_test_list *tail = tests;
	struct bst_test_list *head = tests;

	if (tail) {
		/* First we 'run to end' */
		while (tail->next) {
			tail = tail->next;
		}
	} else {
		if (test_def[idx].test_id != NULL) {
			head = malloc(sizeof(struct bst_test_list));
			head->next = NULL;
			head->test_instance = (struct bst_test_instance *)
						&test_def[idx++];
			tail = head;
		}
	}

	while (test_def[idx].test_id != NULL) {
		tail->next = malloc(sizeof(struct bst_test_list));
		tail = tail->next;
		tail->test_instance = (struct bst_test_instance *)
					&test_def[idx++];
		tail->next = NULL;
	}

	return head;
}

static struct bst_test_instance *bst_test_find(struct bst_test_list *tests,
					  char *test_id)
{
	struct bst_test_list *top = tests;

	while (top != NULL) {
		if (!strcmp(top->test_instance->test_id, test_id)) {
			/* Match found */
			return top->test_instance;
		}
		top = top->next;
	}
	return NULL;
}

void bst_install_tests(void)
{
	int idx = 0;

	if (test_list_top) {
		/* Tests were already installed */
		return;
	}

	/* First execute installers until first test was installed */
	while (!test_list_top && test_installers[idx]) {
		test_list_top = test_installers[idx++](test_list_top);
	}

	/* After that simply add remaining tests to list */
	while (test_installers[idx]) {
		test_installers[idx++](test_list_top);
	}
}

/**
 * Print the tests list displayed with the --testslist command
 * line option
 */
void bst_print_testslist(void)
{
	struct bst_test_list *top;

	/* Install tests */
	bst_install_tests();

	top = test_list_top;
	while (top) {
		bs_trace_raw(0, "TestID: %-10s\t%s\n",
			     top->test_instance->test_id,
			     top->test_instance->test_descr);
		top = top->next;
	}
}

/**
 * Select the testcase to be run from its id
 */
void bst_set_testapp_mode(char *test_id)
{
	/* Install tests */
	bst_install_tests();

	/* By default all tests start as in progress */
	bst_result = In_progress;

	current_test = bst_test_find(test_list_top, test_id);
	if (!current_test) {
		bs_trace_error_line("test id %s doesn't exist\n", test_id);
	}
}

/**
 * Pass to the testcase the command line arguments it may have
 *
 * This function is called after bst_set_testapp_mode
 * and before *init_f
 */
void bst_pass_args(int argc, char **argv)
{
	if (current_test && current_test->test_args_f) {
		current_test->test_args_f(argc, argv);
	}
}

/**
 * Will be called before the CPU is booted
 */
void bst_pre_init(void)
{
	if (current_test && current_test->test_pre_init_f) {
		current_test->test_pre_init_f();
	}
}

/**
 * Will be called when the CPU has gone to sleep for the first time
 */
void bst_post_init(void)
{
	if (current_test && current_test->test_post_init_f) {
		current_test->test_post_init_f();
	}
}

/**
 * Will be called each time the bstest_ticker timer is triggered
 */
void bst_tick(bs_time_t time)
{

	if (current_test == NULL) {
		return;
	}

	if (current_test->test_tick_f) {
		current_test->test_tick_f(time);
	} else if (current_test->test_id) {
		bs_trace_error_line("the test id %s doesn't have a tick handler"
				    " (how come did we arrive here?)\n",
				    current_test->test_id);
	}

}

bool bst_irq_sniffer(int irq_number)
{
	if (current_test && current_test->test_irq_sniffer_f) {
		return current_test->test_irq_sniffer_f(irq_number);
	} else {
		return false;
	}
}

static int bst_fake_device_driver_pre2_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	if (current_test && current_test->test_fake_ddriver_prekernel_f) {
		current_test->test_fake_ddriver_prekernel_f();
	}
	return 0;
}

static int bst_fake_device_driver_post_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	if (current_test && current_test->test_fake_ddriver_postkernel_f) {
		current_test->test_fake_ddriver_postkernel_f();
	}
	return 0;
}

SYS_INIT(bst_fake_device_driver_pre2_init, PRE_KERNEL_1, 0);
SYS_INIT(bst_fake_device_driver_post_init, POST_KERNEL, 0);

void bst_main(void)
{
	if (current_test && current_test->test_main_f) {
		current_test->test_main_f();
	}
}

/**
 * Will be called when the device is being terminated
 */
uint8_t bst_delete(void)
{
	if (current_test && current_test->test_delete_f) {
		current_test->test_delete_f();
	}

	while (test_list_top) {
		struct bst_test_list *tmp = test_list_top->next;

		free(test_list_top);
		test_list_top = tmp;
	}

	return bst_result;
}
