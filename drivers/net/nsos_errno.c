/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsos_errno.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

struct nsos_mid_errno_map {
	/** Zephyr/host error code */
	int err;
	/** NSOS middleground error code */
	int mid_err;
};

#define ERR(_name)				\
	{ _name, NSOS_MID_ ## _name }

static const struct nsos_mid_errno_map map[] = {
	ERR(EPERM),
	ERR(ENOENT),
	ERR(ESRCH),
	ERR(EINTR),
	ERR(EIO),
	ERR(ENXIO),
	ERR(E2BIG),
	ERR(ENOEXEC),
	ERR(EBADF),
	ERR(ECHILD),
	ERR(EAGAIN),
	ERR(ENOMEM),
	ERR(EACCES),
	ERR(EFAULT),
	ERR(ENOTBLK),
	ERR(EBUSY),
	ERR(EEXIST),
	ERR(EXDEV),
	ERR(ENODEV),
	ERR(ENOTDIR),
	ERR(EISDIR),
	ERR(EINVAL),
	ERR(ENFILE),
	ERR(EMFILE),
	ERR(ENOTTY),
	ERR(ETXTBSY),
	ERR(EFBIG),
	ERR(ENOSPC),
	ERR(ESPIPE),
	ERR(EROFS),
	ERR(EMLINK),
	ERR(EPIPE),
	ERR(EDOM),
	ERR(ERANGE),
	ERR(ENOMSG),
	ERR(EDEADLK),
	ERR(ENOLCK),
	ERR(ENOSTR),
	ERR(ENODATA),
	ERR(ETIME),
	ERR(ENOSR),
	ERR(EPROTO),
	ERR(EBADMSG),
	ERR(ENOSYS),
	ERR(ENOTEMPTY),
	ERR(ENAMETOOLONG),
	ERR(ELOOP),
	ERR(EOPNOTSUPP),
	ERR(EPFNOSUPPORT),
	ERR(ECONNRESET),
	ERR(ENOBUFS),
	ERR(EAFNOSUPPORT),
	ERR(EPROTOTYPE),
	ERR(ENOTSOCK),
	ERR(ENOPROTOOPT),
	ERR(ESHUTDOWN),
	ERR(ECONNREFUSED),
	ERR(EADDRINUSE),
	ERR(ECONNABORTED),
	ERR(ENETUNREACH),
	ERR(ENETDOWN),
	ERR(ETIMEDOUT),
	ERR(EHOSTDOWN),
	ERR(EHOSTUNREACH),
	ERR(EINPROGRESS),
	ERR(EALREADY),
	ERR(EDESTADDRREQ),
	ERR(EMSGSIZE),
	ERR(EPROTONOSUPPORT),
	ERR(ESOCKTNOSUPPORT),
	ERR(EADDRNOTAVAIL),
	ERR(ENETRESET),
	ERR(EISCONN),
	ERR(ENOTCONN),
	ERR(ETOOMANYREFS),
	ERR(ENOTSUP),
	ERR(EILSEQ),
	ERR(EOVERFLOW),
	ERR(ECANCELED),
};

int errno_to_nsos_mid(int err)
{
	for (int i = 0; i < ARRAY_SIZE(map); i++) {
		if (map[i].err == err) {
			return map[i].mid_err;
		}
	}

	return err;
}

int errno_from_nsos_mid(int err)
{
	for (int i = 0; i < ARRAY_SIZE(map); i++) {
		if (map[i].mid_err == err) {
			return map[i].err;
		}
	}

	return err;
}
