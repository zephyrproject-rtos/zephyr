/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libc_extensions.h"

#define FN_MISSING() printf("[IMPLEMENTATION MISSING : %s]\n", __func__)

int iscntrl(int c)
{
	/* All the characters placed before the space on the ASCII table
	 * and the 0x7F character (DEL) are control characters.
	 */
	return (int)(c < ' ' || c == 0x7F);
}

size_t strftime(char *dst, size_t dst_size,
		const char *fmt,
		const struct tm *tm)
{
	FN_MISSING();

	return 0;
}

double difftime(time_t end, time_t beg)
{
	return end - beg;
}

struct __strerr_wrap {
	int err;
	const char *errstr;
};

/* Implementation suggested by @rakons in #16527 */
#define STRERR_DEFINE(e)	{e, #e}

static const struct __strerr_wrap error_strings[] = {
	STRERR_DEFINE(EILSEQ),
	STRERR_DEFINE(EDOM),
	STRERR_DEFINE(ERANGE),
	STRERR_DEFINE(ENOTTY),
	STRERR_DEFINE(EACCES),
	STRERR_DEFINE(EPERM),
	STRERR_DEFINE(ENOENT),
	STRERR_DEFINE(ESRCH),
	STRERR_DEFINE(EEXIST),
	STRERR_DEFINE(ENOSPC),
	STRERR_DEFINE(ENOMEM),
	STRERR_DEFINE(EBUSY),
	STRERR_DEFINE(EINTR),
	STRERR_DEFINE(EAGAIN),
	STRERR_DEFINE(ESPIPE),
	STRERR_DEFINE(EXDEV),
	STRERR_DEFINE(EROFS),
	STRERR_DEFINE(ENOTEMPTY),
	STRERR_DEFINE(ECONNRESET),
	STRERR_DEFINE(ETIMEDOUT),
	STRERR_DEFINE(ECONNREFUSED),
	STRERR_DEFINE(EHOSTDOWN),
	STRERR_DEFINE(EHOSTUNREACH),
	STRERR_DEFINE(EADDRINUSE),
	STRERR_DEFINE(EPIPE),
	STRERR_DEFINE(EIO),
	STRERR_DEFINE(ENXIO),
	STRERR_DEFINE(ENOTBLK),
	STRERR_DEFINE(ENODEV),
	STRERR_DEFINE(ENOTDIR),
	STRERR_DEFINE(EISDIR),
	STRERR_DEFINE(ETXTBSY),
	STRERR_DEFINE(ENOEXEC),
	STRERR_DEFINE(EINVAL),
	STRERR_DEFINE(E2BIG),
	STRERR_DEFINE(ELOOP),
	STRERR_DEFINE(ENAMETOOLONG),
	STRERR_DEFINE(ENFILE),
	STRERR_DEFINE(EMFILE),
	STRERR_DEFINE(EBADF),
	STRERR_DEFINE(ECHILD),
	STRERR_DEFINE(EFAULT),
	STRERR_DEFINE(EFBIG),
	STRERR_DEFINE(EMLINK),
	STRERR_DEFINE(ENOLCK),
	STRERR_DEFINE(EDEADLK),
	STRERR_DEFINE(ECANCELED),
	STRERR_DEFINE(ENOSYS),
	STRERR_DEFINE(ENOMSG),
	STRERR_DEFINE(ENOSTR),
	STRERR_DEFINE(ENODATA),
	STRERR_DEFINE(ETIME),
	STRERR_DEFINE(ENOSR),
	STRERR_DEFINE(EPROTO),
	STRERR_DEFINE(EBADMSG),
	STRERR_DEFINE(ENOTSOCK),
	STRERR_DEFINE(EDESTADDRREQ),
	STRERR_DEFINE(EMSGSIZE),
	STRERR_DEFINE(EPROTOTYPE),
	STRERR_DEFINE(ENOPROTOOPT),
	STRERR_DEFINE(EPROTONOSUPPORT),
	STRERR_DEFINE(ESOCKTNOSUPPORT),
	STRERR_DEFINE(ENOTSUP),
	STRERR_DEFINE(EPFNOSUPPORT),
	STRERR_DEFINE(EAFNOSUPPORT),
	STRERR_DEFINE(EADDRNOTAVAIL),
	STRERR_DEFINE(ENETDOWN),
	STRERR_DEFINE(ENETUNREACH),
	STRERR_DEFINE(ENETRESET),
	STRERR_DEFINE(ECONNABORTED),
	STRERR_DEFINE(ENOBUFS),
	STRERR_DEFINE(EISCONN),
	STRERR_DEFINE(ENOTCONN),
	STRERR_DEFINE(ESHUTDOWN),
	STRERR_DEFINE(EALREADY),
	STRERR_DEFINE(EINPROGRESS),
};

static char *strerr_unknown = "UNKNOWN";

char *strerror(int err)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(error_strings); ++i) {
		if (error_strings[i].err == err) {
			return (char *)error_strings[i].errstr;
		}
	}

	return strerr_unknown;
}

int sscanf(const char *s, const char *format, ...)
{
	FN_MISSING();

	return 0;
}

double atof(const char *str)
{
	/* XXX good enough for civetweb uses */
	return (double)atoi(str);
}

long long int strtoll(const char *str, char **endptr, int base)
{
	/* XXX good enough for civetweb uses */
	return (long long int)strtol(str, endptr, base);
}

time_t time(time_t *t)
{
	return 0;
}

/*
 * Most of the wrappers below are copies of the wrappers in net/sockets.h,
 * but they are available only if CONFIG_NET_SOCKETS_POSIX_NAMES is enabled
 * which is impossible here.
 */

int getsockname(int sock, struct sockaddr *addr,
		socklen_t *addrlen)
{
	return zsock_getsockname(sock, addr, addrlen);
}

int poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	return zsock_poll(fds, nfds, timeout);
}

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
		char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags)
{
	return zsock_getnameinfo(addr, addrlen, host, hostlen,
				 serv, servlen, flags);
}

ssize_t send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_send(sock, buf, len, flags);
}

ssize_t recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recv(sock, buf, max_len, flags);
}

int socket(int family, int type, int proto)
{
	return zsock_socket(family, type, proto);
}

int getaddrinfo(const char *host, const char *service,
		const struct zsock_addrinfo *hints,
		struct zsock_addrinfo **res)
{
	return zsock_getaddrinfo(host, service, hints, res);
}

void freeaddrinfo(struct zsock_addrinfo *ai)
{
	zsock_freeaddrinfo(ai);
}

int connect(int sock, const struct sockaddr *addr,
	    socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

int getsockopt(int sock, int level, int optname,
	       void *optval, socklen_t *optlen)
{
	return zsock_getsockopt(sock, level, optname, optval, optlen);
}

int setsockopt(int sock, int level, int optname,
	       const void *optval, socklen_t optlen)
{
	return zsock_setsockopt(sock, level, optname, optval, optlen);
}

int listen(int sock, int backlog)
{
	return zsock_listen(sock, backlog);
}

int bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_bind(sock, addr, addrlen);
}

int accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_accept(sock, addr, addrlen);
}
