/**
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
/* Define sockaddr, etc, before simplelink.h */
#include <net/socket_offload.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/driver.h>
#include "simplelink_support.h"

static int simplelink_socket(int family, int type, int proto)
{
	int retval;

	/* Map Zephyr socket.h AF_INET6 to SimpleLink's: */
	family = (family == AF_INET6 ? SL_AF_INET6 : family);

	/* Ensure other Zephyr definitions match SimpleLink's: */
	__ASSERT(family == SL_AF_INET || family == SL_AF_INET6 \
		 || family == SL_INADDR_ANY,		       \
		 "unrecognized family: %d", family);
	__ASSERT(type == SL_SOCK_STREAM || type == SL_SOCK_DGRAM || \
		 type == SL_SOCK_RAW,				    \
		 "unrecognized type: %d", type);
	__ASSERT(proto == SL_IPPROTO_TCP || proto == SL_IPPROTO_UDP || \
		 proto == SL_IPPROTO_RAW || proto == SL_SEC_SOCKET,    \
		 "unrecognized proto: %d", proto);

	retval = sl_Socket(family, type, proto);

	return _SlDrvSetErrno(retval);
}

static int simplelink_close(int sd)
{
	int retval;

	retval = sl_Close(sd);

	return _SlDrvSetErrno(retval);
}

static SlSockAddr_t *translate_z_to_sl_addrlen(socklen_t addrlen,
					       SlSockAddrIn_t *sl_addr_in,
					       SlSockAddrIn6_t *sl_addr_in6,
					       SlSocklen_t *sl_addrlen)
{
	SlSockAddr_t *sl_addr = NULL;

	if (addrlen == sizeof(struct sockaddr_in)) {
		*sl_addrlen = sizeof(SlSockAddrIn_t);
		sl_addr = (SlSockAddr_t *)sl_addr_in;
	} else if (addrlen == sizeof(struct sockaddr_in6)) {
		*sl_addrlen = sizeof(SlSockAddrIn6_t);
		sl_addr = (SlSockAddr_t *)sl_addr_in6;
	}

	return sl_addr;
}

static SlSockAddr_t *translate_z_to_sl_addrs(const struct sockaddr *addr,
					     socklen_t addrlen,
					     SlSockAddrIn_t *sl_addr_in,
					     SlSockAddrIn6_t *sl_addr_in6,
					     SlSocklen_t *sl_addrlen)
{
	SlSockAddr_t *sl_addr = NULL;

	if (addrlen == sizeof(struct sockaddr_in)) {
		struct sockaddr_in *z_sockaddr_in = (struct sockaddr_in *)addr;

		*sl_addrlen = sizeof(SlSockAddrIn_t);
		sl_addr_in->sin_family = AF_INET;
		sl_addr_in->sin_port = z_sockaddr_in->sin_port;
		sl_addr_in->sin_addr.S_un.S_addr =
			z_sockaddr_in->sin_addr.s_addr;

		sl_addr = (SlSockAddr_t *)sl_addr_in;
	} else if (addrlen == sizeof(struct sockaddr_in6)) {
		struct sockaddr_in6 *z_sockaddr_in6 =
			(struct sockaddr_in6 *)addr;

		*sl_addrlen = sizeof(SlSockAddrIn6_t);
		sl_addr_in6->sin6_family = AF_INET6;
		sl_addr_in6->sin6_port = z_sockaddr_in6->sin6_port;
		memcpy(sl_addr_in6->sin6_addr._S6_un._S6_u32,
		       z_sockaddr_in6->sin6_addr.s6_addr,
		       sizeof(sl_addr_in6->sin6_addr._S6_un._S6_u32));

		sl_addr = (SlSockAddr_t *)sl_addr_in6;
	}

	return sl_addr;
}

