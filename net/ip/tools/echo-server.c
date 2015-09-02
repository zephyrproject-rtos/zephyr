/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Listen UDP (unicast or multicast) messages from tun device and send
 * them back to client that is running in qemu.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ipv6.h>
#include <ifaddrs.h>

#define SERVER_PORT  4242
#define CLIENT_PORT  8484
#define MAX_BUF_SIZE 1280	/* min IPv6 MTU, the actual data is smaller */
#define MAX_TIMEOUT  3		/* in seconds */

static inline void reverse(unsigned char *buf, int len)
{
	int i, last = len - 1;

	for(i = 0; i < len/2; i++) {
		unsigned char tmp = buf[i];
		buf[i] = buf[last - i];
		buf[last - i] = tmp;
	}
}

static int get_ifindex(const char *name)
{
	struct ifreq ifr;
	int sk, err;

	if (!name)
		return -1;

	sk = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (sk < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);

	err = ioctl(sk, SIOCGIFINDEX, &ifr);

	close(sk);

	if (err < 0)
		return -1;

	return ifr.ifr_ifindex;
}

static int get_socket(int family)
{
	int fd;

	fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		perror("socket");
		exit(-errno);
	}
	return fd;
}

static int bind_device(int fd, const char *interface, void *addr, int len)
{
	struct ifreq ifr;
	int ret, val = 1;

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);

	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
		       (void *)&ifr, sizeof(ifr)) < 0) {
		perror("SO_BINDTODEVICE");
		exit(-errno);
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	ret = bind(fd, (struct sockaddr *)addr, len);
	if (ret < 0) {
		perror("bind");
		exit(-errno);
	}
}

static int receive(int fd, unsigned char *buf, int buflen,
		   struct sockaddr *addr, socklen_t *addrlen)
{
	int ret;

	ret = recvfrom(fd, buf, buflen, 0, addr, addrlen);
	if (ret <= 0) {
		perror("recv");
		return -EINVAL;
	}

	return ret;
}

static int reply(int fd, unsigned char *buf, int buflen,
		 struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	ret = sendto(fd, buf, buflen, 0, addr, addrlen);
	if (ret < 0)
		perror("send");

	return ret;
}

static int receive_and_reply(fd_set *rfds, int fd_recv, int fd_send,
			     unsigned char *buf, int buflen)
{
	if (FD_ISSET(fd_recv, rfds)) {
		struct sockaddr from;
		socklen_t fromlen;
		int ret;

		ret = receive(fd_recv, buf, buflen, &from, &fromlen);
		if (ret < 0)
			return ret;

		reverse(buf, ret);

		ret = reply(fd_send, buf, ret, &from, fromlen);
		if (ret < 0)
			return ret;

		fprintf(stderr, ".");
	}

	return 0;
}

#define MY_MCAST_ADDR6 \
	{ { { 0xff,0x84,0,0,0,0,0,0,0,0,0,0,0,0,0,0x2 } } } /* ff84::2 */

#define MY_MCAST_ADDR4 "239.192.0.2"

int family_to_level(int family)
{
	switch (family) {
	case AF_INET:
		return IPPROTO_IP;
	case AF_INET6:
		return IPPROTO_IPV6;
	default:
		return -1;
	}
}

static int join_mc_group(int sock, int ifindex, int family, void *addr,
		       int addr_len)
{
	struct group_req req;
	int ret, off = 0;

	memset(&req, 0, sizeof(req));

	req.gr_interface = ifindex;
	memcpy(&req.gr_group, addr, addr_len);

	ret = setsockopt(sock, family_to_level(family), MCAST_JOIN_GROUP,
			 &req, sizeof(req));
	if (ret < 0)
		perror("setsockopt(MCAST_JOIN_GROUP)");

	switch (family) {
	case AF_INET:
		ret = setsockopt(sock, family_to_level(family),
				 IP_MULTICAST_LOOP, &off, sizeof(off));
		break;
	case AF_INET6:
		ret = setsockopt(sock, family_to_level(family),
				 IPV6_MULTICAST_LOOP, &off, sizeof(off));
		break;
	}
	return ret;
}

static int get_address(int ifindex, int family, void *address)
{
	struct ifaddrs *ifaddr, *ifa;
	int err = -ENOENT;
	char name[IF_NAMESIZE];

	if (!if_indextoname(ifindex, name))
		return -EINVAL;

	if (getifaddrs(&ifaddr) < 0) {
		err = -errno;
		fprintf(stderr, "Cannot get addresses err %d/%s",
			err, strerror(-err));
		return err;
	}

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr)
			continue;

		if (strncmp(ifa->ifa_name, name, IF_NAMESIZE) == 0 &&
					ifa->ifa_addr->sa_family == family) {
			if (family == AF_INET) {
				struct sockaddr_in *in4 = (struct sockaddr_in *)
					ifa->ifa_addr;
				if (in4->sin_addr.s_addr == INADDR_ANY)
					continue;
				if ((in4->sin_addr.s_addr & IN_CLASSB_NET) ==
						((in_addr_t) 0xa9fe0000))
					continue;
				memcpy(address, &in4->sin_addr,
							sizeof(struct in_addr));
			} else if (family == AF_INET6) {
				struct sockaddr_in6 *in6 =
					(struct sockaddr_in6 *)ifa->ifa_addr;
				if (memcmp(&in6->sin6_addr, &in6addr_any,
						sizeof(struct in6_addr)) == 0)
					continue;
				if (IN6_IS_ADDR_LINKLOCAL(&in6->sin6_addr))
					continue;

				memcpy(address, &in6->sin6_addr,
						sizeof(struct in6_addr));
			} else {
				err = -EINVAL;
				goto out;
			}

			err = 0;
			break;
		}
	}

