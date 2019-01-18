/**
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "simplelink_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <stdlib.h>
#include <limits.h>

#include <zephyr.h>
/* Define sockaddr, etc, before simplelink.h */
#include <net/socket_offload.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/driver.h>
#include "simplelink_support.h"

#include "tls_internal.h"

/* Mutex for getaddrinfo() calls: */
K_MUTEX_DEFINE(ga_mutex);

static int simplelink_socket(int family, int type, int proto)
{
	uint8_t sec_method = SL_SO_SEC_METHOD_SSLv3_TLSV1_2;
	int sd;
	int retval = 0;
	int sl_proto = proto;

	/* Map Zephyr socket.h AF_INET6 to SimpleLink's: */
	family = (family == AF_INET6 ? SL_AF_INET6 : family);

	/* Map Zephyr TLS protocols to TI's values: */
	if (proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) {
		sl_proto = SL_SEC_SOCKET;
	} else if (proto >= IPPROTO_DTLS_1_0 && proto <= IPPROTO_DTLS_1_2) {
		/* SimpleLink doesn't handle DTLS yet! */
		retval = EPROTONOSUPPORT;
		goto exit;
	}

	/* Ensure other Zephyr definitions match SimpleLink's: */
	__ASSERT(family == SL_AF_INET || family == SL_AF_INET6 \
		 || family == SL_INADDR_ANY,		       \
		 "unrecognized family: %d", family);
	__ASSERT(type == SL_SOCK_STREAM || type == SL_SOCK_DGRAM || \
		 type == SL_SOCK_RAW,				    \
		 "unrecognized type: %d", type);
	__ASSERT(sl_proto == SL_IPPROTO_TCP || sl_proto == SL_IPPROTO_UDP || \
		 sl_proto == SL_IPPROTO_RAW || sl_proto == SL_SEC_SOCKET,    \
		 "unrecognized proto: %d", sl_proto);

	sd = sl_Socket(family, type, sl_proto);
	if (sd < 0) {
		retval = sd;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	    && sl_proto == SL_SEC_SOCKET) {
		/* Now, set specific TLS version via setsockopt(): */
		sec_method = (proto - IPPROTO_TLS_1_0) + SL_SO_SEC_METHOD_TLSV1;
		retval = sl_SetSockOpt(sd, SL_SOL_SOCKET, SL_SO_SECMETHOD,
				       &sec_method, sizeof(sec_method));
		if (retval < 0) {
			retval = EPROTONOSUPPORT;
			(void)sl_Close(sd);
			goto exit;
		}
	}

	retval = sd;

exit:
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
		sl_addr_in->sin_addr.s_addr =
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
				sl_addr_in->sin_addr.s_addr;
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

	/* TBD: Until we have a good way to get correct date from Zephyr,
	 * log a date validation error as a warning, but continue connection:
	 */
	if (retval == SL_ERROR_BSD_ESECDATEERROR) {
		LOG_WRN("Failed certificate date validation: %d", retval);
		retval = 0;
	}

	/* Warn users when root CA is not in the certificate catalog.
	 * For enhanced security, users should update the catalog with the
	 * certificates for sites the device is expected to connect to. Note
	 * the connection is established successfully even when the root CA
	 * is not part of the catalog.
	 */
	if (retval == SL_ERROR_BSD_ESECUNKNOWNROOTCA) {
		LOG_WRN("Unknown root CA used. For proper security, please "
			"use a root CA that is part of the certificate "
			"catalog in production systems.");
		retval = 0;
	}

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

#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS

/* Iterate through the list of Zephyr's credential types, and
 * map to SimpleLink values, then set stored filenames
 * via SimpleLink's sl_SetSockOpt()
 */
static int map_credentials(int sd, const void *optval, socklen_t optlen)
{
	sec_tag_t *sec_tags = (sec_tag_t *)optval;
	int retval = 0;
	int sec_tags_len;
	sec_tag_t tag;
	int opt;
	int i;
	struct tls_credential *cert;

	if ((optlen % sizeof(sec_tag_t)) != 0 || (optlen == 0)) {
		retval = EINVAL;
		goto exit;
	} else {
		sec_tags_len = optlen / sizeof(sec_tag_t);
	}

	/* For each tag, retrieve the credentials value and type: */
	for (i = 0; i < sec_tags_len; i++) {
		tag = sec_tags[i];
		cert = credential_next_get(tag, NULL);
		while (cert != NULL) {
			/* Map Zephyr cert types to Simplelink cert options: */
			switch (cert->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				opt = SL_SO_SECURE_FILES_CA_FILE_NAME;
				break;
			case TLS_CREDENTIAL_SERVER_CERTIFICATE:
				opt = SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME;
				break;
			case TLS_CREDENTIAL_PRIVATE_KEY:
				opt = SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME;
				break;
			case TLS_CREDENTIAL_NONE:
			case TLS_CREDENTIAL_PSK:
			case TLS_CREDENTIAL_PSK_ID:
			default:
				/* Not handled by SimpleLink: */
				retval = EINVAL;
				goto exit;
			}
			retval = sl_SetSockOpt(sd, SL_SOL_SOCKET, opt,
					       cert->buf,
					       (SlSocklen_t)cert->len);
			if (retval < 0) {
				break;
			}
			cert = credential_next_get(tag, cert);
		}
	}

exit:
	return retval;
}
#else
static int map_credentials(int sd, const void *optval, socklen_t optlen)
{
	return 0;
}
#endif  /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

/* Excerpted from SimpleLink's socket.h:
 * "Unsupported: these are only placeholders to not break BSD code."
 *  Remove once Zephyr has POSIX socket options defined.
 */
#define SO_BROADCAST  (200)
#define SO_REUSEADDR  (201)
#define SO_SNDBUF     (202)
#define TCP_NODELAY   (203)

/* Needed to keep line lengths < 80: */
#define _SEC_DOMAIN_VERIF SL_SO_SECURE_DOMAIN_NAME_VERIFICATION

static int simplelink_setsockopt(int sd, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	int retval;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		/* Handle Zephyr's SOL_TLS secure socket options: */
		switch (optname) {
		case TLS_SEC_TAG_LIST:
			/* Bind credential filenames to this socket: */
			retval = map_credentials(sd, optval, optlen);
			break;
		case TLS_HOSTNAME:
			retval = sl_SetSockOpt(sd, SL_SOL_SOCKET,
					       _SEC_DOMAIN_VERIF,
					       (const char *)optval, optlen);
			break;
		case TLS_CIPHERSUITE_LIST:
		case TLS_PEER_VERIFY:
		case TLS_DTLS_ROLE:
			/* Not yet supported: */
			retval = ENOTSUP;
			break;
		default:
			retval = EINVAL;
			break;
		}
	} else {
		/* Can be SOL_SOCKET or TI specific: */

		/* Note: this logic should match SimpleLink SDK's socket.c: */
		switch (optname) {
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
			/* These sock opts aren't supported by the cc32xx
			 * network stack, so we ignore them and set errno to
			 * EINVAL in order to not break "off-the-shelf" BSD
			 * code.
			 */
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_SNDBUF:
			retval = EINVAL;
			goto exit;
		default:
			break;
		}
		retval = sl_SetSockOpt(sd, SL_SOL_SOCKET, optname, optval,
				       (SlSocklen_t)optlen);
	}

exit:
	return _SlDrvSetErrno(retval);
}

