/*
 * Copyright 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define DT_DRV_COMPAT ap_pwrseq_state

LOG_MODULE_DECLARE(ap_pwrseq, CONFIG_AP_PWRSEQ_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(ap_pwrseq_thread_stack,
			     CONFIG_AP_PWRSEQ_STACK_SIZE);

#define AP_PWRSEQ_EVENT_MASK	(BIT(AP_PWRSEQ_EVENT_COUNT) - 1)
#define AP_PWRSEQ_STATES_MASK	(BIT(AP_POWER_STATE_COUNT) - 1)

struct ap_pwrseq_cb_list {
	uint32_t states;
	sys_slist_t list;
	struct k_spinlock lock;
};

struct ap_pwrseq_data {
	struct ap_pwrseq_sm_data *sm;
	struct k_thread thread;
	struct k_event evt;
	struct ap_pwrseq_cb_list entry_list;
	struct ap_pwrseq_cb_list exit_list;
};

static struct ap_pwrseq_data ap_pwrseq_data_0;

void ap_pwrseq_post_event(const struct device *dev,
			 enum ap_pwrseq_event event)
{
	struct ap_pwrseq_data *const data = dev->data;

	LOG_DBG("Posting Event: 0x%lX", BIT(event));
	k_event_post(&data->evt, BIT(event));
}

int ap_pwrseq_get_current_state(const struct device *dev,
				enum ap_pwrseq_state *state)
{
	struct ap_pwrseq_data *const data = dev->data;

	if ((*state = ap_pwrseq_sm_get_cur_state(data->sm)) ==
	     AP_POWER_STATE_UNDEF) {
		return -EIO;
	}
	return 0;
}

const char *const ap_pwrseq_get_state_str(enum ap_pwrseq_state state)
{
	return ap_pwrseq_sm_get_state_str(state);
}

static int ap_pwrseq_add_state_callback(struct ap_pwrseq_cb_list *cb_list, sys_snode_t *node)
{
	if (!sys_slist_is_empty(&cb_list->list)) {
		if (!sys_slist_find_and_remove(&cb_list->list, node)) {
			return -EINVAL;
		}
	}

	sys_slist_prepend(&cb_list->list, node);

	return 0;
}

int ap_pwrseq_register_state_callback(const struct device *dev,
				      struct ap_pwrseq_state_callback
				      *state_cb)
{
	struct ap_pwrseq_data *data = dev->data;
	struct ap_pwrseq_cb_list *cb_list;

	if (state_cb->action == AP_POWER_STATE_ACTION_ENTRY) {
		cb_list = &data->entry_list;
	} else if (state_cb->action == AP_POWER_STATE_ACTION_EXIT) {
		cb_list = &data->exit_list;
	} else {
		return -EINVAL;
	}

	if (!(state_cb->states_bit_mask & AP_PWRSEQ_STATES_MASK)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&cb_list->lock);
	if (ap_pwrseq_add_state_callback(cb_list, &state_cb->node)) {
		return -EINVAL;
	} else {
		cb_list->states |=
			AP_PWRSEQ_STATES_MASK & state_cb->states_bit_mask;
	}
	k_spin_unlock(&cb_list->lock, key);

	return 0;
}

static void ap_pwrseq_send_callbacks(const struct device *dev,
				     enum ap_pwrseq_state state,
				     uint8_t action)
{
	struct ap_pwrseq_data *const data = dev->data;
	struct ap_pwrseq_state_callback *state_cb, *tmp;

	struct ap_pwrseq_cb_list *cb_list;

	if (action == AP_POWER_STATE_ACTION_ENTRY) {
		cb_list = &data->entry_list;
	} else if (action == AP_POWER_STATE_ACTION_EXIT) {
		cb_list = &data->exit_list;
	} else {
		return;
	}

	if (!(cb_list->states & BIT(state))) {
		return;
	}
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&cb_list->list, state_cb, tmp, node) {
		if (state_cb->states_bit_mask & BIT(state) &&
		    state_cb->action == action &&
		    state_cb->cb) {
			state_cb->cb(dev, state, action);
		}
	}
}

static uint32_t ap_pwrseq_wait_event(const struct device *dev, k_timeout_t timeout)
{
	struct ap_pwrseq_data *const data = dev->data;
	uint32_t events;

	events = k_event_wait(&data->evt, AP_PWRSEQ_EVENT_MASK, false, timeout);
	/* Reset all events posted */
	k_spinlock_key_t  key = k_spin_lock(&data->evt.lock);
	data->evt.events &= ~events;
	k_spin_unlock(&data->evt.lock, key);

	return events;
}

