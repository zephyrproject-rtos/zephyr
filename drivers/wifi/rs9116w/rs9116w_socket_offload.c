/*
 * Copyright (c) 2021 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME wifi_rs9116w_socket_offload
#define LOG_LEVEL	CONFIG_WIFI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/net/socket_offload.h>
#include <zephyr/net/tls_credentials.h>

#include "sockets_internal.h"
#include "tls_internal.h"
#include "rs9116w.h"

#include <rsi_wlan_common_config.h>
#include <rsi_wlan_apis.h>
#include <rsi_wlan_non_rom.h>
#include <rsi_wlan.h>
#include <zephyr/net/net_pkt.h>


extern struct rs9116w_device s_rs9116w_dev[];

/* Dealing with mismatched define values */
/* Protocol families. */
#define Z_PF_UNSPEC   0	       /**< Unspecified protocol family.  */
#define Z_PF_INET     1	       /**< IP protocol family version 4. */
#define Z_PF_INET6    2	       /**< IP protocol family version 6. */
#define Z_PF_PACKET   3	       /**< Packet family.                */
#define Z_PF_CAN      4	       /**< Controller Area Network.      */
#define Z_PF_NET_MGMT 5	       /**< Network management info.      */
#define Z_PF_LOCAL    6	       /**< Inter-process communication   */
#define Z_PF_UNIX     PF_LOCAL /**< Inter-process communication   */

/* Address families. */
#define Z_AF_UNSPEC   Z_PF_UNSPEC   /**< Unspecified address family.   */
#define Z_AF_INET     Z_PF_INET	    /**< IP protocol family version 4. */
#define Z_AF_INET6    Z_PF_INET6    /**< IP protocol family version 6. */
#define Z_AF_PACKET   Z_PF_PACKET   /**< Packet family.                */
#define Z_AF_CAN      Z_PF_CAN	    /**< Controller Area Network.      */
#define Z_AF_NET_MGMT Z_PF_NET_MGMT /**< Network management info.      */
#define Z_AF_LOCAL    Z_PF_LOCAL    /**< Inter-process communication   */
#define Z_AF_UNIX     Z_PF_UNIX	    /**< Inter-process communication   */

/* Socket options for SOL_SOCKET level */
/** sockopt: Enable server address reuse (ignored, for compatibility) */
#define Z_SO_REUSEADDR 2

/* Socket options for IPPROTO_IPV6 level */
/** sockopt: Don't support IPv4 access (ignored, for compatibility) */
#define Z_IPV6_V6ONLY 26

#undef s6_addr
#undef s6_addr32
#undef IPPROTO_TCP
#undef IPPROTO_UDP

#define MAX_PAYLOAD_SIZE 1460

#define FAILED (-1)

/* Increment by 1 to make sure we do not store the value of 0, which has
 * a special meaning in the fdtable subsys.
 */
#define SD_TO_OBJ(sd)  ((void *)(sd + 1))
#define OBJ_TO_SD(obj) (((int)obj) - 1)

/* Needed since only the first closed recv actually behaves correctly */
ATOMIC_DEFINE(closed_socks_map, 10);

/* for adding nonblocking as a socket option */
ATOMIC_DEFINE(nb_socks_map, 10);

extern uint8_t rsi_wlan_get_state(void);

static int rs9116w_socket(int family, int type, int proto)
{
	int sd;
	int retval = 0;
	int rsi_proto = proto;

	/* Map Zephyr socket.h family to WiSeConnect's: */
	switch (family) {
	case Z_AF_INET:
		family = AF_INET;
		break;
	case Z_AF_INET6:
		family = AF_INET6;
		break;
	default:
		LOG_ERR("unsupported family: %d", family);
		errno = EAFNOSUPPORT;
		return -1;
	}

	/* Map Zephyr socket.h type to WiSeConnect's: */
	switch (type) {
	case SOCK_STREAM:
	case SOCK_DGRAM:
	case SOCK_RAW:
		break;
	default:
		LOG_ERR("unrecognized type: %d", type);
		errno = ESOCKTNOSUPPORT;
		return -1;
	}

	/* Map Zephyr protocols to the 9116's values: */
	if (proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) {
		/* Todo: Check TLS enabled*/

		switch (proto) {
		case IPPROTO_TLS_1_0:
			rsi_proto = PROTOCOL_TLS_1_0;
			break;
		case IPPROTO_TLS_1_1:
			rsi_proto = PROTOCOL_TLS_1_1;
			break;
		case IPPROTO_TLS_1_2:
			rsi_proto = PROTOCOL_TLS_1_2;
			break;
		}
	} else {
		switch (proto) {
		case IPPROTO_TCP:
		case IPPROTO_UDP:
			rsi_proto = 0;
			break;
		default:
			LOG_ERR("unrecognized proto: %d", proto);
			errno = EPROTONOSUPPORT;
			return -1;
		}
	}

	if (rsi_wlan_get_state() < RSI_WLAN_STATE_CONNECTED) {
		errno = ENETDOWN;
		return -1;
	}

	sd = rsi_socket(family, type, rsi_proto);

	/* Unfortunately the WiSeConnect config doesn't work quite right, as such
	 * a check had to be added here.
	 */
	if (sd >= CONFIG_WISECONNECT_SOCKETS_COUNT) {
		(void)rsi_shutdown(sd, 0);
		errno = ENFILE;
		return -1;
	}

	if (sd >= 0) {
		if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) &&
		    rsi_proto == 1) {
			/* There is a way to do it with sockopts for the 9116 (I
			 * think), but i don't know if its the best solution
			 */

			/* Now, set specific TLS version via setsockopt(): */
			if (retval < 0) {
				(void)rsi_shutdown(sd, 0);
				errno = EPROTONOSUPPORT;
				return -1;
			}
		}
	}

	retval = sd;

	return retval;
}

