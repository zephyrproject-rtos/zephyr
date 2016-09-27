ccflags-y += -I${ZEPHYR_BASE}/usb/include -I${ZEPHYR_BASE}/include/drivers/usb -I${ZEPHYR_BASE}/include/usb/

ccflags-$(CONFIG_TI_CC2520_LEGACY) += -I${ZEPHYR_BASE}/net/ip/contiki/os
ccflags-$(CONFIG_TI_CC2520_LEGACY) += -I${ZEPHYR_BASE}/net/ip/contiki
ccflags-$(CONFIG_TI_CC2520_LEGACY) += -I${ZEPHYR_BASE}/net/ip

ccflags-$(CONFIG_TI_CC2520_RAW) += -I${ZEPHYR_BASE}/net/yaip

obj-y += wpanusb.o