static void translate_sl_to_z_addr(SlSockAddr_t *sl_addr,
				   SlSocklen_t sl_addrlen,
				   struct sockaddr *addr,
				   socklen_t *addrlen)
{
	SlSockAddrIn_t *sl_addr_in;
	SlSockAddrIn6_t *sl_addr_in6;

	if (sl_addr->sa_family == SL_AF_INET) {
		if (sl_addrlen == (SlSocklen_t)sizeof(SlSockAddrIn_t)) {
			struct sockaddr_in *z_sockaddr_in =
				(struct sockaddr_in *)addr;

			sl_addr_in = (SlSockAddrIn_t *)sl_addr;
			z_sockaddr_in->sin_family = AF_INET;
			z_sockaddr_in->sin_port = sl_addr_in->sin_port;
			z_sockaddr_in->sin_addr.s_addr =
				sl_addr_in->sin_addr.S_un.S_addr;
			*addrlen = sizeof(struct sockaddr_in);
		} else {
			*addrlen = sl_addrlen;
		}
	} else if (sl_addr->sa_family == SL_AF_INET6) {
		if (sl_addrlen == sizeof(SlSockAddrIn6_t)) {
			struct sockaddr_in6 *z_sockaddr_in6 =
				(struct sockaddr_in6 *)addr;
			sl_addr_in6 = (SlSockAddrIn6_t *)sl_addr;

			z_sockaddr_in6->sin6_family = AF_INET6;
			z_sockaddr_in6->sin6_port = sl_addr_in6->sin6_port;
			z_sockaddr_in6->sin6_scope_id =
				(u8_t)sl_addr_in6->sin6_scope_id;
			memcpy(z_sockaddr_in6->sin6_addr.s6_addr,
			       sl_addr_in6->sin6_addr._S6_un._S6_u32,
			       sizeof(z_sockaddr_in6->sin6_addr.s6_addr));
			*addrlen = sizeof(struct sockaddr_in6);
		} else {
			*addrlen = sl_addrlen;
		}
	}
}

static int simplelink_accept(int sd, struct sockaddr *addr, socklen_t *addrlen)
{
	int retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;

	if ((addrlen == NULL) || (addr == NULL)) {
		retval = SL_RET_CODE_INVALID_INPUT;
		goto exit;
	}

	/* Translate between Zephyr's and SimpleLink's sockaddr's: */
	sl_addr = translate_z_to_sl_addrlen(*addrlen, &sl_addr_in, &sl_addr_in6,
				  &sl_addrlen);
	if (sl_addr == NULL) {
		retval = SL_RET_CODE_INVALID_INPUT;
		goto exit;
	}

	retval = sl_Accept(sd, sl_addr, &sl_addrlen);
	if (retval < 0) {
		goto exit;
	}

	/* Translate returned sl_addr into *addr and set *addrlen: */
	translate_sl_to_z_addr(sl_addr, sl_addrlen, addr, addrlen);

exit:
	return _SlDrvSetErrno(retval);
}

static int simplelink_bind(int sd, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	int retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;

	if (addr == NULL) {
		retval = EISDIR;
		goto exit;
	}

	/* Translate to sl_Bind() parameters: */
	sl_addr = translate_z_to_sl_addrs(addr, addrlen, &sl_addr_in,
					  &sl_addr_in6, &sl_addrlen);

	if (sl_addr == NULL) {
		retval = SL_RET_CODE_INVALID_INPUT;
		goto exit;
	}

	retval = sl_Bind(sd, sl_addr, sl_addrlen);

exit:
	return _SlDrvSetErrno(retval);
}

static int simplelink_listen(int sd, int backlog)
{
	int retval;

	retval = (int)sl_Listen(sd, backlog);

	return _SlDrvSetErrno(retval);
}

static int simplelink_connect(int sd, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	int retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;

	__ASSERT_NO_MSG(addr);

	/* Translate to sl_Connect() parameters: */
	sl_addr = translate_z_to_sl_addrs(addr, addrlen, &sl_addr_in,
					  &sl_addr_in6, &sl_addrlen);

	if (sl_addr == NULL) {
		retval = SL_RET_CODE_INVALID_INPUT;
		goto exit;
	}

	retval = sl_Connect(sd, sl_addr, sl_addrlen);

exit:
	return _SlDrvSetErrno(retval);
}

#define ONE_THOUSAND 1000

