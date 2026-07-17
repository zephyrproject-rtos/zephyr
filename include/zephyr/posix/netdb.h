/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Definitions for network database operations.
 * @ingroup posix
 *
 * Provides hostname and service resolution (getaddrinfo() and getnameinfo())
 * and the legacy host, network, protocol, and service database functions.
 *
 * @posix_header{netdb.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_NETDB_H_
#define ZEPHYR_INCLUDE_POSIX_NETDB_H_

#include <zephyr/net/socket.h>

#ifndef NI_MAXSERV
/** Provide a reasonable size for apps using getnameinfo */
/**
 * @brief Reasonable buffer size for getnameinfo() service names.
 */
#define NI_MAXSERV 32
#endif

/**
 * @brief The flags argument to getaddrinfo() contained an invalid value.
 */
#define EAI_BADFLAGS DNS_EAI_BADFLAGS

/**
 * @brief The name does not resolve for the supplied parameters.
 */
#define EAI_NONAME DNS_EAI_NONAME

/**
 * @brief The name could not be resolved at this time.
 */
#define EAI_AGAIN DNS_EAI_AGAIN

/**
 * @brief A non-recoverable error occurred.
 */
#define EAI_FAIL DNS_EAI_FAIL

/**
 * @brief No address is associated with the name.
 */
#define EAI_NODATA DNS_EAI_NODATA

/**
 * @brief There was a memory allocation failure.
 */
#define EAI_MEMORY DNS_EAI_MEMORY

/**
 * @brief A system error occurred; the error code can be found in errno.
 */
#define EAI_SYSTEM DNS_EAI_SYSTEM

/**
 * @brief The service passed was not recognized for the specified socket type.
 */
#define EAI_SERVICE DNS_EAI_SERVICE

/**
 * @brief The intended socket type was not recognized.
 */
#define EAI_SOCKTYPE DNS_EAI_SOCKTYPE

/**
 * @brief The address family was not recognized or the address length was invalid for the
 * specified family.
 */
#define EAI_FAMILY DNS_EAI_FAMILY

/**
 * @brief An argument buffer overflowed.
 */
#define EAI_OVERFLOW DNS_EAI_OVERFLOW

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Host database entry.
 */
struct hostent {
	/**
	 * @brief Official name of the host.
	 */
	char *h_name;
	/**
	 * @brief A pointer to an array of pointers to alternative host names.
	 */
	char **h_aliases;
	/**
	 * @brief Address type.
	 */
	int h_addrtype;
	/**
	 * @brief The length, in bytes, of the address.
	 */
	int h_length;
	/**
	 * @brief A pointer to an array of pointers to network addresses.
	 */
	char **h_addr_list;
};

/**
 * @brief Network database entry.
 */
struct netent {
	/**
	 * @brief Official, fully-qualified name of the host.
	 */
	char *n_name;
	/**
	 * @brief A pointer to an array of pointers to alternative network names.
	 */
	char **n_aliases;
	/**
	 * @brief The address type of the network.
	 */
	int n_addrtype;
	/**
	 * @brief The network number, in host byte order.
	 */
	uint32_t n_net;
};

/**
 * @brief Protocol database entry.
 */
struct protoent {
	/**
	 * @brief Official name of the protocol.
	 */
	char *p_name;
	/**
	 * @brief A pointer to an array of pointers to alternative protocol names.
	 */
	char **p_aliases;
	/**
	 * @brief The protocol number.
	 */
	int p_proto;
};

/**
 * @brief Service database entry.
 */
struct servent {
	/**
	 * @brief Official name of the service.
	 */
	char *s_name;
	/**
	 * @brief A pointer to an array of pointers to alternative service names.
	 */
	char **s_aliases;
	/**
	 * @brief A value that yields the port number in network byte order when converted to
	 * uint16_t.
	 */
	int s_port;
	/**
	 * @brief The name of the protocol to use when contacting the service.
	 */
	char *s_proto;
};

/**
 * @brief Address information structure.
 */
#define addrinfo zsock_addrinfo

/**
 * @brief Close the hosts database (no-op on Zephyr).
 *
 * @posix_func{endhostent}
 */
void endhostent(void);

/**
 * @brief Close the networks database (no-op on Zephyr).
 *
 * @posix_func{endnetent}
 */
void endnetent(void);

/**
 * @brief Close the protocols database (no-op on Zephyr).
 *
 * @posix_func{endprotoent}
 */
void endprotoent(void);

/**
 * @brief Close the services database (no-op on Zephyr).
 *
 * @posix_func{endservent}
 */
void endservent(void);

/**
 * @brief Free the address information list returned by getaddrinfo().
 *
 * @param ai Linked list of address information structures to free.
 *
 * @posix_func{freeaddrinfo}
 */
void freeaddrinfo(struct addrinfo *ai);

/**
 * @brief Return a string describing a getaddrinfo() or getnameinfo() error code.
 *
 * @param errcode Error code returned by getaddrinfo() or getnameinfo().
 *
 * @return Pointer to a string describing the error.
 *
 * @posix_func{gai_strerror}
 */
const char *gai_strerror(int errcode);

/**
 * @brief Translate a hostname and service to a list of socket addresses.
 *
 * @param host    Hostname or numeric address string, or NULL.
 * @param service Service name or numeric port string, or NULL.
 * @param hints   Criteria for address selection, or NULL for defaults.
 * @param res     Output: linked list of matching addresses.
 *
 * @return 0 on success, or a non-zero @c EAI_* error code on failure.
 *
 * @posix_func{getaddrinfo}
 */
int getaddrinfo(const char *host, const char *service, const struct addrinfo *hints,
		struct addrinfo **res);

/**
 * @brief Get the next sequential entry from the hosts database.
 *
 * @return Pointer to a static hostent on success, or NULL at end of database or on error.
 *
 * @posix_func{gethostent}
 */
struct hostent *gethostent(void);

/**
 * @brief Translate a socket address to a hostname and service name.
 *
 * @param addr    Socket address to look up.
 * @param addrlen Size of @p addr, in bytes.
 * @param host    Output buffer for the hostname, or NULL.
 * @param hostlen Size of @p host, in bytes.
 * @param serv    Output buffer for the service name, or NULL.
 * @param servlen Size of @p serv, in bytes.
 * @param flags   @c NI_* flags controlling the conversion.
 *
 * @return 0 on success, or a non-zero @c EAI_* error code on failure.
 *
 * @posix_func{getnameinfo}
 */
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags);

