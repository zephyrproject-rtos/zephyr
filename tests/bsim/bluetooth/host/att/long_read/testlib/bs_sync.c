/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <argparse.h>
#include <posix_native_task.h>
#include <bs_pc_backchannel.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <bs_tracing.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(bs_sync, LOG_LEVEL_INF);

static int n_devs;

static void register_more_cmd_args(void)
{
	static bs_args_struct_t args_struct_toadd[] = {
		{
			.option = "D",
			.name = "number_devices",
			.type = 'i',
			.dest = (void *)&n_devs,
			.descript = "Number of devices which will connect in this phy",
			.is_mandatory = true,
		},
		ARG_TABLE_ENDMARKER,
	};

	bs_add_extra_dynargs(args_struct_toadd);
}
NATIVE_TASK(register_more_cmd_args, PRE_BOOT_1, 100);

static uint *backchannels;
static void setup_backchannels(void)
{
	__ASSERT_NO_MSG(n_devs > 0);
	uint self = get_device_nbr();
	uint device_nbrs[n_devs];
	uint channel_numbers[n_devs];

	for (int i = 0; i < n_devs; i++) {
		device_nbrs[i] = i;
		channel_numbers[i] = 0;
	}

	backchannels =
		bs_open_back_channel(self, device_nbrs, channel_numbers, ARRAY_SIZE(device_nbrs));
	__ASSERT_NO_MSG(backchannels != NULL);
}
NATIVE_TASK(setup_backchannels, PRE_BOOT_3, 100);

void bs_bc_receive_msg_sync(uint ch, size_t size, uint8_t *data)
{
	while (bs_bc_is_msg_received(ch) < size) {
		k_msleep(1);
	}
	bs_bc_receive_msg(ch, data, size);
}

void bs_bc_send_uint(uint ch, uint64_t data)
{
	uint8_t data_bytes[sizeof(data)];

	sys_put_le64(data, data_bytes);
	bs_bc_send_msg(ch, data_bytes, sizeof(data_bytes));
}

uint64_t bs_bc_recv_uint(uint ch)
{
	uint8_t data[sizeof(uint64_t)];

	bs_bc_receive_msg_sync(ch, sizeof(data), data);
	return sys_get_le64(data);
}

void bt_testlib_bs_sync_all(void)
{
	static uint64_t counter;

	LOG_DBG("%llu d%u enter", counter, get_device_nbr());

	/* Device 0 acts as hub. */
	if (get_device_nbr() == 0) {
		for (int i = 1; i < n_devs; i++) {
			uint64_t counter_cfm;

			counter_cfm = bs_bc_recv_uint(backchannels[i]);
			__ASSERT(counter_cfm == counter, "%luu %luu", counter_cfm, counter);
		}
		for (int i = 1; i < n_devs; i++) {
			bs_bc_send_uint(backchannels[i], counter);
		}
	} else {
		uint64_t counter_cfm;

		bs_bc_send_uint(backchannels[0], counter);
		counter_cfm = bs_bc_recv_uint(backchannels[0]);
		__ASSERT(counter_cfm == counter, "%luu %luu", counter_cfm, counter);
	}

	LOG_DBG("%llu d%u exit", counter, get_device_nbr());

	counter++;
}
