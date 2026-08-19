/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRIVERS_NET_NSOS_NETDB_H__
#define __DRIVERS_NET_NSOS_NETDB_H__

enum nsos_resolve_status {
	/** Invalid value for `ai_flags' field */
	NSOS_MID_EAI_BADFLAGS    = -1,
	/** NAME or SERVICE is unknown */
	NSOS_MID_EAI_NONAME      = -2,
	/** Temporary failure in name resolution */
	NSOS_MID_EAI_AGAIN       = -3,
	/** Non-recoverable failure in name res */
	NSOS_MID_EAI_FAIL        = -4,
	/** `ai_family' not supported */
	NSOS_MID_EAI_FAMILY      = -6,
	/** `ai_socktype' not supported */
	NSOS_MID_EAI_SOCKTYPE    = -7,
	/** SRV not supported for `ai_socktype' */
	NSOS_MID_EAI_SERVICE     = -8,
	/** Memory allocation failure */
	NSOS_MID_EAI_MEMORY      = -10,
	/** System error returned in `errno' */
	NSOS_MID_EAI_SYSTEM      = -11,
	/** Argument buffer overflow */
	NSOS_MID_EAI_OVERFLOW    = -12,
};

/** Flags for getaddrinfo() hints */
#define NSOS_MID_AI_PASSIVE     0x1
#define NSOS_MID_AI_CANONNAME   0x2
#define NSOS_MID_AI_NUMERICHOST 0x4
#define NSOS_MID_AI_V4MAPPED    0x8
#define NSOS_MID_AI_ALL         0x10
#define NSOS_MID_AI_ADDRCONFIG  0x20
#define NSOS_MID_AI_NUMERICSERV 0x400

int eai_to_nsos_mid(int err);
int eai_from_nsos_mid(int err);

int addrinfo_flags_to_nsos_mid(int flags, int *flags_mid);
int addrinfo_flags_from_nsos_mid(int flags_mid, int *flags);

#endif /* __DRIVERS_NET_NSOS_NETDB_H__ */
