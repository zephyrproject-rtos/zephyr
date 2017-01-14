ccflags-y += -I${ZEPHYR_BASE}/subsys/net/ip

obj-$(CONFIG_NET_L2_DUMMY) += dummy.o
obj-${CONFIG_NET_L2_ETHERNET} += ethernet.o
obj-$(CONFIG_NET_L2_IEEE802154) += ieee802154/
obj-$(CONFIG_NET_L2_BLUETOOTH) += bluetooth.o
obj-$(CONFIG_NET_L2_BLUETOOTH_SHELL) += bluetooth_shell.o

obj-${CONFIG_NET_ARP} += arp.o