static int rs9116w_close(void *obj)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	retval = rsi_shutdown(sd, 0);

	if (retval < 0) {
		return -1;
	}

	return retval;
}

static struct rsi_sockaddr *translate_z_to_rsi_addrlen(
	socklen_t addrlen, struct rsi_sockaddr_in *rsi_addr_in,
	struct rsi_sockaddr_in6 *rsi_addr_in6, rsi_socklen_t *rsi_addrlen)
{
	struct rsi_sockaddr *rsi_addr = NULL;

	if (addrlen == sizeof(struct sockaddr_in)) {
		*rsi_addrlen = sizeof(struct rsi_sockaddr_in);
		rsi_addr = (struct rsi_sockaddr *)rsi_addr_in;
	} else if (addrlen == sizeof(struct sockaddr_in6)) {
		*rsi_addrlen = sizeof(struct rsi_sockaddr_in6);
		rsi_addr = (struct rsi_sockaddr *)rsi_addr_in6;
	}

	return rsi_addr;
}

static struct rsi_sockaddr *
translate_z_to_rsi_addrs(const struct sockaddr *addr, socklen_t addrlen,
			 struct rsi_sockaddr_in *rsi_addr_in,
			 struct rsi_sockaddr_in6 *rsi_addr_in6,
			 rsi_socklen_t *rsi_addrlen)
{
	struct rsi_sockaddr *rsi_addr = NULL;

	if (addrlen == sizeof(struct sockaddr_in)) {
		memset(rsi_addr_in, 0, sizeof(*rsi_addr_in));
		struct sockaddr_in *z_sockaddr_in = (struct sockaddr_in *)addr;

		*rsi_addrlen = sizeof(struct rsi_sockaddr_in);
		rsi_addr_in->sin_family = AF_INET;
		rsi_addr_in->sin_port =
			sys_be16_to_cpu(z_sockaddr_in->sin_port);
		rsi_addr_in->sin_addr.s_addr = z_sockaddr_in->sin_addr.s_addr;

		rsi_addr = (struct rsi_sockaddr *)rsi_addr_in;
	} else if (addrlen == sizeof(struct sockaddr_in6)) {
		memset(rsi_addr_in6, 0, sizeof(*rsi_addr_in6));
		struct sockaddr_in6 *z_sockaddr_in6 =
			(struct sockaddr_in6 *)addr;

		*rsi_addrlen = sizeof(struct rsi_sockaddr_in6);
		rsi_addr_in6->sin6_family = AF_INET6;
		rsi_addr_in6->sin6_port =
			sys_be16_to_cpu(z_sockaddr_in6->sin6_port);
		memcpy(rsi_addr_in6->sin6_addr._S6_un._S6_u32,
		       z_sockaddr_in6->sin6_addr.s6_addr,
		       sizeof(rsi_addr_in6->sin6_addr._S6_un._S6_u32));
		rsi_addr_in6->sin6_addr._S6_un._S6_u32[0] = sys_be32_to_cpu(
			rsi_addr_in6->sin6_addr._S6_un._S6_u32[0]);
		rsi_addr_in6->sin6_addr._S6_un._S6_u32[1] = sys_be32_to_cpu(
			rsi_addr_in6->sin6_addr._S6_un._S6_u32[1]);
		rsi_addr_in6->sin6_addr._S6_un._S6_u32[2] = sys_be32_to_cpu(
			rsi_addr_in6->sin6_addr._S6_un._S6_u32[2]);
		rsi_addr_in6->sin6_addr._S6_un._S6_u32[3] = sys_be32_to_cpu(
			rsi_addr_in6->sin6_addr._S6_un._S6_u32[3]);
		rsi_addr = (struct rsi_sockaddr *)rsi_addr_in6;
	}

	return rsi_addr;
}

static void translate_rsi_to_z_addr(struct rsi_sockaddr *rsi_addr,
				    rsi_socklen_t rsi_addrlen,
				    struct sockaddr *addr, socklen_t *addrlen)
{
	struct rsi_sockaddr_in *rsi_addr_in;
	struct rsi_sockaddr_in6 *rsi_addr_in6;

