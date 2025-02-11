/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"
#include "main.h"
#include "argparse.h"
#include "bs_pc_backchannel.h"
#include "bstests.h"

#include <zephyr/sys/__assert.h>

void server_procedure(void);
void client_procedure(void);

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * USEC_PER_SEC)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(30)

static int test_round;
static int final_round;
static char *settings_file;

int get_test_round(void)
{
	return test_round;
}

bool is_final_round(void)
{
	return test_round == final_round;
}

char *get_settings_file(void)
{
	return settings_file;
}

static void test_args(int argc, char **argv)
{
	__ASSERT(argc == 3, "Please specify only 3 test arguments\n");

	test_round = atol(argv[0]);
	final_round = atol(argv[1]);
	settings_file = argv[2];

	bs_trace_raw(0, "Test round %u\n", test_round);
	bs_trace_raw(0, "Final round %u\n", final_round);
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = server_procedure,
		.test_args_f = test_args,
	},
	{
		.test_id = "client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = client_procedure,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = { install, NULL };

int main(void)
{
	bst_main();
	return 0;
}


void backchannel_init(void)
{
	uint device_number = get_device_nbr();
	uint channel_numbers[2] = { 0, 0, };
	uint device_numbers[2];
	uint num_ch;
	uint *ch;

	/* No backchannels to next/prev device if only device */
	if (get_test_round() == 0 && is_final_round()) {
		return;
	}

	/* Each `server` round/instance gets a connection to the previous and to
	 * the next instance in the chain. It waits until it is signalled by the
	 * previous instance, then runs its test procedure and finally signals
	 * the next instance in the chain.
	 *
	 * The two ends of the chain get only one channel, hence the difference
	 * in handling.
	 */

	if (get_test_round() == 0) {
		/* send only */
		device_numbers[0] = get_device_nbr() + 1;
		num_ch = 1;

	} else if (is_final_round()) {
		/* receive only */
		device_numbers[0] = get_device_nbr() - 1;
		num_ch = 1;

	} else {
		/* send signal */
		device_numbers[0] = get_device_nbr() + 1;
		/* receive signal */
		device_numbers[1] = get_device_nbr() - 1;
		num_ch = 2;
	}

	printk("Opening backchannels\n");
	ch = bs_open_back_channel(device_number, device_numbers,
				  channel_numbers, num_ch);
	if (!ch) {
		FAIL("Unable to open backchannel\n");
	}
}

#define MSG_SIZE 1

void backchannel_sync_send(uint channel)
{
	uint8_t sync_msg[MSG_SIZE] = { get_device_nbr() };

	printk("Sending sync\n");
	bs_bc_send_msg(channel, sync_msg, ARRAY_SIZE(sync_msg));
}

void backchannel_sync_wait(uint channel)
{
	uint8_t sync_msg[MSG_SIZE];

	while (true) {
		if (bs_bc_is_msg_received(channel) > 0) {
			bs_bc_receive_msg(channel, sync_msg,
					  ARRAY_SIZE(sync_msg));
			if (sync_msg[0] != get_device_nbr()) {
				/* Received a message from another device, exit */
				break;
			}
		}

		k_sleep(K_MSEC(1));
	}

	printk("Sync received\n");
}

/* We can't really kill the device/process without borking the bsim
 * backchannels, so the next best thing is stopping all threads from processing,
 * thus stopping the Bluetooth host from processing the disconnect event (or any
 * event, really) coming from the link-layer.
 */
static void stop_all_threads(void)
{
	/* promote to highest priority */
	k_thread_priority_set(k_current_get(), K_HIGHEST_THREAD_PRIO);
	/* busy-wait loop */
	for (;;) {
		k_busy_wait(1000);
		k_yield();
	}
}

void signal_next_test_round(void)
{
	if (!is_final_round()) {
		backchannel_sync_send(0);
	}

	PASS("round %d over\n", get_test_round());
	stop_all_threads();
}

void wait_for_round_start(void)
{
	backchannel_init();

	if (is_final_round()) {
		backchannel_sync_wait(0);
	} else if (get_test_round() != 0) {
		backchannel_sync_wait(1);
	}
}