static int simplelink_poll(struct pollfd *fds, int nfds, int msecs)
{
	int max_fd = 0;
	struct SlTimeval_t tv, *ptv;
	SlFdSet_t rfds;	 /* Set of read file descriptors */
	SlFdSet_t wfds;	 /* Set of write file descriptors */
	int i, retval, fd;

	if (nfds > SL_FD_SETSIZE) {
		retval = EINVAL;
		goto exit;
	}

	/* Convert time to SlTimeval struct values: */
	if (msecs == K_FOREVER) {
		ptv = NULL;
	} else {
		tv.tv_sec = msecs / ONE_THOUSAND;
		tv.tv_usec = (msecs % ONE_THOUSAND) * ONE_THOUSAND;
		ptv = &tv;
	}

	/* Setup read and write fds for select, based on pollfd fields: */
	SL_SOCKET_FD_ZERO(&rfds);
	SL_SOCKET_FD_ZERO(&wfds);

	for (i = 0; i < nfds; i++) {
		fds[i].revents = 0;
		fd = fds[i].fd;
		if (fd < 0) {
			continue;
		}
		if (fds[i].events & POLLIN) {
			SL_SOCKET_FD_SET(fd, &rfds);
		}
		if (fds[i].events & POLLOUT) {
			SL_SOCKET_FD_SET(fd, &wfds);
		}
		if (fds[i].fd > max_fd) {
			max_fd = fds[i].fd;
		}
	}

	/* Wait for requested read and write fds to be ready: */
	retval = sl_Select(max_fd + 1, &rfds, &wfds, NULL, ptv);
	if (retval > 0) {
		for (i = 0; i < (max_fd + 1); i++) {
			if (SL_SOCKET_FD_ISSET(fds[i].fd, &rfds)) {
				fds[i].revents |= POLLIN;
			}
			if (SL_SOCKET_FD_ISSET(fds[i].fd, &wfds)) {
				fds[i].revents |= POLLOUT;
			}
		}
	}

exit:
	return _SlDrvSetErrno(retval);
}


/* Excerpted from SimpleLink's socket.h:
 * "Unsupported: these are only placeholders to not break BSD code."
 *  Remove once Zephyr has POSIX socket options defined.
 */
#define SO_BROADCAST  (200)
#define SO_REUSEADDR  (201)
#define SO_SNDBUF     (202)
#define TCP_NODELAY   (203)

static int simplelink_setsockopt(int sd, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	int retval;

	/* Note: this logic should match SimpleLink SDK's socket.c: */
	switch (optname) {
	/* TCP_NODELAY is always set by the NWP; true always succeeds */
	case TCP_NODELAY:
		if (optval) {
			/* if user wishes to have TCP_NODELAY = FALSE,
			 * we return EINVAL and fail in the cases below.
			 */
			if (*(u32_t *)optval) {
				retval = 0;
				goto exit;
			}
		}
	/* These sock opts aren't supported by the cc32xx network stack,
	 * so we ignore them and set errno to EINVAL
	 * in order to not break "off-the-shelf" BSD code.
	 */
	case SO_BROADCAST:
	case SO_REUSEADDR:
	case SO_SNDBUF:
		retval = EINVAL;
		goto exit;
	default:
		break;
	}

	retval = sl_SetSockOpt(sd, level, optname, optval, (SlSocklen_t)optlen);

exit:
	return _SlDrvSetErrno(retval);
}

static int simplelink_getsockopt(int sd, int level, int optname,
				 void *optval, socklen_t *optlen)
{
	int retval;

	/* Note: this logic should match SimpleLink SDK's socket.c: */
	switch (optname) {
	/* TCP_NODELAY is always set by the NWP, so we return True */
	case TCP_NODELAY:
		if (optval) {
			(*(_u32 *)optval) = TRUE;
			retval = 0;
			goto exit;
		}
	/* These sock opts aren't supported by the cc32xx network stack,
	 * so we silently ignore them and set errno to EINVAL
	 * in order to not break "off-the-shelf" BSD code.
	 */
	case SO_BROADCAST:
	case SO_REUSEADDR:
	case SO_SNDBUF:
	retval = EINVAL;
		goto exit;
	default:
		break;
	}

	retval = sl_GetSockOpt(sd, level, optname, optval,
			       (SlSocklen_t *)optlen);

exit:
	return _SlDrvSetErrno(retval);
}

/* SimpleLink does not support flags in recv.
 * However, to enable more Zephyr apps to use this socket_offload, rather than
 * failing with ENOTSUP, we can closely emulate the MSG_DONTWAIT feature using
 * SimpleLink socket options.
 */