	if (rsi_addr->sa_family == AF_INET) {
		if (rsi_addrlen ==
		    (rsi_socklen_t)sizeof(struct rsi_sockaddr_in)) {
			struct sockaddr_in *z_sockaddr_in =
				(struct sockaddr_in *)addr;
			rsi_addr_in = (struct rsi_sockaddr_in *)rsi_addr;
			z_sockaddr_in->sin_family = Z_AF_INET;
			z_sockaddr_in->sin_port =
				sys_cpu_to_be16(rsi_addr_in->sin_port);
			z_sockaddr_in->sin_addr.s_addr =
				rsi_addr_in->sin_addr.s_addr;
			*addrlen = sizeof(struct sockaddr_in);
		} else {
			*addrlen = rsi_addrlen;
		}
	} else if (rsi_addr->sa_family == AF_INET6) {
		if (rsi_addrlen == sizeof(struct rsi_sockaddr_in6)) {
			struct sockaddr_in6 *z_sockaddr_in6 =
				(struct sockaddr_in6 *)addr;
			rsi_addr_in6 = (struct rsi_sockaddr_in6 *)rsi_addr;

			z_sockaddr_in6->sin6_family = Z_AF_INET6;
			z_sockaddr_in6->sin6_port =
				sys_cpu_to_be16(rsi_addr_in6->sin6_port);
			z_sockaddr_in6->sin6_scope_id =
				(uint8_t)rsi_addr_in6->sin6_scope_id;
			memcpy(z_sockaddr_in6->sin6_addr.s6_addr,
			       rsi_addr_in6->sin6_addr._S6_un._S6_u32,
			       sizeof(z_sockaddr_in6->sin6_addr.s6_addr));
			z_sockaddr_in6->sin6_addr.s6_addr32[0] =
				sys_cpu_to_be32(
					z_sockaddr_in6->sin6_addr.s6_addr32[0]);
			z_sockaddr_in6->sin6_addr.s6_addr32[1] =
				sys_cpu_to_be32(
					z_sockaddr_in6->sin6_addr.s6_addr32[1]);
			z_sockaddr_in6->sin6_addr.s6_addr32[2] =
				sys_cpu_to_be32(
					z_sockaddr_in6->sin6_addr.s6_addr32[2]);
			z_sockaddr_in6->sin6_addr.s6_addr32[3] =
				sys_cpu_to_be32(
					z_sockaddr_in6->sin6_addr.s6_addr32[3]);
			*addrlen = sizeof(struct sockaddr_in6);
		} else {
			*addrlen = rsi_addrlen;
		}
	}
}

static int rs9116w_accept(void *obj, struct sockaddr *addr, socklen_t *addrlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	struct rsi_sockaddr *rsi_addr;
	struct rsi_sockaddr_in rsi_addr_in;
	struct rsi_sockaddr_in6 rsi_addr_in6;
	rsi_socklen_t rsi_addrlen;

	if ((addrlen == NULL) || (addr == NULL)) {
		errno = EINVAL;
		return -1;
	}

	/* Translate between Zephyr's and WiSeConnects's sockaddr's: */
	rsi_addr = translate_z_to_rsi_addrlen(*addrlen, &rsi_addr_in,
					      &rsi_addr_in6, &rsi_addrlen);
	if (rsi_addr == NULL) {
		errno = EINVAL;
		return -1;
	}

	retval = rsi_accept(sd, rsi_addr, &rsi_addrlen);

	if (retval < 0) {
		errno = rsi_wlan_get_status();
		return -1;
	}

	/* Translate returned rsi_addr into *addr and set *addrlen: */
	translate_rsi_to_z_addr(rsi_addr, rsi_addrlen, addr, addrlen);

	return retval;
}

static int rs9116w_bind(void *obj, const struct sockaddr *addr,
			socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	struct rsi_sockaddr *rsi_addr;
	struct rsi_sockaddr_in rsi_addr_in;
	struct rsi_sockaddr_in6 rsi_addr_in6;

	LOG_DBG("SOCK BIND %d", sd);

	rsi_socklen_t rsi_addrlen;

	if (addr == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* Translate to rsi_bind() parameters: */
	rsi_addr = translate_z_to_rsi_addrs(addr, addrlen, &rsi_addr_in,
					    &rsi_addr_in6, &rsi_addrlen);

	if (rsi_addr == NULL) {
		errno = EINVAL;
		return -1;
	}
	retval = rsi_bind(sd, rsi_addr, rsi_addrlen);

	return retval;
}

static int rs9116w_listen(void *obj, int backlog)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	retval = (int)rsi_listen(sd, backlog);

	return retval;
}

