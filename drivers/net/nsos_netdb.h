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

int eai_to_nsos_mid(int err);
int eai_from_nsos_mid(int err);

#endif /* __DRIVERS_NET_NSOS_NETDB_H__ */
