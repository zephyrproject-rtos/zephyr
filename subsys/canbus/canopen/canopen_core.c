/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "canopen_internal.h"

LOG_MODULE_REGISTER(canopen, CONFIG_CANOPEN_LOG_LEVEL);

#define CANOPEN_NODE_ID_MIN 1U
#define CANOPEN_NODE_ID_MAX 127U

static void canopen_rx_cb(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct canopen_node *node = user_data;

	ARG_UNUSED(dev);

	if (k_msgq_put(&node->rxq, frame, K_NO_WAIT) != 0) {
		atomic_set_bit(&node->flags, CANOPEN_FLAG_RX_OVERRUN);
	}
}

static void canopen_can_state_cb(const struct device *dev, struct can_state_change_callback *cb,
				 enum can_state state, struct can_bus_err_cnt err_cnt)
{
	struct canopen_node *node = CONTAINER_OF(cb, struct canopen_node, can_state_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(err_cnt);

	atomic_set(&node->can_state, state);
	atomic_set_bit(&node->flags, CANOPEN_FLAG_CAN_STATE);
	k_poll_signal_raise(&node->signal, 0);
}

static void canopen_tx_cb(const struct device *dev, int error, void *user_data)
{
	struct canopen_node *node = user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(error);

	k_poll_signal_raise(&node->signal, 0);
}

int canopen_send(struct canopen_node *node, const struct can_frame *frame)
{
	int err;

	if (k_msgq_num_used_get(&node->txq) == 0U) {
		err = can_send(node->cfg->can, frame, K_NO_WAIT, canopen_tx_cb, node);
		if (err != -EAGAIN) {
			if (err != 0) {
				LOG_DBG("failed to send 0x%03x (%d)", frame->id, err);
			}
			return err;
		}
	}

	err = k_msgq_put(&node->txq, frame, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("transmit queue full, dropped 0x%03x", frame->id);
	}

	return err;
}

int canopen_filter_set(struct canopen_node *node, int *filter, uint32_t cob_id)
{
	struct can_filter f = {
		.id = cob_id & CAN_STD_ID_MASK,
		.mask = CAN_STD_ID_MASK,
	};

	if (*filter >= 0) {
		can_remove_rx_filter(node->cfg->can, *filter);
		*filter = -1;
	}

	if ((cob_id & CANOPEN_COB_ID_INVALID) != 0U) {
		return 0;
	}

	*filter = can_add_rx_filter(node->cfg->can, canopen_rx_cb, node, &f);
	if (*filter < 0) {
		LOG_ERR("no CAN filter for 0x%03x (%d)", f.id, *filter);
		return *filter;
	}

	return 0;
}

bool canopen_cob_id_restricted(uint32_t cob_id)
{
	return (cob_id <= 0x07FU) || ((cob_id >= 0x101U) && (cob_id <= 0x180U)) ||
	       ((cob_id >= 0x581U) && (cob_id <= 0x5FFU)) ||
	       ((cob_id >= 0x601U) && (cob_id <= 0x67FU)) ||
	       ((cob_id >= 0x6E0U) && (cob_id <= 0x6FFU)) || (cob_id >= 0x701U);
}

static void canopen_tx_drain(struct canopen_node *node)
{
	struct can_frame frame;
	int err;

	while (k_msgq_peek(&node->txq, &frame) == 0) {
		err = can_send(node->cfg->can, &frame, K_NO_WAIT, canopen_tx_cb, node);
		if (err == -EAGAIN) {
			break;
		}

		if (err != 0) {
			LOG_DBG("failed to send 0x%03x (%d)", frame.id, err);
		}

		(void)k_msgq_get(&node->txq, &frame, K_NO_WAIT);
	}
}

static void canopen_dispatch(struct canopen_node *node, const struct can_frame *frame)
{
	if ((frame->flags & CAN_FRAME_IDE) != 0U) {
		return;
	}

	if ((frame->flags & CAN_FRAME_RTR) != 0U) {
		if (IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
			canopen_guard_rx(node, frame);
		}
		return;
	}

	if (frame->id == CANOPEN_COB_NMT) {
		canopen_nmt_rx(node, frame);
	} else if (frame->id == node->sdo_cob_id[0]) {
		if (node->nmt_state != CANOPEN_NMT_STOPPED) {
			canopen_sdo_server_rx(node, frame);
		}
	} else if (IS_ENABLED(CONFIG_CANOPEN_MASTER) && canopen_sdo_client_rx(node, frame)) {
		return;
	} else if (IS_ENABLED(CONFIG_CANOPEN_TIME) && canopen_time_rx(node, frame)) {
		return;
	} else if (!canopen_sync_rx(node, frame) && !canopen_hb_rx(node, frame)) {
		canopen_pdo_rx(node, frame);
	}
}

static k_timeout_t canopen_next_timeout(struct canopen_node *node)
{
	const k_timepoint_t deadlines[] = {
		canopen_sdo_server_deadline(node),
		canopen_pdo_deadline(node),
		canopen_hb_deadline(node),
		canopen_emcy_deadline(node),
		canopen_sync_deadline(node),
		IS_ENABLED(CONFIG_CANOPEN_MASTER) ? canopen_sdo_client_deadline(node)
						  : sys_timepoint_calc(K_FOREVER),
		IS_ENABLED(CONFIG_CANOPEN_LEDS) ? canopen_leds_deadline(node)
						: sys_timepoint_calc(K_FOREVER),
		IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING) ? canopen_guard_deadline(node)
							 : sys_timepoint_calc(K_FOREVER),
	};
	k_timepoint_t next = canopen_nmt_deadline(node);

	for (size_t i = 0U; i < ARRAY_SIZE(deadlines); i++) {
		if (sys_timepoint_cmp(deadlines[i], next) < 0) {
			next = deadlines[i];
		}
	}