static int rs9116w_connect(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	struct rsi_sockaddr *rsi_addr;
	struct rsi_sockaddr_in rsi_addr_in;
	struct rsi_sockaddr_in6 rsi_addr_in6;
	rsi_socklen_t rsi_addrlen;

	__ASSERT_NO_MSG(addr);

	/* Translate to rsi_connect() parameters: */
	rsi_addr = translate_z_to_rsi_addrs(addr, addrlen, &rsi_addr_in,
					    &rsi_addr_in6, &rsi_addrlen);

	if (rsi_addr == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (rsi_socket_pool[sd].sock_state == RSI_SOCKET_STATE_CONNECTED) {
		errno = EISCONN;
		return -1;
	}
	if (rsi_wlan_get_state() < RSI_WLAN_STATE_CONNECTED) {
		errno = ENETDOWN;
		return -1;
	}
	retval = rsi_connect(sd, rsi_addr, rsi_addrlen);
	return retval;
}

static const struct socket_op_vtable rs9116w_socket_fd_op_vtable;

static int rs9116w_poll(struct zsock_pollfd *fds, int nfds, int msecs)
{
	int max_sd = 0;
	struct rsi_timeval tv, *ptv;
	rsi_fd_set rfds; /* Set of read file descriptors */
	rsi_fd_set wfds; /* Set of write file descriptors */
	rsi_fd_set *prfds = NULL;
	rsi_fd_set *pwfds = NULL;
	int i, retval = 0, sd;
	void *obj;

	if (nfds > FD_SETSIZE) {
		errno = EINVAL;
		return -1;
	}

	/* Convert time to rsi_timeval struct values: */
	if (msecs == SYS_FOREVER_MS) {
		ptv = NULL;
	} else {
		tv.tv_sec = msecs / 1000;
		tv.tv_usec = (msecs % 1000) * 1000;
		ptv = &tv;
	}

	/* Setup read and write fds for select, based on pollfd fields: */
	RSI_FD_ZERO(&rfds);
	RSI_FD_ZERO(&wfds);

	for (i = 0; i < nfds; i++) {
		fds[i].revents = 0;
		if (fds[i].fd < 0) {
			continue;
		} else {
			obj = z_get_fd_obj(
				fds[i].fd,
				(const struct fd_op_vtable
					 *)&rs9116w_socket_fd_op_vtable,
				ENOTSUP);
			if (obj != NULL) {
				/* Offloaded socket found. */
				sd = OBJ_TO_SD(obj);
			} else {
				/* Non-offloaded socket, return an error. */
				errno = EINVAL;
				return -1;
			}
		}
		if (fds[i].events & ZSOCK_POLLIN) {
			RSI_FD_SET(sd, &rfds);
			prfds = &rfds;
		}
		if (fds[i].events & ZSOCK_POLLOUT) {
			RSI_FD_SET(sd, &wfds);
			pwfds = &wfds;
		}
		if (sd > max_sd) {
			max_sd = sd;
		}
	}

	/* Wait for requested read and write fds to be ready: */
	retval = rsi_select(max_sd + 1, prfds, pwfds, NULL, ptv, NULL);

	if (retval > 0) {
		for (i = 0; i < nfds; i++) {
			if (fds[i].fd >= 0) {
				obj = z_get_fd_obj(
					fds[i].fd,
					(const struct fd_op_vtable
						 *)&rs9116w_socket_fd_op_vtable,
					ENOTSUP);
				sd = OBJ_TO_SD(obj);
				if (RSI_FD_ISSET(sd, &rfds)) {
					fds[i].revents |= ZSOCK_POLLIN;
				}
				if (RSI_FD_ISSET(sd, &wfds)) {
					fds[i].revents |= ZSOCK_POLLOUT;
				}
			}
		}
	}

	return retval;
}

#if IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) &&                              \
	IS_ENABLED(CONFIG_RS9116W_TLS_OFFLOAD)
#include <zephyr/sys/base64.h>
uint8_t pem[CONFIG_RS9116W_TLS_PEM_BUF_SZ];

static int map_credentials(int sd, const void *optval, socklen_t optlen)
{
	sec_tag_t *sec_tags = (sec_tag_t *)optval;
	int retval = 0;
	int sec_tags_len;
	sec_tag_t tag;
	int cert_type;
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
		uint8_t cert_idx;
		int offset;
		char *header, *footer;

		while (cert != NULL) {
			/* Map Zephyr cert types to WiSeConnect cert types: */
			switch (cert->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				cert_type = RSI_SSL_CA_CERTIFICATE;
				header = "-----BEGIN CERTIFICATE-----\n";
				footer = "\n-----END CERTIFICATE-----\n";
				cert_idx = 0;
				break;
			case TLS_CREDENTIAL_SERVER_CERTIFICATE:
				/* Todo: set both client and server */
				cert_type = RSI_SSL_CLIENT;
				cert_idx = 0;
				header = "-----BEGIN CERTIFICATE-----\n";
				footer = "\n-----END CERTIFICATE-----\n";
				break;
			case TLS_CREDENTIAL_PRIVATE_KEY:
				/* Todo: set both client and server */
				cert_type = RSI_SSL_CLIENT_PRIVATE_KEY;
				cert_idx = 0;
				header = "-----BEGIN RSA PRIVATE KEY-----\n";
				footer = "\n-----END RSA PRIVATE KEY-----\n";
				break;
			case TLS_CREDENTIAL_NONE:
			case TLS_CREDENTIAL_PSK:
			case TLS_CREDENTIAL_PSK_ID:
			default:
				retval = -EINVAL;
				goto exit;
			}
			uint32_t ce_val = cert_idx;

			memset(pem, 0, sizeof(pem));
			strcpy(pem, header);
			offset = strlen(header);
			size_t written;

			base64_encode(pem + offset,
				      CONFIG_RS9116W_TLS_PEM_BUF_SZ - offset -
					      strlen(footer),
				      &written, cert->buf, cert->len);
			memcpy(pem + offset + written, footer, strlen(footer));
			retval = rsi_wlan_set_certificate_index(
				cert_type, cert_idx, pem, strlen(pem));
			if (retval < 0) {
				break;
			}
			retval = rsi_setsockopt(sd, SOL_SOCKET, SO_CERT_INDEX,
						&ce_val, sizeof(ce_val));
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
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

/* This was borrowed from simplelink, undoubtedly some sockopts aren't defined
 */
#define Z_SO_BROADCAST (200)
#define Z_SO_SNDBUF    (202)

static int rs9116w_setsockopt(void *obj, int level, int optname,
			      const void *optval, socklen_t optlen)
{
	/* Unsure if all the sockopts are actually supported; Documentation is
	 * unclear
	 */
	int sd = OBJ_TO_SD(obj);
	int retval;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		/* Handle Zephyr's SOL_TLS secure socket options: */
		switch (optname) {
		case TLS_SEC_TAG_LIST:
			/* Bind credential filenames to this socket: */
			retval = map_credentials(sd, optval, optlen);
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
					errno = ENOTSUP;
					return -1;
				}
				retval = 0;
			} else {
				errno = EINVAL;
				return -1;
			}
			break;
		case TLS_HOSTNAME:
			return rsi_setsockopt(sd, level, SO_TLS_SNI, optval,
					      (rsi_socklen_t)optlen);
		case TLS_CIPHERSUITE_LIST: /* Todo: deal with ciphersuites */
		case TLS_DTLS_ROLE:
			errno = ENOTSUP;
			return -1;
		default:
			errno = EINVAL;
			return -1;
		}
	} else {
		switch (optname) {
			/* These sockopts do not map to the same values, but are
			 * still supported in WiSeConnect
			 */
		case Z_SO_BROADCAST:
			return rsi_setsockopt(sd, level, SO_BROADCAST, optval,
					      (rsi_socklen_t)optlen);
		case Z_SO_REUSEADDR:
			return rsi_setsockopt(sd, level, SO_REUSEADDR, optval,
					      (rsi_socklen_t)optlen);
		case Z_SO_SNDBUF:
			return rsi_setsockopt(sd, level, SO_SNDBUF, optval,
					      (rsi_socklen_t)optlen);
		case Z_IPV6_V6ONLY:
			errno = EINVAL;
			return -1;
		case SO_BINDTODEVICE:
			return 0;
		default:
			break;
		}
		return rsi_setsockopt(sd, SOL_SOCKET, optname, optval,
				      (rsi_socklen_t)optlen);
	}

	return retval;
}