out:
	freeifaddrs(ifaddr);
	return err;
}

extern int optind, opterr, optopt;
extern char *optarg;

/* The application returns:
 *    < 0 : connection or similar error
 *      0 : no errors, all tests passed
 *    > 0 : could not send all the data to server
 */
int main(int argc, char**argv)
{
	int c, ret, fd4, fd6, fd4m, fd6m, i = 0, timeout = 0;
	struct sockaddr_in6 addr6_recv = { 0 }, maddr6 = { 0 };
	struct in6_addr mcast6_addr = MY_MCAST_ADDR6;
	struct in_addr mcast4_addr = { 0 };
	struct sockaddr_in addr4_recv = { 0 }, maddr4 = { 0 };
	int family;
	unsigned char buf[MAX_BUF_SIZE];
	char addr_buf[INET6_ADDRSTRLEN];
	const struct in6_addr any = IN6ADDR_ANY_INIT;
	const char *interface = NULL;
	fd_set rfds;
	struct timeval tv = {};
	int ifindex = -1;

	opterr = 0;

	while ((c = getopt(argc, argv, "i:")) != -1) {
		switch (c) {
		case 'i':
			interface = optarg;
			break;
		}
	}

	if (!interface) {
		printf("usage: %s -i <iface>\n", argv[0]);
		printf("\t-i Use this network interface.\n");
		exit(-EINVAL);
	}

	ifindex = get_ifindex(interface);
	if (ifindex < 0) {
		printf("Invalid interface %s\n", interface);
		exit(-EINVAL);
	}

	addr4_recv.sin_family = AF_INET;
	addr4_recv.sin_port = htons(SERVER_PORT);

	/* We want to bind to global unicast address here so that
	 * we can listen correct addresses. We do not want to listen
	 * link local addresses in this test.
	 */
	get_address(ifindex, AF_INET, &addr4_recv.sin_addr);
	printf("IPv4: binding to %s\n",
	       inet_ntop(AF_INET, &addr4_recv.sin_addr,
			 addr_buf, sizeof(addr_buf)));

	addr6_recv.sin6_family = AF_INET6;
	addr6_recv.sin6_port = htons(SERVER_PORT);

	/* Bind to global unicast address instead of ll address */
	get_address(ifindex, AF_INET6, &addr6_recv.sin6_addr);
	printf("IPv6: binding to %s\n",
	       inet_ntop(AF_INET6, &addr6_recv.sin6_addr,
			 addr_buf, sizeof(addr_buf)));

	memcpy(&maddr6.sin6_addr, &mcast6_addr, sizeof(struct in6_addr));
	maddr6.sin6_family = AF_INET6;
	maddr6.sin6_port = htons(SERVER_PORT);

	mcast4_addr.s_addr = inet_addr(MY_MCAST_ADDR4);
	memcpy(&maddr4.sin_addr, &mcast4_addr, sizeof(struct in_addr));
	maddr4.sin_family = AF_INET;
	maddr4.sin_port = htons(SERVER_PORT);

	fd4 = get_socket(AF_INET);
	fd6 = get_socket(AF_INET6);
	fd4m = get_socket(AF_INET);
	fd6m = get_socket(AF_INET6);

	bind_device(fd4, interface, &addr4_recv, sizeof(addr4_recv));
	bind_device(fd6, interface, &addr6_recv, sizeof(addr6_recv));

	bind_device(fd4m, interface, &maddr4, sizeof(maddr4));
	bind_device(fd6m, interface, &maddr6, sizeof(maddr6));

	join_mc_group(fd4m, ifindex, AF_INET, &maddr4, sizeof(maddr4));
	join_mc_group(fd6m, ifindex, AF_INET6, &maddr6, sizeof(maddr6));

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

	while (1) {
		int fd = MAX(fd4m, fd6m);

		FD_ZERO(&rfds);
		FD_SET(fd4, &rfds);
		FD_SET(fd6, &rfds);
		FD_SET(fd4m, &rfds);
		FD_SET(fd6m, &rfds);

		tv.tv_sec = MAX_TIMEOUT;
		tv.tv_usec = 0;

		ret = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			break;
		} else if (ret == 0) {
			continue;
		}

		/* Unicast IPv4 */
		if (receive_and_reply(&rfds, fd4, fd4, buf, sizeof(buf)) < 0)
			break;

		/* Unicast IPv6 */
		if (receive_and_reply(&rfds, fd6, fd6, buf, sizeof(buf)) < 0)
			break;

		/* Multicast IPv4 */
		if (receive_and_reply(&rfds, fd4m, fd4, buf, sizeof(buf)) < 0)
			break;

		/* Multicast IPv6 */
		if (receive_and_reply(&rfds, fd6m, fd6, buf, sizeof(buf)) < 0)
			break;
	}

	close(fd4);
	close(fd6);
	close(fd4m);
	close(fd6m);

	printf("\n");

	exit(0);
}
