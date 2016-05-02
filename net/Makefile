obj-$(CONFIG_BLUETOOTH) += bluetooth/
obj-$(CONFIG_NET_BUF) += buf.o
ifeq ($(CONFIG_NET_YAIP),y)
obj-y += yaip/
else
obj-$(CONFIG_NETWORKING) += ip/
endif
