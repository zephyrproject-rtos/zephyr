/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/net/net_if.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/net/if.h>
#include <zephyr/posix/sys/socket.h>

/* From arpa/inet.h */

in_addr_t inet_addr(const char *cp)
{
	int val = 0;
	int len = 0;
	int dots = 0;
	int digits = 0;

	/* error checking */
	if (cp == NULL) {
		return -1;
	}

	for (int i = 0, subdigits = 0; i <= INET_ADDRSTRLEN; ++i, ++len) {
		if (subdigits > 3) {
			return -1;
		}
		if (cp[i] == '\0') {
			break;
		} else if (cp[i] == '.') {
			if (subdigits == 0) {
				return -1;
			}
			++dots;
			subdigits = 0;
			continue;
		} else if (isdigit((int)cp[i])) {
			++digits;
			++subdigits;
			continue;
		} else if (isspace((int)cp[i])) {
			break;
		}

		return -1;
	}

	if (dots != 3 || digits < 4) {
		return -1;
	}

	/* conversion */
	for (int i = 0, tmp = 0; i < len; ++i, ++cp) {
		if (*cp != '.') {
			tmp *= 10;
			tmp += *cp - '0';
		}

		if (*cp == '.' || i == len - 1) {
			val <<= 8;
			val |= tmp;
			tmp = 0;
		}
	}

	return htonl(val);
}

char *inet_ntoa(struct in_addr in)
{
	static char buf[INET_ADDRSTRLEN];
	unsigned char *bytes = (unsigned char *)&in.s_addr;

	snprintf(buf, sizeof(buf), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);

	return buf;
}

char *inet_ntop(sa_family_t family, const void *src, char *dst, size_t size)
{
	return zsock_inet_ntop(family, src, dst, size);
}

int inet_pton(sa_family_t family, const char *src, void *dst)
{
	return zsock_inet_pton(family, src, dst);
}

/* From net/if.h */

char *if_indextoname(unsigned int ifindex, char *ifname)
{
	int ret;

	ret = net_if_get_name(net_if_get_by_index(ifindex), ifname, IF_NAMESIZE);
	if (ret < 0) {
		errno = ENXIO;
		return NULL;
	}

	return ifname;
}

void if_freenameindex(struct if_nameindex *ptr)
{
	size_t n;

	if (ptr == NULL) {
		return;
	}

	NET_IFACE_COUNT(&n);

	for (size_t i = 0; i < n; ++i) {
		if (ptr[i].if_name != NULL) {
			free(ptr[i].if_name);
		}
	}

	free(ptr);
}

struct if_nameindex *if_nameindex(void)
{
	size_t n;
	char *name;
	struct if_nameindex *ni;

	/* FIXME: would be nice to use this without malloc */
	NET_IFACE_COUNT(&n);
	ni = malloc((n + 1) * sizeof(*ni));
	if (ni == NULL) {
		goto return_err;
	}

	for (size_t i = 0; i < n; ++i) {
		ni[i].if_index = i + 1;

		ni[i].if_name = malloc(IF_NAMESIZE);
		if (ni[i].if_name == NULL) {
			goto return_err;
		}

		name = if_indextoname(i + 1, ni[i].if_name);
		__ASSERT_NO_MSG(name != NULL);
	}

	ni[n].if_index = 0;
	ni[n].if_name = NULL;

	return ni;

return_err:
	if_freenameindex(ni);
	errno = ENOBUFS;

	return NULL;
}

unsigned int if_nametoindex(const char *ifname)
{
	int ret;

	ret = net_if_get_by_name(ifname);
	if (ret < 0) {
		return 0;
	}

	return ret;
}

/* From netdb.h */

void endhostent(void)
{
}

void endnetent(void)
{
}

void endprotoent(void)
{
}

void endservent(void)
{
}

void freeaddrinfo(struct zsock_addrinfo *ai)
{
	zsock_freeaddrinfo(ai);
}

const char *gai_strerror(int errcode)
{
	return zsock_gai_strerror(errcode);
}

int getaddrinfo(const char *host, const char *service, const struct zsock_addrinfo *hints,
		struct zsock_addrinfo **res)
{
	return zsock_getaddrinfo(host, service, hints, res);
}

struct hostent *gethostent(void)
{
	return NULL;
}

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags)
{
	return zsock_getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
}

struct netent *getnetbyaddr(uint32_t net, int type)
{
	ARG_UNUSED(net);
	ARG_UNUSED(type);

	return NULL;
}

struct netent *getnetbyname(const char *name)
{
	ARG_UNUSED(name);

	return NULL;
}

struct netent *getnetent(void)
{
	return NULL;
}

int getpeername(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_getpeername(sock, addr, addrlen);
}

struct protoent *getprotobyname(const char *name)
{
	ARG_UNUSED(name);

	return NULL;
}

struct protoent *getprotobynumber(int proto)
{
	ARG_UNUSED(proto);

	return NULL;
}

struct protoent *getprotoent(void)
{
	return NULL;
}

struct servent *getservbyname(const char *name, const char *proto)
{
	ARG_UNUSED(name);
	ARG_UNUSED(proto);

	return NULL;
}

struct servent *getservbyport(int port, const char *proto)
{
	ARG_UNUSED(port);
	ARG_UNUSED(proto);

	return NULL;
}

struct servent *getservent(void)
{
	return NULL;
}

void sethostent(int stayopen)
{
	ARG_UNUSED(stayopen);
}

void setnetent(int stayopen)
{
	ARG_UNUSED(stayopen);
}

void setprotoent(int stayopen)
{
	ARG_UNUSED(stayopen);
}

void setservent(int stayopen)
{
	ARG_UNUSED(stayopen);
}

/* From sys/socket.h */

int accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_accept(sock, addr, addrlen);
}

int bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_bind(sock, addr, addrlen);
}

int connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

int getsockname(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_getsockname(sock, addr, addrlen);
}

int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen)
{
	return zsock_getsockopt(sock, level, optname, optval, optlen);
}

int listen(int sock, int backlog)
{
	return zsock_listen(sock, backlog);
}

ssize_t recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recv(sock, buf, max_len, flags);
}

ssize_t recvfrom(int sock, void *buf, size_t max_len, int flags, struct sockaddr *src_addr,
		 socklen_t *addrlen)
{
	return zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sock, struct msghdr *msg, int flags)
{
	return zsock_recvmsg(sock, msg, flags);
}

ssize_t send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_send(sock, buf, len, flags);
}

ssize_t sendmsg(int sock, const struct msghdr *message, int flags)
{
	return zsock_sendmsg(sock, message, flags);
}

ssize_t sendto(int sock, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr,
	       socklen_t addrlen)
{
	return zsock_sendto(sock, buf, len, flags, dest_addr, addrlen);
}

int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen)
{
	return zsock_setsockopt(sock, level, optname, optval, optlen);
}

int shutdown(int sock, int how)
{
	return zsock_shutdown(sock, how);
}

int sockatmark(int s)
{
	ARG_UNUSED(s);

	errno = ENOSYS;
	return -1;
}

int socket(int family, int type, int proto)
{
	return zsock_socket(family, type, proto);
}

int socketpair(int family, int type, int proto, int sv[2])
{
	return zsock_socketpair(family, type, proto, sv);
}
