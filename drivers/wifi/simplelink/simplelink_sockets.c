/**
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "simplelink_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>

#include <zephyr/kernel.h>
/* Define sockaddr, etc, before simplelink.h */
#include <zephyr/net/socket_offload.h>

#include <errno.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/driver.h>
#include <ti/net/slnetutils.h>
#include <ti/net/slnetif.h>
#include "simplelink_support.h"

#include "sockets_internal.h"
#include "tls_internal.h"

#define FAILED (-1)

/* Increment by 1 to make sure we do not store the value of 0, which has
 * a special meaning in the fdtable subsys.
 */
#define SD_TO_OBJ(sd) ((void *)(sd + 1))
#define OBJ_TO_SD(obj) (((int)obj) - 1)

static int simplelink_socket_accept(void *obj, struct sockaddr *addr,
			     socklen_t *addrlen);

/*
 * Convert SL error codes into BSD errno values
 * note that we are handling the same set of values as in TI SlNetSock
 * minus the ones that are not defined in ti/drivers/net/wifi/errors.h.
 */
static int getErrno(_i32 error)
{
	if (error >= 0) {
		return error;
	}
	/* This switch case block is necessary for translating the NWP error
	 * code to BSD ones. The #ifdef in each case are made in order to
	 * reduce code footprint: These cases are compiled if and only if
	 * there's a discrepancy between the BSD error number and the error
	 * code returned by the NWP.
	 */
	switch (error) {
#if EBADF != SL_ERROR_BSD_EBADF
	case SL_ERROR_BSD_EBADF:
		error = EBADF;
		break;
#endif
#if ENSOCK !=  SL_ERROR_BSD_ENSOCK
	case SL_ERROR_BSD_ENSOCK:
		/* The limit on total # of open sockets has been reached */
		error = ENSOCK;
		break;
#endif
#if EAGAIN != SL_ERROR_BSD_EAGAIN
	case SL_ERROR_BSD_EAGAIN:
		error = EAGAIN;
		break;
#endif
#if ENOMEM != SL_ERROR_BSD_ENOMEM
	case SL_ERROR_BSD_ENOMEM:
		error = ENOMEM;
		break;
#endif
#if EACCES != SL_ERROR_BSD_EACCES
	case SL_ERROR_BSD_EACCES:
		error = EACCES;
		break;
#endif
#if EFAULT != SL_ERROR_BSD_EFAULT
	case SL_ERROR_BSD_EFAULT:
		error = EFAULT;
		break;
#endif
#if EINVAL != SL_ERROR_BSD_EINVAL
	case SL_ERROR_BSD_EINVAL:
		error = EINVAL;
		break;
#endif
#if EDESTADDRREQ != SL_ERROR_BSD_EDESTADDRREQ
	case SL_ERROR_BSD_EDESTADDRREQ:
		error = EDESTADDRREQ;
		break;
#endif
#if EPROTOTYPE != SL_ERROR_BSD_EPROTOTYPE
	case SL_ERROR_BSD_EPROTOTYPE:
		error = EPROTOTYPE;
		break;
#endif
#if ENOPROTOOPT != SL_ERROR_BSD_ENOPROTOOPT
	case SL_ERROR_BSD_ENOPROTOOPT:
		error = ENOPROTOOPT;
		break;
#endif
#if EPROTONOSUPPORT != SL_ERROR_BSD_EPROTONOSUPPORT
	case SL_ERROR_BSD_EPROTONOSUPPORT:
		error = EPROTONOSUPPORT;
		break;
#endif
#if EOPNOTSUPP != SL_ERROR_BSD_EOPNOTSUPP
	case SL_ERROR_BSD_EOPNOTSUPP:
		error = EOPNOTSUPP;
		break;
#endif
#if EAFNOSUPPORT != SL_ERROR_BSD_EAFNOSUPPORT
	case SL_ERROR_BSD_EAFNOSUPPORT:
		error = EAFNOSUPPORT;
		break;
#endif
#if EADDRINUSE != SL_ERROR_BSD_EADDRINUSE
	case SL_ERROR_BSD_EADDRINUSE:
		error = EADDRINUSE;
		break;
#endif
#if EADDRNOTAVAIL != SL_ERROR_BSD_EADDRNOTAVAIL
	case SL_ERROR_BSD_EADDRNOTAVAIL:
		error = EADDRNOTAVAIL;
		break;
#endif
#if ENETUNREACH != SL_ERROR_BSD_ENETUNREACH
	case SL_ERROR_BSD_ENETUNREACH:
		error = ENETUNREACH;
		break;
#endif
#if ENOBUFS != SL_ERROR_BSD_ENOBUFS
	case SL_ERROR_BSD_ENOBUFS:
		error = ENOBUFS;
		break;
#endif
#if EISCONN != SL_ERROR_BSD_EISCONN
	case SL_ERROR_BSD_EISCONN:
		error = EISCONN;
		break;
#endif
#if ENOTCONN != SL_ERROR_BSD_ENOTCONN
	case SL_ERROR_BSD_ENOTCONN:
		error = ENOTCONN;
		break;
#endif
#if ETIMEDOUT != SL_ERROR_BSD_ETIMEDOUT
	case SL_ERROR_BSD_ETIMEDOUT:
		error = ETIMEDOUT;
		break;
#endif
#if ECONNREFUSED != SL_ERROR_BSD_ECONNREFUSED
	case SL_ERROR_BSD_ECONNREFUSED:
		error = ECONNREFUSED;
		break;
#endif
	/* The cases below are proprietary driver errors, which can
	 * be returned by the SimpleLink Driver, in various cases of failure.
	 * Each is mapped to the corresponding BSD error.
	 */
	case SL_POOL_IS_EMPTY:
	case SL_RET_CODE_NO_FREE_ASYNC_BUFFERS_ERROR:
	case SL_RET_CODE_MALLOC_ERROR:
		error = ENOMEM;
		break;
	case SL_RET_CODE_INVALID_INPUT:
	case SL_EZEROLEN:
	case SL_ESMALLBUF:
	case SL_INVALPARAM:
		error = EINVAL;
		break;
	default:
	/* Do nothing ..
	 * If no case is true, that means that the BSD error
	 * code and the code returned by the NWP are either identical,
	 * or no proprietary error has occurred.
	 */
		break;
	}

	return error;
}

