/** @file
 * @brief Network shell handler
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_SHELL_H
#define __NET_SHELL_H

#if defined(CONFIG_NET_SHELL)
extern int net_shell_init(void);
#else
static inline int net_shell_init(void)
{
	return 0;
}
#endif

#endif /* __NET_SHELL_H */
