/* null_compression.c - null header compression */

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

#include <stdio.h>
#include <string.h>

#include <net/buf.h>
#include "contiki/netstack.h"
#include "contiki/sicslowpan/null_compression.h"

#define DEBUG 0
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

static void init(void)
{
}

static int compress(struct net_buf *buf)
{
	return 1;
}

static int uncompress(struct net_buf *buf)
{
	return 1;
}

const struct compression null_compression = {
	.init = init,
	.compress = compress,
	.uncompress = uncompress,
};
