/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "version.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/hostname.h>
#include <zephyr/posix/sys/utsname.h>

#ifdef CONFIG_NET_HOSTNAME_ENABLE
#define UTSNAME_NODENAME CONFIG_NET_HOSTNAME
#else
#define UTSNAME_NODENAME "zephyr"
#endif

#if defined(__DATE__) && defined(__TIME__)
#define UTSNAME_VERSION(_ver) _ver " " __DATE__ " " __TIME__
#else
#define UTSNAME_VERSION(_ver) _ver
#endif

#ifdef BUILD_VERSION
#define VERSION_BUILD STRINGIFY(BUILD_VERSION)
#else
#define VERSION_BUILD KERNEL_VERSION_STRING
#endif

#define UTSNAME_INITIALIZER(_sys, _node, _rel, _ver, _mach)                                        \
	{                                                                                          \
		.sysname = _sys, .nodename = _node, .release = _rel,                               \
		.version = UTSNAME_VERSION(_ver), .machine = _mach,                                \
	}

static const struct utsname z_name = UTSNAME_INITIALIZER(
	"Zephyr", UTSNAME_NODENAME, KERNEL_VERSION_STRING, VERSION_BUILD, CONFIG_ARCH);

BUILD_ASSERT(sizeof(z_name.sysname) >= sizeof("Zephyr"));
BUILD_ASSERT(sizeof(z_name.release) >= sizeof(KERNEL_VERSION_STRING));
BUILD_ASSERT(sizeof(z_name.version) >= sizeof(UTSNAME_VERSION(VERSION_BUILD)));
BUILD_ASSERT(sizeof(z_name.machine) >= sizeof(CONFIG_ARCH));

int uname(struct utsname *name)
{
	memcpy(name, &z_name, sizeof(*name));
	if (IS_ENABLED(CONFIG_NET_HOSTNAME_ENABLE)) {
		strncpy(name->nodename, net_hostname_get(), sizeof(name->nodename));
		name->nodename[sizeof(name->nodename) - 1] = '\0';
	}
	return 0;
}
