ccflags-y += -I${srctree}/net/ip
obj-y = net_core.o \
	net_if.o \
	net_context.o \
	nbuf.o \
	utils.o

obj-y += l2/

obj-$(CONFIG_NET_IPV6) += icmpv6.o
obj-$(CONFIG_NET_IPV4) += icmpv4.o
