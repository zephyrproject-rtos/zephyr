ifneq ($(CONFIG_KERNEL_V2),y)
ccflags-$(CONFIG_MICROKERNEL) += -I$(srctree)/include/microkernel
endif
ccflags-y += -I$(CURDIR)/misc/generated/sysgen

obj-y = kernel_main.o
