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

int net_shell_cmd_allocs(int argc, char *argv[]);
int net_shell_cmd_conn(int argc, char *argv[]);
int net_shell_cmd_dns(int argc, char *argv[]);
int net_shell_cmd_iface(int argc, char *argv[]);
int net_shell_cmd_mem(int argc, char *argv[]);
int net_shell_cmd_nbr(int argc, char *argv[]);
int net_shell_cmd_ping(int argc, char *argv[]);
int net_shell_cmd_route(int argc, char *argv[]);
int net_shell_cmd_stacks(int argc, char *argv[]);
int net_shell_cmd_stats(int argc, char *argv[]);
int net_shell_cmd_tcp(int argc, char *argv[]);

#endif /* __NET_SHELL_H */
