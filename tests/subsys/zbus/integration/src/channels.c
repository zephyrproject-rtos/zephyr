/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/zbus/zbus.h>

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 1023) /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(sensor_data_chan,	 /* Name */
		 struct sensor_data_msg, /* Message type */

		 NULL,			   /* Validator */
		 NULL,			   /* User data */
		 ZBUS_OBSERVERS(core_sub), /* observers */
		 ZBUS_MSG_INIT(0)	   /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(net_pkt_chan,	     /* Name */
		 struct net_pkt_msg, /* Message type */

		 NULL,			   /* Validator */
		 NULL,			   /* User data */
		 ZBUS_OBSERVERS(net_sub),  /* observers */
		 ZBUS_MSG_INIT(.total = 0) /* Initial value */
);

ZBUS_CHAN_DEFINE(start_measurement_chan, /* Name */
		 struct action_msg,	 /* Message type */

		 NULL,					       /* Validator */
		 NULL,					       /* User data */
		 ZBUS_OBSERVERS(peripheral_sub, critical_lis), /* observers */
		 ZBUS_MSG_INIT(.status = false)		       /* Initial value */
);

ZBUS_CHAN_DEFINE(busy_chan,	    /* Name */
		 struct action_msg, /* Message type */

		 NULL,				/* Validator */
		 NULL,				/* User data */
		 ZBUS_OBSERVERS_EMPTY,		/* observers */
		 ZBUS_MSG_INIT(.status = false) /* Initial value */
);
