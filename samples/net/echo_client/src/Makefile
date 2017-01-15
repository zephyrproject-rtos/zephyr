obj-y = echo-client.o

ifeq ($(CONFIG_TI_CC2520),y)

ifeq ($(CONFIG_BOARD_ARDUINO_101),y)
ccflags-y +=-I${ZEPHYR_BASE}/include/drivers/
obj-y += ../../common/cc2520_a101.o
endif

ifeq ($(CONFIG_BOARD_FRDM_K64F),y)
ccflags-y +=-I${ZEPHYR_BASE}/drivers/
ccflags-y +=-I${ZEPHYR_BASE}/include/drivers/
obj-y += ../../common/cc2520_frdm_k64f.o
endif

endif

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=0
endif

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/
obj-y += ../../../bluetooth/gatt/ipss.o
endif
