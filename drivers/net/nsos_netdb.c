/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * netdb.h related code common to Zephyr (top: nsos_sockets.c) and Linux
 * (bottom: nsos_adapt.c).
 *
 * It is needed by both sides to share the same macro definitions/values
 * (prefixed with NSOS_MID_), which is not possible to achieve with two separate
 * standard libc libraries, since they use different values for the same
 * symbols.
 */

#include "nsos_netdb.h"
#include "nsi_errno.h"

#ifdef __ZEPHYR__

#include <zephyr/net/socket.h>
#define ERR(_name)				\
	{ DNS_ ## _name, NSOS_MID_ ## _name }

#define AI_FLAG(_name)				\
	{ ZSOCK_ ## _name, NSOS_MID_ ## _name }

#else

#include <netdb.h>
#define ERR(_name)				\
	{ _name, NSOS_MID_ ## _name }

#define AI_FLAG(_name)				\
	{ _name, NSOS_MID_ ## _name }

#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

struct nsos_eai_map {
	int err;
	int mid_err;
};

static const struct nsos_eai_map map[] = {
	ERR(EAI_BADFLAGS),
	ERR(EAI_NONAME),
	ERR(EAI_AGAIN),
	ERR(EAI_FAIL),
	ERR(EAI_FAMILY),
	ERR(EAI_SOCKTYPE),
	ERR(EAI_SERVICE),
	ERR(EAI_MEMORY),
	ERR(EAI_SYSTEM),
	ERR(EAI_OVERFLOW),
};

int eai_to_nsos_mid(int err)
{
	for (int i = 0; i < ARRAY_SIZE(map); i++) {
		if (map[i].err == err) {
			return map[i].mid_err;
		}
	}

	return err;
}

int eai_from_nsos_mid(int err)
{
	for (int i = 0; i < ARRAY_SIZE(map); i++) {
		if (map[i].mid_err == err) {
			return map[i].err;
		}
	}

	return err;
}

struct nsos_flag_map {
	int flag;
	int mid_flag;
};

static const struct nsos_flag_map ai_flags_map[] = {
	AI_FLAG(AI_PASSIVE),
	AI_FLAG(AI_CANONNAME),
	AI_FLAG(AI_NUMERICHOST),
	AI_FLAG(AI_V4MAPPED),
	AI_FLAG(AI_ALL),
	AI_FLAG(AI_ADDRCONFIG),
	AI_FLAG(AI_NUMERICSERV),
};

int addrinfo_flags_to_nsos_mid(int flags, int *flags_mid)
{
	*flags_mid = 0;

	for (int i = 0; i < ARRAY_SIZE(ai_flags_map); i++) {
		if (flags & ai_flags_map[i].flag) {
			*flags_mid |= ai_flags_map[i].mid_flag;
			flags &= ~ai_flags_map[i].flag;
		}
	}

	if (flags) {
		return -NSI_ERRNO_MID_EINVAL;
	}

	return 0;
}

int addrinfo_flags_from_nsos_mid(int flags_mid, int *flags)
{
	*flags = 0;

	for (int i = 0; i < ARRAY_SIZE(ai_flags_map); i++) {
		if (flags_mid & ai_flags_map[i].mid_flag) {
			*flags |= ai_flags_map[i].flag;
			flags_mid &= ~ai_flags_map[i].mid_flag;
		}
	}

	if (flags_mid) {
		return -NSI_ERRNO_MID_EINVAL;
	}

	return 0;
}
