/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NETDB_H_
#define ZEPHYR_INCLUDE_POSIX_NETDB_H_

#include <zephyr/net/socket.h>

#ifndef NI_MAXSERV
/** Provide a reasonable size for apps using getnameinfo */
#define NI_MAXSERV 32
#endif

#define EAI_BADFLAGS DNS_EAI_BADFLAGS
#define EAI_NONAME DNS_EAI_NONAME
#define EAI_AGAIN DNS_EAI_AGAIN
#define EAI_FAIL DNS_EAI_FAIL
#define EAI_NODATA DNS_EAI_NODATA
#define EAI_MEMORY DNS_EAI_MEMORY
#define EAI_SYSTEM DNS_EAI_SYSTEM
#define EAI_SERVICE DNS_EAI_SERVICE
#define EAI_SOCKTYPE DNS_EAI_SOCKTYPE
#define EAI_FAMILY DNS_EAI_FAMILY
#define EAI_OVERFLOW DNS_EAI_OVERFLOW

#ifdef __cplusplus
extern "C" {
#endif

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};

struct netent {
	char *n_name;
	char **n_aliases;
	int n_addrtype;
	uint32_t n_net;
};

struct protoent {
	char *p_name;
	char **p_aliases;
	int p_proto;
};

struct servent {
	char *s_name;
	char **s_aliases;
	int s_port;
	char *s_proto;
};

#define addrinfo zsock_addrinfo

void endhostent(void);
void endnetent(void);
void endprotoent(void);
void endservent(void);
void freeaddrinfo(struct zsock_addrinfo *ai);
const char *gai_strerror(int errcode);
int getaddrinfo(const char *host, const char *service, const struct zsock_addrinfo *hints,
		struct zsock_addrinfo **res);
struct hostent *gethostent(void);
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags);
struct netent *getnetbyaddr(uint32_t net, int type);
struct netent *getnetbyname(const char *name);
struct netent *getnetent(void);
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int proto);
struct protoent *getprotoent(void);
struct servent *getservbyname(const char *name, const char *proto);
struct servent *getservbyport(int port, const char *proto);
struct servent *getservent(void);
void sethostent(int stayopen);
void setnetent(int stayopen);
void setprotoent(int stayopen);
void setservent(int stayopen);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_NETDB_H_ */