static int rs9116w_getsockopt(void *obj, int level, int optname, void *optval,
			      socklen_t *optlen)
{
	/* Unsure if all the sockopts are actuall supported; Documentation is
	 * unclear
	 */
	int sd = OBJ_TO_SD(obj);

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		/* Handle Zephyr's SOL_TLS secure socket options: */
		switch (optname) {
		case TLS_SEC_TAG_LIST:
		case TLS_CIPHERSUITE_LIST:
		case TLS_CIPHERSUITE_USED:
			/* Not yet supported: */
			errno = ENOTSUP;
			return -1;
		default:
			errno = EINVAL;
			return -1;
		}
	} else {
		/* Can be SOL_SOCKET or TI specific: */
		switch (optname) {
			/* TCP_NODELAY always set by the NWP, so return True */
		case Z_SO_BROADCAST:
			return rsi_getsockopt(sd, SOL_SOCKET, SO_BROADCAST,
					      optval, *(rsi_socklen_t *)optlen);
		case Z_SO_REUSEADDR:
			return rsi_getsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
					      optval, *(rsi_socklen_t *)optlen);
		case Z_SO_SNDBUF:
			return rsi_getsockopt(sd, SOL_SOCKET, SO_SNDBUF, optval,
					      *(rsi_socklen_t *)optlen);
		case Z_IPV6_V6ONLY:
			errno = EINVAL;
			return -1;
		default:
			break;
		}
		/* Optlen is actually unused? */
		return rsi_getsockopt(sd, SOL_SOCKET, optname, optval,
				      *(rsi_socklen_t *)optlen);
	}
}

static ssize_t rs9116w_recvfrom(void *obj, void *buf, size_t len, int flags,
				struct sockaddr *from, socklen_t *fromlen)
{
	int sd = OBJ_TO_SD(obj);
	ssize_t retval;
	struct rsi_sockaddr *rsi_addr = NULL;
	struct rsi_sockaddr_in rsi_addr_in;
	struct rsi_sockaddr_in6 rsi_addr_in6;
	rsi_socklen_t rsi_addrlen;

	len = MIN(len, MAX_PAYLOAD_SIZE);

	if (len == 0) {
		errno = EINVAL;
		return -1;
	}

	/* 1k to account for TLS overhead */
	size_t per_rcv = MAX_PAYLOAD_SIZE;

	if (rsi_socket_pool[sd].sock_bitmap & RSI_SOCKET_FEAT_SSL) {
		per_rcv -= RSI_SSL_HEADER_SIZE;
	}

	bool is_udp = (rsi_socket_pool[sd].sock_type & 0xF) == SOCK_DGRAM;
	bool waitall = false;

	if (flags & ~ZSOCK_MSG_DONTWAIT & ~ZSOCK_MSG_WAITALL) {
		errno = ENOTSUP;
		return -1;
	}

	if (atomic_test_bit(closed_socks_map, sd)) {
		return 0;
	}

	if (atomic_test_bit(nb_socks_map, sd)) {
		flags |= ZSOCK_MSG_DONTWAIT;
	}

	/*
	 * Non-blocking is only able to be set on socket creation
	 * (Also doesn't affect receive anyways)
	 * Therefore, this is the simplest solution...
	 */
	if (flags & ZSOCK_MSG_DONTWAIT) {
		if (rsi_socket_pool[sd].sock_state < RSI_SOCKET_STATE_BIND) {
			errno = EAGAIN;
			return -1;
		}
		struct rsi_timeval ptv;
		rsi_fd_set rfds;

		ptv.tv_sec = 0;
		ptv.tv_usec = 0;
		RSI_FD_ZERO(&rfds);
		RSI_FD_SET(sd, &rfds);
		rsi_select(sd + 1, &rfds, NULL, NULL, &ptv, NULL);
		if (!RSI_FD_ISSET(sd, &rfds)) {
			errno = EAGAIN;
			return -1;
		}
	} else if (flags & ZSOCK_MSG_WAITALL) {
		waitall = true;
	}

