obj-$(CONFIG_NET_BUF) += buf.o
ifeq ($(CONFIG_NET_YAIP),y)
ifeq ($(CONFIG_NET_L2_RAW_CHANNEL),y)
obj-y += yaip/nbuf.o
else
obj-y += yaip/
endif
else
obj-$(CONFIG_NETWORKING) += ip/
endif
