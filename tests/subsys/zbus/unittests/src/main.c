/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "messages.h"

#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

static bool hard_msg_validator(const void *msg, size_t msg_size)
{
	ARG_UNUSED(msg_size);

	const struct hard_msg *ref = (struct hard_msg *)msg;

	return (ref->range >= 0) && (ref->range <= 1023) && (ref->binary <= 1) &&
	       (ref->pointer != NULL);
}

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 1023) /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(aux1_chan,	/* Name */
		 struct s1_msg, /* Message type */

		 NULL,			   /* Validator */
		 NULL,			   /* User data */
		 ZBUS_OBSERVERS(fast_lis), /* observers */
		 ZBUS_MSG_INIT(0)	   /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(aux2_chan,	    /* Name */
		 struct action_msg, /* Message type */

		 NULL,			   /* Validator */
		 NULL,			   /* User data */
		 ZBUS_OBSERVERS(fast_lis), /* observers */
		 ZBUS_MSG_INIT(0)	   /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(aux3_on_change_chan, /* Name */
		 struct action_msg,   /* Message type */

		 NULL,			   /* Validator */
		 NULL,			   /* User data */
		 ZBUS_OBSERVERS(fast_lis), /* observers */
		 ZBUS_MSG_INIT(0)	   /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(go_busy_chan,	    /* Name */
		 struct action_msg, /* Message type */

		 NULL,			   /* Validator */
		 NULL,			   /* User data */
		 ZBUS_OBSERVERS(busy_lis), /* observers */
		 ZBUS_MSG_INIT(0)	   /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(hard_chan,	  /* Name */
		 struct hard_msg, /* Message type */

		 hard_msg_validator,   /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(0)      /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(stuck_chan,	  /* Name */
		 struct hard_msg, /* Message type */

		 hard_msg_validator,   /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(0)      /* Initial value major 0, minor 1, build 1023 */
);

static int count_fast;

static void callback(const struct zbus_channel *chan)
{
	++count_fast;
}

ZBUS_LISTENER_DEFINE(fast_lis, callback);

ZBUS_LISTENER_DEFINE(rt_fast_lis, callback);

static int isr_return;

enum operation {
	PUB_ISR_INVAL,
	READ_ISR_INVAL,
	NOTIFY_ISR_INVAL,
	CLAIM_ISR_INVAL,
	FINISH_ISR_INVAL,
	ADD_OBS_ISR_INVAL,
	RM_OBS_ISR_INVAL,
	PUB_ISR,
	READ_ISR,
	NOTIFY_ISR,
	CLAIM_ISR,
	FINISH_ISR,
	ADD_OBS_ISR,
	RM_OBS_ISR,
	NONE
};

static enum operation current_isr_operation = NONE;

static struct action_msg ga;

static void isr_handler(const void *operation)
{
	enum operation *op = (enum operation *)operation;

	switch (*op) {
	case PUB_ISR_INVAL:
		isr_return = zbus_chan_pub(&aux2_chan, &ga, K_MSEC(500));
		break;
	case READ_ISR_INVAL:
		isr_return = zbus_chan_read(&aux2_chan, &ga, K_MSEC(500));
		break;
	case NOTIFY_ISR_INVAL:
		isr_return = zbus_chan_notify(&aux2_chan, K_MSEC(100));
		break;
	case CLAIM_ISR_INVAL:
		isr_return = zbus_chan_claim(&aux2_chan, K_MSEC(200));
		break;
	case FINISH_ISR_INVAL:
		isr_return = zbus_chan_finish(NULL);
		break;
	case ADD_OBS_ISR_INVAL:
		isr_return = zbus_chan_add_obs(&aux2_chan, &fast_lis, K_MSEC(200));
		break;
	case RM_OBS_ISR_INVAL:
		isr_return = zbus_chan_rm_obs(&aux2_chan, &fast_lis, K_MSEC(200));
		break;
	case PUB_ISR:
		isr_return = zbus_chan_pub(&aux2_chan, &ga, K_NO_WAIT);
		break;
	case READ_ISR:
		isr_return = zbus_chan_read(&aux2_chan, &ga, K_NO_WAIT);
		break;
	case NOTIFY_ISR:
		isr_return = zbus_chan_notify(&aux2_chan, K_NO_WAIT);
		break;
	case CLAIM_ISR:
		isr_return = zbus_chan_claim(&aux2_chan, K_NO_WAIT);
		break;
	case FINISH_ISR:
		isr_return = zbus_chan_finish(&aux2_chan);
		break;
	case ADD_OBS_ISR:
		isr_return = zbus_chan_add_obs(&aux2_chan, NULL, K_MSEC(200));
		break;
	case RM_OBS_ISR:
		isr_return = zbus_chan_rm_obs(&aux2_chan, NULL, K_MSEC(200));
		break;
	case NONE:
		break;
	}
}

static void busy_callback(const struct zbus_channel *chan)
{
	zbus_chan_claim(&go_busy_chan, K_NO_WAIT);
}

ZBUS_LISTENER_DEFINE(busy_lis, busy_callback);

#define ISR_OP(_op, _exp)                                                                          \
	isr_return = 0;                                                                            \
	current_isr_operation = _op;                                                               \
	irq_offload(isr_handler, &current_isr_operation);                                          \
	zassert_equal(_exp, isr_return, "isr return wrong, it is %d", isr_return);                 \
	k_msleep(100);

struct aux2_wq_data {
	struct k_work work;
};

static struct aux2_wq_data wq_handler;

static void wq_dh_cb(struct k_work *item)
{
	struct action_msg a = {0};

	zassert_equal(-EBUSY, zbus_chan_pub(&aux2_chan, &a, K_NO_WAIT), "It must not be valid");

	zassert_equal(-EBUSY, zbus_chan_read(&aux2_chan, &a, K_NO_WAIT), "It must not be valid");

	zassert_equal(-EBUSY, zbus_chan_notify(&aux2_chan, K_NO_WAIT), "It must not be invalid");

	zassert_equal(-EFAULT, zbus_chan_finish(NULL), "It must be invalid");
}

ZBUS_SUBSCRIBER_DEFINE(sub1, 1);

ZTEST(basic, test_specification_based__zbus_chan)
{
	struct action_msg a = {0};

	/* Trying invalid parameters */
	zassert_equal(-EFAULT, zbus_chan_pub(NULL, &a, K_NO_WAIT), "It must be -EFAULT");

	k_msleep(100);

	zassert_equal(-EFAULT, zbus_chan_pub(&aux2_chan, NULL, K_NO_WAIT), "It must be -EFAULT");

	k_msleep(100);

	zassert_equal(-EFAULT, zbus_chan_read(NULL, &a, K_NO_WAIT), "It must be -EFAULT");

	k_msleep(100);

	zassert_equal(-EFAULT, zbus_chan_read(&aux2_chan, NULL, K_NO_WAIT), "It must be -EFAULT");

	k_msleep(100);

	zassert_equal(-EFAULT, zbus_chan_notify(NULL, K_NO_WAIT), "It must be -EFAULT");

	zassert_equal(-EFAULT, zbus_chan_claim(NULL, K_NO_WAIT), "It must be -EFAULT");

	zassert_equal(-EFAULT, zbus_chan_finish(NULL), "It must be -EFAULT");

	zassert_equal(-EFAULT, zbus_chan_add_obs(NULL, &sub1, K_MSEC(200)), NULL);

	zassert_equal(-EFAULT, zbus_chan_add_obs(&aux2_chan, NULL, K_MSEC(200)), NULL);

	zassert_equal(-EFAULT, zbus_chan_rm_obs(NULL, &sub1, K_MSEC(200)), NULL);

	zassert_equal(-EFAULT, zbus_chan_rm_obs(&aux2_chan, NULL, K_MSEC(200)), NULL);
	/* Trying valid parameters */
	zassert_equal(0, zbus_chan_pub(&aux2_chan, &a, K_NO_WAIT), "It must be valid");

	k_msleep(100);

	zassert_equal(0, zbus_chan_read(&aux2_chan, &a, K_NO_WAIT), "It must be valid");

	k_msleep(100);

	zassert_equal(0, zbus_chan_notify(&aux2_chan, K_NO_WAIT), "It must be valid");

	zassert_equal(0, zbus_chan_claim(&aux2_chan, K_NO_WAIT), "It must be valid");

	k_work_init(&wq_handler.work, wq_dh_cb);
	k_work_submit(&wq_handler.work);

	k_msleep(100);

	zassert_equal(0, zbus_chan_pub(&aux2_chan, &a, K_NO_WAIT), "It must not be valid");

	zassert_equal(0, zbus_chan_read(&aux2_chan, &a, K_NO_WAIT), "It must not be valid");

	zassert_equal(0, zbus_chan_notify(&aux2_chan, K_NO_WAIT), "It must not be invalid");

	zassert_equal(0, zbus_chan_finish(&aux2_chan), "It must finish correctly");

	struct action_msg repeated = {.status = false};

	zbus_chan_pub(&aux3_on_change_chan, &repeated, K_NO_WAIT);

	k_msleep(100);

	zbus_chan_pub(&aux3_on_change_chan, &repeated, K_NO_WAIT);

	k_msleep(100);

	zassert_equal(0, zbus_chan_pub(&go_busy_chan, &repeated, K_NO_WAIT),
		      "It must be ok, but it causes an error inside the event dispatcher telling "
		      "the channel is busy.");

	k_msleep(100);

	zassert_equal(0, zbus_chan_add_obs(&stuck_chan, &sub1, K_MSEC(200)), NULL);

	zassert_equal(0, zbus_chan_notify(&stuck_chan, K_MSEC(200)), "It must finish correctly");

	zassert_equal(-EFAULT, zbus_chan_notify(&stuck_chan, K_MSEC(200)),
		      "It must finish correctly");

	/* Trying to call the zbus functions in a ISR context. None must work */
	ISR_OP(PUB_ISR, -EFAULT);
	ISR_OP(PUB_ISR_INVAL, -EFAULT);
	ISR_OP(READ_ISR, -EFAULT);
	ISR_OP(READ_ISR_INVAL, -EFAULT);
	ISR_OP(NOTIFY_ISR, -EFAULT);
	ISR_OP(NOTIFY_ISR_INVAL, -EFAULT);
	ISR_OP(CLAIM_ISR, -EFAULT);
	ISR_OP(CLAIM_ISR_INVAL, -EFAULT);
	ISR_OP(FINISH_ISR, -EFAULT);
	ISR_OP(FINISH_ISR_INVAL, -EFAULT);
	ISR_OP(ADD_OBS_ISR, -EFAULT);
	ISR_OP(ADD_OBS_ISR_INVAL, -EFAULT);
	ISR_OP(RM_OBS_ISR, -EFAULT);
	ISR_OP(RM_OBS_ISR_INVAL, -EFAULT);
}

#if defined(CONFIG_ZBUS_STRUCTS_ITERABLE_ACCESS)
static bool always_true_chan_iterator(const struct zbus_channel *chan)
{
	return true;
}

static bool always_true_obs_iterator(const struct zbus_observer *obs)
{
	return true;
}

static bool always_false_chan_iterator(const struct zbus_channel *chan)
{
	return false;
}

static bool always_false_obs_iterator(const struct zbus_observer *obs)
{
	return false;
}

static bool check_chan_iterator(const struct zbus_channel *chan)
{
	static int chan_idx;

	LOG_DBG("Idx %d - Channel %s", chan_idx, chan->name);

	switch (chan_idx) {
	case 0:
		zassert_mem_equal__(zbus_chan_name(chan), "aux1_chan", 9, "Must be equal");
		break;
	case 1:
		zassert_mem_equal__(zbus_chan_name(chan), "aux2_chan", 9, "Must be equal");
		break;
	case 2:
		zassert_mem_equal__(zbus_chan_name(chan), "aux3_on_change_chan", 19,
				    "Must be equal");
		break;
	case 3:
		zassert_mem_equal__(zbus_chan_name(chan), "go_busy_chan", 12, "Must be equal");
		break;
	case 4:
		zassert_mem_equal__(zbus_chan_name(chan), "hard_chan", 9, "Must be equal");
		break;
	case 5:
		zassert_mem_equal__(zbus_chan_name(chan), "stuck_chan", 10, "Must be equal");
		break;
	case 6:
		zassert_mem_equal__(zbus_chan_name(chan), "version_chan", 12, "Must be equal");
		break;
	default:
		zassert_unreachable(NULL);
	}
	++chan_idx;

	return true;
}

static bool check_obs_iterator(const struct zbus_observer *obs)
{
	static int obs_idx;

	LOG_DBG("Idx %d - Observer %s", obs_idx, obs->name);

	switch (obs_idx) {
	case 0:
		zassert_mem_equal__(zbus_obs_name(obs), "busy_lis", 8, "Must be equal");
		break;
	case 1:
		zassert_mem_equal__(zbus_obs_name(obs), "fast_lis", 8, "Must be equal");
		break;
	case 2:
		zassert_mem_equal__(zbus_obs_name(obs), "foo_sub", 7, "Must be equal");
		break;
	case 3:
		zassert_mem_equal__(zbus_obs_name(obs), "rt_fast_lis", 11, "Must be equal");
		break;
	case 4:
		zassert_mem_equal__(zbus_obs_name(obs), "sub1", 4, "Must be equal");
		break;
	default:
		zassert_unreachable(NULL);
	}
	++obs_idx;

	return true;
}

static int oracle;
static int idx = -1;

static bool count_false_chan_iterator(const struct zbus_channel *chan)
{
	++idx;

	LOG_DBG("chan_idx %d, oracle %d", idx, oracle);

	return (bool)(idx != oracle);
}

static bool count_false_obs_iterator(const struct zbus_observer *obs)
{
	++idx;

	LOG_DBG("obs_idx %d, oracle %d", idx, oracle);

	return (bool)(idx != oracle);
}

ZTEST(basic, test_iterators)
{
	zassert_equal(true, zbus_iterate_over_channels(always_true_chan_iterator), "Must be true");

	zassert_equal(true, zbus_iterate_over_observers(always_true_obs_iterator), "Must be true");

	zassert_equal(false, zbus_iterate_over_channels(always_false_chan_iterator),
		      "Must be false");

	zassert_equal(false, zbus_iterate_over_observers(always_false_obs_iterator),
		      "Must be false");

	zassert_equal(true, zbus_iterate_over_channels(check_chan_iterator), "Must be true");

	zassert_equal(true, zbus_iterate_over_observers(check_obs_iterator), "Must be true");

	int chan_count = 0;

	STRUCT_SECTION_COUNT(zbus_channel, &chan_count);

	chan_count -= 1;

	for (int i = 0; i < chan_count; ++i) {
		oracle = i;

		zassert_equal(false, zbus_iterate_over_channels(count_false_chan_iterator),
			      "Must be false");

		k_msleep(100);

		idx = -1;
	}
	int obs_count = 0;

	STRUCT_SECTION_COUNT(zbus_observer, &obs_count);

	obs_count -= 1;

	LOG_DBG("Counts obs %d, chan %d", obs_count, chan_count);

	for (int i = 0; i < obs_count; ++i) {
		oracle = i;

		zassert_equal(false, zbus_iterate_over_observers(count_false_obs_iterator),
			      "Must be false");

		idx = -1;
	}
}
#else
ZTEST(basic, test_iterators)
{
	ztest_test_skip();
}
#endif

ZTEST(basic, test_hard_channel)
{
	struct hard_msg valid = {.range = 10, .binary = 1, .pointer = &valid.range};

	zbus_chan_pub(&hard_chan, &valid, K_NO_WAIT);

	struct hard_msg current;

	zbus_chan_read(&hard_chan, &current, K_NO_WAIT);

	zassert_equal(valid.range, current.range, "Range must be equal");
	zassert_equal(valid.binary, current.binary, "Binary must be equal");
	zassert_equal(valid.pointer, current.pointer, "Pointer must be equal");

	struct hard_msg invalid = {.range = 10000, .binary = 1, .pointer = &valid.range};
	int err = zbus_chan_pub(&hard_chan, &invalid, K_NO_WAIT);

	zassert_equal(err, -ENOMSG, "Err must be -ENOMSG, the channel message is invalid");

	invalid = (struct hard_msg){.range = 1, .binary = 3, .pointer = &invalid.range};
	err = zbus_chan_pub(&hard_chan, &invalid, K_NO_WAIT);
	zassert_equal(err, -ENOMSG, "Err must be -ENOMSG, the channel message is invalid");

	invalid = (struct hard_msg){.range = 1, .binary = 0, .pointer = NULL};
	err = zbus_chan_pub(&hard_chan, &invalid, K_NO_WAIT);
	zassert_equal(err, -ENOMSG, "Err must be -ENOMSG, the channel message is invalid");
}

ZTEST(basic, test_specification_based__zbus_obs_set_enable)
{
	count_fast = 0;

	zassert_equal(-EFAULT, zbus_obs_set_enable(NULL, false), NULL);

	zassert_equal(0, zbus_obs_set_enable(&rt_fast_lis, false),
		      "Must be zero. The observer must be disabled");

	zassert_equal(0, zbus_chan_add_obs(&aux1_chan, &rt_fast_lis, K_MSEC(200)), NULL);

	zassert_equal(0, zbus_obs_set_enable(&fast_lis, false),
		      "Must be zero. The observer must be disabled");

	zbus_chan_notify(&aux1_chan, K_NO_WAIT);

	k_msleep(300);

	zassert_equal(count_fast, 0, "Must be zero. No callback called");

	zassert_equal(0, zbus_obs_set_enable(&fast_lis, true),
		      "Must be zero. The observer must be enabled");

	zassert_equal(0, zbus_obs_set_enable(&rt_fast_lis, true),
		      "Must be zero. The observer must be enabled");

	zbus_chan_notify(&aux1_chan, K_NO_WAIT);

	k_msleep(300);

	zassert_equal(count_fast, 2, "Must be 2. Callback must be called once");

	zassert_equal(0, zbus_chan_rm_obs(&aux1_chan, &rt_fast_lis, K_MSEC(200)), NULL);
}

ZBUS_SUBSCRIBER_DEFINE(foo_sub, 1);

static void isr_sub_wait(const void *operation)
{
	const struct zbus_channel *chan;

	/* All the calls must not work. Zbus cannot work in IRSs */
	zassert_equal(-EFAULT, zbus_sub_wait(NULL, NULL, K_NO_WAIT), NULL);
	zassert_equal(-EFAULT, zbus_sub_wait(&foo_sub, NULL, K_NO_WAIT), NULL);
	zassert_equal(-EFAULT, zbus_sub_wait(&foo_sub, &chan, K_NO_WAIT), NULL);
}

ZTEST(basic, test_specification_based__zbus_sub_wait)
{
	count_fast = 0;
	const struct zbus_channel *chan;

	zassert_equal(-EFAULT, zbus_sub_wait(NULL, NULL, K_NO_WAIT), NULL);
	zassert_equal(-EFAULT, zbus_sub_wait(&foo_sub, NULL, K_NO_WAIT), NULL);

	/* It must run but return a -ENOMSG because of the K_NO_WAIT */
	zassert_equal(-ENOMSG, zbus_sub_wait(&foo_sub, &chan, K_NO_WAIT), NULL);

	irq_offload(isr_sub_wait, NULL);
}

ZTEST_SUITE(basic, NULL, NULL, NULL, NULL, NULL);
