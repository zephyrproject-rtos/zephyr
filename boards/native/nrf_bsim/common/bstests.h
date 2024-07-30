/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BSTESTS_ENTRY_H
#define _BSTESTS_ENTRY_H

#include "bs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TEST HOOKS:
 *
 * You may register some of these functions in your testbench
 *
 * Note that you can overwrite this function pointers on the fly
 */
/*
 * Will be called with the command line arguments for the testcase.
 * This is BEFORE any SW has run, and before the HW has been initialized
 * This is also before a possible initialization delay.
 * Note that this function can be used for test pre-initialization steps
 * like opening the back-channels. But you should not interact yet with the
 * test ticker or other HW models.
 */
typedef void (*bst_test_args_t)(int, char**);
/* It will be called (in the HW models thread) before the CPU is booted,
 * after the HW models have been initialized. Note that a possible delayed
 * initialization may delay the execution of this function vs other devices
 * tests pre-initialization
 */
typedef void (*bst_test_pre_init_t)(void);
/*
 * It will be called (in the HW models thread) when the CPU goes to sleep
 * for the first time
 */
typedef void (*bst_test_post_init_t)(void);
/* It will be called (in the HW models thread) each time the bst_timer ticks */
typedef void (*bst_test_tick_t)(bs_time_t time);
/*
 * It will be called (in the HW models thread) when the execution is being
 * terminated (clean up memory and close your files here)
 */
typedef void (*bst_test_delete_t)(void);
/*
 * It will be called (in SW context) when a HW interrupt is raised.
 * If it returns true, the normal interrupt handler will NOT be called and
 *  Zephyr will only see a spurious wake
 * Note: Use this only to perform special tasks, like sniffing interrupts,
 * or any other interrupt related cheat, but not as a normal interrupt handler
 */
typedef bool (*bst_test_irq_sniffer_t)(int irq_number);
/*
 * This function will be called (in SW context) as a Zephyr PRE_KERNEL_1
 * device driver initialization function
 * Note that the app's main() has not executed yet, and the kernel is not yet
 * fully ready => You canNOT spawn new threads without wait time yet (or it
 * will crash)
 */
typedef void (*bst_test_fake_ddriver_prekernel_t)(void);
/*
 * This function will be called (in SW context) as a Zephyr POST_KERNEL
 * device driver initialization function
 * You may spawn any test threads you may need here.
 * Note that the app main() has not executed yet.
 */
typedef void (*bst_test_fake_ddriver_postkernel_t)(void);
/*
 * This function will be called (in SW context) as the Zephyr application main
 */
typedef void (*bst_test_main_t)(void);

struct bst_test_instance {
	char *test_id;
	char *test_descr;
	bst_test_args_t                    test_args_f;
	bst_test_pre_init_t                test_pre_init_f;
	bst_test_post_init_t               test_post_init_f;
	bst_test_tick_t                    test_tick_f;
	bst_test_delete_t                  test_delete_f;
	bst_test_irq_sniffer_t             test_irq_sniffer_f;
	bst_test_fake_ddriver_prekernel_t  test_fake_ddriver_prekernel_f;
	bst_test_fake_ddriver_postkernel_t test_fake_ddriver_postkernel_f;
	bst_test_main_t                    test_main_f;
};

#define BSTEST_END_MARKER \
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}

struct bst_test_list {
	struct bst_test_instance  *test_instance;
	struct bst_test_list    *next;
};

typedef struct bst_test_list *(*bst_test_install_t)(struct bst_test_list
							*test_tail);

struct bst_test_list *bst_add_tests(struct bst_test_list *tests,
				    const struct bst_test_instance *test_def);
void bst_set_testapp_mode(char *test_id);
void bst_pass_args(int argc, char **argv);
void bst_pre_init(void);
void bst_post_init(void);
void bst_main(void);
void bst_tick(bs_time_t Absolute_device_time);
bool bst_irq_sniffer(int irq_number);
uint8_t bst_delete(void);

/* These return codes need to fit in a uint8_t (0..255), where 0 = successful */
enum bst_result_t {Passed = 0, In_progress = 1, Failed = 2};

void bst_print_testslist(void);

/**
 * Interface for the fake HW device (timer) dedicated to the tests
 */
void bst_ticker_set_period(bs_time_t tick_period);
void bst_ticker_set_next_tick_absolute(bs_time_t absolute_time);
void bst_ticker_set_next_tick_delta(bs_time_t absolute_time);
void bst_awake_cpu_asap(void);

#ifdef __cplusplus
}
#endif

#endif
