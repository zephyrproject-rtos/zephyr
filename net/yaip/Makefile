obj-y = net_core.o \
	net_if.o \
	net_context.o \
	nbuf.o \
	utils.o

obj-y += l2/

obj-$(CONFIG_NET_IPV6) += icmpv6.o nbr.o ipv6.o
obj-$(CONFIG_NET_IPV4) += icmpv4.o
obj-$(CONFIG_NET_6LO) += 6lo.o

ifeq ($(CONFIG_NET_UDP),y)
	obj-$(CONFIG_NET_UDP) += connection.o
else
	ifeq ($(CONFIG_NET_TCP),y)
		obj-$(CONFIG_NET_TCP) += connection.o
	endif
endif