static int simplelink_socket_family_from_posix(int family, int *family_sl)
{
	switch (family) {
	case AF_INET:
		*family_sl = SL_AF_INET;
		break;
	case AF_INET6:
		*family_sl = SL_AF_INET6;
		break;
	default:
		return -EAFNOSUPPORT;
	}

	return 0;
}

static int simplelink_socket_type_from_posix(int type, int *type_sl)
{
	switch (type) {
	case SOCK_STREAM:
		*type_sl = SL_SOCK_STREAM;
		break;
	case SOCK_DGRAM:
		*type_sl = SL_SOCK_DGRAM;
		break;
	case SOCK_RAW:
		*type_sl = SL_SOCK_RAW;
		break;
	default:
		return -ESOCKTNOSUPPORT;
	}

	return 0;
}

static int simplelink_socket_proto_from_zephyr(int proto, int *proto_sl)
{
	if (proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) {
		*proto_sl = SL_SEC_SOCKET;
	} else if (proto >= IPPROTO_DTLS_1_0 && proto <= IPPROTO_DTLS_1_2) {
		/* SimpleLink doesn't handle DTLS yet! */
		return -EPROTONOSUPPORT;
	} else {
		switch (proto) {
		case IPPROTO_TCP:
			*proto_sl = SL_IPPROTO_TCP;
			break;
		case IPPROTO_UDP:
			*proto_sl = SL_IPPROTO_UDP;
			break;
		default:
			return -EPROTONOSUPPORT;
		}
	}

	return 0;
}

