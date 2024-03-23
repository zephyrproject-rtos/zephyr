/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NET_IF_H_
#define ZEPHYR_INCLUDE_POSIX_NET_IF_H_

#ifdef CONFIG_NET_INTERFACE_NAME_LEN
#define IF_NAMESIZE CONFIG_NET_INTERFACE_NAME_LEN
#else
#define IF_NAMESIZE 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct if_nameindex {
	unsigned int if_index;
	char *if_name;
};

char *if_indextoname(unsigned int ifindex, char *ifname);
void if_freenameindex(struct if_nameindex *ptr);
struct if_nameindex *if_nameindex(void);
unsigned int if_nametoindex(const char *ifname);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_NET_IF_H_ */
