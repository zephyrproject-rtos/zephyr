# USB headers
ccflags-y += -I${srctree}/include/drivers/usb -I${srctree}/include/usb/ -I${srctree}/usb/include/ -I${srctree}/subsys/usb/class

# IP headers
ccflags-y += -I${srctree}/subsys/net/ip

obj-$(CONFIG_USB_DEVICE_NETWORK) += netusb.o
obj-$(CONFIG_USB_DEVICE_NETWORK_ECM) += function_ecm.o