static int handle_recv_flags(int sd, int flags, bool set, int *nb_enabled)
{
	ssize_t retval = 0;
	SlSocklen_t optlen = sizeof(SlSockNonblocking_t);
	SlSockNonblocking_t enableOption;

	if (flags & MSG_PEEK) {
		retval = ENOTSUP;
	} else if (flags & MSG_DONTWAIT) {
		if (set) {
			/* Get previous state, to restore later: */
			sl_GetSockOpt(sd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
				      (_u8 *)&enableOption, &optlen);
			*nb_enabled = enableOption.NonBlockingEnabled;

			/* Now, set to non_blocking if not already set: */
			if (!*nb_enabled) {
				enableOption.NonBlockingEnabled = 1;
				sl_SetSockOpt(sd, SL_SOL_SOCKET,
					      SL_SO_NONBLOCKING,
					      (_u8 *)&enableOption,
					      sizeof(enableOption));
			}
		} else {
			/* Restore socket to previous state: */
			enableOption.NonBlockingEnabled = *nb_enabled;
			sl_SetSockOpt(sd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
				      (_u8 *)&enableOption,
				      sizeof(enableOption));
		}
	}

	return retval;
}

static ssize_t simplelink_recv(int sd, void *buf, size_t max_len, int flags)
{
	ssize_t retval;
	int nb_enabled;

	retval = handle_recv_flags(sd, flags, TRUE, &nb_enabled);
	if (!retval) {
		retval = (ssize_t)sl_Recv(sd, buf, max_len, 0);
		handle_recv_flags(sd, flags, FALSE, &nb_enabled);
	}

	return (ssize_t)(_SlDrvSetErrno(retval));
}

static ssize_t simplelink_recvfrom(int sd, void *buf, short int len,
				   short int flags, struct sockaddr *from,
				   socklen_t *fromlen)
{
	ssize_t retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;
	int nb_enabled;

	__ASSERT_NO_MSG(fromlen);

	retval = handle_recv_flags(sd, flags, TRUE, &nb_enabled);

	if (!retval) {
		/* Translate to sl_RecvFrom() parameters: */
		sl_addr = translate_z_to_sl_addrlen(*fromlen, &sl_addr_in,
						    &sl_addr_in6,
						    &sl_addrlen);
		if (sl_addr == NULL) {
			retval = SL_RET_CODE_INVALID_INPUT;
		} else {
			retval = (ssize_t)sl_RecvFrom(sd, buf, len, 0, sl_addr,
						      &sl_addrlen);
			handle_recv_flags(sd, flags, FALSE, &nb_enabled);
		}

		if (retval >= 0) {
			/* Translate sl_addr into *addr and set *addrlen: */
			translate_sl_to_z_addr(sl_addr, sl_addrlen, from,
					       fromlen);
		}
	}

	return _SlDrvSetErrno(retval);
}

static ssize_t simplelink_send(int sd, const void *buf, size_t len,
			       int flags)
{
	ssize_t retval;

	retval = (ssize_t)sl_Send(sd, buf, len, flags);

	return _SlDrvSetErrno(retval);
}

static ssize_t simplelink_sendto(int sd, const void *buf, size_t len,
				    int flags, const struct sockaddr *to,
				    socklen_t tolen)
{
	ssize_t retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;

	/* Translate to sl_SendTo() parameters: */
	sl_addr = translate_z_to_sl_addrs(to, tolen, &sl_addr_in,
					  &sl_addr_in6, &sl_addrlen);

	if (sl_addr == NULL) {
		retval = SL_RET_CODE_INVALID_INPUT;
		goto exit;
	}

	retval = sl_SendTo(sd, buf, (u16_t)len, flags,
				    sl_addr, sl_addrlen);
exit:
	return _SlDrvSetErrno(retval);
}

const struct socket_offload simplelink_ops = {
	.socket = simplelink_socket,
	.close = simplelink_close,
	.accept = simplelink_accept,
	.bind = simplelink_bind,
	.listen = simplelink_listen,
	.connect = simplelink_connect,
	.poll = simplelink_poll,
	.setsockopt = simplelink_setsockopt,
	.getsockopt = simplelink_getsockopt,
	.recv = simplelink_recv,
	.recvfrom = simplelink_recvfrom,
	.send = simplelink_send,
	.sendto = simplelink_sendto,
};