static void ap_pwrseq_thread(void *arg, void *unused1, void *unused2)
{
	struct device *const dev = (struct device *)arg;
	struct ap_pwrseq_data *const data = dev->data;
	enum ap_pwrseq_state cur_state;
	k_timeout_t timeout = K_NO_WAIT;

	LOG_INF("Power Sequence thread start");
	while(true) {
		uint32_t events = ap_pwrseq_wait_event(dev, timeout);
		ap_pwrseq_sm_set_events(data->sm, events);
		if (events) {
			LOG_DBG("Events posted: 0x%X", events);
		}

		cur_state = ap_pwrseq_sm_get_cur_state(data->sm);
		if (ap_pwrseq_sm_run_state(data->sm)) {
			break;
		}

		if (cur_state != ap_pwrseq_sm_get_cur_state(data->sm)) {
			LOG_INF("Power state: %s --> %s", ap_pwrseq_get_state_str(cur_state),
				ap_pwrseq_get_state_str(ap_pwrseq_sm_get_cur_state(data->sm)));
			/* Previous state generates callbacks for exit actions */
			ap_pwrseq_send_callbacks(dev,
				ap_pwrseq_sm_get_prev_state(data->sm),
				AP_POWER_STATE_ACTION_EXIT);

			/* New state generates callbacks for entry actions */
			ap_pwrseq_send_callbacks(dev,
				ap_pwrseq_sm_get_cur_state(data->sm),
				AP_POWER_STATE_ACTION_ENTRY);
			timeout = K_NO_WAIT;
		} else {
			/* No state transition, wait for any event */
			timeout = K_FOREVER;
		}
	}
}

static int ap_pwrseq_driver_init(const struct device *dev)
{
	struct ap_pwrseq_data *const data = dev->data;

	data->sm = ap_pwrseq_sm_get_instance();
	k_event_init(&data->evt);
	ap_pwrseq_sm_init(data->sm);

	k_thread_create(&data->thread,
		ap_pwrseq_thread_stack,
		K_KERNEL_STACK_SIZEOF(ap_pwrseq_thread_stack),
		(k_thread_entry_t)ap_pwrseq_thread,
		(void *)dev, NULL, NULL,
		CONFIG_AP_PWRSEQ_THREAD_PRIORITY, 0,
		IS_ENABLED(CONFIG_AP_PWRSEQ_AUTOSTART) ? K_NO_WAIT
						       : K_FOREVER);

	k_thread_name_set(&data->thread, "ap_pwrseq_task");

	return 0;
}

void ap_pwrseq_start(const struct device *dev)
{
	struct ap_pwrseq_data *const data = dev->data;

	if (!IS_ENABLED(CONFIG_AP_PWRSEQ_AUTOSTART)) {
		k_thread_start(&data->thread);
	}
}

DEVICE_DEFINE(ap_pwrseq_dev, "ap_pwrseq_drv",
		      ap_pwrseq_driver_init, NULL,
		      &ap_pwrseq_data_0, NULL,
		      APPLICATION,
		      CONFIG_APPLICATION_INIT_PRIORITY, NULL);

const struct device * ap_pwrseq_get_instance(void)
{
	return DEVICE_GET(ap_pwrseq_dev);
}
