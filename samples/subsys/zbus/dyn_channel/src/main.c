/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DEFINE(pkt_chan,		   /* Name */
		 struct external_data_msg, /* Message type */

		 NULL,			     /* Validator */
		 NULL,			     /* User data */
		 ZBUS_OBSERVERS(filter_lis), /* observers */
		 ZBUS_MSG_INIT(0)	     /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,						   /* Validator */
		 NULL,						   /* User data */
		 ZBUS_OBSERVERS_EMPTY,				   /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1, .build = 1) /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(data_ready_chan, /* Name */
		 struct ack_msg,  /* Message type */

		 NULL,			       /* Validator */
		 NULL,			       /* User data */
		 ZBUS_OBSERVERS(consumer_sub), /* observers */
		 ZBUS_MSG_INIT(0)	       /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(ack,		 /* Name */
		 struct ack_msg, /* Message type */

		 NULL,			       /* Validator */
		 NULL,			       /* User data */
		 ZBUS_OBSERVERS(producer_sub), /* observers */
		 ZBUS_MSG_INIT(0)	       /* Initial value {0} */
);

static void filter_cb(const struct zbus_channel *chan)
{
	/* Note that the listener does not change the actual message.
	 * It changes the external dynamic memory area but not the reference and size of the
	 * channel's message struct. Use that with care.
	 */

	const struct external_data_msg *chan_message;

	__ASSERT_NO_MSG(chan == &pkt_chan);

	chan_message = zbus_chan_const_msg(chan);

	if (chan_message->size % 2) {
		memset(chan_message->reference, 0, chan_message->size);
	}

	struct ack_msg dr = {1};

	zbus_chan_pub(&data_ready_chan, &dr, K_NO_WAIT);
}

ZBUS_LISTENER_DEFINE(filter_lis, filter_cb);

void main(void)
{
	const struct version_msg *v = zbus_chan_const_msg(&version_chan);

	LOG_DBG(" -> Dynamic channel sample version %u.%u-%u\n\n", v->major, v->minor, v->build);
}
