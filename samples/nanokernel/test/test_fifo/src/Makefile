ccflags-y += ${PROJECTINCLUDE} -I${srctree}/samples/include
ccflags-$(CONFIG_PLATFORM_QUARK_D2000) += -DFIBER_STACKSIZE=256

obj-y = fifo.o fifo_timeout.o
