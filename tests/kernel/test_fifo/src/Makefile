ccflags-y += -I${srctree}/tests/include
ccflags-$(CONFIG_SOC_QUARK_D2000) += -DFIBER_STACKSIZE=256

obj-$(CONFIG_MICROKERNEL) = micro_fifo.o
obj-$(CONFIG_NANOKERNEL) = nano_fifo.o fifo_timeout.o