static int simplelink_socket(int family, int type, int proto)
{
	uint8_t sec_method = SL_SO_SEC_METHOD_SSLv3_TLSV1_2;
	int sd;
	int retval = 0;
	int sl_proto = proto;
	int err;

	/* Map Zephyr socket.h family to SimpleLink's: */
	err = simplelink_socket_family_from_posix(family, &family);
	if (err) {
		LOG_ERR("unsupported family: %d", family);
		retval = slcb_SetErrno(-err);
		goto exit;
	}

	/* Map Zephyr socket.h type to SimpleLink's: */
	err = simplelink_socket_type_from_posix(type, &type);
	if (err) {
		LOG_ERR("unsupported type: %d", type);
		retval = slcb_SetErrno(-err);
		goto exit;
	}

	/* Map Zephyr protocols to TI's values: */
	err = simplelink_socket_proto_from_zephyr(proto, &sl_proto);
	if (err) {
		LOG_ERR("unsupported proto: %d", proto);
		retval = slcb_SetErrno(-err);
		goto exit;
	}

	sd = sl_Socket(family, type, sl_proto);
	if (sd >= 0) {
		if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		    && sl_proto == SL_SEC_SOCKET) {
			/* Now, set specific TLS version via setsockopt(): */
			sec_method = (proto - IPPROTO_TLS_1_0) +
				SL_SO_SEC_METHOD_TLSV1;
			retval = sl_SetSockOpt(sd, SL_SOL_SOCKET,
				SL_SO_SECMETHOD,
				&sec_method, sizeof(sec_method));
			if (retval < 0) {
				retval = slcb_SetErrno(EPROTONOSUPPORT);
				(void)sl_Close(sd);
				goto exit;
			}
		}
	}

	retval = sd;

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

exit:
	return retval;
}

static int simplelink_close(void *obj)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	retval = sl_Close(sd);

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

	return retval;
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
		sl_addr_in->sin_family = SL_AF_INET;
		sl_addr_in->sin_port = z_sockaddr_in->sin_port;
		sl_addr_in->sin_addr.s_addr =
			z_sockaddr_in->sin_addr.s_addr;

		sl_addr = (SlSockAddr_t *)sl_addr_in;
	} else if (addrlen == sizeof(struct sockaddr_in6)) {
		struct sockaddr_in6 *z_sockaddr_in6 =
			(struct sockaddr_in6 *)addr;

		*sl_addrlen = sizeof(SlSockAddrIn6_t);
		sl_addr_in6->sin6_family = SL_AF_INET6;
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
				(uint8_t)sl_addr_in6->sin6_scope_id;
			memcpy(z_sockaddr_in6->sin6_addr.s6_addr,
			       sl_addr_in6->sin6_addr._S6_un._S6_u32,
			       sizeof(z_sockaddr_in6->sin6_addr.s6_addr));
			*addrlen = sizeof(struct sockaddr_in6);
		} else {
			*addrlen = sl_addrlen;
		}
	}
}

static int simplelink_accept(void *obj, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	int sd = OBJ_TO_SD(obj);
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
	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

	return retval;
}

static int simplelink_bind(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;

	if (addr == NULL) {
		retval = slcb_SetErrno(EISDIR);
		return retval;
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
	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

	return retval;
}

static int simplelink_listen(void *obj, int backlog)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	retval = (int)sl_Listen(sd, backlog);

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

	return retval;
}

static int simplelink_connect(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
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
	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

	return retval;
}

#define ONE_THOUSAND 1000

static const struct socket_op_vtable simplelink_socket_fd_op_vtable;

