ccflags-y += -I${ZEPHYR_BASE}/usb/include -I${ZEPHYR_BASE}/include/drivers/usb -I${ZEPHYR_BASE}/include/usb/

ccflags-$(CONFIG_IEEE802154_CC2520_LEGACY) += -I${ZEPHYR_BASE}/net/ip/contiki/os
ccflags-$(CONFIG_IEEE802154_CC2520_LEGACY) += -I${ZEPHYR_BASE}/net/ip/contiki
ccflags-$(CONFIG_IEEE802154_CC2520_LEGACY) += -I${ZEPHYR_BASE}/net/ip

ccflags-$(CONFIG_IEEE802154_CC2520_RAW) += -I${ZEPHYR_BASE}/subsys/net/ip

obj-y += wpanusb.o