	return sys_timepoint_timeout(next);
}

static void canopen_thread(void *p1, void *p2, void *p3)
{
	struct canopen_node *node = p1;
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
					 &node->rxq),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
					 &node->signal),
	};
	struct can_frame frame;
	k_timeout_t timeout;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&node->lock, K_FOREVER);
	timeout = canopen_next_timeout(node);
	k_mutex_unlock(&node->lock);

	while (true) {
		(void)k_poll(events, ARRAY_SIZE(events), timeout);
		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;
		k_poll_signal_reset(&node->signal);

		k_mutex_lock(&node->lock, K_FOREVER);

		if (atomic_test_and_clear_bit(&node->flags, CANOPEN_FLAG_RX_OVERRUN)) {
			LOG_WRN("receive queue overrun");
			canopen_emcy_signal(node, CANOPEN_EMCY_CAN_OVERRUN,
					    CANOPEN_ERR_REG_COMMUNICATION);
		}

		if (atomic_test_and_clear_bit(&node->flags, CANOPEN_FLAG_CAN_STATE)) {
			canopen_emcy_can_state(node, (enum can_state)atomic_get(&node->can_state));
		}

		while (k_msgq_get(&node->rxq, &frame, K_NO_WAIT) == 0) {
			canopen_dispatch(node, &frame);
		}

		canopen_tx_drain(node);
		canopen_nmt_process(node);
		canopen_sdo_server_process(node);
		if (IS_ENABLED(CONFIG_CANOPEN_MASTER)) {
			canopen_sdo_client_process(node);
		}
		canopen_sync_process(node);
		canopen_hb_process(node);
		if (IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
			canopen_guard_process(node);
		}
		canopen_pdo_process(node);
		canopen_emcy_process(node);
		if (IS_ENABLED(CONFIG_CANOPEN_LEDS)) {
			canopen_leds_process(node);
		}
		timeout = canopen_next_timeout(node);

		k_mutex_unlock(&node->lock);
	}
}

int canopen_node_init(struct canopen_node *node, const struct canopen_node_config *cfg)
{
	k_tid_t tid;

	if ((cfg->node_id < CANOPEN_NODE_ID_MIN) || (cfg->node_id > CANOPEN_NODE_ID_MAX)) {
		return -EINVAL;
	}

	if ((cfg->od != NULL) && (canopen_od_validate(cfg->od) != 0)) {
		return -EINVAL;
	}

	if (!device_is_ready(cfg->can)) {
		return -ENODEV;
	}

	node->cfg = cfg;
	if ((canopen_pdo_init(node) != 0) || (canopen_hb_init(node) != 0)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_CANOPEN_LEDS) && (canopen_leds_init(node) != 0)) {
		return -ENODEV;
	}

	node->nmt_state = CANOPEN_NMT_INITIALISING;
	atomic_clear(&node->flags);
	k_mutex_init(&node->lock);
	k_msgq_init(&node->rxq, (char *)node->rxq_buf, sizeof(struct can_frame),
		    ARRAY_SIZE(node->rxq_buf));
	k_msgq_init(&node->txq, (char *)node->txq_buf, sizeof(struct can_frame),
		    ARRAY_SIZE(node->txq_buf));
	k_poll_signal_init(&node->signal);
	node->can_state_last = CAN_STATE_ERROR_ACTIVE;
	canopen_sync_init(node);
	if (IS_ENABLED(CONFIG_CANOPEN_MASTER)) {
		canopen_sdo_client_init(node);
	}
	if (IS_ENABLED(CONFIG_CANOPEN_TIME)) {
		canopen_time_init(node);
	}
	if (IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
		canopen_guard_init(node);
	}
	canopen_emcy_reset(node, true);
	canopen_od_comm_reset(node);
	canopen_sdo_server_reset(node);

	tid = k_thread_create(&node->thread, node->stack, K_KERNEL_STACK_SIZEOF(node->stack),
			      canopen_thread, node, NULL, NULL, CONFIG_CANOPEN_THREAD_PRIORITY, 0,
			      K_FOREVER);
	k_thread_name_set(tid, "canopen");

	if (IS_ENABLED(CONFIG_CANOPEN_SHELL)) {
		canopen_shell_register(node);
	}

	return 0;
}

int canopen_node_start(struct canopen_node *node)
{
	const struct device *can = node->cfg->can;
	struct can_filter filter = {
		.id = CANOPEN_COB_NMT,
		.mask = CAN_STD_ID_MASK,
	};
	int nmt_filter;
	int err;

	nmt_filter = can_add_rx_filter(can, canopen_rx_cb, node, &filter);
	if (nmt_filter < 0) {
		return nmt_filter;
	}

	filter.id = node->sdo_cob_id[0];
	err = can_add_rx_filter(can, canopen_rx_cb, node, &filter);
	if (err < 0) {
		can_remove_rx_filter(can, nmt_filter);
		return err;
	}

	if (IS_ENABLED(CONFIG_CANOPEN_STORAGE)) {
		err = settings_subsys_init();
		if (err != 0) {
			LOG_WRN("parameters not restored (%d)", err);
		}
	}

	can_init_state_change_callback(&node->can_state_cb, canopen_can_state_cb);
	err = can_add_state_change_callback(can, &node->can_state_cb);
	if (err != 0) {
		LOG_WRN("CAN bus state not reported (%d)", err);
	}

	err = can_start(can);
	if ((err != 0) && (err != -EALREADY)) {
		return -EIO;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	canopen_nmt_boot(node, true);
	if (IS_ENABLED(CONFIG_CANOPEN_LEDS)) {
		canopen_leds_process(node);
	}
	k_mutex_unlock(&node->lock);

	k_thread_start(&node->thread);

	return 0;
}