static int simplelink_poll(struct zsock_pollfd *fds, int nfds, int msecs)
{
	int max_sd = 0;
	struct SlTimeval_t tv, *ptv;
	SlFdSet_t rfds;	 /* Set of read file descriptors */
	SlFdSet_t wfds;	 /* Set of write file descriptors */
	int i, retval, sd;
	void *obj;

	if (nfds > SL_FD_SETSIZE) {
		retval = slcb_SetErrno(EINVAL);
		goto exit;
	}

	/* Convert time to SlTimeval struct values: */
	if (msecs == SYS_FOREVER_MS) {
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
		if (fds[i].fd < 0) {
			continue;
		} else {
			obj = z_get_fd_obj(fds[i].fd,
					   (const struct fd_op_vtable *)
						&simplelink_socket_fd_op_vtable,
					   ENOTSUP);
			if (obj != NULL) {
				/* Offloaded socket found. */
				sd = OBJ_TO_SD(obj);
			} else {
				/* Non-offloaded socket, return an error. */
				retval = slcb_SetErrno(EINVAL);
				goto exit;
			}
		}
		if (fds[i].events & ZSOCK_POLLIN) {
			SL_SOCKET_FD_SET(sd, &rfds);
		}
		if (fds[i].events & ZSOCK_POLLOUT) {
			SL_SOCKET_FD_SET(sd, &wfds);
		}
		if (sd > max_sd) {
			max_sd = sd;
		}
	}

	/* Wait for requested read and write fds to be ready: */
	retval = sl_Select(max_sd + 1, &rfds, &wfds, NULL, ptv);
	if (retval > 0) {
		for (i = 0; i < nfds; i++) {
			if (fds[i].fd >= 0) {
				obj = z_get_fd_obj(
					fds[i].fd,
					(const struct fd_op_vtable *)
						&simplelink_socket_fd_op_vtable,
					ENOTSUP);
				sd = OBJ_TO_SD(obj);
				if (SL_SOCKET_FD_ISSET(sd, &rfds)) {
					fds[i].revents |= ZSOCK_POLLIN;
				}
				if (SL_SOCKET_FD_ISSET(sd, &wfds)) {
					fds[i].revents |= ZSOCK_POLLOUT;
				}
			}
		}
	}

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

exit:
	return retval;
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
				retval = getErrno(retval);
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

/* Needed to keep line lengths < 80: */
#define _SEC_DOMAIN_VERIF SL_SO_SECURE_DOMAIN_NAME_VERIFICATION

static int simplelink_setsockopt(void *obj, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		/* Handle Zephyr's SOL_TLS secure socket options: */
		switch (optname) {
		case TLS_SEC_TAG_LIST:
			/* Bind credential filenames to this socket: */
			retval = map_credentials(sd, optval, optlen);
			if (retval != 0) {
				retval = slcb_SetErrno(retval);
				goto exit;
			}
			break;
		case TLS_HOSTNAME:
			retval = sl_SetSockOpt(sd, SL_SOL_SOCKET,
					       _SEC_DOMAIN_VERIF,
					       (const char *)optval, optlen);
			break;
		case TLS_PEER_VERIFY:
			if (optval) {
				/*
				 * Not currently supported. Verification
				 * is automatically performed if a CA
				 * certificate is set. We are returning
				 * success here to allow
				 * mqtt_client_tls_connect()
				 * to proceed, given it requires
				 * verification and it is indeed
				 * performed when the cert is set.
				 */
				if (*(uint32_t *)optval != 2U) {
					retval = slcb_SetErrno(ENOTSUP);
					goto exit;
				} else {
					retval = 0;
				}
			} else {
				retval = slcb_SetErrno(EINVAL);
				goto exit;
			}
			break;
		case TLS_CIPHERSUITE_LIST:
		case TLS_DTLS_ROLE:
			/* Not yet supported: */
			retval = slcb_SetErrno(ENOTSUP);
			goto exit;
		default:
			retval = slcb_SetErrno(EINVAL);
			goto exit;
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
				if (*(uint32_t *)optval) {
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
			retval = slcb_SetErrno(EINVAL);
			goto exit;
		default:
			break;
		}
		retval = sl_SetSockOpt(sd, SL_SOL_SOCKET, optname, optval,
				       (SlSocklen_t)optlen);
	}

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

exit:
	return retval;
}

static int simplelink_getsockopt(void *obj, int level, int optname,
				 void *optval, socklen_t *optlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		/* Handle Zephyr's SOL_TLS secure socket options: */
		switch (optname) {
		case TLS_SEC_TAG_LIST:
		case TLS_CIPHERSUITE_LIST:
		case TLS_CIPHERSUITE_USED:
			/* Not yet supported: */
			retval = slcb_SetErrno(ENOTSUP);
			goto exit;
		default:
			retval = slcb_SetErrno(EINVAL);
			goto exit;
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
			retval = slcb_SetErrno(EINVAL);
			goto exit;
		default:
			break;
		}
		retval = sl_GetSockOpt(sd, SL_SOL_SOCKET, optname, optval,
				       (SlSocklen_t *)optlen);
	}

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

exit:
	return retval;
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

	if (flags & ZSOCK_MSG_PEEK) {
		retval = ENOTSUP;
	} else if (flags & ZSOCK_MSG_DONTWAIT) {
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

static ssize_t simplelink_recvfrom(void *obj, void *buf, size_t len, int flags,
				   struct sockaddr *from, socklen_t *fromlen)
{
	int sd = OBJ_TO_SD(obj);
	ssize_t retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;
	int nb_enabled;

	retval = handle_recv_flags(sd, flags, TRUE, &nb_enabled);

	if (!retval) {
		/* Translate to sl_RecvFrom() parameters: */
		if (fromlen != NULL) {
			sl_addr = translate_z_to_sl_addrlen(*fromlen,
							    &sl_addr_in,
							    &sl_addr_in6,
							    &sl_addrlen);
			retval = (ssize_t)sl_RecvFrom(sd, buf, len, 0, sl_addr,
						      &sl_addrlen);
		} else {
			retval = (ssize_t)sl_Recv(sd, buf, len, 0);
		}

		handle_recv_flags(sd, flags, FALSE, &nb_enabled);
		if (retval >= 0) {
			if (fromlen != NULL) {
				/*
				 * Translate sl_addr into *addr and set
				 * *addrlen
				 */
				translate_sl_to_z_addr(sl_addr, sl_addrlen,
						       from, fromlen);
			}
		} else {
			retval = slcb_SetErrno(getErrno(retval));
		}
	} else {
		retval = slcb_SetErrno(retval);
	}

	return retval;
}

static ssize_t simplelink_sendto(void *obj, const void *buf, size_t len,
				 int flags, const struct sockaddr *to,
				 socklen_t tolen)
{
	int sd = OBJ_TO_SD(obj);
	ssize_t retval;
	SlSockAddr_t *sl_addr;
	SlSockAddrIn_t sl_addr_in;
	SlSockAddrIn6_t sl_addr_in6;
	SlSocklen_t sl_addrlen;

	if (to != NULL) {
		/* Translate to sl_SendTo() parameters: */
		sl_addr = translate_z_to_sl_addrs(to, tolen, &sl_addr_in,
						  &sl_addr_in6, &sl_addrlen);

		if (sl_addr == NULL) {
			retval = SL_RET_CODE_INVALID_INPUT;
			goto exit;
		}

		retval = sl_SendTo(sd, buf, (uint16_t)len, flags,
				   sl_addr, sl_addrlen);
	} else {
		retval = (ssize_t)sl_Send(sd, buf, len, flags);
	}

exit:
	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

	return retval;
}

static ssize_t simplelink_sendmsg(void *obj, const struct msghdr *msg,
				  int flags)
{
	errno = -ENOTSUP;
	return -1;
}

/* Adds address info entry to a list */
static int set_addr_info(const struct SlNetUtil_addrInfo_t *sl_ai,
			 struct zsock_addrinfo **res)
{
	struct zsock_addrinfo *ai;
	struct sockaddr *ai_addr;
	int retval = 0;

	ai = calloc(1, sizeof(struct zsock_addrinfo));
	if (!ai) {
		retval = DNS_EAI_MEMORY;
		goto exit;
	} else {
		/* Now, alloc the embedded sockaddr struct: */
		ai_addr = calloc(1, sizeof(struct sockaddr));
		if (!ai_addr) {
			retval = DNS_EAI_MEMORY;
			free(ai);
			goto exit;
		}
	}

	/* Now, fill in the fields of res (addrinfo struct): */
	ai->ai_family = (sl_ai->ai_family == SL_AF_INET6 ? AF_INET6 : AF_INET);
	ai->ai_socktype = (sl_ai->ai_socktype == SLNETSOCK_SOCK_DGRAM ?
		SOCK_DGRAM : SOCK_STREAM);
	ai->ai_protocol = (sl_ai->ai_protocol == SLNETSOCK_PROTO_UDP ?
		IPPROTO_UDP : IPPROTO_TCP);

	/* Fill sockaddr struct fields based on family: */
	if (ai->ai_family == AF_INET) {
		SlNetSock_AddrIn_t *sl_addr =
			(SlNetSock_AddrIn_t *)sl_ai->ai_addr;

		net_sin(ai_addr)->sin_family = ai->ai_family;
		net_sin(ai_addr)->sin_addr.s_addr = sl_addr->sin_addr.s_addr;
		net_sin(ai_addr)->sin_port = sl_addr->sin_port;
		ai->ai_addrlen = sizeof(struct sockaddr_in);
	} else {
		SlNetSock_AddrIn6_t *sl_addr =
			(SlNetSock_AddrIn6_t *)sl_ai->ai_addr;

		net_sin6(ai_addr)->sin6_family = ai->ai_family;
		net_sin6(ai_addr)->sin6_addr.s6_addr32[0] =
			sl_addr->sin6_addr._S6_un._S6_u32[0];
		net_sin6(ai_addr)->sin6_addr.s6_addr32[1] =
			sl_addr->sin6_addr._S6_un._S6_u32[1];
		net_sin6(ai_addr)->sin6_addr.s6_addr32[2] =
			sl_addr->sin6_addr._S6_un._S6_u32[2];
		net_sin6(ai_addr)->sin6_addr.s6_addr32[3] =
			sl_addr->sin6_addr._S6_un._S6_u32[3];
		net_sin6(ai_addr)->sin6_port = sl_addr->sin6_port;
		ai->ai_addrlen = sizeof(struct sockaddr_in6);
	}
	ai->ai_addr = ai_addr;
	ai->ai_next = *res;
	*res = ai;

exit:
	return retval;
}

static int simplelink_getaddrinfo(const char *node, const char *service,
				  const struct zsock_addrinfo *hints,
				  struct zsock_addrinfo **res)
{
	int32_t retval;
	struct SlNetUtil_addrInfo_t sl_hints;
	struct SlNetUtil_addrInfo_t *sl_res, *sl_ai;

	/* Initialize sl_hints to the defaults */
	memset(&sl_hints, 0, sizeof(sl_hints));

	/* Check args: */
	if (!res) {
		retval = DNS_EAI_NONAME;
		goto exit;
	}

	if (hints) {
		/*
		 * SlNetUtil only supports AI_NUMERICHOST and AI_PASSIVE, so
		 * the rest are ignored.
		 */
		sl_hints.ai_flags |= ((hints->ai_flags & AI_PASSIVE) ?
			SLNETUTIL_AI_PASSIVE : 0);
		sl_hints.ai_flags |= ((hints->ai_flags & AI_NUMERICHOST) ?
			SLNETUTIL_AI_NUMERICHOST : 0);
		if (hints->ai_family == AF_UNSPEC) {
			sl_hints.ai_family = SLNETSOCK_AF_UNSPEC;
		} else {
			sl_hints.ai_family = (hints->ai_family == AF_INET6 ?
				SLNETSOCK_AF_INET6 : SLNETSOCK_AF_INET);
		}
		if (hints->ai_socktype == 0) {
			sl_hints.ai_socktype = 0;
		} else {
			sl_hints.ai_socktype =
				(hints->ai_socktype == SOCK_DGRAM ?
				SLNETSOCK_SOCK_DGRAM : SLNETSOCK_SOCK_STREAM);
		}
		if (hints->ai_protocol == 0) {
			sl_hints.ai_protocol = 0;
		} else {
			sl_hints.ai_protocol =
				(hints->ai_protocol == IPPROTO_UDP ?
				SLNETSOCK_PROTO_UDP : SLNETSOCK_PROTO_TCP);
		}

	}

	/* Now, try to resolve host name: */
	retval = SlNetUtil_getAddrInfo(SLNETIF_ID_1, node,
		service, &sl_hints, &sl_res);

	if (retval < 0) {
		LOG_ERR("Could not resolve name: %s, retval: %d",
			    node, retval);
		retval = DNS_EAI_NONAME;
		goto exit;
	}

	sl_ai = sl_res;
	*res = NULL;
	while (sl_ai != NULL) {
		retval = set_addr_info(sl_ai, res);
		if (retval < 0) {
			LOG_ERR("Unable to set address info, retval: %d",
				retval);
			goto exit;
		}
		sl_ai = sl_ai->ai_next;
	}
	SlNetUtil_freeAddrInfo(sl_res);
exit:
	return retval;
}

static void simplelink_freeaddrinfo(struct zsock_addrinfo *res)
{
	__ASSERT_NO_MSG(res);

	free(res->ai_addr);
	free(res);
}

static int simplelink_fcntl(int sd, int cmd, va_list args)
{
	int retval = 0;
	SlSockNonblocking_t enableOption;
	SlSocklen_t optlen = sizeof(SlSockNonblocking_t);

	switch (cmd) {
	case F_GETFL:
		retval = sl_GetSockOpt(sd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
			(_u8 *)&enableOption, &optlen);
		if (retval == 0) {
			if (enableOption.NonBlockingEnabled) {
				retval |= O_NONBLOCK;
			}
		}
		break;
	case F_SETFL:
		if ((va_arg(args, int) & O_NONBLOCK) != 0) {
			enableOption.NonBlockingEnabled = 1;
		} else {
			enableOption.NonBlockingEnabled = 0;
		}
		retval = sl_SetSockOpt(sd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
			&enableOption, optlen);
		break;
	default:
		LOG_ERR("Invalid command: %d", cmd);
		retval = slcb_SetErrno(EINVAL);
		goto exit;
	}

	if (retval < 0) {
		retval = slcb_SetErrno(getErrno(retval));
	}

exit:
	return retval;
}

static int simplelink_ioctl(void *obj, unsigned int request, va_list args)
{
	int sd = OBJ_TO_SD(obj);

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds;
		int timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return simplelink_poll(fds, nfds, timeout);
	}

	/* Otherwise, just forward to offloaded fcntl()
	 * In Zephyr, fcntl() is just an alias of ioctl().
	 */
	default:
		return simplelink_fcntl(sd, request, args);
	}
}