/**
 * @brief Look up a network by address.
 *
 * @param net  Network number, in host byte order.
 * @param type Address type of the network (@c AF_INET).
 *
 * @return Pointer to a static netent on success, or NULL on failure.
 *
 * @posix_func{getnetbyaddr}
 */
struct netent *getnetbyaddr(uint32_t net, int type);

/**
 * @brief Look up a network by name.
 *
 * @param name Network name.
 *
 * @return Pointer to a static netent on success, or NULL on failure.
 *
 * @posix_func{getnetbyname}
 */
struct netent *getnetbyname(const char *name);

/**
 * @brief Get the next sequential entry from the networks database.
 *
 * @return Pointer to a static netent on success, or NULL at end of database or on error.
 *
 * @posix_func{getnetent}
 */
struct netent *getnetent(void);

/**
 * @brief Look up a protocol by name.
 *
 * @param name Protocol name (e.g. "tcp").
 *
 * @return Pointer to a static protoent on success, or NULL on failure.
 *
 * @posix_func{getprotobyname}
 */
struct protoent *getprotobyname(const char *name);

/**
 * @brief Look up a protocol by number.
 *
 * @param proto Protocol number.
 *
 * @return Pointer to a static protoent on success, or NULL on failure.
 *
 * @posix_func{getprotobynumber}
 */
struct protoent *getprotobynumber(int proto);

/**
 * @brief Get the next sequential entry from the protocols database.
 *
 * @return Pointer to a static protoent on success, or NULL at end of database or on error.
 *
 * @posix_func{getprotoent}
 */
struct protoent *getprotoent(void);

/**
 * @brief Look up a service by name and protocol.
 *
 * @param name  Service name (e.g. "http").
 * @param proto Protocol name ("tcp" or "udp"), or NULL for any protocol.
 *
 * @return Pointer to a static servent on success, or NULL on failure.
 *
 * @posix_func{getservbyname}
 */
struct servent *getservbyname(const char *name, const char *proto);

/**
 * @brief Look up a service by port number and protocol.
 *
 * @param port  Port number, in network byte order.
 * @param proto Protocol name ("tcp" or "udp"), or NULL for any protocol.
 *
 * @return Pointer to a static servent on success, or NULL on failure.
 *
 * @posix_func{getservbyport}
 */
struct servent *getservbyport(int port, const char *proto);

/**
 * @brief Get the next sequential entry from the services database.
 *
 * @return Pointer to a static servent on success, or NULL at end of database or on error.
 *
 * @posix_func{getservent}
 */
struct servent *getservent(void);

/**
 * @brief Open the hosts database and reset its sequential position.
 *
 * @param stayopen Non-zero to keep the database open between queries.
 *
 * @posix_func{sethostent}
 */
void sethostent(int stayopen);

/**
 * @brief Open the networks database and reset its sequential position.
 *
 * @param stayopen Non-zero to keep the database open between queries.
 *
 * @posix_func{setnetent}
 */
void setnetent(int stayopen);

/**
 * @brief Open the protocols database and reset its sequential position.
 *
 * @param stayopen Non-zero to keep the database open between queries.
 *
 * @posix_func{setprotoent}
 */
void setprotoent(int stayopen);

/**
 * @brief Open the services database and reset its sequential position.
 *
 * @param stayopen Non-zero to keep the database open between queries.
 *
 * @posix_func{setservent}
 */
void setservent(int stayopen);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_NETDB_H_ */