static int simplelink_getsockopt(int sd, int level, int optname,
				 void *optval, socklen_t *optlen)
{
	int retval;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		/* Handle Zephyr's SOL_TLS secure socket options: */
		switch (optname) {
		case TLS_SEC_TAG_LIST:
		case TLS_CIPHERSUITE_LIST:
		case TLS_CIPHERSUITE_USED:
			/* Not yet supported: */
			retval = ENOTSUP;
			break;
		default:
			retval = EINVAL;
			break;
		}
	} else {
		/* Can be SOL_SOCKET or TI specific: */

		/* Note: this logic should match SimpleLink SDK's socket.c: */
		switch (optname) {
			/* TCP_NODELAY always set by the NWP, so return True */
		case TCP_NODELAY:
			if (optval) {
				(*(_u32 *)optval) = TRUE;
				retval = 0;
				goto exit;
			}
			/* These sock opts aren't supported by the cc32xx
			 * network stack, so we silently ignore them and set
			 * errno to EINVAL in order to not break "off-the-shelf"
			 * BSD code.
			 */
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_SNDBUF:
			retval = EINVAL;
			goto exit;
		default:
			break;
		}
		retval = sl_GetSockOpt(sd, SL_SOL_SOCKET, optname, optval,
				       (SlSocklen_t *)optlen);
	}
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