	size_t vlen = MIN(len, per_rcv);

	/* Translate to rsi_recvfrom() parameters: */
	if (fromlen != NULL) {
		rsi_addr = translate_z_to_rsi_addrlen(
			*fromlen, &rsi_addr_in, &rsi_addr_in6, &rsi_addrlen);
		retval = (ssize_t)rsi_recvfrom(sd, buf, vlen, 0, rsi_addr,
					       &rsi_addrlen);
	} else {
		retval = (ssize_t)rsi_recv(sd, buf, vlen, 0);
	}

	/*
	 * Larger receives are a bit wonky, also occasionally the receive will
	 * not get all available data, so to account for both, receive is tried
	 * until there is no longer any data available.
	 */
	if (retval && len != retval && !is_udp && retval != -1) {
		size_t remaining_len = len - retval;
		size_t offset;
		ssize_t rv = 0;

		while (remaining_len && (rv >= 0 || waitall)) {
			offset = len - remaining_len;
			rv = rs9116w_recvfrom(obj, (uint8_t *)buf + retval,
					      MIN(per_rcv, remaining_len),
					      waitall ? 0 : ZSOCK_MSG_DONTWAIT,
					      from, fromlen);
			if (rv == -1 && errno == EAGAIN && !waitall) {
				errno = 0;
				break;
			} else if (rv == -1) {
				return -1;
			} else if (rv == 0) {
				return retval;
			}
			retval += rv;
			remaining_len -= rv;
		}
	}

	if (retval >= 0) {
		if (fromlen != NULL) {
			/* Translate rsi_addr into *addr and set addrlen */
			translate_rsi_to_z_addr(rsi_addr, rsi_addrlen, from,
						fromlen);
		}
	}

	if (!retval) {
		atomic_set_bit(closed_socks_map, sd);
	}

	return retval;
}

static ssize_t rs9116w_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int sd = OBJ_TO_SD(obj);
	ssize_t retval = 0;
	struct rsi_sockaddr *rsi_addr;
	struct rsi_sockaddr_in rsi_addr_in;
	struct rsi_sockaddr_in6 rsi_addr_in6;
	rsi_socklen_t rsi_addrlen;

	if (!buf || !len) {
		errno = EINVAL;
		return -1;
	}

	if (to != NULL) {
		/* Translate to rsi_sendto() parameters: */
		rsi_addr = translate_z_to_rsi_addrs(
			to, tolen, &rsi_addr_in, &rsi_addr_in6, &rsi_addrlen);

		if (rsi_addr == NULL) {
			errno = EINVAL;
			return -1;
		}
	}
	if (to != NULL) {
		retval = rsi_sendto(sd, (int8_t *)buf, len, flags, rsi_addr,
				    rsi_addrlen);
	} else {
		if (rsi_socket_pool[sd].sock_state != RSI_SOCKET_STATE_CONNECTED) {
			errno = ENOTCONN;
			return -1;
		}
		retval = rsi_send(sd, (int8_t *)buf, len, flags);
	}

	return retval;
}

static ssize_t rs9116w_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	ssize_t sent = 0;
	int rc;

	LOG_DBG("msg_iovlen:%zd flags:%d", msg->msg_iovlen, flags);

	for (int i = 0; i < msg->msg_iovlen; i++) {
		const char *buf = msg->msg_iov[i].iov_base;
		size_t len = msg->msg_iov[i].iov_len;

		while (len > 0) {
			rc = rs9116w_sendto(obj, buf, len, flags, msg->msg_name,
					    msg->msg_namelen);
			if (rc < 0) {
				sent = rc;
				break;
			}
			sent += rc;
			buf += rc;
			len -= rc;
		}
	}

	return (ssize_t)sent;
}

static int rs9116w_fnctl(int sd, int cmd, va_list args)
{
	switch (cmd) {
	case F_GETFL:
		return atomic_test_bit(nb_socks_map, sd) ? O_NONBLOCK : 0;
	case F_SETFL:
		if ((va_arg(args, int) & O_NONBLOCK) != 0) {
			atomic_set_bit(nb_socks_map, sd);
			return 0;
		}
		errno = EINVAL;
		return -1;
	default:
		LOG_ERR("Invalid command: %d", cmd);
		errno = EINVAL;
		return -1;
	}
}

static int rs9116w_ioctl(void *obj, unsigned int request, va_list args)
{
	ARG_UNUSED(obj);

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

		return rs9116w_poll(fds, nfds, timeout);
	}

	case ZFD_IOCTL_SET_LOCK: {
		return -EOPNOTSUPP;
	}

	default: {
		int sd = OBJ_TO_SD(obj);

		return rs9116w_fnctl(sd, request, args);
	}
	}
}

