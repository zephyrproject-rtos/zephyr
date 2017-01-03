obj-$(CONFIG_NET_BUF) += buf.o

ifeq ($(CONFIG_NETWORKING),y)
ifeq ($(CONFIG_NET_L2_RAW_CHANNEL),y)
obj-y += ip/nbuf.o
else
obj-y += ip/
endif

endif

include lib/Makefile
obj-y += lib/
