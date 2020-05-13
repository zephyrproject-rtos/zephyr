/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_mbox_kinit(void);
extern void test_mbox_kdefine(void);
extern void test_enhance_capability(void);
extern void test_define_multi_mbox(void);
extern void test_mbox_put_get_null(void);
extern void test_mbox_put_get_buffer(void);
extern void test_mbox_async_put_get_buffer(void);
extern void test_mbox_async_put_get_block(void);
extern void test_mbox_target_source_thread_buffer(void);
extern void test_mbox_target_source_thread_block(void);
extern void test_mbox_incorrect_receiver_tid(void);
extern void test_mbox_incorrect_transmit_tid(void);
extern void test_mbox_timed_out_mbox_get(void);
extern void test_mbox_block_get_invalid_pool(void);
extern void test_mbox_msg_tid_mismatch(void);
extern void test_mbox_block_get_buff_to_pool(void);
extern void test_mbox_block_get_buff_to_smaller_pool(void);
extern void test_mbox_dispose_size_0_msg(void);
extern void test_mbox_clean_up_tx_pool(void);
extern void test_mbox_async_put_to_waiting_get(void);
extern void test_mbox_get_waiting_put_incorrect_tid(void);
extern void test_mbox_async_multiple_put(void);
extern void test_mbox_multiple_waiting_get(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(mbox_api,
			 ztest_1cpu_unit_test(test_mbox_kinit),/*keep init first!*/
			 ztest_1cpu_unit_test(test_mbox_kdefine),
			 ztest_unit_test(test_enhance_capability),
			 ztest_unit_test(test_define_multi_mbox),
			 ztest_1cpu_unit_test(test_mbox_put_get_null),
			 ztest_unit_test(test_mbox_put_get_buffer),
			 ztest_1cpu_unit_test(test_mbox_async_put_get_buffer),
			 ztest_1cpu_unit_test(test_mbox_async_put_get_block),
			 ztest_unit_test(test_mbox_target_source_thread_buffer),
			 ztest_1cpu_unit_test(test_mbox_target_source_thread_block),
			 ztest_unit_test(test_mbox_incorrect_receiver_tid),
			 ztest_1cpu_unit_test(test_mbox_incorrect_transmit_tid),
			 ztest_1cpu_unit_test(test_mbox_timed_out_mbox_get),
			 ztest_unit_test(test_mbox_block_get_invalid_pool),
			 ztest_unit_test(test_mbox_msg_tid_mismatch),
			 ztest_1cpu_unit_test(test_mbox_block_get_buff_to_pool),
			 ztest_1cpu_unit_test(
				test_mbox_block_get_buff_to_smaller_pool),
			 ztest_1cpu_unit_test(test_mbox_dispose_size_0_msg),
			 ztest_1cpu_unit_test(test_mbox_clean_up_tx_pool),
			 ztest_1cpu_unit_test(test_mbox_async_put_to_waiting_get),
			 ztest_unit_test(
				test_mbox_get_waiting_put_incorrect_tid),
			 ztest_1cpu_unit_test(test_mbox_async_multiple_put),
			 ztest_1cpu_unit_test(test_mbox_multiple_waiting_get));
	ztest_run_test_suite(mbox_api);
}
