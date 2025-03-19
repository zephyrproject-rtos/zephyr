/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsi_errno.h"
#include "nsi_utils.h"

struct nsi_errno_mid_map {
	/** Embedded/host error code */
	int err;
	/** NSI_errno middleground error code */
	int mid_err;
};

#define ERR(_name) {_name, NSI_ERRNO_MID_##_name}

static const struct nsi_errno_mid_map map[] = {
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

int nsi_errno_to_mid(int err)
{
	if (err == 0) {
		return err;
	}

	for (int i = 0; i < NSI_ARRAY_SIZE(map); i++) {
		if (map[i].err == err) {
			return map[i].mid_err;
		}
	}

	return err;
}

int nsi_errno_from_mid(int err)
{
	if (err == 0) {
		return err;
	}

	for (int i = 0; i < NSI_ARRAY_SIZE(map); i++) {
		if (map[i].mid_err == err) {
			return map[i].err;
		}
	}

	return err;
}

#if defined(NSI_RUNNER_BUILD)
int nsi_get_errno_in_mid(void)
{
	return nsi_errno_to_mid(errno);
}
#endif

int nsi_host_get_errno(void)
{
	return nsi_errno_from_mid(nsi_get_errno_in_mid());
}
