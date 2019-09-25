/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <net/socket.h>

#define pollfd zsock_pollfd
#define fcntl zsock_fcntl

#define POLLIN ZSOCK_POLLIN
#define POLLOUT ZSOCK_POLLOUT

#define addrinfo zsock_addrinfo

#define F_SETFD 2
#define FD_CLOEXEC 1

size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
int iscntrl(int c);

double atof(const char *str);
long long int strtoll(const char *str, char **endptr, int base);
int sscanf(const char *s, const char *format, ...);
char *strerror(int err);
unsigned long long int strtoull(const char *str, char **endptr, int base);

time_t time(time_t *t);
struct tm *gmtime(const time_t *ptime);
size_t strftime(char *dst, size_t dst_size, const char *fmt,
		const struct tm *tm);
double difftime(time_t end, time_t beg);
struct tm *localtime(const time_t *timer);

int fileno(FILE *stream);
int ferror(FILE *stream);
int fclose(FILE *stream);
int fseeko(FILE *stream, off_t offset, int whence);
FILE *fopen(const char *filename, const char *mode);
char *fgets(char *str, int num, FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
int remove(const char *filename);

int getsockname(int sock, struct sockaddr *addr, socklen_t *addrlen);
int poll(struct zsock_pollfd *fds, int nfds, int timeout);

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
		char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags);

ssize_t send(int sock, const void *buf, size_t len, int flags);
ssize_t recv(int sock, void *buf, size_t max_len, int flags);
int socket(int family, int type, int proto);
int getaddrinfo(const char *host, const char *service,
		const struct zsock_addrinfo *hints,
		struct zsock_addrinfo **res);

void freeaddrinfo(struct zsock_addrinfo *ai);
int connect(int sock, const struct sockaddr *addr, socklen_t addrlen);
int getsockopt(int sock, int level, int optname,
	       void *optval, socklen_t *optlen);
int setsockopt(int sock, int level, int optname,
	       const void *optval, socklen_t optlen);
int listen(int sock, int backlog);
int accept(int sock, struct sockaddr *addr, socklen_t *addrlen);
int bind(int sock, const struct sockaddr *addr, socklen_t addrlen);