/*
 * Later SimpleLink SDK versions implement the full getaddrinfo semantics,
 * returning potentially multiple IP addresses.
 * This version implements a simple gethostbyname() API for client only.
 */
static int simplelink_getaddrinfo(const char *node, const char *service,
				  const struct addrinfo *hints,
				  struct addrinfo **res)
{
	_u8 sl_family = SL_AF_INET;
	unsigned long port = 0;
	int socktype = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct addrinfo *ai;
	struct sockaddr *ai_addr;
	_i16 retval;
	_u32 ipaddr[4];

	/* Check args: */
	if (!node) {
		retval = EAI_NONAME;
		goto exit;
	}
	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > USHRT_MAX) {
			retval = EAI_SERVICE;
			goto exit;
		}
	}
	if (!res) {
		retval = EAI_NONAME;
		goto exit;
	}

	/* See if any hints for family; otherwise, default to AF_INET. */
	if (hints) {
		/* Note: SimpleLink SDK doesn't support AF_UNSPEC: */
		sl_family = (hints->ai_family == AF_INET6 ?
			     SL_AF_INET6 : SL_AF_INET);
	}

	/* Now, try to resolve host name: */
	k_mutex_lock(&ga_mutex, K_FOREVER);
	retval = sl_NetAppDnsGetHostByName((signed char *)node, strlen(node),
					   ipaddr, sl_family);
	k_mutex_unlock(&ga_mutex);

	if (retval < 0) {
		LOG_ERR("Could not resolve name: %s, retval: %d",
			    node, retval);
		retval = EAI_NONAME;
		goto exit;
	}

	/* Allocate out res (addrinfo) struct.	Just one. */
	*res = calloc(1, sizeof(struct addrinfo));
	ai = *res;
	if (!ai) {
		retval = EAI_MEMORY;
		goto exit;
	} else {
		/* Now, alloc the embedded sockaddr struct: */
		ai_addr = calloc(1, sizeof(struct sockaddr));
		if (!ai_addr) {
			retval = EAI_MEMORY;
			free(*res);
			goto exit;
		}
	}

	/* Now, fill in the fields of res (addrinfo struct): */
	ai->ai_family = (sl_family == SL_AF_INET6 ? AF_INET6 : AF_INET);
	if (hints) {
		socktype = hints->ai_socktype;
	}
	ai->ai_socktype = socktype;

	if (socktype == SOCK_DGRAM) {
		proto = IPPROTO_UDP;
	}
	ai->ai_protocol = proto;

	/* Fill sockaddr struct fields based on family: */
	if (ai->ai_family == AF_INET) {
		net_sin(ai_addr)->sin_family = ai->ai_family;
		net_sin(ai_addr)->sin_addr.s_addr = htonl(ipaddr[0]);
		net_sin(ai_addr)->sin_port = htons(port);
		ai->ai_addrlen = sizeof(struct sockaddr_in);
	} else {
		net_sin6(ai_addr)->sin6_family = ai->ai_family;
		net_sin6(ai_addr)->sin6_addr.s6_addr32[0] = htonl(ipaddr[0]);
		net_sin6(ai_addr)->sin6_addr.s6_addr32[1] = htonl(ipaddr[1]);
		net_sin6(ai_addr)->sin6_addr.s6_addr32[2] = htonl(ipaddr[2]);
		net_sin6(ai_addr)->sin6_addr.s6_addr32[3] = htonl(ipaddr[3]);
		net_sin6(ai_addr)->sin6_port = htons(port);
		ai->ai_addrlen = sizeof(struct sockaddr_in6);
	}
	ai->ai_addr = ai_addr;

exit:
	return retval;
}

static void simplelink_freeaddrinfo(struct addrinfo *res)
{
	__ASSERT_NO_MSG(res);

	free(res->ai_addr);
	free(res);
}

static int simplelink_fctnl(int fd, int cmd, va_list args)
{
	ARG_UNUSED(fd);
	ARG_UNUSED(cmd);
	ARG_UNUSED(args);

	errno = ENOTSUP;
	return -1;
}

void simplelink_sockets_init(void)
{
	k_mutex_init(&ga_mutex);
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
	.getaddrinfo = simplelink_getaddrinfo,
	.freeaddrinfo = simplelink_freeaddrinfo,
	.fcntl = simplelink_fctnl,
};
