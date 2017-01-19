/** @file
 @brief Network shell handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_SHELL_H
#define __NET_SHELL_H

#if defined(CONFIG_NET_SHELL)
void net_shell_init(void);
#else
#define net_shell_init(...)
#endif /* CONFIG_NET_SHELL */

#endif /* __NET_SHELL_H */