static ssize_t rs9116w_read(void *obj, void *buffer, size_t count)
{
	return rs9116w_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t rs9116w_write(void *obj, const void *buffer, size_t count)
{
	return rs9116w_sendto(obj, buffer, count, 0, NULL, 0);
}

static const struct socket_op_vtable rs9116w_socket_fd_op_vtable = {
	.fd_vtable = {
			.read = rs9116w_read,
			.write = rs9116w_write,
			.close = rs9116w_close,
			.ioctl = rs9116w_ioctl,
		},
	.bind = rs9116w_bind,
	.connect = rs9116w_connect,
	.listen = rs9116w_listen,
	.accept = rs9116w_accept,
	.sendto = rs9116w_sendto,
	.sendmsg = rs9116w_sendmsg,
	.recvfrom = rs9116w_recvfrom,
	.getsockopt = rs9116w_getsockopt,
	.setsockopt = rs9116w_setsockopt,
};

static bool rs9116w_is_supported(int family, int type, int proto)
{
	/* TODO offloading always enabled for now. */
	return true;
}

int rs9116w_socket_create(int family, int type, int proto)
{
	int fd = z_reserve_fd();
	int sock;

	if (fd < 0) {
		return -1;
	}

	sock = rs9116w_socket(family, type, proto);

	if (sock >= 0) {
		atomic_clear_bit(closed_socks_map, sock);
	}

	LOG_DBG("rs9116w_socket returned %d", sock);
	if (sock < 0) {
		z_free_fd(fd);
		return -1;
	}

	z_finalize_fd(
		fd, SD_TO_OBJ(sock),
		(const struct fd_op_vtable *)&rs9116w_socket_fd_op_vtable);

	return fd;
}

#if IS_ENABLED(CONFIG_RS9116W_OVERRIDE_DEFAULT_PRIO)
#define NET_SOCKET_RS9116_PRIO CONFIG_RS9116W_SOCKET_PRIO
#else
#define NET_SOCKET_RS9116_PRIO NET_SOCKET_DEFAULT_PRIO
#endif

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
NET_SOCKET_REGISTER(rs9116w, NET_SOCKET_RS9116_PRIO, AF_UNSPEC,
		    rs9116w_is_supported, rs9116w_socket_create);
#endif

#include <rsi_nwk.h>
#include <rsi_utils.h>

#if IS_ENABLED(CONFIG_RS9116_DNS_SERVER_IP_ADDRESSES)
char *dns_servers[] = {
	CONFIG_RS9116_IPV4_DNS_SERVER1,
	CONFIG_RS9116_IPV4_DNS_SERVER2,
	CONFIG_RS9116_IPV6_DNS_SERVER1,
	CONFIG_RS9116_IPV6_DNS_SERVER2,
};
#endif

#undef s6_addr32
/* Adds address info entry to a list */
static int set_addr_info(uint8_t *addr, bool ipv6, uint8_t socktype,
			 uint16_t port, struct zsock_addrinfo **res)
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
	ai->ai_family = (ipv6 ? Z_AF_INET6 : Z_AF_INET);
	ai->ai_socktype = socktype;
	ai->ai_protocol =
		ai->ai_socktype == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP;

	/* Fill sockaddr struct fields based on family: */
	if (ai->ai_family == Z_AF_INET) {
		net_sin(ai_addr)->sin_family = ai->ai_family;
		net_sin(ai_addr)->sin_addr.s_addr = rsi_bytes4R_to_uint32(addr);
		net_sin(ai_addr)->sin_port = htons(port);
		ai->ai_addrlen = sizeof(struct sockaddr_in);
	} else {

		net_sin6(ai_addr)->sin6_family = ai->ai_family;
		net_sin6(ai_addr)->sin6_addr.s6_addr32[0] =
			sys_cpu_to_be32(rsi_bytes4R_to_uint32(&addr[0]));
		net_sin6(ai_addr)->sin6_addr.s6_addr32[1] =
			sys_cpu_to_be32(rsi_bytes4R_to_uint32(&addr[4]));
		net_sin6(ai_addr)->sin6_addr.s6_addr32[2] =
			sys_cpu_to_be32(rsi_bytes4R_to_uint32(&addr[8]));
		net_sin6(ai_addr)->sin6_addr.s6_addr32[3] =
			sys_cpu_to_be32(rsi_bytes4R_to_uint32(&addr[12]));
		net_sin6(ai_addr)->sin6_port = htons(port);
		ai->ai_addrlen = sizeof(struct sockaddr_in6);
	}
	ai->ai_addr = ai_addr;
	ai->ai_next = *res;
	*res = ai;

exit:
	return retval;
}

static void rs9116w_freeaddrinfo(struct zsock_addrinfo *res)
{
	__ASSERT_NO_MSG(res);
	free(res->ai_addr);
	struct zsock_addrinfo *next = res->ai_next, *tmp = NULL;

	while (next) {
		tmp = next;
		next = next->ai_next;
		free(tmp->ai_addr);
		free(tmp);
	}
	free(res);
}

static int rs9116w_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints,
			       struct zsock_addrinfo **res)
{
	int32_t retval = -1, retval4 = -1, retval6 = -1;
#if IS_ENABLED(CONFIG_NET_IPV4)
	rsi_rsp_dns_query_t qr4;
#endif
#if IS_ENABLED(CONFIG_NET_IPV6)
	rsi_rsp_dns_query_t qr6;
#endif
	uint32_t port = 0;
	uint8_t type = SOCK_STREAM;

	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}
#if IS_ENABLED(CONFIG_NET_IPV4)
	memset(&qr4, 0, sizeof(qr4));
#endif
#if IS_ENABLED(CONFIG_NET_IPV6)
	memset(&qr6, 0, sizeof(qr6));
