/* net_driver_slip.c - Slip (serial line IP) driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/ip_buf.h>

#include "contiki/os/dev/slip.h"

static int net_driver_slip_open(void)
{
	NET_DBG("Initialized slip driver\n");

	return 0;
}

static int net_driver_slip_send(struct net_buf *buf)
{
	NET_DBG("Sending %d bytes, application data %d bytes\n",
		ip_buf_len(buf), ip_buf_appdatalen(buf));

	if (!slip_send(buf)) {
		/* Release the buffer because we sent all the data
		 * successfully.
		 */
		ip_buf_unref(buf);
		return 1;
	}

	return 0;
}

static struct net_driver net_driver_slip = {
	.head_reserve = 0,
	.open = net_driver_slip_open,
	.send = net_driver_slip_send,
};

int net_driver_slip_init(void)
{
	net_register_driver(&net_driver_slip);

	return 0;
}
