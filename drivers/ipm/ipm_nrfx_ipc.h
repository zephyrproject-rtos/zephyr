/*
 * Copyright (c) 2019, Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_ipc.h>

#define NRFX_IPC_ID_MAX_VALUE	 IPC_CONF_NUM

/*
 * Group IPC signals, events and channels into message channels.
 * Message channels are one-way connections between cores.
 *
 * For example Message Channel 0 is configured as TX on core 0
 * and as RX on core 1:
 *
 * [C0]			    [C1]
 * SIGNAL0 -> CHANNEL0 -> EVENT0
 *
 * Message Channel 1 is configured as RX on core 0 and as TX
 * on core 1:
 * [C0]			    [C1]
 * EVENT1 <- CHANNEL1 <- SIGNAL1
 */

#define IPC_EVENT_BIT(idx) \
	((IS_ENABLED(CONFIG_IPM_MSG_CH_##idx##_RX)) << idx)

#define IPC_EVENT_BITS		\
	(			\
	 IPC_EVENT_BIT(0)  |	\
	 IPC_EVENT_BIT(1)  |	\
	 IPC_EVENT_BIT(2)  |	\
	 IPC_EVENT_BIT(3)  |	\
	 IPC_EVENT_BIT(4)  |	\
	 IPC_EVENT_BIT(5)  |	\
	 IPC_EVENT_BIT(6)  |	\
	 IPC_EVENT_BIT(7)  |	\
	 IPC_EVENT_BIT(8)  |	\
	 IPC_EVENT_BIT(9)  |	\
	 IPC_EVENT_BIT(10) |	\
	 IPC_EVENT_BIT(11) |	\
	 IPC_EVENT_BIT(12) |	\
	 IPC_EVENT_BIT(13) |	\
	 IPC_EVENT_BIT(14) |	\
	 IPC_EVENT_BIT(15)	\
	)

static const nrfx_ipc_config_t ipc_cfg = {
	.send_task_config = {
		[0] = BIT(0),
		[1] = BIT(1),
		[2] = BIT(2),
		[3] = BIT(3),
		[4] = BIT(4),
		[5] = BIT(5),
		[6] = BIT(6),
		[7] = BIT(7),
		[8] = BIT(8),
		[9] = BIT(9),
		[10] = BIT(10),
		[11] = BIT(11),
		[12] = BIT(12),
		[13] = BIT(13),
		[14] = BIT(14),
		[15] = BIT(15),
	},
	.receive_event_config = {
		[0] = BIT(0),
		[1] = BIT(1),
		[2] = BIT(2),
		[3] = BIT(3),
		[4] = BIT(4),
		[5] = BIT(5),
		[6] = BIT(6),
		[7] = BIT(7),
		[8] = BIT(8),
		[9] = BIT(9),
		[10] = BIT(10),
		[11] = BIT(11),
		[12] = BIT(12),
		[13] = BIT(13),
		[14] = BIT(14),
		[15] = BIT(15),
	},
	.receive_events_enabled = IPC_EVENT_BITS,
};
