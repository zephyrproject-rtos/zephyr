/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RPMSG_CONFIG_H
#define _RPMSG_CONFIG_H

/*
 * RPMsg config values.
 * See $ZEPHYR_BASE/ext/lib/ipc/rpmsg_lite/lib/include/rpmsg_default_config.h
 * for the list of all config items.
 */

#define RL_MS_PER_INTERVAL (1)

#define RL_BUFFER_PAYLOAD_SIZE (496)

#define RL_API_HAS_ZEROCOPY (1)

#define RL_USE_STATIC_API (0)

#endif /* _RPMSG_CONFIG_H */
