ccflags-y += ${PROJECTINCLUDE}
ccflags-y +=-I$(ZEPHYR_BASE)/include/drivers
ccflags-y +=-I$(ZEPHYR_BASE)/drivers -I$(ZEPHYR_BASE)/arch/x86
ccflags-y +=-I${ZEPHYR_BASE}/drivers/wifi/winc1500/
ccflags-y +=-I${ZEPHYR_BASE}/ext/drivers/atmel/winc1500/
ccflags-y +=-I${ZEPHYR_BASE}/ext/drivers/atmel/winc1500/
ccflags-y +=-I${ZEPHYR_BASE}/lib/json/

obj-y += main.o
obj-y += ipm.o
obj-y += wifi.o
obj-y += mqtt.o