static ssize_t simplelink_read(void *obj, void *buffer, size_t count)
{
	return simplelink_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t simplelink_write(void *obj, const void *buffer,
					  size_t count)
{
	return simplelink_sendto(obj, buffer, count, 0, NULL, 0);
}

static const struct socket_op_vtable simplelink_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = simplelink_read,
		.write = simplelink_write,
		.close = simplelink_close,
		.ioctl = simplelink_ioctl,
	},
	.bind = simplelink_bind,
	.connect = simplelink_connect,
	.listen = simplelink_listen,
	.accept = simplelink_socket_accept,
	.sendto = simplelink_sendto,
	.sendmsg = simplelink_sendmsg,
	.recvfrom = simplelink_recvfrom,
	.getsockopt = simplelink_getsockopt,
	.setsockopt = simplelink_setsockopt,
};

static bool simplelink_is_supported(int family, int type, int proto)
{
	int dummy;
	int err;

	err = simplelink_socket_family_from_posix(family, &dummy);
	if (err) {
		return false;
	}

	err = simplelink_socket_type_from_posix(type, &dummy);
	if (err) {
		return false;
	}

	err = simplelink_socket_proto_from_zephyr(proto, &dummy);
	if (err) {
		return false;
	}

	return true;
}

int simplelink_socket_create(int family, int type, int proto)
{
	int fd = z_reserve_fd();
	int sock;

	if (fd < 0) {
		return -1;
	}

	sock = simplelink_socket(family, type, proto);
	if (sock < 0) {
		z_free_fd(fd);
		return -1;
	}

	z_finalize_fd(fd, SD_TO_OBJ(sock),
		      (const struct fd_op_vtable *)
					&simplelink_socket_fd_op_vtable);

	return fd;
}

static int simplelink_socket_accept(void *obj, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	int fd = z_reserve_fd();
	int sock;

	if (fd < 0) {
		return -1;
	}

	sock = simplelink_accept(obj, addr, addrlen);
	if (sock < 0) {
		z_free_fd(fd);
		return -1;
	}

	z_finalize_fd(fd, SD_TO_OBJ(sock),
		      (const struct fd_op_vtable *)
					&simplelink_socket_fd_op_vtable);

	return fd;
}

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
NET_SOCKET_OFFLOAD_REGISTER(simplelink, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,
			    simplelink_is_supported, simplelink_socket_create);
#endif

void simplelink_sockets_init(void)
{
}

const struct socket_dns_offload simplelink_dns_ops = {
	.getaddrinfo = simplelink_getaddrinfo,
	.freeaddrinfo = simplelink_freeaddrinfo,
};
