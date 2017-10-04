ccflags-y += -I${ZEPHYR_BASE}/subsys/net/ip
obj-y = main.o

ifeq ($(CONFIG_NET_L2_IEEE802154), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ifeq ($(CONFIG_NET_APP_SETTINGS), y)
obj-y += ../../common/ieee802154_settings.o
endif
endif