#endif

	/* Check args: */
	if (!res) {
		retval = DNS_EAI_NONAME;
		goto exit;
	}

	bool v4 = true, v6 = true;

	if (hints) {
		if (hints->ai_family == Z_AF_INET) {
			v6 = false;
		} else if (hints->ai_family == Z_AF_INET6) {
			v4 = false;
		}
		type = hints->ai_socktype;
	}

#if IS_ENABLED(CONFIG_NET_IPV4)
	uint8_t *dns4_1 = NULL, *dns4_2 = NULL;
#endif
#if IS_ENABLED(CONFIG_NET_IPV6)
	uint8_t *dns6_1 = NULL, *dns6_2 = NULL;
#endif

#if IS_ENABLED(CONFIG_RS9116_DNS_SERVER_IP_ADDRESSES)
	struct sockaddr v4_addresses[2];
	struct sockaddr v6_addresses[2];
	int a4i = 0, a6i = 0;
#if IS_ENABLED(CONFIG_NET_IPV4)
	for (int i = 0; i < 4; i++) {
		if (strcmp(dns_servers[i], "")) {
			char *server = dns_servers[i];
			struct sockaddr addr;

			if (net_ipaddr_parse(server, strlen(server), &addr)) {
				if (addr.sa_family == Z_AF_INET && a4i < 2) {
					memcpy(&v4_addresses[a4i], &addr,
					       sizeof(addr));
					a4i++;
				} else if (addr.sa_family == Z_AF_INET6 &&
					   a6i < 2) {
					memcpy(&v6_addresses[a6i], &addr,
					       sizeof(addr));
					a6i++;
				}
			}
		}
	}
	if (a4i > 0) {
		dns4_1 = net_sin(&v4_addresses[0])->sin_addr.s4_addr;
		if (a4i > 1) {
			dns4_2 = net_sin(&v4_addresses[1])->sin_addr.s4_addr;
		}
	}
#endif
#if IS_ENABLED(CONFIG_NET_IPV6)
	if (a6i > 0) {
		dns6_1 = net_sin6(&v6_addresses[0])->sin6_addr.s6_addr;
		((uint32_t *)dns6_1)[0] =
			sys_be32_to_cpu(((uint32_t *)dns6_1)[0]);
		((uint32_t *)dns6_1)[1] =
			sys_be32_to_cpu(((uint32_t *)dns6_1)[1]);
		((uint32_t *)dns6_1)[2] =
			sys_be32_to_cpu(((uint32_t *)dns6_1)[2]);
		((uint32_t *)dns6_1)[3] =
			sys_be32_to_cpu(((uint32_t *)dns6_1)[3]);
		if (a6i > 1) {
			dns6_2 = net_sin6(&v6_addresses[1])->sin6_addr.s6_addr;
			((uint32_t *)dns6_2)[0] =
				sys_be32_to_cpu(((uint32_t *)dns6_2)[0]);
			((uint32_t *)dns6_2)[1] =
				sys_be32_to_cpu(((uint32_t *)dns6_2)[1]);
			((uint32_t *)dns6_2)[2] =
				sys_be32_to_cpu(((uint32_t *)dns6_2)[2]);
			((uint32_t *)dns6_2)[3] =
				sys_be32_to_cpu(((uint32_t *)dns6_2)[3]);
		}
	}
#endif
#endif

#if IS_ENABLED(CONFIG_NET_IPV4)
	if (v4 && s_rs9116w_dev[0].has_ipv4) {
		retval4 = rsi_dns_req(4, (uint8_t *)node, dns4_1, dns4_2, &qr4,
				      sizeof(qr4));
	}
#endif

#if IS_ENABLED(CONFIG_NET_IPV6)
	if (v6 && s_rs9116w_dev[0].has_ipv6) {
		retval6 = rsi_dns_req(6, (uint8_t *)node, dns6_1, dns6_2, &qr6,
				      sizeof(qr6));
	}
#endif

	if (retval4 != 0 && retval6 != 0) {
		LOG_ERR("Could not resolve name: %s, retval: %d", node, retval);
		retval = DNS_EAI_NONAME;
		goto exit;
	}

	*res = NULL;
#if IS_ENABLED(CONFIG_NET_IPV4)
	for (int i = 0; i < rsi_bytes2R_to_uint16(qr4.ip_count); i++) {
		retval = set_addr_info(qr4.ip_address[i].ipv4_address, false,
				       type, (uint16_t)port, res);
		if (retval < 0) {
			rs9116w_freeaddrinfo(*res);
			LOG_ERR("Unable to set address info, retval: %d",
				retval);
			goto exit;
		}
	}
#endif
#if IS_ENABLED(CONFIG_NET_IPV6)
	for (int i = 0; i < rsi_bytes2R_to_uint16(qr6.ip_count); i++) {
		retval = set_addr_info(qr6.ip_address[i].ipv6_address, true,
				       type, (uint16_t)port, res);
		if (retval < 0) {
			rs9116w_freeaddrinfo(*res);
			LOG_ERR("Unable to set address info, retval: %d",
				retval);
			goto exit;
		}
	}
#endif
exit:
	return retval;
}

const struct socket_dns_offload rs9116w_dns_ops = {
	.getaddrinfo = rs9116w_getaddrinfo,
	.freeaddrinfo = rs9116w_freeaddrinfo,
};

int rs9116w_socket_offload_init(void)
{
	socket_offload_dns_register(&rs9116w_dns_ops);
	return 0;
}
