ccflags-$(CONFIG_NEWLIB_LIBC) += -D__LINUX_ERRNO_EXTENSIONS__
obj-y = net_core.o \
	net_if.o \
	net_context.o \
	nbuf.o \
	utils.o

obj-y += l2/

obj-$(CONFIG_NET_IPV6) += icmpv6.o nbr.o ipv6.o
obj-$(CONFIG_NET_IPV4) += icmpv4.o ipv4.o
obj-$(CONFIG_NET_6LO) += 6lo.o
obj-$(CONFIG_NET_TRICKLE) += trickle.o
obj-$(CONFIG_NET_DHCPV4) += dhcpv4.o
obj-$(CONFIG_NET_ROUTE) += route.o
obj-$(CONFIG_NET_RPL) += rpl.o
obj-$(CONFIG_NET_RPL_MRHOF) += rpl-mrhof.o
obj-$(CONFIG_NET_RPL_OF0) += rpl-of0.o
obj-$(CONFIG_NET_MGMT_EVENT) += net_mgmt.o
obj-$(CONFIG_NET_TCP) += tcp.o
obj-$(CONFIG_NET_SHELL) += net_shell.o
obj-$(CONFIG_NET_STATISTICS) += net_stats.o

ifeq ($(CONFIG_NET_UDP),y)
	obj-$(CONFIG_NET_UDP) += connection.o
else
	ifeq ($(CONFIG_NET_TCP),y)
		obj-$(CONFIG_NET_TCP) += connection.o
	endif
endif
